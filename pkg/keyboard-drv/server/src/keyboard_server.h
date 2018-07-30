#pragma once

#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/cxx/ipc_string>
#include <l4/sys/err.h>

#include <set>
#include <string>
#include <vector>

class Keyboard_server : public L4::Epiface_t<Keyboard_server, Keyboard>
{
public:
  void hold_key(std::string const &key);
  void release_key(std::string const &key);

  int op_is_held(Keyboard::Rights, L4::Ipc::String<> key, bool &res);

private:
  bool is_key(std::string const &key) const;
  bool is_held(std::string const &key);
  std::vector<std::string> held_keys();

  std::set<std::string> _held_keys;
};
