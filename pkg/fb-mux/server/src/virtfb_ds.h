#pragma once

#include <l4/re/dataspace>
#include <l4/re/util/dataspace_svr>
#include <l4/re/video/goos>
#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/sys/l4int.h>

class Virtfb_ds : public L4Re::Util::Dataspace_svr,
                  public L4::Epiface_t<Virtfb_ds, L4Re::Dataspace>
{
public:
  Virtfb_ds(L4::Cap<L4Re::Video::Goos> const &goos);

  bool is_valid() const { return _valid; }
  bool is_mapped() const { return _mapped; }

  void map(l4_addr_t fb_base_phys);
  void unmap();

private:
  bool _valid = false;
  bool _mapped = false;

  l4_addr_t _fb_base_swap;
  unsigned long _fb_size;
};
