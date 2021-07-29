#include <Arduino.h>
#include "NextionGraphics.h"
#include "Config.h"

/* Public */

/**
 * Clears the lcd by filling it with the specified color pixels. 
 * @param Color Integer value (0 to 65535) to represent the color to fill the screen with - represented in 16-bit 565 color format
 */
void NextionGraphics::clearScreen( uint16_t color ) {
  NEXTION_SERIAL.printf( "cls %d%c%c%c", color, 0xff, 0xff, 0xff );
}

/**
 * Draws a string on the lcd at the specified xy position. 
 * @param x x position.
 * @param y y position.
 * @param w Width of string area.
 * @param h Height of string area.
 * @param font Font ID to use (0:smallest - 7:largest).
 * @param fontcolor Color Integer value (0 to 65535) to represent the font color of the string - represented in 16-bit 565 color format.
 * @param backcolor Color Integer value (0 to 65535) to represent the background color of the string - represented in 16-bit 565 color format.
 * @param xcenter Horizontal alignment (0: left-aligned, 1: entered, 2: right-aligned).
 * @param ycenter Vertical alignment (0: upper-aligned, 1: entered, 2: lower-aligned).
 * @param str String content.
 */
void NextionGraphics::drawString( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t font, uint16_t fontcolor, uint16_t backcolor, uint8_t xcenter, uint8_t ycenter, const char* str ) {
  NEXTION_SERIAL.printf( "xstr %d,%d,%d,%d,%d,%d,%d,%d,%d,1,\"%s\"%c%c%c", x, y, w, h, font, fontcolor, backcolor, xcenter, ycenter, str, 0xff, 0xff, 0xff );
}

/**
 * Draws a pixel on the lcd at the specified xy position. 
 * @param x x position
 * @param y y position
 * @param color Color of the pixel (0 to 65535) - represented in 16-bit 565 color format.
 */
void NextionGraphics::drawPixel( uint16_t x, uint16_t y, uint16_t color ) {
  NEXTION_SERIAL.printf( "fill %d,%d,1,1,%d%c%c%c", x, y, color, 0xff, 0xff, 0xff );
}

/**
 * Draws a line on the lcd from x1,y1 to x2,y2. 
 * @param x1 x coordinate starting position 
 * @param y1 y coordinate starting position
 * @param x2 x coordinate ending position
 * @param y2 y coordinate ending position
 * @param color Color of the line (0 to 65535) - represented in 16-bit 565 color format.
 */
void NextionGraphics::drawLine( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color ) {
  NEXTION_SERIAL.printf( "line %d,%d,%d,%d,%d%c%c%c", x1, y1, x2, y2, color, 0xff, 0xff, 0xff );
}

/**
 * Draws a rectangle on the lcd from x,y with width w and height h. 
 * @param x x coordinate of top-left corner position 
 * @param y y coordinate of top-left corner position 
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param color Color of the rectangle (0 to 65535) - represented in 16-bit 565 color format.
 */
void NextionGraphics::drawRectangle( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color ) {
  NEXTION_SERIAL.printf( "draw %d,%d,%d,%d,%d%c%c%c", x, y, x+w, y+h, color, 0xff, 0xff, 0xff );
}

/**
 * Draws a filled rectangle on the lcd from x,y with width w and height h. 
 * @param x x coordinate of top-left corner position 
 * @param y y coordinate of top-left corner position 
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param color Color of the rectangle (0 to 65535) - represented in 16-bit 565 color format.
 */
void NextionGraphics::fillRectangle( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color ) {
  NEXTION_SERIAL.printf( "fill %d,%d,%d,%d,%d%c%c%c", x, y, w, h, color, 0xff, 0xff, 0xff );
}

/**
 * Draws a circle on the lcd at x,y with a radius of r.
 * @param x x coordinate position
 * @param y y coordinate position
 * @param r Radius of the circle
 * @param color Color of the circle (0 to 65535) - represented in 16-bit 565 color format.
 */
void NextionGraphics::drawCircle( uint16_t x, uint16_t y, uint16_t r, uint16_t color ) {
  NEXTION_SERIAL.printf( "cir %d,%d,%d,%d%c%c%c", x, y, r, color, 0xff, 0xff, 0xff );
}

/**
 * Draws a filled circle on the lcd at x,y with a radius of r.
 * @param x x coordinate position
 * @param y y coordinate position
 * @param r Radius of the circle
 * @param color Color of the circle (0 to 65535) - represented in 16-bit 565 color format.
 */
void NextionGraphics::fillCircle( uint16_t x, uint16_t y, uint16_t r, uint16_t color ) {
  NEXTION_SERIAL.printf( "cir %d,%d,%d,%d%c%c%c", x, y, r, color, 0xff, 0xff, 0xff );
}
