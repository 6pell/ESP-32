#pragma once
#include "Messages.h"
#include "Effect.h"

namespace Backlight {

  namespace Factory {

    // The effect ID must have up to 13 chars. When the effect is persisted,
    // the "fx" letters are prepended to ID to avoid possible names collisions.
    // 2 + 13 = 15 - the max possible ESP32 preferences key length.
    static const char* const LIGHT_EFFECT_ID[] = {
      "",
      "static",            // 1
      "wavein",            // 2
      "rndcolors",         // 3
      "ripple",            // 4
      "fireworks",         // 5
      "rain",              // 6
      "twinkles",          // 7
      "glitter",           // 8
      "comet",             // 9
      "noise1",            // 10
    };

    static const char* const LIGHT_EFFECT_TITLE[] = {
      Messages::EFFECT_SELECT,
      "Static light",
      "Wave-in light",
      "Random colors",
      "Ripple",
      "Fireworks",
      "Rain",
      "Color twinkles",
      "Glitter",
      "Comet",
      "Noise 1",
    };

    /**
     * The factory method to instantiate effects.
     */
    Effect* createEffect( const String& id, NeoPixelWrapper* strip, NeoPixelAnimator& animations );

    constexpr uint8_t getEffectsCount() {
      return sizeof(LIGHT_EFFECT_ID) / sizeof(LIGHT_EFFECT_ID[0]);
    }

    const String getEffectId( uint8_t index );

    const String getEffectTitle( uint8_t index );

    int8_t findEffectById( const String& effectId );
  }
}

