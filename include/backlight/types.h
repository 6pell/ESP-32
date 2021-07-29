#pragma once
#include <stdint.h>

namespace Backlight {

  typedef uint8_t   fract8;       // ANSI unsigned short _Fract. range is 0 to 0.99609375 in steps of 0.00390625
  typedef uint16_t  fract16;      // ANSI: unsigned _Fract.  range is 0 to 0.99998474121 in steps of 0.00001525878
  typedef uint16_t  accum88;      // ANSI: unsigned short _Accum.  8 bits int, 8 bits fraction

  #define saccum87  int16_t


  #define AVG15(U,V)                (avg15((U),(V)))
  #define EASE16(x)                 (ease16InOutQuad(x))
  #define FADE(x)                   scale16(x,x)
  #define LERP(a,b,u)               lerp15by16(a,b,u)
  #define P(x)                      FL_PGM_READ_BYTE_NEAR(p + x)
  #define FL_PGM_READ_BYTE_NEAR(x)  (*((const  uint8_t*)(x)))


  struct Capabilities {
    bool hasSpeed     : 1;
    bool hasIntensity : 1;
    bool hasColor     : 1;
    bool hasPalette   : 1;
  };

  typedef enum { NOBLEND=0, LINEARBLEND=1 } BlendType;

  typedef uint32_t RGBPalette16Type[16];

  //  You can also define a static RGB palette very compactly in terms of a series
  //  of connected color gradients.
  //  For example, if you want the first 3/4ths of the palette to be a slow
  //  gradient ramping from black to red, and then the remaining 1/4 of the
  //  palette to be a quicker ramp to white, you specify just three points: the
  //  starting black point (at index 0), the red midpoint (at index 192),
  //  and the final white point (at index 255).  It looks like this:
  //
  //    index:  0                                    192          255
  //            |----------r-r-r-rrrrrrrrRrRrRrRrRRRR-|-RRWRWWRWWW-|
  //    color: (0,0,0)                           (255,0,0)    (255,255,255)
  //
  //  Here's how you'd define that gradient palette:
  //
  //    DEFINE_GRADIENT_PALETTE( black_to_red_to_white_p ) {
  //          0,      0,  0,  0,    /* at index 0, black(0,0,0) */
  //        192,    255,  0,  0,    /* at index 192, red(255,0,0) */
  //        255,    255,255,255    /* at index 255, white(255,255,255) */
  //    };
  //
  //  This format is designed for compact storage.  The example palette here
  //  takes up just 12 bytes of PROGMEM (flash) storage, and zero bytes
  //  of SRAM when not currently in use.
  //
  //  To use one of these gradient palettes, simply assign it into a
  //  CRGBPalette16 or a CRGBPalette256, like this:
  //
  //    CRGBPalette16 pal = black_to_red_to_white_p;
  //
  //  When the assignment is made, the gradients are expanded out into
  //  either 16 or 256 palette entries, depending on the kind of palette
  //  object they're assigned to.
  //
  //  IMPORTANT NOTES & CAVEATS:
  //
  //  - The last 'index' position MUST BE 255!  Failure to end with
  //    index 255 will result in program hangs or crashes.

  //#define DEFINE_GRADIENT_PALETTE(X) RGBGradientPaletteByteType X[]

  typedef union {
    struct {
        uint8_t index;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    uint32_t dword;
    uint8_t  bytes[4];
  } RGBGradientPaletteEntry;


  // Representation of an HSV pixel (hue, saturation, value (aka brightness)).
  struct CHSV {
    union {
      struct {
        union {
          uint8_t hue;
          uint8_t h;
        };
        union {
          uint8_t saturation;
          uint8_t sat;
          uint8_t s;
        };
        union {
          uint8_t value;
          uint8_t val;
          uint8_t v; };
        };
        uint8_t raw[3];
    };

    /// default values are UNITIALIZED
    inline CHSV() __attribute__((always_inline)) {
    }

    /// allow construction from H, S, V
    inline CHSV( uint8_t ih, uint8_t is, uint8_t iv) __attribute__((always_inline))
      : h(ih), s(is), v(iv) {
    }

    /// allow copy construction
    inline CHSV(const CHSV& rhs) __attribute__((always_inline)) {
      h = rhs.h;
      s = rhs.s;
      v = rhs.v;
    }

    inline CHSV& operator= (const CHSV& rhs) __attribute__((always_inline)) {
      h = rhs.h;
      s = rhs.s;
      v = rhs.v;
      return *this;
    }

    inline CHSV& setHSV(uint8_t ih, uint8_t is, uint8_t iv) __attribute__((always_inline)) {
      h = ih;
      s = is;
      v = iv;
      return *this;
    }
  };


  // fill_gradient - fill an array of colors with a smooth HSV gradient
  //                 between two specified HSV colors.
  //                 Since 'hue' is a value around a color wheel,
  //                 there are always two ways to sweep from one hue
  //                 to another.
  //                 This function lets you specify which way you want
  //                 the hue gradient to sweep around the color wheel:
  //                   FORWARD_HUES: hue always goes clockwise
  //                   BACKWARD_HUES: hue always goes counter-clockwise
  //                   SHORTEST_HUES: hue goes whichever way is shortest
  //                   LONGEST_HUES: hue goes whichever way is longest
  //                 The default is SHORTEST_HUES, as this is nearly
  //                 always what is wanted.
  //
  // fill_gradient can write the gradient colors EITHER
  //     (1) into an array of CRGBs (e.g., into leds[] array, or an RGB Palette)
  //   OR
  //     (2) into an array of CHSVs (e.g. an HSV Palette).
  //
  //   In the case of writing into a CRGB array, the gradient is
  //   computed in HSV space, and then HSV values are converted to RGB
  //   as they're written into the RGB array.

  typedef enum { FORWARD_HUES, BACKWARD_HUES, SHORTEST_HUES, LONGEST_HUES } TGradientDirectionCode;

}

