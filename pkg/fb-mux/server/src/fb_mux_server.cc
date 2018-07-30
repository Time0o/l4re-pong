#include <l4/fb-mux/fb_mux.h>

#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/sys/capability>
#include <l4/sys/err.h>
#include <l4/sys/l4int.h>

#include "fb_mux_server.h"
#include "virtfb_ds.h"

Fb_mux_server::Fb_mux_server(L4::Cap<L4Re::Video::Goos> const &goos,
                             Virtfb_ds &virtfb_ds_main,
                             Virtfb_ds &virtfb_ds_secondary)
  : _virtfb_ds_main(virtfb_ds_main), _virtfb_ds_secondary(virtfb_ds_secondary)
{
  L4Re::Util::Video::Goos_fb goos_fb(goos);
  _fb_base_phys = reinterpret_cast<l4_addr_t>(goos_fb.attach_buffer());

  _virtfb_ds_main.map(_fb_base_phys);
}

int Fb_mux_server::op_switch_buffer(Fb_mux::Rights rights)
{
  (void)rights;

  if (_virtfb_ds_main.is_mapped())
    {
      _virtfb_ds_main.unmap();
      _virtfb_ds_secondary.map(_fb_base_phys);
    }
  else
    {
      _virtfb_ds_secondary.unmap();
      _virtfb_ds_main.map(_fb_base_phys);
    }

  return L4_EOK;
}
