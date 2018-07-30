#pragma once

#include <l4/re/util/video/goos_svr>
#include <l4/re/video/goos>
#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_epiface>

class Virtfb_server : public L4Re::Util::Video::Goos_svr,
                      public L4::Epiface_t<Virtfb_server, L4Re::Video::Goos>
{
public:
  Virtfb_server(L4::Cap<L4Re::Video::Goos> const &goos, L4::Cap<void> virtfb_ds);

  bool is_set_up() const { return _set_up; }

private:
  bool _set_up = false;
};
