#pragma once

#include <l4/fb-mux/fb_mux.h>

#include <l4/re/video/goos>
#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/l4int.h>

#include "virtfb_ds.h"

class Fb_mux_server : public L4::Epiface_t<Fb_mux_server, Fb_mux>
{
public:
  Fb_mux_server(L4::Cap<L4Re::Video::Goos> const &goos,
                Virtfb_ds &virtfb_ds_main, Virtfb_ds &virtfb_ds_secondary);

  int op_switch_buffer(Fb_mux::Rights rights);

private:
  Virtfb_ds &_virtfb_ds_main;
  Virtfb_ds &_virtfb_ds_secondary;

  l4_addr_t _fb_base_phys;
};
