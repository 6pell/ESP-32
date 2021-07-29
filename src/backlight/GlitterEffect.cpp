#include "backlight/GlitterEffect.h"
#include "backlight/utils.h"

using namespace Backlight;

GlitterEffect::GlitterEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
  : DynamicEffect(strip, animator) {
  capabilities.hasSpeed = true;
  capabilities.hasIntensity = true;
}

void GlitterEffect::perform() {
  animator.StartAnimation( 0, FRAMETIME, [this](const AnimationParam& param) {
    if( param.state == AnimationState_Completed ) {
      drawPalette();
      if( intensity > random( 0, 255 )) {
        strip.setPixelColor( random( 0, strip.getPixelsCount() - 1 ), RgbColor( 255, 255, 255 ));
      }
      animator.RestartAnimation( 0 );
    }
  });
  startTicker();
}

void GlitterEffect::drawPalette() {
  uint16_t counter = 0;
  if( speed != 0 ) {
    counter = (millis() * ((speed >> 3) + 1)) & 0xFFFF;
    counter = counter >> 8;
  }

  //bool noWrap = (paletteBlend == 2 || (paletteBlend == 0 && speed == 0));
  const int16_t count = strip.getPixelsCount();
  for( uint16_t i = 0; i < count; i++ ) {
    uint8_t colorIndex = (i * 255 / count) - counter;
    //if (noWrap) colorIndex = map( colorIndex, 0, 255, 0, 240 ); //cut off blend at palette "end"
    //setPixelColor(SEGMENT.start + i, color_from_palette(colorIndex, false, true, 255));
    strip.setPixelColor( i, Utils::colorFromPalette( *palette, colorIndex, 255, LINEARBLEND ));
    //strip.setPixelColor( i, Utils::colorWheelValue( colorIndex ));
  }
}