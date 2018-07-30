#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/sys/cxx/ipc_string>
#include <l4/sys/err.h>

#include <cctype>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "keyboard_server.h"

static std::set<std::string> const recognized_keys {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "Q", "W", "E", "R", "T",
  "Y", "U", "I", "G", "H", "J", "K", "L", "Z", "O", "P", "A", "S", "D", "F",
  "X", "C", "V", "B", "N", "M"
};

static std::mutex key_lock;

void Keyboard_server::hold_key(std::string const &key)
{
  key_lock.lock();
  _held_keys.insert(key);
  key_lock.unlock();
}

void Keyboard_server::release_key(std::string const &key)
{
  key_lock.lock();
  _held_keys.erase(key);
  key_lock.unlock();
}

std::vector<std::string> Keyboard_server::held_keys()
{
  key_lock.lock();
  std::vector<std::string> res(_held_keys.begin(), _held_keys.end());
  key_lock.unlock();

  return res;
}

int Keyboard_server::op_is_held(Keyboard::Rights, L4::Ipc::String<> key,
                                bool &res)
{
  std::string key_str(key.data, key.length - 1u);

  for (auto &c : key_str)
    c = std::toupper(c);

  if (!is_key(key_str))
    return L4_EINVAL;

  res = is_held(key_str);

  return L4_EOK;
}

bool Keyboard_server::is_key(std::string const &key) const
{
  return recognized_keys.find(key) != recognized_keys.end();
}

bool Keyboard_server::is_held(std::string const &key)
{
  key_lock.lock();
  bool res = _held_keys.find(key) != _held_keys.end();
  key_lock.unlock();

  return res;
}
