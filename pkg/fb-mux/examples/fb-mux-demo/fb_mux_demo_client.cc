#include <l4/fb-mux/fb_mux.h>

#include <l4/re/env>
#include <l4/sys/capability>
#include <l4/sys/err.h>
#include <l4/util/util.h>

#include <cstdlib>
#include <iostream>

enum {Fb_switch_timeout = 1500};

char const *const Fb_mux_registry_name = "fbmux";

int
main()
{
  L4::Cap<Fb_mux> mux = L4Re::Env::env()->get_cap<Fb_mux>(Fb_mux_registry_name);
  if (!mux.is_valid())
    {
      std::cerr << "Failed to obtain framebuffer mux capability\n";
      exit(EXIT_FAILURE);
    }

  for (;;)
    {
      l4_sleep(Fb_switch_timeout);

      std::cout << "Switching buffers\n";

      if (mux->switch_buffer() != L4_EOK)
        std::cerr << "Error while talking to mux server\n";
    }

  exit(EXIT_SUCCESS);
}
