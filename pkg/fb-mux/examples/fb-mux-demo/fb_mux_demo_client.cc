#include <l4/fb-mux/fb_mux.h>

#include <l4/cxx/exceptions>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/sys/capability>
#include <l4/sys/err.h>
#include <l4/util/util.h>

#include <cstdlib>
#include <iostream>

using L4Re::chkcap;

enum {Fb_switch_timeout = 1500};

namespace
{

char const *const Fb_mux_registry_name = "fbmux";

void
run()
{
  auto mux = L4Re::Env::env()->get_cap<Fb_mux>(Fb_mux_registry_name);
  chkcap(mux, "Failed to obtain framebuffer mux capability");

  for (;;)
    {
      l4_sleep(Fb_switch_timeout);

      std::cout << "Switching buffers\n";

      if (mux->switch_buffer() != L4_EOK)
        std::cerr << "Error while talking to mux server\n";
    }
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
      std::cerr << e.str() << '\n';
      exit_status = EXIT_FAILURE;
    }
  catch (...)
    {
      std::cerr << "Uncaught exception\n";
      exit_status = EXIT_FAILURE;
    }

  exit(exit_status);
}
