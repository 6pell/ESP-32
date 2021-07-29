#include <ArduinoLog.h>
#include "backlight/utils.h"

using namespace Backlight;

static uint8_t const p[] = { 151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,151
};

const uint8_t b_m16_interleave[] = { 0, 49, 49, 41, 90, 27, 117, 10 };


RgbColor Utils::sumColors( const RgbColor& color1, const RgbColor& color2 ) {
  uint8_t r = qadd8( color1.R, color2.R );
  uint8_t g = qadd8( color1.G, color2.G );
  uint8_t b = qadd8( color1.B, color2.B );
  return RgbColor( r, g, b );
}


/**
 * Calculate an integer average of two signed 15-bit integers (int16_t)
 * If the first argument is even, result is rounded down.
 * If the first argument is odd, result is result up.
 */
int16_t Utils::avg15( int16_t i, int16_t j ) {
  return ((int32_t)((int32_t)(i) + (int32_t)(j)) >> 1) + (i & 0x1);
}


/**
 * beat8 generates an 8-bit 'sawtooth' wave at a given BPM
 */
inline uint8_t Utils::beat8( accum88 beats_per_minute, uint32_t timebase ) {
  return beat16( beats_per_minute, timebase ) >> 8;
}


/**
 * beat16 generates a 16-bit 'sawtooth' wave at a given BPM
 */
inline uint16_t Utils::beat16( accum88 beats_per_minute, uint32_t timebase ) {
  // Convert simple 8-bit BPM's to full Q8.8 accum88's if needed
  if( beats_per_minute < 256 ) beats_per_minute <<= 8;
  return beat88( beats_per_minute, timebase );
}


/**
 * beat16 generates a 16-bit 'sawtooth' wave at a given BPM,
 * with BPM specified in Q8.8 fixed-point format; e.g.
 * for this function, 120 BPM MUST BE specified as 120*256 = 30720.
 * If you just want to specify "120", use beat16 or beat8.
 */
uint16_t Utils::beat88( accum88 beats_per_minute_88, uint32_t timebase ) {
  // BPM is 'beats per minute', or 'beats per 60000ms'.
  // To avoid using the (slower) division operator, we
  // want to convert 'beats per 60000ms' to 'beats per 65536ms',
  // and then use a simple, fast bit-shift to divide by 65536.
  //
  // The ratio 65536:60000 is 279.620266667:256; we'll call it 280:256.
  // The conversion is accurate to about 0.05%, more or less,
  // e.g. if you ask for "120 BPM", you'll get about "119.93".
  return ((millis() - timebase) * beats_per_minute_88 * 280) >> 16;
}


/**
 * beatsin8 generates an 8-bit sine wave at a given BPM, that oscillates within a given range.
 */
uint8_t Utils::beatsin8( accum88 beats_per_minute, uint8_t lowest, uint8_t highest, uint32_t timebase, uint8_t phase_offset ) {
  uint8_t beat = beat8( beats_per_minute, timebase );
  uint8_t beatsin = sin8( beat + phase_offset );
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8( beatsin, rangewidth );
  uint8_t result = lowest + scaledbeat;
  return result;
}


RgbColor Utils::colorBlend( const RgbColor& color1, const RgbColor& color2, uint8_t blend ) {
  if( blend == 0 )   return color1;
  if( blend == 255 ) return color2;

  int r1 = color1.R;
  int g1 = color1.G;
  int b1 = color1.B;

  int r2 = color2.R;
  int g2 = color2.G;
  int b2 = color2.B;

  uint32_t r3 = ((r2 * blend) + (r1 * (255 - blend))) >> 8;
  uint32_t g3 = ((g2 * blend) + (g1 * (255 - blend))) >> 8;
  uint32_t b3 = ((b2 * blend) + (b1 * (255 - blend))) >> 8;

  return RgbColor( r3, g3, b3 );
}


RgbColor Utils::colorFromPalette( const RgbPalette16& pal, uint8_t index, uint8_t brightness, BlendType blendType ) {
  uint8_t hi4 = index >> 4;
  uint8_t lo4 = index & 0x0F;

  // const CRGB* entry = &(pal[0]) + hi4;
  // since hi4 is always 0..15, hi4 * sizeof(CRGB) can be a single-byte value,
  // instead of the two byte 'int' that avr-gcc defaults to.
  // So, we multiply hi4 X sizeof(CRGB), giving hi4XsizeofCRGB;
  uint8_t hi4XsizeofRGB = hi4 * sizeof(RgbColor);
  // We then add that to a base array pointer.
  const RgbColor* entry = (RgbColor*)( (uint8_t*)(&(pal[0])) + hi4XsizeofRGB);

  //RgbColor cc = pal[hi4];
  //Log.verbose( "%d %x %x" CR, index, (cc.R*65536 + cc.G*256 + cc.B), (entry->R*65536 + entry->G*256 + entry->B) );
  //Log.verbose( "index=%d hi4=%d %x" CR, index, hi4XsizeofRGB, (entry->R*65536 + entry->G*256 + entry->B) );

  uint8_t blend = lo4 && (blendType != NOBLEND);
  uint8_t red1   = entry->R;
  uint8_t green1 = entry->G;
  uint8_t blue1  = entry->B;

  if( blend ) {
    if( hi4 == 15 ) {
      entry = &(pal[0]);
    } else {
      entry++;
    }

    uint8_t f2 = lo4 << 4;
    uint8_t f1 = 255 - f2;

    //    rgb1.nscale8(f1);
    uint8_t red2 = entry->R;
    red1 = scale8( red1, f1 );
    red2 = scale8( red2, f2 );
    red1 += red2;

    uint8_t green2 = entry->G;
    green1 = scale8( green1, f1 );
    green2 = scale8( green2, f2 );
    green1 += green2;

    uint8_t blue2 = entry->B;
    blue1 = scale8( blue1, f1 );
    blue2 = scale8( blue2, f2 );
    blue1 += blue2;
  }

  if( brightness != 255) {
    if( brightness ) {
      brightness++; // adjust for rounding
      // Now, since brightness is nonzero, we don't need the full scale8_video logic;
      // we can just to scale8 and then add one (unless scale8 fixed) to all nonzero inputs.
      if( red1 ) {
        red1 = scale8( red1, brightness );
      }
      if( green1 ) {
        green1 = scale8( green1, brightness );
      }
      if( blue1 )  {
        blue1 = scale8( blue1, brightness );
      }
    } else {
      red1 = 0;
      green1 = 0;
      blue1 = 0;
    }
  }
  return RgbColor( red1, green1, blue1 );
}


/*
 * Put a value 0 to 255 in to get a color value.
 * The colours are a transition r -> g -> b -> back to r.
 * Picked up from https://github.com/Aircoookie/WLED.
 */
RgbColor Utils::colorWheelValue( uint8_t pos ) {
  uint32_t color;
  pos = 255 - pos;
  if( pos < 85 ) {
    color = ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
  } else if( pos < 170 ) {
    pos -= 85;
    color = ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
  } else {
    pos -= 170;
    color = ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
  }
  return RgbColor( HtmlColor( color ));
}

/**
 * cubicwave8: cubic waveform generator.  Spends visibly more time
 *             at the limits than 'sine' does.
 */
uint8_t Utils::cubicwave8( uint8_t in ) {
  return ease8InOutCubic( triwave8( in ));
}


/**
 *  ease8InOutCubic: 8-bit cubic ease-in / ease-out function
 */
fract8 Utils::ease8InOutCubic( fract8 i ) {
  uint8_t ii  = scale8(  i, i );
  uint8_t iii = scale8( ii, i );

  uint16_t r1 = (3 * (uint16_t)(ii)) - ( 2 * (uint16_t)(iii));

  /* the code generated for the above *'s automatically
     cleans up R1, so there's no need to explicitily call
     cleanup_R1(); */
  uint8_t result = r1;

  // if we got "256", return 255:
  if( r1 & 0x100 ) {
    result = 255;
  }
  return result;
}


/**
 * ease16InOutQuad: 16-bit quadratic ease-in / ease-out function
 */
uint16_t Utils::ease16InOutQuad( uint16_t i ) {
  uint16_t j = i;
  if( j & 0x8000 ) {
    j = 65535 - j;
  }
  uint16_t jj  = scale16( j, j);
  uint16_t jj2 = jj << 1;
  if( i & 0x8000 ) {
    jj2 = 65535 - jj2;
  }
  return jj2;
}


bool Utils::equals( const RgbColor& color1, const RgbColor& color2 ) {
  return (color1.R == color2.R) && (color1.G == color2.G) && (color1.B == color2.B);
}


int16_t Utils::grad16( uint8_t hash, int16_t x, int16_t y, int16_t z ) {
  hash = hash&15;
  int16_t u = hash < 8 ? x : y;
  int16_t v = hash < 4 ? y : hash == 12 || hash == 14 ? x : z;
  if( hash & 1 ) { u = -u; }
  if( hash & 2 ) { v = -v; }

  return AVG15( u, v );
}


/**
 * scaled 16 bit noise functions
 *
 * 16 bit, fixed point implementation of perlin's Simplex Noise.  Coordinates are
 * 16.16 fixed point values, 32 bit integers with integral coordinates in the high 16
 * bits and fractional in the low 16 bits, and the function takes 1d, 2d, and 3d coordinate
 * values.  These functions are scaled to return 0-65535
 */
uint16_t Utils::inoise16( uint32_t x, uint32_t y, uint32_t z ) {
  int32_t ans = inoise16_raw( x, y, z );
  ans = ans + 19052L;
  uint32_t pan = ans;
  // pan = (ans * 220L) >> 7.  That's the same as:
  // pan = (ans * 440L) >> 8.  And this way avoids a 7X four-byte shift-loop on AVR.
  // Identical math, except for the highest bit, which we don't care about anyway,
  // since we're returning the 'middle' 16 out of a 32-bit value anyway.
  pan *= 440L;
  return (pan>>8);
}


/**
 * 16 bit raw versions of the noise functions.  These values are not scaled/altered and have
 * output values roughly in the range (-18k,18k)
 */
int16_t Utils::inoise16_raw( uint32_t x, uint32_t y, uint32_t z ) {
  // Find the unit cube containing the point
  uint8_t X = (x>>16) & 0xFF;
  uint8_t Y = (y>>16) & 0xFF;
  uint8_t Z = (z>>16) & 0xFF;

  // Hash cube corner coordinates
  uint8_t A  = P(X)+Y;
  uint8_t AA = P(A)+Z;
  uint8_t AB = P(A+1)+Z;
  uint8_t B  = P(X+1)+Y;
  uint8_t BA = P(B) + Z;
  uint8_t BB = P(B+1)+Z;

  // Get the relative position of the point in the cube
  uint16_t u = x & 0xFFFF;
  uint16_t v = y & 0xFFFF;
  uint16_t w = z & 0xFFFF;

  // Get a signed version of the above for the grad function
  int16_t xx = (u >> 1) & 0x7FFF;
  int16_t yy = (v >> 1) & 0x7FFF;
  int16_t zz = (w >> 1) & 0x7FFF;
  uint16_t N = 0x8000L;

  u = EASE16(u); v = EASE16(v); w = EASE16(w);

  // skip the log fade adjustment for the moment, otherwise here we would
  // adjust fade values for u,v,w
  int16_t X1 = LERP(grad16(P(AA), xx, yy, zz), grad16(P(BA), xx - N, yy, zz), u);
  int16_t X2 = LERP(grad16(P(AB), xx, yy-N, zz), grad16(P(BB), xx - N, yy - N, zz), u);
  int16_t X3 = LERP(grad16(P(AA+1), xx, yy, zz-N), grad16(P(BA+1), xx - N, yy, zz-N), u);
  int16_t X4 = LERP(grad16(P(AB+1), xx, yy-N, zz-N), grad16(P(BB+1), xx - N, yy - N, zz - N), u);

  int16_t Y1 = LERP(X1,X2,v);
  int16_t Y2 = LERP(X3,X4,v);

  int16_t ans = LERP(Y1,Y2,w);

  return ans;
}


/**
 * linear interpolation between two signed 15-bit values, with 8-bit fraction
 */
int16_t Utils::lerp15by16( int16_t a, int16_t b, fract16 frac ) {
  int16_t result;
  if( b > a) {
      uint16_t delta = b - a;
      uint16_t scaled = scale16( delta, frac);
      result = a + scaled;
  } else {
      uint16_t delta = a - b;
      uint16_t scaled = scale16( delta, frac);
      result = a - scaled;
  }
  return result;
}


/**
 * Scale down a RGB to N 256ths of it's current brightness, using
 * 'plain math' dimming rules, which means that if the low light levels
 * may dim all the way to 100% black.
 */
RgbColor Utils::nscale8x3( const RgbColor& color, fract8 scale ) {
  uint16_t scale_fixed = scale + 1;
  uint8_t r = (((uint16_t)color.R) * scale_fixed) >> 8;
  uint8_t g = (((uint16_t)color.G) * scale_fixed) >> 8;
  uint8_t b = (((uint16_t)color.B) * scale_fixed) >> 8;
  return RgbColor( r, g, b );
}


/**
 * Scale three one byte values by a fourth one, which is treated as
 * the numerator of a fraction whose demominator is 256
 * In other words, it computes r,g,b * (scale / 256), ensuring
 * that non-zero values passed in remain non zero, no matter how low the scale
 * argument.
 */
RgbColor Utils::nscale8x3_video( const RgbColor& color, fract8 scale ) {
  uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
  uint8_t r = (color.R == 0) ? 0 : (((int)color.R * (int)(scale) ) >> 8) + nonzeroscale;
  uint8_t g = (color.G == 0) ? 0 : (((int)color.G * (int)(scale) ) >> 8) + nonzeroscale;
  uint8_t b = (color.B == 0) ? 0 : (((int)color.B * (int)(scale) ) >> 8) + nonzeroscale;
  return RgbColor( r, g, b );
}


inline RgbColor Utils::toRgbColor( uint32_t hexColor ) {
  return RgbColor( HtmlColor( hexColor ));
}


/**
 * Add one byte to another, saturating at 0xFF
 * @param i - first byte to add
 * @param j - second byte to add
 * @returns the sum of i & j, capped at 0xFF
 */
inline uint8_t Utils::qadd8( uint8_t i, uint8_t j ) {
  unsigned int t = i + j;
  if( t > 255) t = 255;
  return t;
}


/**
 * Returns a new, random wheel index with a minimum distance of 42 from pos.
 * Picked up from https://github.com/Aircoookie/WLED.
 */
uint8_t Utils::randomWheelIndex( uint8_t pos ) {
  uint8_t r = 0, x = 0, y = 0, d = 0;

  while( d < 42 ) {
    r = random( 0, 255 );
    x = abs( pos - r );
    y = 255 - x;
    d = min( x, y );
  }
  return r;
}


uint8_t Utils::scale8( uint8_t i, fract8 scale ) {
  return (((uint16_t)i) * ((uint16_t)(scale)+1)) >> 8;
}

/** The "video" version of scale8 guarantees that the output will
 *  be only be zero if one or both of the inputs are zero.  If both
 *  inputs are non-zero, the output is guaranteed to be non-zero.
 *  This makes for better 'video'/LED dimming, at the cost of
 *  several additional cycles.
 */
uint8_t Utils::scale8_video( uint8_t i, fract8 scale ) {
  return (((int)i * (int)scale) >> 8) + ((i && scale)? 1 : 0);
}

/**
 * scale a 16-bit unsigned value by a 16-bit value,
 * considered as numerator of a fraction whose denominator
 * is 65536. In other words, it computes i * (scale / 65536)
 */
uint16_t Utils::scale16( uint16_t i, fract16 scale ) {
  uint16_t result = ((uint32_t)(i) * (1+(uint32_t)(scale))) / 65536;
  return result;
}


/**
 * Fast 8-bit approximation of sin(x). This approximation never varies more than
 * 2% from the floating point value you'd get by doing
 *   float s = (sin(x) * 128.0) + 128;
 *
 * @param theta input angle from 0-255
 * @returns sin of theta, value between 0 and 255
 */
uint8_t Utils::sin8( uint8_t theta ) {
  uint8_t offset = theta;
  if( theta & 0x40 ) {
      offset = (uint8_t)255 - offset;
  }
  offset &= 0x3F; // 0..63

  uint8_t secoffset  = offset & 0x0F; // 0..15
  if( theta & 0x40 ) secoffset++;

  uint8_t section = offset >> 4; // 0..3
  uint8_t s2 = section * 2;
  const uint8_t* p = b_m16_interleave;
  p += s2;
  uint8_t b   =  *p;
  p++;
  uint8_t m16 =  *p;

  uint8_t mx = (m16 * secoffset) >> 4;

  int8_t y = mx + b;
  if( theta & 0x80 ) y = -y;

  y += 128;

  return y;
}

/**
 * triwave8: triangle (sawtooth) wave generator.  Useful for
 *           turning a one-byte ever-increasing value into a
 *           one-byte value that oscillates up and down.
 *
 *           input         output
 *           0..127        0..254 (positive slope)
 *           128..255      254..0 (negative slope)
 */
uint8_t Utils::triwave8( uint8_t in ) {
  if( in & 0x80 ) {
    in = 255 - in;
  }
  uint8_t out = in << 1;
  return out;
}
