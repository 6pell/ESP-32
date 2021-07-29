#include "Config.h"
#include "backlight/Effect.h"
#include "backlight/utils.h"

using namespace Backlight;

/* Effect public */

const String Effect::getOptions() {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> doc;
  // brightness option
  doc[BRIGHTNESS_KEY] = strip.getBrightness();
  // color option
  if( capabilities.hasColor ) {
    char buf[8];
    HtmlColor html = HtmlColor( color );
    html.ToNumericalString( buf, sizeof(buf) );
    doc[COLOR_KEY] = buf;
  }
  // intensity option
  if( capabilities.hasIntensity ) {
    doc[INTENSITY_KEY] = intensity;
  }
  // speed option
  if( capabilities.hasSpeed ) {
    doc[SPEED_KEY] = speed;
  }
  return doc.as<String>();
}

void Effect::setOptions( const String& json ) {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> doc;
  DeserializationError rc = deserializeJson( doc, json );
  if( rc == DeserializationError::Ok && doc.size() > 0 ) {
    readOptions( doc );
  }
}


/* Effect protected */

/*
 * Blurs segment content, source: FastLED colorutils.cpp
 */
void Effect::blur( uint8_t blur_amount ) {
  uint8_t keep = 255 - blur_amount;
  uint8_t seep = blur_amount >> 1;
  RgbColor carryover = RgbColor( 0, 0, 0 );

  const uint16_t count = strip.getPixelsCount();
  for( uint16_t i = 0; i < count; i++ ) {
    RgbColor c = strip.getPixelColor( i );
    RgbColor part = Utils::nscale8x3( c, seep );
    RgbColor cur = Utils::nscale8x3( c, keep );
    cur = Utils::sumColors( cur, carryover );
    if( i > 0 ) {
      RgbColor prev = strip.getPixelColor( i - 1 );
      strip.setPixelColor( i - 1, Utils::sumColors( prev, part ));
    }
    strip.setPixelColor( i, cur );
    carryover = part;
  }
}

/**
 * Fade out function, higher rate = quicker fade.
 */
void Effect::fadeOut( uint8_t rate ) {
  rate = (255-rate) >> 1;
  float mappedRate = float(rate) + 1.1;

  const RgbColor target = RgbColor( 0, 0, 0 );
  int r2 = target.R;
  int g2 = target.G;
  int b2 = target.B;

  const uint16_t count = strip.getPixelsCount();
  for( uint16_t i = 0; i < count; i++ ) {
    const RgbColor color = strip.getPixelColor( i );
    int r1 = color.R;
    int g1 = color.G;
    int b1 = color.B;

    int rdelta = (r2 - r1) / mappedRate;
    int gdelta = (g2 - g1) / mappedRate;
    int bdelta = (b2 - b1) / mappedRate;

    // if fade isn't complete, make sure delta is at least 1 (fixes rounding issues)
    rdelta += (r2 == r1) ? 0 : (r2 > r1) ? 1 : -1;
    gdelta += (g2 == g1) ? 0 : (g2 > g1) ? 1 : -1;
    bdelta += (b2 == b1) ? 0 : (b2 > b1) ? 1 : -1;

    strip.setPixelColor( i, RgbColor( r1 + rdelta, g1 + gdelta, b1 + bdelta ));
  }
}

void Effect::readOptions( const JsonDocument& doc ) {
  // brightness option
  if( doc.containsKey( BRIGHTNESS_KEY )) {
    const uint8_t v = doc[BRIGHTNESS_KEY];
    strip.setBrightness( v );
  }
  // color option: convert hex string to RgbColor
  if( capabilities.hasColor && doc.containsKey( COLOR_KEY )) {
    HtmlColor html;
    html.Parse<HtmlShortColorNames>( doc[COLOR_KEY].as<String>() );
    color = RgbColor( html );
  }
  // intensity option
  if( capabilities.hasIntensity && doc.containsKey( INTENSITY_KEY )) {
    intensity = doc[INTENSITY_KEY];
  }
  // speed option
  if( capabilities.hasSpeed && doc.containsKey( SPEED_KEY )) {
    speed = doc[SPEED_KEY];
  }
}

uint16_t Effect::speedFormulaValue() {
  // #define SPEED_FORMULA_L  5 + (50*(255 - SEGMENT.speed))/SEGMENT_LENGTH
  return 1 + (50 * (255 - speed)) / strip.getPixelsCount();
}