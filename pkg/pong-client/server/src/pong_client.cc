#include <l4/fb-mux/fb_mux.h>
#include <l4/keyboard-drv/keyboard_drv.h>
#include <l4/pong/logging.h>

#include <l4/cxx/ipc_stream>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/sys/types.h>
#include <l4/util/util.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include <getopt.h>

#include "paddle.h"

enum {
  Paddle_connect_retry_timeout_ms = 100,
  Paddle_move_timeout = 10,
  Paddle_start_pos = 180,
  Switch_console_read_timeout = 10,
  Switch_console_cooldown = 100,
  Default_lifes = 10,
  Query_lifes_timeout = 100
};

static char const* const Paddle_left_up_default = "F";
static char const* const Paddle_left_down_default = "D";
static char const* const Paddle_right_up_default = "K";
static char const* const Paddle_right_down_default = "J";
static char const* const Switch_console_default = "Q";

static char const* const Pong_server_registry_name = "pongserver";
static char const* const Keyboard_registry_name = "keyboard";
static char const* const Fb_mux_registry_name = "fbmux";

static bool quit = false; // Don't really need a mutex here.

static void
new_paddle(l4_cap_idx_t server_cap_idx, l4_cap_idx_t paddle_cap_idx,
           L4::Cap<Keyboard> keyboard, std::string key_up, std::string key_down)
{
  Paddle p(server_cap_idx, paddle_cap_idx, keyboard, key_up, key_down);

  int pos = Paddle_start_pos;

  while (!quit)
    {
      pos = p.move(pos);

      l4_sleep(Paddle_move_timeout);
    }
}

static void
query_console_switch(std::string switch_console, L4::Cap<Fb_mux> fbmux,
                     L4::Cap<Keyboard> keyboard)
{
  bool switch_console_held;

  while (!quit)
    {
      switch_console_held = false;
      keyboard->is_held(switch_console.c_str(), &switch_console_held);

      if (switch_console_held)
        {
          fbmux->switch_buffer();

          l4_sleep(Switch_console_cooldown);
        }

      l4_sleep(Switch_console_read_timeout);
    }
}

int
main(int argc, char **argv)
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
                exit(EXIT_FAILURE);
              }
          }
          break;
        case Endless:
          endless = true;
          break;
        default:
          exit(EXIT_FAILURE);
        }
    }

  if (argv[optind])
    {
      std::cerr << "Trailing command line argument garbage\n";
      exit(EXIT_FAILURE);
    }

  // Obtain capabilities.
  auto server = L4Re::Env::env()->get_cap<void>(Pong_server_registry_name);
  if (!server.is_valid())
    {
      std::cerr << "Failed to obtain pong server capability\n";
      exit(EXIT_FAILURE);
    }

  auto keyboard = L4Re::Env::env()->get_cap<Keyboard>(Keyboard_registry_name);
  if (!keyboard.is_valid())
    {
      std::cerr << "Failed to obtain keyboard capability\n";
      exit(EXIT_FAILURE);
    }

  auto fbmux = L4Re::Env::env()->get_cap<Fb_mux>(Fb_mux_registry_name);
  if (!fbmux.is_valid())
    {
      std::cerr << "Failed to obtain framebuffer mux capability\n";
      exit(EXIT_FAILURE);
    }

  auto paddle1 = L4Re::Util::cap_alloc.alloc<void>();
  auto paddle2 = L4Re::Util::cap_alloc.alloc<void>();
  if (!paddle1.is_valid() || !paddle2.is_valid())
    {
      std::cerr << "Failed to allocate paddle capabilities\n";
      exit(EXIT_FAILURE);
    }

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
              l4_sleep(Paddle_connect_retry_timeout_ms);
            else
              {
                std::cerr << "Failed to connect to paddle\n";
                exit(EXIT_FAILURE);
              }
          }
        else
          return paddle_cap_idx;
     }
  };

  std::cout << "Connecting first paddle\n";
  auto paddle1_cap_idx = connect_paddle(server, paddle1);

  std::cout << "Connecting second paddle\n";
  auto paddle2_cap_idx = connect_paddle(server, paddle2);

  // Set initial player lifes.
  L4::Ipc::Iostream ios(l4_utcb());
  ios << 2UL;
  ios << lifes;

  if (l4_msgtag_has_error(ios.call(paddle1_cap_idx))
      || l4_msgtag_has_error(ios.call(paddle2_cap_idx)))
    {
      std::cerr << "Failed to set player lifes\n";
      exit(EXIT_FAILURE);
    }

  std::cout << "Set player lifes\n";

  // Print welcome message.
  std::cout << "Welcome to pong!\n";

  // Create paddles.
  std::cout << "Starting first paddle thread\n";
  std::thread paddle1_thread(new_paddle, server.cap(), paddle1_cap_idx,
                             keyboard, paddle_left_up, paddle_left_down);

  std::cout << "Starting second paddle thread\n";
  std::thread paddle2_thread(new_paddle, server.cap(), paddle2_cap_idx,
                             keyboard, paddle_right_up, paddle_right_down);

  // Enable console switching.
  std::cout << "Enabling console switching\n";
  std::thread query_console_switch_thread(query_console_switch, switch_console,
                                          fbmux, keyboard);

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

  int paddle1_lifes = 0;
  int paddle2_lifes = 0;
  int tmp;

  if (endless)
    {
      for (;;)
        ;
    }

  for (;;)
    {
      tmp = query_lifes(paddle1_cap_idx);
      if (tmp != -1 && tmp != paddle1_lifes)
        {
          std::cout << "Player 1: " << tmp << " lifes remaining\n";

          if (tmp == 0)
            {
              std::cout << "Player 2 wins!\n";
              break;
            }

          paddle1_lifes = tmp;
        }

      tmp = query_lifes(paddle2_cap_idx);
      if (tmp != -1 && tmp != paddle2_lifes)
        {
          std::cout << "Player 2: " << tmp << " lifes remaining\n";

          if (tmp == 0)
            {
              std::cout << "Player 1 wins!\n";
              break;
            }

          paddle2_lifes = tmp;
        }

      l4_sleep(Query_lifes_timeout);
    }

  quit = true;

  paddle1_thread.join();
  paddle2_thread.join();
  query_console_switch_thread.join();

  L4Re::Util::cap_alloc.free(paddle1);
  L4Re::Util::cap_alloc.free(paddle2);

  exit(EXIT_SUCCESS);
};
