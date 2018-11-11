#include <l4/libgfxbitmap/font.h>
#include <l4/re/env>
#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/sys/capability>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "fb_textbox.h"

namespace
{

bool
gfxcolor(std::string const &name, unsigned *color)
{
  struct color_rgb {
    char const* name;
    uint8_t r, g, b;
  };

  color_rgb const colors[] = {
    {"white", 255, 255, 255},
    {"black", 0, 0, 0},
    {"red", 255, 0, 0},
    {"green", 0, 255, 0},
    {"blue", 0, 0, 255},
    {"yellow", 255, 255, 0},
    {"magenta", 255, 0, 255},
    {"teal", 0, 255, 255}
  };

  for (color_rgb col : colors)
    {
      if (col.name == name)
        {
          *color = ((col.r & 0xf8) << 8)
                 | ((col.g & 0xfc) << 3)
                 | (((col.b & 0xf8) >> 3) & 0xf8);

          return true;
        }
    }

  return false;
}

}

Fb_textbox::Fb_textbox(L4::Cap<L4Re::Video::Goos> fb,
                       std::string const &text_bg_color,
                       unsigned border_left, unsigned width,
                       unsigned border_top, unsigned height)
  : _goos_fb(fb), _border_left(border_left), _border_top(border_top)
{
  std::exception_ptr eptr;

  try
    {
      // Read framebuffer information.
      _fb_base = _goos_fb.attach_buffer();

      if (_goos_fb.view_info(&_fb_info))
        throw std::runtime_error("Could not read framebuffer information\n");

      // Initialize font.
      if (gfxbitmap_font_init())
        throw std::runtime_error("Failed to initialize font rendering\n");

      if (!(_text_font_height = gfxbitmap_font_height(_text_font)))
        throw std::runtime_error("Failed to determine font height\n");

      if (!(_text_font_width = gfxbitmap_font_width(_text_font)))
        throw std::runtime_error("Failed to determine font width\n");

      if (!gfxcolor(text_bg_color.c_str(), &_text_bg_color))
        gfxcolor(Default_text_bg_color, &_text_bg_color);

      // Determine textbox dimensions.
      auto round_to_multiple = [=](unsigned dim, unsigned multiple) {
        return ((dim + multiple - 1) / multiple) * multiple;
      };

      width = round_to_multiple(width, _text_font_width);
      height = round_to_multiple(height, _text_font_height);

      unsigned fb_height = static_cast<unsigned>(_fb_info.height);
      unsigned fb_width = static_cast<unsigned>(_fb_info.width);

      if (height == 0u)
        _border_bottom = fb_height;
      else
        _border_bottom = std::min(_border_top + height, fb_height);

      _border_bottom -= _border_bottom % _text_font_height;

      if (width == 0u)
        _border_right = fb_width;
      else
        _border_right = std::min(_border_left + width, fb_width);

      _border_right -= _border_right % _text_font_width;

      _displayable_lines = (_border_bottom - _border_top) / _text_font_height;
      _displayable_columns = (_border_right - _border_left) / _text_font_width;

      // Clear screen.
      clear_screen();

      // Indicate availability of framebuffer textbox.
      _active = true;
    }
  catch (...)
    {
      _active = false;
      eptr = std::current_exception();
    }

  if (eptr)
    {
      try
        {
          std::rethrow_exception(eptr);
        }
      catch (std::exception const &e)
        {
          std::cout << "Failed to activate framebuffer textbox: "
                    << e.what() << '\n';
        }
    }
}

void
Fb_textbox::print_line(std::string const &msg, std::string const &color)
{
  unsigned text_fg_color;
  if (!gfxcolor(color, &text_fg_color))
    gfxcolor(Default_text_fg_color, &text_fg_color);

  _print_lock.lock();

  if (_current_line == _displayable_lines)
    {
      scroll_up();
      display(msg, text_fg_color, _current_line - 1u);
    }
  else
    {
      display(msg, text_fg_color, _current_line);
      ++_current_line;
    }

  _history.push_back({msg, text_fg_color});

  _print_lock.unlock();
}

void
Fb_textbox::display(std::string const &msg, unsigned color, unsigned line)
{
  unsigned offset = line * _text_font_height;

  std::string padded_msg(msg);
  if (msg.size() < _displayable_columns)
    padded_msg.append(_displayable_columns - msg.size(), ' ');

  gfxbitmap_font_text(
    _fb_base, reinterpret_cast<l4re_video_view_info_t *>(&_fb_info),
     _text_font, padded_msg.c_str(), GFXBITMAP_USE_STRLEN,
    _border_left, _border_top + offset, color, _text_bg_color
  );
}

void
Fb_textbox::clear_screen()
{
  for (unsigned line = 0u; line <= _displayable_lines; ++line)
    display(std::string(_displayable_columns + 1u, ' '), _text_bg_color, line);
}

void
Fb_textbox::scroll_up()
{
  unsigned line = _displayable_lines - 2u;
  for (auto hist = 0u; hist < _displayable_lines - 1u; ++hist)
    {
      auto tmp = _history[_history.size() - 1u - hist];
      display(std::get<0>(tmp), std::get<1>(tmp), line);
      --line;
    }
}
