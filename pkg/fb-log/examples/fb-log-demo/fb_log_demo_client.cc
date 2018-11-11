#include <l4/fb-log/fb_log.h>

#include <l4/cxx/exceptions>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/sys/capability>
#include <l4/sys/err.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

using L4Re::chkcap;

enum {Fb_log_timeout = 250};

namespace
{

char const *const Fb_log_session_registry_name = "fblog";

void
run()
{
  auto log = L4Re::Env::env()->get_cap<Fb_log>(Fb_log_session_registry_name);
  chkcap(log, "Failed to obtain log capability");

  int counter = 0;
  for (;;)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(Fb_log_timeout));

      std::string msg("hello no. ");
      msg += std::to_string(counter++);

      if (log->log(msg.c_str()) != L4_EOK)
        std::cerr << "Error while talking to log server\n";
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
