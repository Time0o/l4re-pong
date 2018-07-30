#pragma once

#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_string>
#include <l4/sys/kobject>

enum {Keyboard_proto = 0x53};

struct Keyboard : public L4::Kobject_t<Keyboard, L4::Kobject, Keyboard_proto>
{
  L4_INLINE_RPC(int, is_held, (L4::Ipc::String<> key, bool *res));
  typedef L4::Typeid::Rpcs<is_held_t> Rpcs;
};
