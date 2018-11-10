#pragma once

#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_string>
#include <l4/sys/cxx/ipc_types>
#include <l4/sys/irq>
#include <l4/sys/kobject>

enum {Keyboard_proto = 0x53};

struct Keyboard : public L4::Kobject_t<Keyboard, L4::Kobject, Keyboard_proto,
                                       L4::Type_info::Demand_t<1>>
{
  L4_INLINE_RPC(int, map_irq, (L4::Ipc::Cap<L4::Irq> irq)); // TODO: restrict keys
  L4_INLINE_RPC(int, is_held, (L4::Ipc::String<> key, bool *res));

  typedef L4::Typeid::Rpcs<map_irq_t, is_held_t> Rpcs;
};
