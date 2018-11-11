#include <l4/cxx/exceptions>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/br_manager>
#include <l4/re/util/object_registry>
#include <l4/re/video/goos>

#include <cstdlib>
#include <iostream>

#include <getopt.h>

#include "fb_textbox.h"
#include "fb_log_session_server.h"

using L4Re::chkcap;

namespace
{

char const* const Framebuffer_registry_name = "fb";
char const* const Fb_log_session_registry_name = "fblog";

void
run(int argc, char **argv)
{
  static struct option const long_options[] =
  {
    {"bg-color", required_argument, nullptr, 'b'},
    {nullptr, 0, nullptr, 0}
  };

  std::string bg_color(Fb_textbox::Default_text_bg_color);

  int c;
  while ((c = getopt_long(argc, argv, "", long_options, nullptr)) != -1)
    {
      if (c == 'b')
        {
          bg_color = optarg;
          break;
        }
      else
        throw std::runtime_error("Failed to parse command line arguments");
    }

  if (argv[optind])
    throw std::runtime_error("Trailing command line argument garbage");

  // Create registry server.
  L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> registry_server;
  auto registry = registry_server.registry();

  // Initialize frame buffer textbox.
  auto fb =
    L4Re::Env::env()->get_cap<L4Re::Video::Goos>(Framebuffer_registry_name);

  chkcap(fb, "Failed to obtain fb capability");

  Fb_textbox fb_textbox(fb, bg_color);

  std::cout << "Initialized framebuffer\n";

  // Initialize and register log session server.
  Fb_log_session_server session_server(registry_server, fb_textbox);

  chkcap(registry->register_obj(&session_server, Fb_log_session_registry_name),
         "Failed to register service");

  std::cout << "Set up log session service\n";

  registry_server.loop();
}

}

int
main(int argc, char **argv)
{
  int exit_status;

  try
    {
      run(argc, argv);
      exit_status = EXIT_SUCCESS;
    }
  catch (L4::Runtime_error const &e)
    {
      std::cerr << e.str() << '\n';
      exit_status = EXIT_FAILURE;
    }
  catch (std::runtime_error const &e)
    {
      std::cerr << e.what() << '\n';
      exit_status = EXIT_FAILURE;
    }
  catch (...)
    {
      std::cerr << "Uncaught exception\n";
      exit_status = EXIT_FAILURE;
    }

  exit(exit_status);
}
