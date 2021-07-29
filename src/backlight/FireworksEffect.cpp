#include "backlight/FireworksEffect.h"
#include "backlight/utils.h"

using namespace Backlight;

FireworksEffect::FireworksEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
    : DynamicEffect(strip, animator) {
  capabilities.hasSpeed = true;
  capabilities.hasIntensity = true;
}


void FireworksEffect::perform() {
  aux1 = UINT16_MAX;
  aux2 = UINT16_MAX;
  animator.StartAnimation( 0, FRAMETIME, [this](const AnimationParam& param) {
    if( param.state == AnimationState_Completed ) {
      drawFireworks();
      animator.RestartAnimation( 0 );
    }
  });
  startTicker();
}

void FireworksEffect::drawFireworks() {
  fadeOut( 0 );
  uint16_t count = strip.getPixelsCount();
  bool valid1 = (aux1 < count && aux1 >= 0);
  bool valid2 = (aux2 < count && aux2 >= 0);
  RgbColor sv1, sv2;
  if (valid1) sv1 = strip.getPixelColor( aux1 );
  if (valid2) sv2 = strip.getPixelColor( aux2 );
  blur( 255 - speed );
  if (valid1) strip.setPixelColor( aux1, sv1 );
  if (valid2) strip.setPixelColor( aux2, sv2 );

  for( uint16_t i = 0; i < max( 1, count / 20 ); i++ ) {
    if( random( 0, 129 - (intensity >> 1)) == 0 ) {
      uint16_t index = random( 0, count );
      //strip.setPixelColor( index, Utils::colorWheelValue( random( 0, 255 )));
      const RgbColor c = Utils::colorFromPalette( *palette, random(0, 255), 255, NOBLEND );
      strip.setPixelColor( index, c );

      aux2 = aux1;
      aux1 = index;
    }
  }
}
