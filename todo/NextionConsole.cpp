#include <algorithm>
//#include <ArduinoLog.h>
#include "NextionConsole.h"

// Nextion display related constants.
// Unfortunately there is no API to obtain these values dynamically from display.

const int FONT_ID     = 0;        // FontID (Nextion display font index) used bn console.
const int FONT_WIDTH  = 8;        // Font width.
const int FONT_HEIGHT = 16;       // Font height.
const int BG_COLOR    = BLACK;
const int TEXT_COLOR  = WHITE;

/* Public */

NextionConsole::NextionConsole( uint16_t w, uint16_t h )
    : screen_width(w), screen_height(h) {
  max_lines = h / FONT_HEIGHT;
}

void NextionConsole::clear() {
  log_lines.clear();
  clearScreen( BG_COLOR );
}

void NextionConsole::redraw() {
  uint8_t size = log_lines.size();
  uint16_t y = 0;

  for( uint8_t i = 0; i < max_lines; i++ ) {
    drawString( 0, y, screen_width, FONT_HEIGHT, FONT_ID, TEXT_COLOR, BG_COLOR, 0, 0, i < size ? log_lines[i].c_str() : "" ); 
    y += FONT_HEIGHT;
  }
}

size_t NextionConsole::write( uint8_t symbol ) {
  if( symbol == '\n' || symbol == '\r' || symbol == '\0' ) {
     if( accumulator.length() == 0 ) {
       // Ignore line separators and terminators when local accumulator is empty.
       return 1;
     }
     append( accumulator );
     accumulator = "";
  } else {
     accumulator += (char)symbol;
  }
  return 1;
}

/* Private */

void NextionConsole::append( String& line ) {
  // It's need to escape double quote symbols for Nextions.
  line.replace( "\"", "\\\"" );    

  if( log_lines.size() >= max_lines ) {
    log_lines.pop_front();
  }
  log_lines.push_back( line );

  // Redraw the display only when active flag is set.
  if( active ) {
    redraw();
  }
}
