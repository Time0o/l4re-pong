#include <l4/re/dataspace>
#include <l4/re/env>
#include <l4/re/rm>
#include <l4/re/util/cap_alloc>
#include <l4/sys/consts.h>
#include <l4/util/lock.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <errno.h>

enum : long {
  Freelist_size_sz = static_cast<long>(sizeof(long)),
  Freelist_prev_ptr_sz = static_cast<long>(sizeof(char *)),
  Freelist_next_ptr_sz = static_cast<long>(sizeof(char *)),

  Freelist_prev_ptr_offs = static_cast<long>(sizeof(long)),
  Freelist_next_ptr_offs = static_cast<long>((sizeof(long) + sizeof(void *))),

  Chunk_min = 8L,
  Inc_min = static_cast<long>(L4_PAGESIZE),
  Inc_max = LONG_MAX - LONG_MAX % static_cast<long>(L4_PAGESIZE)
};

static char *freelist_head;
static l4util_simple_lock_t freelist_lock;

static long
get_chunk_size(char *chunk)
{
  return *reinterpret_cast<long *>(chunk) + Freelist_size_sz;
}

static long
get_avail_size(char *chunk)
{
  return *reinterpret_cast<long *>(chunk);
}

static char *
get_prev_chunk(char *chunk)
{
  return *reinterpret_cast<char **>(chunk + Freelist_prev_ptr_offs);
}

static char *
get_next_chunk(char *chunk)
{
  return *reinterpret_cast<char **>(chunk + Freelist_next_ptr_offs);
}

static void *
get_buffer(char *chunk)
{
  return reinterpret_cast<void *>(chunk + Freelist_size_sz);
}

static char *
get_chunk(void *buffer)
{
  return reinterpret_cast<char *>(buffer) - Freelist_size_sz;
}

static void
set_avail_size(char *chunk, long size)
{
  memcpy(chunk, &size, Freelist_size_sz);
}

static void
set_prev_chunk(char *chunk, char *prev)
{
  memcpy(chunk + Freelist_prev_ptr_offs, &prev, Freelist_prev_ptr_sz);
}

static void
set_next_chunk(char *chunk, char *next)
{
  memcpy(chunk + Freelist_next_ptr_offs, &next, Freelist_next_ptr_sz);
}

static char *
get_new_chunk(long size)
{
  L4::Cap<L4Re::Dataspace> ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

  if (!ds.is_valid())
    return nullptr;

  if (L4Re::Env::env()->mem_alloc()->alloc(size, ds))
    return nullptr;

  l4_addr_t addr;
  if (L4Re::Env::env()->rm()->attach(&addr, size, L4Re::Rm::Search_addr, ds))
    return nullptr;

  static_assert(sizeof(char *) <= sizeof(l4_addr_t),
                "l4_addr_t representable by char *");

  char *chunk = reinterpret_cast<char *>(addr);

  set_avail_size(chunk, size - Freelist_size_sz);

  return chunk;
}

static bool
coalesce_chunks(char *chunk1, char *chunk2)
{
  long chunk1_size = get_chunk_size(chunk1);
  long chunk2_size = get_chunk_size(chunk2);

  if (chunk1 + chunk1_size == chunk2)
    {
      long avail_size = chunk1_size + chunk2_size - Freelist_size_sz;
      set_avail_size(chunk1, avail_size);

      char *next_chunk = get_next_chunk(chunk2);
      set_next_chunk(chunk1, next_chunk);

      if (next_chunk)
        set_prev_chunk(next_chunk, chunk1);

      return true;
    }

  return false;
}

static void
freelist_prepend(char *chunk)
{
  l4_simple_lock(&freelist_lock);

  if (!freelist_head) /* Create initial free list chunk. */
    {
      set_prev_chunk(chunk, nullptr);
      set_next_chunk(chunk, nullptr);

      freelist_head = chunk;
    }
  else
    {
      /* Check if new cunk and free list head form contiguous memory block and
         merge them if they do, otherwise make the new chunk the new free list
         head chunk. */

      if (!coalesce_chunks(chunk, freelist_head))
        {
          set_next_chunk(chunk, freelist_head);
          set_prev_chunk(freelist_head, chunk);
        }

      set_prev_chunk(chunk, nullptr);

      freelist_head = chunk;
    }

  l4_simple_unlock(&freelist_lock);
}

static bool
freelist_init(void)
{
  char *init = get_new_chunk(Inc_min);
  if (!init)
    return false;

  set_avail_size(init, Inc_min - Freelist_size_sz);

  freelist_prepend(init);

  return true;
}

static char *
freelist_find(long min_avail_size)
{
  static bool first_call = true;

  if (first_call)
    {
      l4_simple_unlock(&freelist_lock);
      first_call = false;
    }

  l4_simple_lock(&freelist_lock);

  char *current_chunk = freelist_head;

  /* Try to find memory chunk on free list using first fit. */
  while (current_chunk)
    {
      long avail_size = get_avail_size(current_chunk);
      char *prev_chunk = get_prev_chunk(current_chunk);
      char *next_chunk = get_next_chunk(current_chunk);

      if (avail_size >= min_avail_size)
        {
          long remaining_size = avail_size - min_avail_size - Freelist_size_sz;

          /* Split chunk and return only part of it. */
          if (remaining_size >= Chunk_min)
            {
              char *new_chunk =
                current_chunk + min_avail_size + Freelist_size_sz;

              set_avail_size(new_chunk, remaining_size);
              set_prev_chunk(new_chunk, prev_chunk);
              set_next_chunk(new_chunk, next_chunk);

              if (prev_chunk)
                set_next_chunk(prev_chunk, new_chunk);

              if (next_chunk)
                set_prev_chunk(next_chunk, new_chunk);

              if (current_chunk == freelist_head)
                freelist_head = new_chunk;

              set_avail_size(current_chunk, min_avail_size);

              l4_simple_unlock(&freelist_lock);

              return current_chunk;
            }

          /* Return a whole chunk. */
          if (!prev_chunk)
            {
              if (!next_chunk) /* Don't give away the last remaining chunk. */
                return nullptr;

              set_prev_chunk(next_chunk, nullptr);

              freelist_head = next_chunk;
            }
          else
            {
              set_next_chunk(prev_chunk, next_chunk);

              if (next_chunk)
                {
                  set_prev_chunk(next_chunk, prev_chunk);
                  coalesce_chunks(prev_chunk, next_chunk);
                }
            }

          l4_simple_unlock(&freelist_lock);

          return current_chunk;
        }

      current_chunk = next_chunk;
    }

  l4_simple_unlock(&freelist_lock);

  return nullptr;
}

void
freelist_dump(void);

void
freelist_dump(void)
{
  /* This function is only used for debugging purposes so implementing
     reader-writer synchronisation would be overkill. */
  l4_simple_lock(&freelist_lock);

  char *current_chunk = freelist_head;

  while (current_chunk)
    {
      long chunk_size = get_chunk_size(current_chunk);
      long avail_size = get_avail_size(current_chunk);
      char *prev_chunk = get_prev_chunk(current_chunk);
      char *next_chunk = get_next_chunk(current_chunk);

      printf("CHUNK: %p\n", current_chunk);
      printf("  - SIZE: %ld (%ld useable)\n", chunk_size, avail_size);
      printf("  - PREV: %p\n", prev_chunk);
      printf("  - NEXT: %p\n", next_chunk);

      current_chunk = next_chunk;
    }

  l4_simple_unlock(&freelist_lock);
}

void *
malloc(std::size_t size) throw()
{
  if (size == 0u)
    return nullptr;

  if (sizeof(std::size_t) > sizeof(long) && size > LONG_MAX)
    {
      errno = ENOMEM;
      return nullptr;
    }

  if (!freelist_head)
    {
      if (!freelist_init())
        {
          errno = ENOMEM;
          return nullptr;
        }
    }

  long lsize = static_cast<long>(size);

  char *chunk = freelist_find(lsize);
  if (chunk)
    return get_buffer(chunk);

  l4_addr_t inc = l4_round_page(static_cast<l4_addr_t>(lsize));

  long linc;
  if (inc < static_cast<l4_addr_t>(Inc_min))
    linc = Inc_min;
  else if (inc <= static_cast<l4_addr_t>(Inc_max))
    linc = static_cast<long>(inc);
  else
    linc = Inc_max;

  char *new_chunk = get_new_chunk(linc);
  if (!new_chunk)
    {
      errno = ENOMEM;
      return nullptr;
    }

  return get_buffer(new_chunk);
}

void
free(void *p) throw()
{
  freelist_prepend(get_chunk(p));
}

void *
realloc(void *p, std::size_t size) throw()
{
  if (size == 0u)
    return nullptr;

  if (sizeof(std::size_t) > sizeof(long) && size > LONG_MAX)
    {
      errno = ENOMEM;
      return nullptr;
    }

  char *chunk = get_chunk(p);

  if (get_avail_size(chunk) >= static_cast<long>(size))
    return p;

  free(p);

  return malloc(size);
}
