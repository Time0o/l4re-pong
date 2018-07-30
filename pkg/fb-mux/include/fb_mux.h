#pragma once

#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_string>
#include <l4/sys/kobject>

enum {Fb_mux_proto = 0x75};

struct Fb_mux : public L4::Kobject_t<Fb_mux, L4::Kobject, Fb_mux_proto>
{
  L4_INLINE_RPC(int, switch_buffer, ());
  typedef L4::Typeid::Rpcs<switch_buffer_t> Rpcs;
};
