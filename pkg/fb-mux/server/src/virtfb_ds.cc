#include <l4/re/dataspace>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/dataspace_svr>
#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/sys/capability>
#include <l4/sys/l4int.h>

#include <cstring>
#include <iostream>

#include "virtfb_ds.h"

static void swap(l4_addr_t new_addr, l4_addr_t old_addr, unsigned long size)
{
  l4_addr_t page = old_addr;

  do
    {
      L4Re::Env::env()->task()->unmap(
        l4_fpage(page, L4_PAGESHIFT, L4_FPAGE_RWX), L4_FP_OTHER_SPACES);

      page += L4_PAGESIZE;
    }
  while (page < old_addr + size);

  memcpy(reinterpret_cast<void *>(new_addr),
         reinterpret_cast<void *>(old_addr), size);
}

Virtfb_ds::Virtfb_ds(L4::Cap<L4Re::Video::Goos> const &goos)
{
  // Determine swap framebuffer size.
  L4Re::Util::Video::Goos_fb goos_fb(goos);
  _fb_size = goos_fb.buffer()->size();

  // Create swap framebuffer.
  L4::Cap<L4Re::Dataspace> fb_ds_swap =
    L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

  if (!fb_ds_swap.is_valid())
    {
      std::cerr << "Failed to create swap framebuffer capability\n";
      return;
    }

  if (L4Re::Env::env()->mem_alloc()->alloc(_fb_size, fb_ds_swap))
    {
      std::cerr << "Failed to allocate swap framebuffer\n";
      return;
    }

  if (L4Re::Env::env()->rm()->attach(&_fb_base_swap,
                                     _fb_size, L4Re::Rm::Search_addr,
                                     fb_ds_swap))
    {
      std::cerr << "Failed to attach swap framebuffer\n";
      return;
    }

  // Null out swap framebuffer.
  memset(reinterpret_cast<void *>(_fb_base_swap), 0, _fb_size);

  // Initialize dataspace.
  _ds_start = _fb_base_swap;
  _ds_size = _fb_size;
  _rw_flags = Writable;

  _valid = true;
}

void Virtfb_ds::map(l4_addr_t fb_base_phys)
{
  swap(fb_base_phys, _fb_base_swap, _fb_size);
  _ds_start = fb_base_phys;
  _mapped = true;
}

void Virtfb_ds::unmap()
{
  swap(_fb_base_swap, _ds_start, _fb_size);
  _ds_start = _fb_base_swap;
  _mapped = false;
}
