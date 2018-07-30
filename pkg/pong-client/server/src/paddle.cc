#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/cxx/ipc_stream>
#include <l4/sys/capability>
#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/util/util.h>

#include <iostream>
#include <string>

#include "paddle.h"

Paddle::Paddle(unsigned id, l4_cap_idx_t server_idx, l4_cap_idx_t paddle_idx,
               L4::Cap<Keyboard> const &keyboard_cap,
               std::string const &key_up, std::string const &key_down)

  : _id(id), _server_idx(server_idx), _paddle_idx(paddle_idx),
    _keyboard_cap(keyboard_cap), _key_up(key_up), _key_down(key_down)
{
  // Connect to pong server.
  for (;;)
    {
      out() << "Connecting to pong server...\n";

      L4::Ipc::Iostream ios(l4_utcb());
      ios << 1UL;
      ios << L4::Small_buf(_paddle_idx);

      if (!l4_msgtag_has_error(ios.call(server_idx)))
        {
          out() << "Successfully connected to paddle\n";
          _connected = true;
          break;
        }
      else
        {
          switch (l4_utcb_tcr()->error)
            {
            case L4_IPC_ENOT_EXISTENT:
              err() << "Did not find paddle, retrying...\n";
              l4_sleep(Paddle_connect_retry_timeout_ms);
              ios.reset();
              break;
            default:
              err() << "Failed to find paddle\n";
              return;
            }
        }
   }
}

void
Paddle::loop()
{
  // Set initial paddle lives.
  //out() << "Setting initial lifes" << std::endl;

  //_ios << 2UL;
  //_ios << Paddle_initial_lifes;
  //_ios.call(_paddle_idx);

  // Control paddle movement.
  out() << "Starting movement loop\n";

  int pos = Paddle_start_pos;
  bool key_up_held, key_down_held;

  for (;;)
    {
      // XXX query lifes

      L4::Ipc::Iostream ios(l4_utcb());
      ios << 1UL;
      ios << pos;
      if (l4_msgtag_has_error(ios.call(_paddle_idx)))
        err() << "Error while sending updated paddle positiion to server\n";

      l4_sleep(Paddle_move_timeout);

      key_up_held = key_down_held = false;

      if (_keyboard_cap->is_held(_key_up.c_str(), &key_up_held) != L4_EOK)
        err() << "Error while querying 'up' key\n";

      if (_keyboard_cap->is_held(_key_down.c_str(), &key_down_held) != L4_EOK)
        err() << "Error while querying 'up' key\n";

      if (key_up_held && !key_down_held)
        pos = pos - Paddle_speed;
      else if (key_down_held && !key_up_held)
        pos = pos + Paddle_speed;

      if (pos > Paddle_max_pos)
        pos = Paddle_max_pos;

      if (pos < Paddle_min_pos)
        pos = Paddle_min_pos;
    }
}

std::ostream &
Paddle::out() const
{
  std::cout << "Paddle " << _id << ": ";
  return std::cout;
}

std::ostream &
Paddle::err() const
{
  std::cerr << "Paddle " << _id << ": ";
  return std::cerr;
}
