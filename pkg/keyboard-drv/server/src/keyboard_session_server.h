#pragma once

#include <l4/re/util/br_manager>
#include <l4/re/util/object_registry>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/cxx/ipc_varg>
#include <l4/sys/cxx/ipc_types>
#include <l4/sys/l4int.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "keyboard_server.h"

class Keyboard_session_server
  : public L4::Epiface_t<Keyboard_session_server, L4::Factory>
{
enum {Keyboard_session_proto = 0};

static std::set<std::string> const recognized_keys;

public:
  Keyboard_session_server(
    L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> &registry_server)
      : _registry_server(registry_server) {}

  void hold_key(std::string const &key) const;
  void release_key(std::string const &key) const;

  int op_create(L4::Factory::Rights rights, L4::Ipc::Cap<void> &res,
                l4_umword_t type, L4::Ipc::Varg_list<> &&args);

private:
  L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> &_registry_server;
  std::map<std::string, std::vector<Keyboard_server *>> _listeners;
};
