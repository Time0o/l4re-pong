#include <l4/cxx/exceptions>
#include <l4/io/io.h>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/br_manager>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/sys/icu.h>
#include <l4/sys/ipc.h>
#include <l4/sys/l4int.h>
#include <l4/util/port_io.h>
#include <l4/util/util.h>
#include <pthread-l4.h>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "keyboard_session_server.h"

using L4Re::chkcap;
using L4Re::chksys;

enum {
  Io_port = 0x60, Irq_num = 1,
  Irq_thread_label = 0xaffe,
  Irq_poll_timeout_ms = 10
};

namespace
{

char const* const Keyboard_session_registry_name = "keyboard";

// Query keyboard input.
void
query_keyboard(Keyboard_session_server &session_server, L4::Cap<L4::Irq> &irq)
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
                session_server.release_key(keycode->second);
            }
          else
            {
              auto keycode = keycodes.find(in);

              if (keycode != keycodes.end())
                session_server.hold_key(keycode->second);
            }
        }
    }
}

void
run()
{
  // Set up ICU and IRQ.
  chksys(l4io_request_ioport(Io_port, 1), "Ioport request failed");

  auto icu = chkcap(L4Re::Env::env()->get_cap<L4::Icu>("icu"),
                    "Failed to obtain ICU capability");

  auto irq = chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>(),
                    "Failed to allocate IRQ capability slot");

  chksys(L4Re::Env::env()->factory()->create<L4::Irq>(irq),
         "Failed to create IRQ oject");

  // Initialize and register keyboard session server.
  L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> registry_server;
  auto registry = registry_server.registry();

  Keyboard_session_server session_server(registry_server);

  chkcap(registry->register_obj(&session_server, Keyboard_session_registry_name),
         "Failed to register service");

  // Start IRQ thread and bind IRQ to ICU.
  std::thread query_keyboard_thread(query_keyboard,
                                    std::ref(session_server), std::ref(irq));

  L4::Cap<L4::Thread> query_keyboard_thread_cap(
    pthread_l4_cap(query_keyboard_thread.native_handle()));

  chksys(irq->bind_thread(query_keyboard_thread_cap, Irq_thread_label),
         "Failed to attach to IRQ");

  chksys(icu->bind(Irq_num, irq), "Failed to bind IRQ to ICU");

  // Start server loop.
  std::cout << "Starting server loop\n";

  registry_server.loop();
}

}

int
main()
{
  int exit_status;

  try
    {
      run();
      exit_status = EXIT_SUCCESS;
    }
  catch (L4::Runtime_error const &e)
    {
      std::cerr << e.str() << std::endl;
      exit_status = EXIT_FAILURE;
    }
  catch (...)
    {
      std::cerr << "Uncaught exception" << std::endl;
      exit_status = EXIT_FAILURE;
    }

  exit(exit_status);
}
