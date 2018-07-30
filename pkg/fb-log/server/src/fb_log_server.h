#pragma once

#include <l4/fb-log/fb_log.h>

#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/cxx/ipc_string>

#include <string>

#include "fb_textbox.h"

enum {Fb_log_tag_max_len = 8};

class Fb_log_server : public L4::Epiface_t<Fb_log_server, Fb_log>
{
public:
  Fb_log_server(std::string const &tag, std::string const &color,
                Fb_textbox &fb_textbox) : _tag(tag), _color(color),
                                          _fb_textbox(fb_textbox) {}

  int op_log(Fb_log::Rights, L4::Ipc::String<> msg);

private:
  std::string _tag;
  std::string _color;
  Fb_textbox &_fb_textbox;
};
