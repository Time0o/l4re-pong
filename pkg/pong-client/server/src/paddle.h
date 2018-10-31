#pragma once

#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/sys/capability>
#include <l4/sys/types.h>

#include <ostream>
#include <string>

class Paddle
{
enum {
  Paddle_min_pos = 0,
  Paddle_max_pos = 1023,
  Paddle_speed = 10,
  Paddle_initial_lifes = 5
};

public:
  Paddle(l4_cap_idx_t server_cap_idx, l4_cap_idx_t paddle_cap_idx,
         L4::Cap<Keyboard> const &keyboard_cap,
         std::string const &key_up, std::string const &key_down)

    : _server_cap_idx(server_cap_idx), _paddle_cap_idx(paddle_cap_idx),
      _keyboard_cap(keyboard_cap), _key_up(key_up), _key_down(key_down) {}

  int move(int current_pos);

private:
  unsigned _id;

  l4_cap_idx_t _server_cap_idx;
  l4_cap_idx_t _paddle_cap_idx;
  L4::Cap<Keyboard> _keyboard_cap;

  std::string _key_up, _key_down;
};
