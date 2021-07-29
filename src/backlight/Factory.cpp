#include "backlight/Factory.h"

#include "backlight/StaticLightEffect.h"
#include "backlight/WaveInLightEffect.h"
#include "backlight/RandomColorsEffect.h"
#include "backlight/RippleEffect.h"
#include "backlight/FireworksEffect.h"
#include "backlight/RainEffect.h"
#include "backlight/ColorTwinklesEffect.h"
#include "backlight/GlitterEffect.h"
#include "backlight/CometEffect.h"
#include "backlight/Noise1Effect.h"

using namespace Backlight;

/* The factory method to instantiate effects */

Effect* Factory::createEffect( const String& id, NeoPixelWrapper* strip, NeoPixelAnimator& animations ) {
  switch( findEffectById( id )) {
    case 1:      return new StaticLightEffect( *strip, animations );
    case 2:      return new WaveInLightEffect( *strip, animations );
    case 3:      return new RandomColorsEffect( *strip, animations );
    case 4:      return new RippleEffect( *strip, animations );
    case 5:      return new FireworksEffect( *strip, animations );
    case 6:      return new RainEffect( *strip, animations );
    case 7:      return new ColorTwinklesEffect( *strip, animations );
    case 8:      return new GlitterEffect( *strip, animations );
    case 9:      return new CometEffect( *strip, animations );
    case 10:     return new Noise1Effect( *strip, animations );
    default:     return nullptr;
  }
}

const String Factory::getEffectId( uint8_t index ) {
  return LIGHT_EFFECT_ID[index];
}

const String Factory::getEffectTitle( uint8_t index ) {
  return LIGHT_EFFECT_TITLE[index];
}

int8_t Factory::findEffectById( const String& effectId ) {
  uint8_t count = sizeof(LIGHT_EFFECT_ID) / sizeof(LIGHT_EFFECT_ID[0]);
  for( uint8_t i = 0; i < count; i++ ) {
    const String id = LIGHT_EFFECT_ID[i];
    if( id == effectId ) {
      return i;
    }
  }
  return -1;
}