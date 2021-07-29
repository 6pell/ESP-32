#include "str_switch.h"
#include "backlight/types.h"
#include "backlight/RgbPalette.h"
#include "backlight/utils.h"

using namespace Backlight;

// HSV color ramp: blue purple ping red orange yellow (and back)
// Basically, everything but the greens, which tend to make
// people's skin look unhealthy.  This palette is good for
// lighting at a club or party, where it'll be shining on people.
const RGBPalette16Type RgbPalette16::PartyColorsPalette = {
  0x5500AB, 0x84007C, 0xB5004B, 0xE5001B,
  0xE81700, 0xB84700, 0xAB7700, 0xABAB00,
  0xAB5500, 0xDD2200, 0xF2000E, 0xC2003E,
  0x8F0071, 0x5F00A1, 0x2F00D0, 0x0007F9
};

const RGBPalette16Type RgbPalette16::OceanPalette {
  0x191970, 0x00008B, 0x191970, 0x000080,
  0x00008B, 0x0000CD, 0x2E8B57, 0x008080,
  0x5F9EA0, 0x0000FF, 0x008B8B, 0x6495ED,
  0x7FFFD4, 0x2E8B57, 0x00FFFF, 0x87CEFA
};

const RGBPalette16Type RgbPalette16::ForestPalette = {
  0x006400, 0x006400, 0x556B2F, 0x006400,
  0x008000, 0x228B22, 0x6B8E23, 0x008000,
  0x2E8B57, 0x66CDAA, 0x32CD32, 0x9ACD32,
  0x90EE90, 0x7CFC00, 0x66CDAA, 0x228B22
};

// HSV Rainbow
const RGBPalette16Type RgbPalette16::RainbowPalette {
    0xFF0000, 0xD52A00, 0xAB5500, 0xAB7F00,
    0xABAB00, 0x56D500, 0x00FF00, 0x00D52A,
    0x00AB55, 0x0056AA, 0x0000FF, 0x2A00D5,
    0x5500AB, 0x7F0081, 0xAB0055, 0xD5002B
};

// Gradient palette "GMT_drywet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/gmt/tn/GMT_drywet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.
const RGBGradientPaletteEntry RgbPalette16::DrywetPalette[] = {
  {   0,  47,  30,   2 },
  {  42, 213, 147,  24 },
  {  84, 103, 219,  52 },
  { 127,   3, 219, 207 },
  { 170,   1,  48, 214 },
  { 212,   1,   1, 111 },
  { 255,   1,   7,  33 }
};

// Gradient palette "lava_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/elem/tn/lava.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 52 bytes of program space.
const RGBGradientPaletteEntry RgbPalette16::FirePalette[] = {
  {   0,   0,   0,   0 },
  {  46,  18,   0,   0 },
  {  96, 113,   0,   0 },
  { 108, 142,   3,   1 },
  { 119, 175,  17,   1 },
  { 146, 213,  44,   2 },
  { 174, 255,  82,   4 },
  { 188, 255, 115,   4 },
  { 202, 255, 156,   4 },
  { 218, 255, 203,   4 },
  { 234, 255, 255,   4 },
  { 244, 255, 255,  71 },
  { 255, 255, 255, 255 }
};

// Custom palette by Aircoookie
const RGBGradientPaletteEntry RgbPalette16::TiamatPalette[] = {
  {   0,   1,   2,  14 }, //gc
  {  33,   2,   5,  35 }, //gc from 47, 61,126
  { 100,  13, 135,  92 }, //gc from 88,242,247
  { 120,  43, 255, 193 }, //gc from 135,255,253
  { 140, 247,   7, 249 }, //gc from 252, 69,253
  { 160, 193,  17, 208 }, //gc from 231, 96,237
  { 180,  39, 255, 154 }, //gc from 130, 77,213
  { 200,   4, 213, 236 }, //gc from 57,122,248
  { 220,  39, 252, 135 }, //gc from 177,254,255
  { 240, 193, 213, 253 }, //gc from 203,239,253
  { 255, 255, 249, 255 }
};

RgbPalette16* RgbPalette16::fromId( const String& id ) {
  SWITCH( id.c_str() ) {
    CASE( "drywet" ):     return new RgbPalette16( DrywetPalette );
    CASE( "ocean" ):      return new RgbPalette16( OceanPalette );
    CASE( "forest" ):     return new RgbPalette16( ForestPalette );
    CASE( "rainbow" ):    return new RgbPalette16( RainbowPalette );
    CASE( "fire" ):       return new RgbPalette16( FirePalette );
    CASE( "tiamat" ):     return new RgbPalette16( TiamatPalette );
    CASE( "party" ):
    DEFAULT_CASE:         return new RgbPalette16( PartyColorsPalette );
  }
}

RgbPalette16::RgbPalette16( const RGBGradientPaletteEntry* pal ) {
  RGBGradientPaletteEntry u;
  // Count entries
  uint16_t count = 0;
  do {
      u = *(pal + count);
      count++;;
  } while( u.index != 255 );

  int8_t lastSlotUsed = -1;

  u = *pal;
  RgbColor rgbstart( u.r, u.g, u.b );

  int indexstart = 0;
  uint8_t istart8 = 0;
  uint8_t iend8 = 0;
  while( indexstart < 255 ) {
    pal++;
    u = *pal;
    int indexend  = u.index;
    RgbColor rgbend( u.r, u.g, u.b );
    istart8 = indexstart / 16;
    iend8   = indexend   / 16;
    if( count < 16 ) {
      if( (istart8 <= lastSlotUsed) && (lastSlotUsed < 15) ) {
        istart8 = lastSlotUsed + 1;
        if( iend8 < istart8) {
          iend8 = istart8;
        }
      }
      lastSlotUsed = iend8;
    }
    fill_gradient_rgb( &(entries[0]), istart8, rgbstart, iend8, rgbend );
    indexstart = indexend;
    rgbstart = rgbend;
  }
}

int8_t RgbPalette16::findPaletteById( const String& effectId ) {
  uint8_t count = sizeof(PALETTE_ID) / sizeof(PALETTE_ID[0]);
  for( uint8_t i = 0; i < count; i++ ) {
    const String id = PALETTE_ID[i];
    if( id == effectId ) {
      return i;
    }
  }
  return -1;
}

void RgbPalette16::fill_gradient( RgbColor* target, uint16_t startpos, CHSV startcolor, uint16_t endpos, CHSV endcolor, TGradientDirectionCode directionCode ) {
  // if the points are in the wrong order, straighten them
  if( endpos < startpos ) {
    uint16_t t = endpos;
    CHSV tc = endcolor;
    endcolor = startcolor;
    endpos = startpos;
    startpos = t;
    startcolor = tc;
  }

  // If we're fading toward black (val=0) or white (sat=0),
  // then set the endhue to the starthue.
  // This lets us ramp smoothly to black or white, regardless
  // of what 'hue' was set in the endcolor (since it doesn't matter)
  if( endcolor.value == 0 || endcolor.saturation == 0) {
    endcolor.hue = startcolor.hue;
  }

  // Similarly, if we're fading in from black (val=0) or white (sat=0)
  // then set the starthue to the endhue.
  // This lets us ramp smoothly up from black or white, regardless
  // of what 'hue' was set in the startcolor (since it doesn't matter)
  if( startcolor.value == 0 || startcolor.saturation == 0) {
    startcolor.hue = endcolor.hue;
  }

  saccum87 huedistance87;
  saccum87 satdistance87;
  saccum87 valdistance87;

  satdistance87 = (endcolor.sat - startcolor.sat) << 7;
  valdistance87 = (endcolor.val - startcolor.val) << 7;

  uint8_t huedelta8 = endcolor.hue - startcolor.hue;

  if( directionCode == SHORTEST_HUES ) {
    directionCode = FORWARD_HUES;
    if( huedelta8 > 127) {
      directionCode = BACKWARD_HUES;
    }
  }

  if( directionCode == LONGEST_HUES ) {
    directionCode = FORWARD_HUES;
    if( huedelta8 < 128) {
      directionCode = BACKWARD_HUES;
    }
  }

  if( directionCode == FORWARD_HUES ) {
    huedistance87 = huedelta8 << 7;
  }
  else {
    // directionCode == BACKWARD_HUES
    huedistance87 = (uint8_t)(256 - huedelta8) << 7;
    huedistance87 = -huedistance87;
  }

  uint16_t pixeldistance = endpos - startpos;
  int16_t divisor = pixeldistance ? pixeldistance : 1;

  saccum87 huedelta87 = huedistance87 / divisor;
  saccum87 satdelta87 = satdistance87 / divisor;
  saccum87 valdelta87 = valdistance87 / divisor;

  huedelta87 *= 2;
  satdelta87 *= 2;
  valdelta87 *= 2;

  accum88 hue88 = startcolor.hue << 8;
  accum88 sat88 = startcolor.sat << 8;
  accum88 val88 = startcolor.val << 8;
  for( uint16_t i = startpos; i <= endpos; i++) {
    const CHSV hsv = CHSV( hue88 >> 8, sat88 >> 8, val88 >> 8);
    hsv2rgb_rainbow( hsv, target[i] );

    hue88 += huedelta87;
    sat88 += satdelta87;
    val88 += valdelta87;
  }
}

void RgbPalette16::fill_gradient( RgbColor* target, uint16_t numLeds, const CHSV& c1, const CHSV& c2, const CHSV& c3, const CHSV& c4 ) {
  uint16_t onethird = (numLeds / 3);
  uint16_t twothirds = ((numLeds * 2) / 3);
  uint16_t last = numLeds - 1;
  fill_gradient( target,         0, c1,  onethird, c2 );
  fill_gradient( target,  onethird, c2, twothirds, c3 );
  fill_gradient( target, twothirds, c3,      last, c4 );
}

/**
 * fillGradientRGB
 * Fill a range of LEDs with a smooth RGB gradient between two specified RGB colors.
 * Unlike HSV, there is no 'color wheel' in RGB space, and therefore there's only
 *  one 'direction' for the gradient to go, and no 'direction code' is needed.
 */
void RgbPalette16::fill_gradient_rgb( RgbColor* target, uint16_t startpos, RgbColor startcolor, uint16_t endpos, RgbColor endcolor ) {
  // if the points are in the wrong order, straighten them
  if( endpos < startpos ) {
    uint16_t t = endpos;
    RgbColor tc = endcolor;
    endcolor = startcolor;
    endpos = startpos;
    startpos = t;
    startcolor = tc;
  }

  saccum87 rdistance87;
  saccum87 gdistance87;
  saccum87 bdistance87;

  rdistance87 = (endcolor.R - startcolor.R) << 7;
  gdistance87 = (endcolor.G - startcolor.G) << 7;
  bdistance87 = (endcolor.B - startcolor.B) << 7;

  uint16_t pixeldistance = endpos - startpos;
  int16_t divisor = pixeldistance ? pixeldistance : 1;

  saccum87 rdelta87 = rdistance87 / divisor;
  saccum87 gdelta87 = gdistance87 / divisor;
  saccum87 bdelta87 = bdistance87 / divisor;

  rdelta87 *= 2;
  gdelta87 *= 2;
  bdelta87 *= 2;

  accum88 r88 = startcolor.R << 8;
  accum88 g88 = startcolor.G << 8;
  accum88 b88 = startcolor.B << 8;

  for( uint16_t i = startpos; i <= endpos; i++ ) {
    target[i] = RgbColor( r88 >> 8, g88 >> 8, b88 >> 8 );
    r88 += rdelta87;
    g88 += gdelta87;
    b88 += bdelta87;
  }
}

#define K255 255
#define K171 171
#define K170 170
#define K85  85

// Sometimes the compiler will do clever things to reduce
// code size that result in a net slowdown, if it thinks that
// a variable is not used in a certain location.
// This macro does its best to convince the compiler that
// the variable is used in this location, to help control
// code motion and de-duplication that would result in a slowdown.
#define FORCE_REFERENCE(var)  asm volatile( "" : : "r" (var) )

void RgbPalette16::hsv2rgb_rainbow( const CHSV& hsv, RgbColor& rgb ) {
  // Yellow has a higher inherent brightness than
  // any other color; 'pure' yellow is perceived to
  // be 93% as bright as white.  In order to make
  // yellow appear the correct relative brightness,
  // it has to be rendered brighter than all other
  // colors.
  // Level Y1 is a moderate boost, the default.
  // Level Y2 is a strong boost.
  const uint8_t Y1 = 1;
  const uint8_t Y2 = 0;

  // G2: Whether to divide all greens by two.
  // Depends GREATLY on your particular LEDs
  const uint8_t G2 = 0;

  // Gscale: what to scale green down by.
  // Depends GREATLY on your particular LEDs
  const uint8_t Gscale = 0;

  uint8_t hue = hsv.hue;
  uint8_t sat = hsv.sat;
  uint8_t val = hsv.val;

  uint8_t offset = hue & 0x1F; // 0..31

  // offset8 = offset * 8
  uint8_t offset8 = offset;
  offset8 <<= 3;

  uint8_t third = Utils::scale8( offset8, (256 / 3) ); // max = 85

  uint8_t r, g, b;

  if( !(hue & 0x80) ) {
    // 0XX
    if( !(hue & 0x40) ) {
      // 00X
      //section 0-1
      if( !(hue & 0x20) ) {
        // 000
        //case 0: // R -> O
        r = K255 - third;
        g = third;
        b = 0;
        FORCE_REFERENCE(b);
      } else {
        // 001
        //case 1: // O -> Y
        if( Y1 ) {
          r = K171;
          g = K85 + third ;
          b = 0;
          FORCE_REFERENCE(b);
        }
        if( Y2 ) {
          r = K170 + third;
          //uint8_t twothirds = (third << 1);
          uint8_t twothirds = Utils::scale8( offset8, ((256 * 2) / 3) ); // max=170
          g = K85 + twothirds;
          b = 0;
          FORCE_REFERENCE(b);
        }
      }
    } else {
      //01X
      // section 2-3
      if( !(hue & 0x20) ) {
        // 010
        //case 2: // Y -> G
        if( Y1 ) {
          //uint8_t twothirds = (third << 1);
          uint8_t twothirds = Utils::scale8( offset8, ((256 * 2) / 3) ); // max=170
          r = K171 - twothirds;
          g = K170 + third;
          b = 0;
          FORCE_REFERENCE(b);
        }
        if( Y2 ) {
          r = K255 - offset8;
          g = K255;
          b = 0;
          FORCE_REFERENCE(b);
        }
      } else {
        // 011
        // case 3: // G -> A
        r = 0;
        FORCE_REFERENCE(r);
        g = K255 - third;
        b = third;
      }
    }
  } else {
    // section 4-7
    // 1XX
    if( !(hue & 0x40) ) {
      // 10X
      if( !( hue & 0x20) ) {
        // 100
        //case 4: // A -> B
        r = 0;
        FORCE_REFERENCE(r);
        //uint8_t twothirds = (third << 1);
        uint8_t twothirds = Utils::scale8( offset8, ((256 * 2) / 3) ); // max=170
        g = K171 - twothirds; //K170?
        b = K85  + twothirds;
      } else {
        // 101
        //case 5: // B -> P
        r = third;
        g = 0;
        FORCE_REFERENCE(g);
        b = K255 - third;
      }
    } else {
      if( !(hue & 0x20) ) {
        // 110
        //case 6: // P -- K
        r = K85 + third;
        g = 0;
        FORCE_REFERENCE(g);
        b = K171 - third;
      } else {
        // 111
        //case 7: // K -> R
        r = K170 + third;
        g = 0;
        FORCE_REFERENCE(g);
        b = K85 - third;
      }
    }
  }

  // This is one of the good places to scale the green down,
  // although the client can scale green down as well.
  if( G2 ) g = g >> 1;
  if( Gscale ) g = Utils::scale8_video( g, Gscale );

  // Scale down colors if we're desaturated at all
  // and add the brightness_floor to r, g, and b.
  if( sat != 255 ) {
    if( sat == 0 ) {
      r = 255; b = 255; g = 255;
    } else {
      //nscale8x3_video( r, g, b, sat);
      if( r ) r = Utils::scale8( r, sat );
      if( g ) g = Utils::scale8( g, sat );
      if( b ) b = Utils::scale8( b, sat );

      uint8_t desat = 255 - sat;
      desat = Utils::scale8( desat, desat );

      uint8_t brightness_floor = desat;
      r += brightness_floor;
      g += brightness_floor;
      b += brightness_floor;
    }
  }

  // Now scale everything down if we're at value < 255.
  if( val != 255 ) {
    val = Utils::scale8_video( val, val );
    if( val == 0 ) {
      r = 0; g = 0; b = 0;
    } else {
      // nscale8x3_video( r, g, b, val);
      if( r ) r = Utils::scale8_video( r, val );
      if( g ) g = Utils::scale8_video( g, val );
      if( b ) b = Utils::scale8_video( b, val );
    }
  }

  rgb.R = r;
  rgb.G = g;
  rgb.B = b;
}
