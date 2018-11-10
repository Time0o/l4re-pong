#include <l4/re/util/br_manager>
#include <l4/re/util/object_registry>
#include <l4/sys/cxx/ipc_varg>
#include <l4/sys/cxx/ipc_types>
#include <l4/sys/err.h>
#include <l4/sys/l4int.h>

#include <set>
#include <string>

#include "keyboard_server.h"
#include "keyboard_session_server.h"

std::set<std::string> const Keyboard_session_server::recognized_keys {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "Q", "W", "E", "R", "T",
  "Y", "U", "I", "G", "H", "J", "K", "L", "Z", "O", "P", "A", "S", "D", "F",
  "X", "C", "V", "B", "N", "M"
};

void
Keyboard_session_server::hold_key(std::string const &key) const
{
  auto it = _listeners.find(key);
  if (it != _listeners.end())
    {
      for (auto const &server : it->second)
        server->hold_key(key);
    }
}

void
Keyboard_session_server::release_key(std::string const &key) const
{
  auto it = _listeners.find(key);
  if (it != _listeners.end())
    {
      for (auto const &server : it->second)
        server->release_key(key);
    }
}

int
Keyboard_session_server::op_create(
  L4::Factory::Rights rights, L4::Ipc::Cap<void> &res,
  l4_umword_t type, L4::Ipc::Varg_list<> &&args)
{
  (void)rights;

  if (type != Keyboard_session_proto)
    return -L4_ENODEV;

  // Read requested keys.
  std::set<std::string> requested_keys;

  L4::Ipc::Varg key = args.next();
  while (!key.is_nil())
    {
      if (!key.is_of<char const *>())
        return -L4_EINVAL;

      std::string key_str(key.value<char const *>());

      if (recognized_keys.find(key_str) == recognized_keys.end())
        return -L4_EINVAL;

      requested_keys.insert(key_str);

      key = args.next();
    }

  // Create and register keyboard server.
  Keyboard_server *keyboard_server = new Keyboard_server();

  for (std::string const &key : requested_keys)
    {
      auto it = _listeners.find(key);
      if (it == _listeners.end())
        _listeners[key] = {keyboard_server};
      else
        it->second.push_back(keyboard_server);
    }

  _registry_server.registry()->register_obj(keyboard_server);

  res = L4::Ipc::make_cap_rw(keyboard_server->obj_cap());

  return L4_EOK;
}
