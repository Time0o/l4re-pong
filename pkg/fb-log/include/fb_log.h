#pragma once

#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_string>
#include <l4/sys/kobject>

enum {Fb_log_proto = 0x42};

struct Fb_log : public L4::Kobject_t<Fb_log, L4::Kobject, Fb_log_proto>
{
  L4_INLINE_RPC(int, log, (L4::Ipc::String<> msg));
  typedef L4::Typeid::Rpcs<log_t> Rpcs;
};
