#pragma once

#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/cxx/ipc_string>
#include <l4/sys/cxx/ipc_types>
#include <l4/sys/irq>

#include <mutex>
#include <set>
#include <string>
#include <vector>

class Keyboard_server : public L4::Epiface_t<Keyboard_server, Keyboard>
{
public:
  void hold_key(std::string const &key);
  void release_key(std::string const &key);

  int op_map_irq(Keyboard::Rights, L4::Ipc::Snd_fpage const &irq);
  int op_is_held(Keyboard::Rights, L4::Ipc::String<> key, bool &res);

private:
  void trigger_irqs();

  std::vector<L4::Cap<L4::Irq>> _irqs;
  std::set<std::string> _held_keys;
  std::mutex _mutex;
};
