#ifndef _NEXTION_GRAPHICS_H_
#define _NEXTION_GRAPHICS_H_

#include <stdint.h>

#define BLACK   0
#define BLUE    31
#define BROWN   48192
#define GREEN   2016
#define YELLOW  65504
#define RED     63488
#define GRAY    33840
#define WHITE   65535

class NextionGraphics {
public:
  // Basic graphics functions
  void clearScreen( uint16_t color );
  void drawString( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t font, uint16_t fontcolor, uint16_t backcolor, uint8_t xcenter, uint8_t ycenter, const char* str );
  void drawPixel( uint16_t x, uint16_t y, uint16_t color );
  void drawLine( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color );
  void drawRectangle( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color );
  void fillRectangle( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color );
  void drawCircle( uint16_t x, uint16_t y, uint16_t r, uint16_t color );
  void fillCircle( uint16_t x, uint16_t y, uint16_t r, uint16_t color );

};

#endif // _NEXTION_GRAPHICS_H_
