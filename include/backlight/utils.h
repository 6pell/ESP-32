#pragma once
#include <NeoPixelBus.h>
#include "RgbPalette.h"
#include "types.h"

namespace Backlight {

  namespace Utils {

    RgbColor    colorFromPalette( const RgbPalette16& pal, uint8_t index, uint8_t brightness=255, BlendType blendType=LINEARBLEND );
    RgbColor    colorWheelValue( uint8_t pos );
    uint8_t     randomWheelIndex( uint8_t pos );

    RgbColor    colorBlend( const RgbColor& color1, const RgbColor& color2, uint8_t blend );
    bool        equals( const RgbColor& color1, const RgbColor& color2 );
    RgbColor    nscale8x3( const RgbColor& color, fract8 scale );
    RgbColor    nscale8x3_video( const RgbColor& color, fract8 scale );
    RgbColor    sumColors( const RgbColor& color1, const RgbColor& color2 );
    RgbColor    toRgbColor( uint32_t hexColor );

    int16_t     avg15( int16_t i, int16_t j );
    uint8_t     beat8( accum88 beats_per_minute, uint32_t timebase = 0 );
    uint16_t    beat16( accum88 beats_per_minute, uint32_t timebase = 0 );
    uint16_t    beat88( accum88 beats_per_minute_88, uint32_t timebase = 0 );
    uint8_t     beatsin8( accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0 );
    uint8_t     cubicwave8( uint8_t in );
    fract8      ease8InOutCubic( fract8 i );
    uint16_t    ease16InOutQuad( uint16_t i );
    int16_t     grad16( uint8_t hash, int16_t x, int16_t y, int16_t z );
    uint16_t    inoise16( uint32_t x, uint32_t y, uint32_t z );
    int16_t     inoise16_raw( uint32_t x, uint32_t y, uint32_t z );
    int16_t     lerp15by16( int16_t a, int16_t b, fract16 frac );
    uint8_t     qadd8( uint8_t i, uint8_t j );
    uint8_t     scale8( uint8_t i, fract8 scale );
    uint8_t     scale8_video( uint8_t i, fract8 scale );
    uint16_t    scale16( uint16_t i, fract16 scale );
    uint8_t     sin8( uint8_t theta );
    uint8_t     triwave8( uint8_t in );
  }
}