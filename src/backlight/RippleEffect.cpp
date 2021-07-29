#include "backlight/RippleEffect.h"
#include "backlight/utils.h"

using namespace Backlight;

RippleEffect::RippleEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
    : DynamicEffect(strip, animator) {
  capabilities.hasSpeed = true;
  capabilities.hasIntensity = true;
  maxRipples = strip.getPixelsCount() / 4;
  fillColor = RgbColor( 0, 0, 0 );
  // pre-fill the ripples store
  uint16_t cnt = maxRipples;
  while( cnt-- ) {
    store.push_back( new RippleData() );
  }
}

RippleEffect::~RippleEffect() {
  for( auto p : store ) {
    delete p;
  }
  store.clear();
}

void RippleEffect::perform() {
  //Log.verbose( "FX ripple started, maxRipples=%d intencity=%d" CR, maxRipples, strip.getBrightness() );
  animator.StartAnimation( 0, FRAMETIME, [this](const AnimationParam& param) {
    if( param.state == AnimationState_Completed ) {
      drawWave();
      animator.RestartAnimation( 0 );
    }
  });
  startTicker();
}

void RippleEffect::drawWave() {
  if( maxRipples == 0 ) {
    strip.clearTo( color );
  } else {
    strip.clearTo( fillColor );
    // draw wave
    for( uint16_t rippleIndex = 0; rippleIndex < maxRipples; rippleIndex++ ) {
      RippleData* ripple = store.at( rippleIndex );
      uint16_t state = ripple->state;
      if( state ) {
        uint8_t decay = (speed >> 4) + 1;                                       // faster decay if faster propagation
        uint16_t propagation = ((state / decay - 1) * speed);
        int16_t propI = propagation >> 8;
        uint8_t propF = propagation & 0xFF;
        int16_t left = ripple->waveOrigin - propI - 1;
        uint8_t amp = (state < 17) ? Utils::triwave8( (state - 1) * 8 ) : map( state, 17, 255, 255, 2 );

        //const RgbColor col = Utils::colorWheelValue( ripple->colorIndex );
        const RgbColor col = Utils::colorFromPalette( *palette, ripple->colorIndex, 255, LINEARBLEND );

        for( int16_t v = left; v < left + 4; v++ ) {
          uint8_t mag = Utils::scale8( Utils::cubicwave8( (propF >> 2) + (v - left) * 64 ), amp );
          if( v >= 0 ) {
            strip.setPixelColor( v, Utils::colorBlend( strip.getPixelColor( v ), col, mag ));
          }
          int16_t w = left + propI * 2 + 3 - (v-left);
          if( w <= strip.getPixelsCount() && w >= 0 ) {
            strip.setPixelColor( w, Utils::colorBlend( strip.getPixelColor( w ), col, mag ));
          }
        }
        state += decay;
        ripple->state = (state > 254) ? 0 : state;
      } else {
        // Randomly create a new wave.
        if( random( 0, 5100 + 10000 ) <= intensity ) {
          ripple->state = 1;
          ripple->waveOrigin = random( 0, strip.getPixelsCount() - 1 );
          ripple->colorIndex = random( 0, 255 );
          //Log.verbose( "FX wave index=%d, origin=%d, color=%d" CR, &ripple, rippleIndex, ripple->waveOrigin, ripple->colorIndex );
        }
      }
    }
  }
}