#include <l4/fb-log/fb_log.h>

#include <l4/sys/cxx/ipc_string>
#include <l4/sys/err.h>

#include <iostream>
#include <string>

#include "fb_log_server.h"

int Fb_log_server::op_log(Fb_log::Rights, L4::Ipc::String<> msg)
{
  std::string msg_str(msg.data, msg.length - 1u);
  msg_str = _tag + "| " + msg_str;

  if (_fb_textbox.active())
    _fb_textbox.print_line(msg_str, _color);
  else
    std::cout << msg_str << '\n';

  return L4_EOK;
}
