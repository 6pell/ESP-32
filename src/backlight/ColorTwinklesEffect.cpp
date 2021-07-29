#include "backlight/ColorTwinklesEffect.h"
#include "backlight/utils.h"

using namespace Backlight;

ColorTwinklesEffect::ColorTwinklesEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
    : DynamicEffect(strip, animator), data(strip.getPixelsCount()) {
  capabilities.hasSpeed = true;
  capabilities.hasIntensity = true;
}

void ColorTwinklesEffect::perform() {
  fadeUpAmount = 10 + (speed / 4);
  fadeDownAmount = 5 + (speed / 7);

  animator.StartAnimation( 0, FRAMETIME, [this](const AnimationParam& param) {
    if( param.state == AnimationState_Completed ) {
      draw();
      animator.RestartAnimation( 0 );
    }
  });
  startTicker();
}

void ColorTwinklesEffect::draw() {
  const int16_t count = strip.getPixelsCount();
  for( uint16_t i = 0; i < count; i++ ) {
    const RgbColor pixelColor = strip.getPixelColor( i );
    const bool fadeUp = data[i];
    if( fadeUp ) {
      RgbColor newColor = Utils::sumColors( pixelColor, Utils::nscale8x3( pixelColor, fadeUpAmount ));
      strip.setPixelColor( i, newColor );
      if( newColor.R == 255 || newColor.G == 255 || newColor.B == 255 ) {
        data[i] = false;
      }
      // fix "stuck" pixels
      newColor = strip.getPixelColor( i );
      if( Utils::equals( pixelColor, newColor )) {
        strip.setPixelColor( i, Utils::sumColors( pixelColor, pixelColor ));
      }
    } else {
      const RgbColor pixelColor = strip.getPixelColor( i );
      strip.setPixelColor( i, Utils::nscale8x3( pixelColor, 255 - fadeDownAmount ));
    }
  }

  for( uint16_t j = 0; j <= count / 50; j++ ) {
    if( random( 0, 255 ) <= intensity ) {
      // attempt to spawn a new pixel 5 times
      for( uint8_t times = 0; times < 5; times++ ) {
        const int i = random( 0, count-1 );
        const HtmlColor hc = strip.getPixelColor( i );
        if( hc.Color == 0 ) {
          const RgbColor pixelColor = Utils::colorFromPalette( *palette, random(0, 255), 64, NOBLEND );
          data[i] = true;
          strip.setPixelColor( i, pixelColor );
          // only spawn 1 new pixel per frame per 50 LEDs
          break;
        }
      }
    }
  }
}