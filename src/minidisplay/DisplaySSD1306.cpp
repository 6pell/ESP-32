#include <ArduinoLog.h>
#include "Config.h"
#include "minidisplay/DisplaySSD1306.h"

/* Display_SSD1306 */

size_t Display_SSD1306::printLine( const String& s ) {
  size_t n = 0;

  String key;
  bool have_key = false;
  const char* content = s.c_str();
  int length = s.length();

  while( length-- ) {
    char c = *content++;
    if( c == '~' ) continue;
    if( have_key ) {
      if( c == '%' ) {
        n += print( resolveTemplateKey( key ));
        have_key = false;
        key = "";
      } else if( key.length() < 9 ) {     // SWITCH macro requires 9 chars max
        key += c;
      }
    } else {
      if( c == '%' ) {
        have_key = true;
      } else {
        n += print( c );
      }
    }
  }
  n += println();
  return n;
}

// This write() method is mostly copy-pasted from the corresponding Adafruit_GFX one.
// The only difference is to used a textLeftPadding value instead of 0.
size_t Display_SSD1306::write( uint8_t c ) {
  if(!gfxFont) {                                            // 'Classic' built-in font
    if(c == '\n') {                                         // Newline?
      cursor_x  = textLeftPadding;                          // Reset x to zero,
      cursor_y += textsize_y * 8;                           // advance y one line
    } else if(c != '\r') {                                  // Ignore carriage returns
      if(wrap && ((cursor_x + textsize_x * 6) > _width)) {  // Off right?
        cursor_x  = textLeftPadding;                        // Reset x to zero,
        cursor_y += textsize_y * 8;                         // advance y one line
      }
      drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
      cursor_x += textsize_x * 6;                           // Advance x one char
    }
  } else {                                                  // Custom font
    if(c == '\n') {
      cursor_x  = textLeftPadding;
      cursor_y += (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
    } else if(c != '\r') {
      uint8_t first = pgm_read_byte(&gfxFont->first);
      if((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
        GFXglyph *glyph = gfxFont->glyph + (c - first);
        uint8_t   w     = pgm_read_byte(&glyph->width),
                  h     = pgm_read_byte(&glyph->height);
        if((w > 0) && (h > 0)) {                                // Is there an associated bitmap?
          int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset);  // sic
          if(wrap && ((cursor_x + textsize_x * (xo + w)) > _width)) {
            cursor_x  = textLeftPadding;
            cursor_y += (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
          }
          drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
        }
        cursor_x += (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize_x;
      }
    }
  }
  return 1;
}

String Display_SSD1306::resolveTemplateKey( const String& key ) {
  return templateParams.has( key ) ? templateParams.get( key ) : "";
}