#include <l4/fb-mux/fb_mux.h>
#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/sys/err.h>
#include <l4/util/util.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <getopt.h>

#include "paddle.h"

enum {Switch_console_read_timeout = 10, Switch_console_cooldown = 100};

static char const* const Paddle_left_up_default = "F";
static char const* const Paddle_left_down_default = "D";
static char const* const Paddle_right_up_default = "K";
static char const* const Paddle_right_down_default = "J";
static char const* const Switch_console_default = "Q";

static char const* const Pong_server_registry_name = "pongserver";
static char const* const Keyboard_registry_name = "keyboard";
static char const* const Fb_mux_registry_name = "fbmux";

static void
new_paddle(unsigned id, std::string key_up, std::string key_down,
           L4::Cap<Keyboard> keyboard)
{
  auto server = L4Re::Env::env()->get_cap<void>(Pong_server_registry_name);
  auto paddle = L4Re::Util::cap_alloc.alloc<void>();

  if (!server.is_valid())
    std::cerr << "Failed to obtain pong server capability\n";
  else if (!paddle.is_valid())
    std::cerr << "Failed to allocate paddle capability\n";
  else
    {
      Paddle paddle_instance(id, server.cap(), paddle.cap(), keyboard,
                             key_up, key_down);

      if (paddle_instance.connected())
        paddle_instance.loop();

      for (;;)
        ;
    }
}

int
main(int argc, char **argv)
{
  enum {
    Paddle_left_up, Paddle_left_down, Paddle_right_up, Paddle_right_down,
    Switch_console
  };

  static struct option const long_options[] =
  {
    {"paddle-left-up", required_argument, nullptr, Paddle_left_up},
    {"paddle-left-down", required_argument, nullptr, Paddle_left_down},
    {"paddle-right-up", required_argument, nullptr, Paddle_right_up},
    {"paddle-right-down", required_argument, nullptr, Paddle_right_down},
    {"switch-console", required_argument, nullptr, Switch_console},
    {nullptr, 0, nullptr, 0}
  };

  std::string paddle_left_up(Paddle_left_up_default);
  std::string paddle_left_down(Paddle_left_down_default);
  std::string paddle_right_up(Paddle_right_up_default);
  std::string paddle_right_down(Paddle_right_down_default);
  std::string switch_console(Switch_console_default);

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
        default:
          exit(EXIT_FAILURE);
        }
    }

  if (argv[optind])
    {
      std::cerr << "Trailing command line argument garbage\n";
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

  std::cout << "Creating paddles...\n";

  std::thread paddle_thread1(new_paddle, 1u, paddle_left_up, paddle_left_down,
                             keyboard);

  std::thread paddle_thread2(new_paddle, 2u, paddle_right_up, paddle_right_down,
                             keyboard);

  bool switch_console_held;
  for (;;)
    {
      l4_sleep(Switch_console_read_timeout);

      switch_console_held = false;
      if (keyboard->is_held(switch_console.c_str(),
                            &switch_console_held) != L4_EOK)
        {
          std::cerr << "Error while querying 'switch to console' key\n";
        }

      if (switch_console_held)
        {
          if (fbmux->switch_buffer() != L4_EOK)
            std::cerr << "Error while talking to framebuffer mux server\n";

          l4_sleep(Switch_console_cooldown);
        }
    }
};
