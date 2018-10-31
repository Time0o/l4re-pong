#include <l4/re/env>
#include <l4/re/util/br_manager>
#include <l4/re/util/object_registry>
#include <l4/re/video/goos>

#include <cstdlib>
#include <iostream>

#include "fb_mux_server.h"
#include "virtfb_ds.h"
#include "virtfb_server.h"

static char const *const Framebuffer_registry_name = "fb";
static char const *const Fb_mux_registry_name = "fbmux";
static char const *const Virtfb_main_registry_name = "virtfb_main";
static char const *const Virtfb_secondary_registry_name = "virtfb_secondary";

int
main()
{
  // Create registry server.
  L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> registry_server;
  auto registry = registry_server.registry();

  // Obtain framebuffer capability.
  auto fb =
    L4Re::Env::env()->get_cap<L4Re::Video::Goos>(Framebuffer_registry_name);

  if (!fb.is_valid())
    {
      std::cerr << "Failed to obtain framebuffer capability\n";
      exit(EXIT_FAILURE);
    }

  // Set up virtual framebuffer dataspaces.
  Virtfb_ds virtfb_ds_main(fb);
  Virtfb_ds virtfb_ds_secondary(fb);

  if (!virtfb_ds_main.is_valid() || !virtfb_ds_secondary.is_valid())
    {
      std::cerr << "Failed to set up virtual framebuffers\n";
      exit(EXIT_FAILURE);
    }

  if (!registry->register_obj(&virtfb_ds_main).is_valid())
    {
      std::cerr << "Failed to register main virtual framebuffer dataspace\n";
      exit(EXIT_FAILURE);
    }

  if (!registry->register_obj(&virtfb_ds_secondary).is_valid())
    {
      std::cerr << "Failed to register secondary virtual framebuffer dataspace\n";
      exit(EXIT_FAILURE);
    }

  std::cout << "Set up virtual framebuffer dataspaces\n";

  // Set up virtual framebuffer servers.
  Virtfb_server virtfb_server_main(fb, virtfb_ds_main.obj_cap());
  Virtfb_server virtfb_server_secondary(fb, virtfb_ds_secondary.obj_cap());

  if (!virtfb_server_main.is_set_up() || !virtfb_server_secondary.is_set_up())
    {
      std::cerr << "Failed to set up virtual framebuffer servers\n";
      exit(EXIT_FAILURE);
    }

  auto virtfb_main = registry->register_obj(&virtfb_server_main,
                                            Virtfb_main_registry_name);
  if (!virtfb_main.is_valid())
    {
      std::cerr << "Failed to register main virtual framebuffer service\n";
      exit(EXIT_FAILURE);
    }

  auto virtfb_secondary = registry->register_obj(&virtfb_server_secondary,
                                                 Virtfb_secondary_registry_name);
  if (!virtfb_secondary.is_valid())
    {
      std::cerr << "Failed to register secondary virtual framebuffer service\n";
      exit(EXIT_FAILURE);
    }

  std::cout << "Set up virtual framebuffer service\n";

  // Set up framebuffer multiplexing server.
  Fb_mux_server fb_mux_server(fb, virtfb_ds_main, virtfb_ds_secondary);

  auto fb_mux = registry->register_obj(&fb_mux_server, Fb_mux_registry_name);
  if (!fb_mux.is_valid())
    {
      std::cerr << "Failed to register framebuffer multiplexing service\n";
      exit(EXIT_FAILURE);
    }

  std::cout << "Set up framebuffer multiplexing service\n";

  // Start server loop.
  registry_server.loop();
}
