#pragma once

#include <l4/libgfxbitmap/font.h>
#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/sys/capability>

#include <mutex>
#include <string>
#include <utility>
#include <vector>

class Fb_textbox
{
public:
  enum {Text_color_max_strlen = 7};

  constexpr static char const *const Default_text_fg_color = "white";
  constexpr static char const *const Default_text_bg_color = "black";

  Fb_textbox(L4::Cap<L4Re::Video::Goos> fb,
             std::string const &text_bg_color = Default_text_bg_color,
             unsigned border_left = 0u, unsigned width = 0u,
             unsigned border_top = 0u, unsigned height = 0u);

  bool active() const { return _active; }

  void print_line(std::string const &msg, std::string const &color);

private:
  void display(std::string const &msg, unsigned color, unsigned line);
  void clear_screen();
  void scroll_up();

  bool _active;

  L4Re::Util::Video::Goos_fb _goos_fb;
  L4Re::Video::View::Info _fb_info;
  void *_fb_base;

  unsigned _border_left;
  unsigned _border_right;
  unsigned _border_top;
  unsigned _border_bottom;

  gfxbitmap_font_t _text_font = GFXBITMAP_DEFAULT_FONT;
  unsigned _text_font_height;
  unsigned _text_font_width;
  unsigned _text_bg_color;

  unsigned _current_line = 0u;
  unsigned _displayable_lines;
  unsigned _displayable_columns;

  std::vector<std::pair<std::string, unsigned>> _history;

  std::mutex _print_lock;
};
