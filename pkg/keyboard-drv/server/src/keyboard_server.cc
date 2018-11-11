#include <l4/keyboard-drv/keyboard_drv.h>

#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/cxx/ipc_string>
#include <l4/sys/err.h>
#include <l4/sys/irq>

#include <string>
#include <vector>

#include "keyboard_server.h"

void
Keyboard_server::hold_key(std::string const &key)
{
  _mutex.lock();

  auto p = _held_keys.insert(key);
  if (p.second)
    trigger_irqs();

  _mutex.unlock();
}

void
Keyboard_server::release_key(std::string const &key)
{
  _mutex.lock();

  if (_held_keys.erase(key) > 0)
    trigger_irqs();

  _mutex.unlock();
}

int
Keyboard_server::op_map_irq(Keyboard::Rights, L4::Ipc::Snd_fpage const &fpage)
{
  if (!fpage.cap_received())
    return -L4_EMSGMISSARG;

  auto irq = server_iface()->rcv_cap<L4::Irq>(0);
  if (!irq.is_valid())
    return -L4_EINVAL;

  if (server_iface()->realloc_rcv_cap(0) < 0)
    return -L4_ENOMEM;

  _mutex.lock();
  _irqs.push_back(irq);
  _mutex.unlock();

  return L4_EOK;
}

int
Keyboard_server::op_is_held(Keyboard::Rights, L4::Ipc::String<> key, bool &res)
{
  std::string key_str(key.data, key.length - 1u);

  for (auto &c : key_str)
    c = std::toupper(c);

  _mutex.lock();
  res = _held_keys.find(key_str) != _held_keys.end();
  _mutex.unlock();

  return L4_EOK;
}

void
Keyboard_server::trigger_irqs()
{
  for (auto const &irq : _irqs)
    irq->trigger();
}
