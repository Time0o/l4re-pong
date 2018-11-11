#pragma once

#include <l4/fb-log/fb_log.h>

#include <l4/re/env>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

char const* const Fb_log_registry_name = "fblog";

}

class Log_buffer : public std::basic_streambuf<char>
{
public:
  Log_buffer() {}

  Log_buffer(std::basic_ios<char> &os, L4::Cap<Fb_log> fblog) : _fblog(fblog)
  {
    _sb = os.rdbuf(this);
  }

protected:
  int_type overflow(int_type m) override
  {
    char ch = traits_type::to_char_type(m);

    if (ch == '\n')
      {
        _fblog->log(_buffer.str().c_str());
        _buffer.str(std::string());
      }
    else
      _buffer << ch;

    return _sb->sputc(m);
  }

private:
  std::basic_streambuf<char> *_sb;
  std::stringstream _buffer;
  L4::Cap<Fb_log> _fblog;
};

void
redirect_to_log(std::basic_ios<char> &os);

void
redirect_to_log(std::basic_ios<char> &os)
{
  auto fblog = L4Re::Env::env()->get_cap<Fb_log>(Fb_log_registry_name);
  if (!fblog.is_valid())
    {
      fprintf(stderr, "Redirection failed\n");
      return;
    }

  // We can't really avoid this memory leak without changing the pong server.
  Log_buffer *log_buffer = new Log_buffer(os, fblog);
  (void)log_buffer;
}
