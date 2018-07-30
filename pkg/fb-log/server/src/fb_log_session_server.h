#pragma once

#include <l4/re/util/br_manager>
#include <l4/re/util/object_registry>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/cxx/ipc_varg>
#include <l4/sys/cxx/ipc_types>
#include <l4/sys/l4int.h>

#include "fb_textbox.h"

class Fb_log_session_server
  : public L4::Epiface_t<Fb_log_session_server, L4::Factory>
{
enum {Fb_log_session_proto = 0};

public:
  Fb_log_session_server(
    L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> &registry_server,
    Fb_textbox &fb_textbox)
      : _registry_server(registry_server), _fb_textbox(fb_textbox) {}

  int op_create(L4::Factory::Rights rights, L4::Ipc::Cap<void> &res,
                l4_umword_t type, L4::Ipc::Varg_list<> &&args);

private:
  L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> &_registry_server;
  Fb_textbox &_fb_textbox;
};
