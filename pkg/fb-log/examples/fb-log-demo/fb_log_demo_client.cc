#include <l4/fb-log/fb_log.h>

#include <l4/re/env>
#include <l4/sys/capability>
#include <l4/sys/err.h>
#include <l4/util/util.h>

#include <cstdlib>
#include <iostream>
#include <string>

enum {Fb_log_timeout = 250};

char const *const Fb_Log_session_server_registry_name = "fblog";

int
main()
{
  L4::Cap<Fb_log> log =
    L4Re::Env::env()->get_cap<Fb_log>(Fb_Log_session_server_registry_name);

  if (!log.is_valid())
    exit(EXIT_FAILURE);

  int counter = 0;
  for (;;)
    {
      l4_sleep(Fb_log_timeout);

      std::string msg("hello no. ");
      msg += std::to_string(counter++);

      if (log->log(msg.c_str()) != L4_EOK)
        std::cerr << "Error while talking to log server\n";
    }

  exit(EXIT_SUCCESS);
}
