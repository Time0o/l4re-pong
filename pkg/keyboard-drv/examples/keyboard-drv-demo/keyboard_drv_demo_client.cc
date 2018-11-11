#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/cxx/exceptions>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>
#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_varg>
#include <l4/sys/irq>
#include <l4/sys/thread>
#include <pthread-l4.h>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using L4Re::chksys;
using L4Re::chkcap;

enum {Irq_thread_label = 0xaffe};

namespace
{

char const *const Keyboard_registry_name = "keyboard";

void
query_keyboard(std::vector<char const *> const &requested_keys,
               L4::Cap<Keyboard> &keyboard, L4::Cap<L4::Irq> &irq)
{
  try
    {
      for (;;)
        {
          chksys(irq->receive(), "Error on IRQ receive");

          std::cout << "Key state change:\n";

          bool is_held = false;
          for (char const *key : requested_keys)
            {
              chksys(keyboard->is_held(key, &is_held),
                     "Failed to query keyboard state");

              std::cout << key << "[" << (is_held ? "x" : " ") << "]\n";
            }
        }
    }
  catch (L4::Runtime_error const &e)
    {
      std::cerr << "Keyboard query thread exited with exception: " << e.str()
                << std::endl;
    }
}

void
run(std::vector<char const *> const &requested_keys)
{
  // Obtain keyboard capability.
  auto keyboard =
    chkcap(L4Re::Env::env()->get_cap<Keyboard>(Keyboard_registry_name),
           "Failed to obtain keyboard capability");

  // Allocate keyboard IRQ capability.
  auto irq = chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>(),
                    "Failed to allocate IRQ capability slot");

  // Create keyboard IRQ kernel object.
  chksys(L4Re::Env::env()->factory()->create(irq),
         "Failed to create IRQ kernel object");

  // Map IRQ capability to server.
  std::cout << "Mapping keyboard IRQ\n";

  chksys(keyboard->map_irq(irq),
         "Failed to map IRQ capability to keyboard server");

  // Attach thread to IRQ.
  std::cout << "Starting to listen to keyboard\n";

  std::thread query_keyboard_thread(query_keyboard, requested_keys,
                                    std::ref(keyboard), std::ref(irq));

  L4::Cap<L4::Thread> query_keyboard_thread_cap(
    pthread_l4_cap(query_keyboard_thread.native_handle()));

  chksys(irq->bind_thread(query_keyboard_thread_cap, Irq_thread_label),
         "Failed to attach to keyboard IRQ");

  query_keyboard_thread.join();
}

}

int
main(int argc, char **argv)
{
  int exit_status;

  std::vector<char const *> requested_keys(argc - 1);
  for (int i = 1; i < argc; ++i)
    requested_keys[i - 1] = argv[i];

  try
    {
      run(requested_keys);
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
