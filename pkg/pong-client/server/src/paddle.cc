#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/cxx/ipc_stream>
#include <l4/sys/capability>
#include <l4/sys/err.h>
#include <l4/sys/types.h>

#include <chrono>
#include <string>
#include <thread>

#include "paddle.h"

void
Paddle::move(direction dir)
{
  _dir_mutex.lock();
  _dir = dir;
  _dir_mutex.unlock();

  if (_dir == Direction_none && _moving)
    {
      _moving = false;
      _move_thread.join();
    }
  else if (!_moving)
    {
      _moving = true;
      _move_thread = std::thread(&Paddle::_move, this);
    }
}

void
Paddle::_move()
{
  direction tmp;

  bool stop = false;

  while (!stop)
    {
      _dir_mutex.lock();
      tmp = _dir;
      _dir_mutex.unlock();

      if (tmp == Direction_none)
        break;

      switch (tmp)
        {
        case Direction_none:
          stop = true;
          break;
        case Direction_up:
          _current_pos -= Paddle_speed;
          if (_current_pos < Paddle_min_pos)
            _current_pos = Paddle_min_pos;
          break;
        case Direction_down:
          _current_pos += Paddle_speed;
          if (_current_pos > Paddle_max_pos)
            _current_pos = Paddle_max_pos;
          break;
        }

      L4::Ipc::Iostream ios(l4_utcb());
      ios << 1UL;
      ios << _current_pos;
      ios.call(_paddle_cap_idx);

      std::this_thread::sleep_for(
        std::chrono::milliseconds(Paddle_move_timeout));
    }
}
