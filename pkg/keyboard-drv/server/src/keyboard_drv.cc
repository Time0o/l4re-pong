#include <l4/io/io.h>
#include <l4/re/env>
#include <l4/re/util/br_manager>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/sys/icu.h>
#include <l4/sys/ipc.h>
#include <l4/sys/l4int.h>
#include <l4/util/port_io.h>
#include <l4/util/util.h>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <pthread-l4.h>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "keyboard_server.h"

enum {
  Io_port = 0x60, Irq_num = 1,
  Irq_thread_label = 0xaffe,
  Irq_poll_timeout_ms = 10
};

static char const* const Keyboard_server_registry_name = "keyboard";

// Query keyboard input.
static void
query_keyboard(Keyboard_server &keyboard_server, L4::Cap<L4::Irq> &irq)
{
  std::cout << "Ready to receive keyboard input\n";

  std::map<l4_uint8_t, char const*> const keycodes {
    {0x02, "1"}, {0x03, "2"}, {0x04, "3"}, {0x05, "4"}, {0x06, "5"}, {0x07, "6"},
    {0x08, "7"}, {0x09, "8"}, {0x0a, "9"}, {0x0b, "0"}, {0x10, "Q"}, {0x11, "W"},
    {0x12, "E"}, {0x13, "R"}, {0x14, "T"}, {0x15, "Y"}, {0x16, "U"}, {0x17, "I"},
    {0x18, "O"}, {0x19, "P"}, {0x1e, "A"}, {0x1f, "S"}, {0x20, "D"}, {0x21, "F"},
    {0x22, "G"}, {0x23, "H"}, {0x24, "J"}, {0x25, "K"}, {0x26, "L"}, {0x2c, "Z"},
    {0x2d, "X"}, {0x2e, "C"}, {0x2f, "V"}, {0x30, "B"}, {0x31, "N"}, {0x32, "M"}
  };

  for (;;)
    {
      if (l4_error(irq->receive()))
        std::cerr << "Error on IRQ receive\n";
      else
        {
          l4_uint8_t in = l4util_in8(Io_port);

          if (in & 0x80)
            {
              auto keycode = keycodes.find(in & 0x7f);

              if (keycode != keycodes.end())
                keyboard_server.release_key(keycode->second);
            }
          else
            {
              auto keycode = keycodes.find(in);

              if (keycode != keycodes.end())
                keyboard_server.hold_key(keycode->second);
            }
        }
    }
}

int
main()
{
  // Set up ICU and IRQ.
  if (l4io_request_ioport(Io_port, 1))
    {
      std::cerr << "Ioport request failed\n";
      exit(EXIT_FAILURE);
    }

  auto icu = L4Re::Env::env()->get_cap<L4::Icu>("icu");
  if (!icu.is_valid())
    {
      std::cerr << "Failed to obtain ICU capability\n";
      exit(EXIT_FAILURE);
    }

  auto irq = L4Re::Util::cap_alloc.alloc<L4::Irq>();
  if (!irq.is_valid())
    {
      std::cerr << "Failed to allocate IRQ capability slot\n";
      exit(EXIT_FAILURE);
    }

  if (l4_error(L4Re::Env::env()->factory()->create<L4::Irq>(irq)))
    {
      std::cerr << "Failed to create IRQ oject\n";
      exit(EXIT_FAILURE);
    }

  // Register service.
  Keyboard_server keyboard_server;
  L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> registry_server;

  auto registry = registry_server.registry();

  auto keyboard = registry->register_obj(&keyboard_server,
                                         Keyboard_server_registry_name);
  if (!keyboard.is_valid())
    {
      std::cerr << "Failed to register service\n";
      exit(EXIT_FAILURE);
    }

  // Start IRQ thread and bind IRQ to ICU.
  std::thread query_keyboard_thread(query_keyboard, std::ref(keyboard_server),
                                    std::ref(irq));

  auto query_keyboard_thread_cap = // Not very elegant but it works.
    L4::Cap<L4::Thread>(pthread_l4_cap(query_keyboard_thread.native_handle()));

  if (l4_error(irq->attach(Irq_thread_label, query_keyboard_thread_cap)))
    {
      std::cerr << "Failed to attach to IRQ\n";
      exit(EXIT_FAILURE);
    }

  if (l4_error(icu->bind(Irq_num, irq)))
    {
      std::cerr << "Failed to bind IRQ to ICU\n";
      exit(EXIT_FAILURE);
    }

  // Start server loop.
  std::cout << "Starting server loop\n";

  registry_server.loop();
}
