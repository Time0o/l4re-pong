#include <l4/fb-mux/fb_mux.h>
#include <l4/keyboard-drv/keyboard_drv.h>
#include <l4/pong/logging.h>

#include <l4/cxx/exceptions>
#include <l4/cxx/ipc_stream>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>
#include <l4/sys/irq>
#include <l4/sys/types.h>
#include <pthread-l4.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <thread>

#include <getopt.h>

#include "paddle.h"

using L4Re::chksys;
using L4Re::chkcap;

enum {
  Paddle_connect_retry_timeout_ms = 100,
  Default_lifes = 10,
  Query_lifes_timeout = 100
};

namespace
{

char const* const Paddle_left_up_default = "F";
char const* const Paddle_left_down_default = "D";
char const* const Paddle_right_up_default = "K";
char const* const Paddle_right_down_default = "J";
char const* const Switch_console_default = "Q";

char const* const Pong_server_registry_name = "pongserver";
char const* const Keyboard_registry_name = "keyboard";
char const* const Fb_mux_registry_name = "fbmux";

bool quit = false; // Don't really need a mutex here.

L4::Cap<L4::Irq>
create_keyboard_irq(L4::Cap<Keyboard> const &keyboard)
{
  auto irq = chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>(),
                    "Failed to allocate IRQ capability slot");

  chksys(L4Re::Env::env()->factory()->create(irq),
         "Failed to create IRQ kernel object");

  chksys(keyboard->map_irq(irq),
         "Failed to map IRQ capability to keyboard server");

  return irq;
}

void
bind_keyboard_irq(L4::Cap<L4::Irq> const &irq, std::thread &thr, int label)
{
  L4::Cap<L4::Thread> thread_cap(pthread_l4_cap(thr.native_handle()));

  chksys(irq->bind_thread(thread_cap, label),
         "Failed to attach thread to keyboard IRQ");
}

void
query_keyboard(l4_cap_idx_t server_cap_idx,
               l4_cap_idx_t paddle_left_cap_idx,
               l4_cap_idx_t paddle_right_cap_idx,
               L4::Cap<Keyboard> const &keyboard,
               L4::Cap<L4::Irq> const &keyboard_irq,
               L4::Cap<Fb_mux> const &fbmux,
               std::string const &paddle_left_up,
               std::string const &paddle_left_down,
               std::string const &paddle_right_up,
               std::string const &paddle_right_down,
               std::string const &switch_console)
{
  Paddle paddle_left(server_cap_idx, paddle_left_cap_idx,
                     paddle_left_up, paddle_left_down);

  Paddle paddle_right(server_cap_idx, paddle_right_cap_idx,
                      paddle_right_up, paddle_right_down);

  Paddle::direction paddle_left_dir = Paddle::Direction_none;
  Paddle::direction paddle_right_dir = Paddle::Direction_none;

  auto move_paddle = [&](Paddle &paddle, Paddle::direction &dir,
                         std::string const &key_up, std::string const &key_down)
  {
    bool key_up_held, key_down_held;

    Paddle::direction new_dir = Paddle::Direction_none;

    chksys(keyboard->is_held(key_up.c_str(), &key_up_held),
           "Failed to query keyboard state");

    chksys(keyboard->is_held(key_down.c_str(), &key_down_held),
           "Failed to query keyboard state");

    if (key_up_held && !key_down_held)
      new_dir = Paddle::Direction_up;
    else if (key_down_held && !key_up_held)
      new_dir = Paddle::Direction_down;
    else
      new_dir = Paddle::Direction_none;

    if (new_dir != dir)
      {
        dir = new_dir;
        paddle.move(dir);
      }
  };

  try
    {
      while (!quit)
        {
          chksys(keyboard_irq->receive(), "Failed to receive keyboard IRQ");

          move_paddle(paddle_left, paddle_left_dir,
                      paddle_left_up, paddle_left_down);

          move_paddle(paddle_right, paddle_right_dir,
                      paddle_right_up, paddle_right_down);

          bool switch_console_held = false;
          chksys(keyboard->is_held(switch_console.c_str(), &switch_console_held),
                 "Failed to query keyboard state");

          if (switch_console_held)
            chksys(fbmux->switch_buffer(), "Failed to switch framebuffer");
        }
    }
  catch (L4::Runtime_error const &e)
    {
      std::cerr << "Paddle thread exited with exception: " << e.str() << '\n';
    }
}

void
run(int argc, char **argv)
{
  // Redirect output to framebuffer.
  redirect_to_log(std::cout);
  redirect_to_log(std::cerr);

  // Parse command line options.
  std::cout << "Parsing command line options\n";

  enum {
    Paddle_left_up, Paddle_left_down, Paddle_right_up, Paddle_right_down,
    Switch_console, Lifes, Endless
  };

  static struct option const long_options[] =
  {
    {"paddle-left-up", required_argument, nullptr, Paddle_left_up},
    {"paddle-left-down", required_argument, nullptr, Paddle_left_down},
    {"paddle-right-up", required_argument, nullptr, Paddle_right_up},
    {"paddle-right-down", required_argument, nullptr, Paddle_right_down},
    {"switch-console", required_argument, nullptr, Switch_console},
    {"lifes", required_argument, nullptr, Lifes},
    {"endless", no_argument, nullptr, Endless},
    {nullptr, 0, nullptr, 0}
  };

  std::string paddle_left_up(Paddle_left_up_default);
  std::string paddle_left_down(Paddle_left_down_default);
  std::string paddle_right_up(Paddle_right_up_default);
  std::string paddle_right_down(Paddle_right_down_default);
  std::string switch_console(Switch_console_default);

  int lifes = Default_lifes;
  bool endless = false;

  int c;
  while ((c = getopt_long(argc, argv, "", long_options, nullptr)) != -1)
    {
      switch (c)
        {
        case Paddle_left_up:
          paddle_left_up = optarg;
          break;
        case Paddle_left_down:
          paddle_left_down = optarg;
          break;
        case Paddle_right_up:
          paddle_right_up = optarg;
          break;
        case Paddle_right_down:
          paddle_right_down = optarg;
          break;
        case Switch_console:
          switch_console = optarg;
          break;
        case Lifes:
          {
            std::size_t idx;

            try
              {
                lifes = std::stoi(optarg, &idx);

                if (idx != strlen(optarg))
                  throw std::invalid_argument("Trailing garbage");
              }
            catch (std::invalid_argument const &e)
              {
                std::cerr << "Not a number: " << optarg << '\n';
                throw std::runtime_error("Failed to parse command line arguments");
              }
          }
          break;
        case Endless:
          endless = true;
          break;
        default:
          throw std::runtime_error("Failed to parse command line arguments");
        }
    }

  if (argv[optind])
    throw std::runtime_error("Trailing command line argument garbage");

  // Obtain capabilities.
  auto server = L4Re::Env::env()->get_cap<void>(Pong_server_registry_name);
  chkcap(server, "Failed to obtain pong server capability");

  auto keyboard = L4Re::Env::env()->get_cap<Keyboard>(Keyboard_registry_name);
  chkcap(keyboard, "Failed to obtain keyboard capability");

  auto fbmux = L4Re::Env::env()->get_cap<Fb_mux>(Fb_mux_registry_name);
  chkcap(fbmux, "Failed to obtain framebuffer mux capability");

  auto paddle_left = L4Re::Util::cap_alloc.alloc<void>();
  chkcap(paddle_left, "Failed to obtain paddle capability");

  auto paddle_right = L4Re::Util::cap_alloc.alloc<void>();
  chkcap(paddle_right, "Failed to obtain paddle capability");

  // Connect to pong server.
  std::cout << "Connecting to pong server\n";

  auto connect_paddle = [](L4::Cap<void> const &server,
                           L4::Cap<void> &paddle) -> l4_cap_idx_t
  {
    l4_cap_idx_t paddle_cap_idx = paddle.cap();

    for (;;)
      {
        L4::Ipc::Iostream ios(l4_utcb());
        ios << 1UL;
        ios << L4::Small_buf(paddle_cap_idx);

        if (l4_msgtag_has_error(ios.call(server.cap())))
          {
            if (l4_utcb_tcr()->error == L4_IPC_ENOT_EXISTENT)
              {
                 std::this_thread::sleep_for(std::chrono::milliseconds(
                   Paddle_connect_retry_timeout_ms));
              }
            else
              throw std::runtime_error("Failed to connect to paddle");
          }
        else
          return paddle_cap_idx;
     }
  };

  std::cout << "Connecting first paddle\n";
  auto paddle_left_cap_idx = connect_paddle(server, paddle_left);

  std::cout << "Connecting second paddle\n";
  auto paddle_right_cap_idx = connect_paddle(server, paddle_right);

  // Set initial player lifes.
  L4::Ipc::Iostream ios(l4_utcb());
  ios << 2UL;
  ios << lifes;

  if (l4_msgtag_has_error(ios.call(paddle_left_cap_idx))
      || l4_msgtag_has_error(ios.call(paddle_right_cap_idx)))
    {
      throw std::runtime_error("Failed to set player lifes");
    }

  std::cout << "Set player lifes\n";

  // Print welcome message.
  std::cout << "Welcome to pong!\n";

  // Start the game.
  auto keyboard_irq = create_keyboard_irq(keyboard);

  std::thread query_keyboard_thread(query_keyboard,
    server.cap(), paddle_left_cap_idx, paddle_right_cap_idx,
    keyboard, keyboard_irq, fbmux,
    paddle_left_up, paddle_left_down,
    paddle_right_up, paddle_right_down,
    switch_console);

  bind_keyboard_irq(keyboard_irq, query_keyboard_thread, 0);

  // Periodically query remeaning player lifes.
  auto query_lifes = [](l4_cap_idx_t paddle_cap_idx) -> int
  {
      L4::Ipc::Iostream ios(l4_utcb());
      ios << 3UL;

      if (!l4_msgtag_has_error(ios.call(paddle_cap_idx)))
        {
          int lifes;
          ios >> lifes;
          return lifes;
        }
      else
        return -1;
  };

  int paddle_left_lifes = 0;
  int paddle_right_lifes = 0;
  int tmp;

  if (endless)
    {
      for (;;)
        ;
    }

  for (;;)
    {
      tmp = query_lifes(paddle_left_cap_idx);
      if (tmp != -1 && tmp != paddle_left_lifes)
        {
          std::cout << "Player 1: " << tmp << " lifes remaining\n";

          if (tmp == 0)
            {
              std::cout << "Player 2 wins!\n";
              break;
            }

          paddle_left_lifes = tmp;
        }

      tmp = query_lifes(paddle_right_cap_idx);
      if (tmp != -1 && tmp != paddle_right_lifes)
        {
          std::cout << "Player 2: " << tmp << " lifes remaining\n";

          if (tmp == 0)
            {
              std::cout << "Player 1 wins!\n";
              break;
            }

          paddle_right_lifes = tmp;
        }

      std::this_thread::sleep_for(
        std::chrono::milliseconds(Query_lifes_timeout));
    }

  quit = true;

  query_keyboard_thread.join();
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
