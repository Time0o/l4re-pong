#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/cxx/ipc_stream>
#include <l4/sys/capability>
#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/util/util.h>

#include <iostream>
#include <string>

#include "paddle.h"

int
Paddle::move(int current_pos)
{
  bool key_up_held, key_down_held;

  L4::Ipc::Iostream ios(l4_utcb());
  ios << 1UL;
  ios << current_pos;
  ios.call(_paddle_cap_idx);

  key_up_held = key_down_held = false;
  _keyboard_cap->is_held(_key_up.c_str(), &key_up_held);
  _keyboard_cap->is_held(_key_down.c_str(), &key_down_held);

  if (key_up_held && !key_down_held)
    current_pos = current_pos - Paddle_speed;
  else if (key_down_held && !key_up_held)
    current_pos = current_pos + Paddle_speed;

  if (current_pos > Paddle_max_pos)
    current_pos = Paddle_max_pos;

  if (current_pos < Paddle_min_pos)
    current_pos = Paddle_min_pos;

  return current_pos;
}
