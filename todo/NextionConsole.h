#ifndef _NEXTION_CONSOLE_H_
#define _NEXTION_CONSOLE_H_

#include <deque>
#include <Print.h>
#include <WString.h>

#include "NextionGraphics.h"


class NextionConsole : public NextionGraphics, public Print {
private:
  uint16_t screen_width, screen_height;
  size_t max_lines;
  std::deque<String> log_lines;
  String accumulator;
  bool active;

public:
  NextionConsole( uint16_t w, uint16_t h );
  void clear();
  void redraw();
  void setActive( bool value )  { active = value; }
  virtual size_t write( uint8_t symbol );

private:  
  void append( String& line );
};

#endif // _NEXTION_CONSOLE_H_
