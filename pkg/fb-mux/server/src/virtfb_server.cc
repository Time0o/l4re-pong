#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_epiface>

#include <iostream>

#include "virtfb_server.h"

Virtfb_server::Virtfb_server(L4::Cap<L4Re::Video::Goos> const &goos,
                             L4::Cap<void> virtfb_ds)
{
  // Obtain framebuffer information.
  L4Re::Util::Video::Goos_fb goos_fb(goos);
  if (goos_fb.view_info(&_view_info))
    {
      std::cerr << "Failed to read framebuffer information\n";
      return;
    }

  _fb_ds = L4::cap_cast<L4Re::Dataspace>(virtfb_ds);
  _set_up = true;
}
