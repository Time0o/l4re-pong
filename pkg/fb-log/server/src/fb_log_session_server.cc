#include <l4/re/util/br_manager>
#include <l4/re/util/object_registry>
#include <l4/sys/cxx/ipc_varg>
#include <l4/sys/cxx/ipc_types>
#include <l4/sys/err.h>
#include <l4/sys/l4int.h>

#include "fb_log_server.h"
#include "fb_log_session_server.h"
#include "fb_textbox.h"

namespace
{

std::string
maxlen_string(char const *str, unsigned maxlen)
{
  unsigned i = 0;
  while (i < maxlen && str[i++] != '\0') {}

  if (i < maxlen)
    return std::string(str);
  else
    return std::string(str, maxlen);
}

}

int Fb_log_session_server::op_create(
  L4::Factory::Rights rights, L4::Ipc::Cap<void> &res,
  l4_umword_t type, L4::Ipc::Varg_list<> &&args)
{
  (void)rights;

  if (type != Fb_log_session_proto)
    return -L4_ENODEV;

  // Read tag argument.
  L4::Ipc::Varg tag = args.next();
  if (!tag.is_of<char const *>())
    return -L4_EINVAL;

  char const *tag_arr = tag.value<char const*>();
  std::string tag_str(maxlen_string(tag_arr, Fb_log_tag_max_len));
  tag_str += std::string(Fb_log_tag_max_len - tag_str.length(), ' ');

  // Read (optional) color argument.
  std::string color_str(Fb_textbox::Default_text_fg_color);
  L4::Ipc::Varg color = args.next();
  if (!color.is_nil())
    {
      color_str = maxlen_string(color.value<char const*>(),
                                Fb_textbox::Text_color_max_strlen);
    }

  Fb_log_server *log_server = new Fb_log_server(tag_str, color_str, _fb_textbox);
  _registry_server.registry()->register_obj(log_server);

  res = L4::Ipc::make_cap_rw(log_server->obj_cap());

  return L4_EOK;
}
