#pragma once
#include <ArduinoLog.h>
#include <NeoPixelBus.h>
#include "Messages.h"
#include "backlight/types.h"

namespace Backlight {

  // The palette ID must have up to 9 chars
  static const char* const PALETTE_ID[] = {
    "",
    "default",           // 1    // Default palette. Differs depending on effect.
    "random",            // 2    // Periodically generate a new random palette.
    "party",             // 3
    "drywet",            // 4
    "ocean",             // 5
    "forest",            // 6
    "rainbow",           // 7
    "fire",              // 8
    "tiamat",            // 9
  };

  static const char* const PALETTE_TITLE[] = {
    Messages::PALETTE_SELECT,
    "Default",
    "Random",
    "Party",
    "Drywet",
    "Ocean",
    "Forest",
    "Rainbow",
    "Fire",
    "Tiamat",
  };

  class RgbPalette16 {
    public:
      static const RGBPalette16Type         PartyColorsPalette;
      static const RGBPalette16Type         OceanPalette;
      static const RGBPalette16Type         ForestPalette;
      static const RGBPalette16Type         RainbowPalette;
      static const RGBGradientPaletteEntry  DrywetPalette[];
      static const RGBGradientPaletteEntry  FirePalette[];
      static const RGBGradientPaletteEntry  TiamatPalette[];

      static RgbPalette16* fromId( const String& id );

      /**/

      RgbColor entries[16];

      RgbPalette16( const CHSV& c1, const CHSV& c2, const CHSV& c3, const CHSV& c4 ) {
        fill_gradient( &(entries[0]), 16, c1, c2, c3, c4 );
      }

      RgbPalette16( const RGBPalette16Type& rhs ) {
        for( uint8_t i = 0; i < 16; i++ ) {
          entries[i] = HtmlColor( *( rhs + i ));
        }
      }

      RgbPalette16( const RGBGradientPaletteEntry* pal );

      const RgbColor& operator[] (uint8_t x) const {
        return entries[x];
      }

      static uint8_t getPalettesCount() {
        return sizeof(PALETTE_ID) / sizeof(PALETTE_ID[0]);
      }

      static const String getPaletteId( uint8_t index ) {
        return PALETTE_ID[index];
      }

      static const String getPaletteTitle( uint8_t index ) {
        return PALETTE_TITLE[index];
      }

    private:
      static int8_t findPaletteById( const String& effectId );

      static void fill_gradient( RgbColor* target, uint16_t startpos, CHSV startcolor, uint16_t endpos, CHSV endcolor,
                                 TGradientDirectionCode directionCode  = SHORTEST_HUES );
      static void fill_gradient( RgbColor* target, uint16_t numLeds, const CHSV& c1, const CHSV& c2, const CHSV& c3, const CHSV& c4 );
      static void fill_gradient_rgb( RgbColor* target, uint16_t startpos, RgbColor startcolor, uint16_t endpos, RgbColor endcolor );
      static void hsv2rgb_rainbow( const CHSV& hsv, RgbColor& rgb );
  };
}

