#pragma once

#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/sys/capability>
#include <l4/sys/types.h>

#include <ostream>
#include <string>

class Paddle
{
enum {Paddle_connect_retry_timeout_ms = 100, Paddle_move_timeout = 10,
      Paddle_start_pos = 180, Paddle_min_pos = 0, Paddle_max_pos = 1023,
      Paddle_speed = 10, Paddle_initial_lifes = 5};

public:
  Paddle(unsigned id, l4_cap_idx_t server_idx, l4_cap_idx_t paddle_idx,
         L4::Cap<Keyboard> const &keyboard_cap,
         std::string const &key_up, std::string const &key_down);

  bool connected() const { return _connected; }
  void loop();

private:
  std::ostream &out() const;
  std::ostream &err() const;

  unsigned _id;
  bool _connected = false;

  l4_cap_idx_t _server_idx, _paddle_idx;
  L4::Cap<Keyboard> _keyboard_cap;

  std::string _key_up, _key_down;
};
