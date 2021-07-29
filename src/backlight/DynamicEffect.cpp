#include "backlight/DynamicEffect.h"
#include "backlight/RgbPalette.h"

using namespace Backlight;

RgbPalette16* DynamicEffect::createPalette( const String& id ) {
  // Default palette differs dependig on effect.
  if( id == "default" ) {
    return RgbPalette16::fromId( getDefaultPaletteId() );
  }
  // Periodically replace palette with a random one.
  else if( id == "random" ) {
    return new RgbPalette16(
      CHSV( random(0, 255), 255, random(128, 255) ),
      CHSV( random(0, 255), 255, random(128, 255) ),
      CHSV( random(0, 255), 192, random(128, 255) ),
      CHSV( random(0, 255), 255, random(128, 255) )
    );
  }
  else {
    return RgbPalette16::fromId( id );
  }
}

/*
void WS2812FX::handle_palette(void)
{
  bool singleSegmentMode = (_segment_index == _segment_index_palette_last);
  _segment_index_palette_last = _segment_index;

  byte paletteIndex = SEGMENT.palette;
  if (SEGMENT.mode == FX_MODE_GLITTER && paletteIndex == 0) paletteIndex = 11;
  if (SEGMENT.mode >= FX_MODE_METEOR && paletteIndex == 0) paletteIndex = 4;

  switch (paletteIndex)
  {
    case 0: {//default palette. Differs depending on effect
      switch (SEGMENT.mode)
      {
        case FX_MODE_FIRE_2012  : targetPalette = gGradientPalettes[22]; break;//heat palette
        case FX_MODE_COLORWAVES : targetPalette = gGradientPalettes[13]; break;//landscape 33
        case FX_MODE_FILLNOISE8 : targetPalette = OceanColors_p;         break;
        case FX_MODE_NOISE16_1  : targetPalette = gGradientPalettes[17]; break;//Drywet
        case FX_MODE_NOISE16_2  : targetPalette = gGradientPalettes[30]; break;//Blue cyan yellow
        case FX_MODE_NOISE16_3  : targetPalette = gGradientPalettes[22]; break;//heat palette
        case FX_MODE_NOISE16_4  : targetPalette = gGradientPalettes[13]; break;//landscape 33
        //case FX_MODE_GLITTER    : targetPalette = RainbowColors_p;       break;

        default: targetPalette = PartyColors_p; break;//palette, bpm
      }
      break;}
    case 1: {//periodically replace palette with a random one. Doesn't work with multiple FastLED segments
      if (!singleSegmentMode)
      {
        targetPalette = PartyColors_p; break; //fallback
      }
      if (millis() - _lastPaletteChange > 1000 + ((uint32_t)(255-SEGMENT.intensity))*100)
      {
        targetPalette = CRGBPalette16(
                        CHSV(random8(), 255, random8(128, 255)),
                        CHSV(random8(), 255, random8(128, 255)),
                        CHSV(random8(), 192, random8(128, 255)),
                        CHSV(random8(), 255, random8(128, 255)));
        _lastPaletteChange = millis();
      } break;}
    case 2: {//primary color only
      CRGB prim = col_to_crgb(SEGCOLOR(0));
      targetPalette = CRGBPalette16(prim); break;}
    case 3: {//based on primary
      //considering performance implications
      CRGB prim = col_to_crgb(SEGCOLOR(0));
      CHSV prim_hsv = rgb2hsv_approximate(prim);
      targetPalette = CRGBPalette16(
                      CHSV(prim_hsv.h, prim_hsv.s, prim_hsv.v), //color itself
                      CHSV(prim_hsv.h, max(prim_hsv.s - 50,0), prim_hsv.v), //less saturated
                      CHSV(prim_hsv.h, prim_hsv.s, max(prim_hsv.v - 50,0)), //darker
                      CHSV(prim_hsv.h, prim_hsv.s, prim_hsv.v)); //color itself
      break;}
    case 4: {//primary + secondary
      CRGB prim = col_to_crgb(SEGCOLOR(0));
      CRGB sec  = col_to_crgb(SEGCOLOR(1));
      targetPalette = CRGBPalette16(sec,prim); break;}
    case 5: {//based on primary + secondary
      CRGB prim = col_to_crgb(SEGCOLOR(0));
      CRGB sec  = col_to_crgb(SEGCOLOR(1));
      CRGB ter  = col_to_crgb(SEGCOLOR(2));
      targetPalette = CRGBPalette16(ter,sec,prim); break;}
    case 6: //Party colors
      targetPalette = PartyColors_p; break;
    case 7: //Cloud colors
      targetPalette = CloudColors_p; break;
    case 8: //Lava colors
      targetPalette = LavaColors_p; break;
    case 9: //Ocean colors
      targetPalette = OceanColors_p; break;
    case 10: //Forest colors
      targetPalette = ForestColors_p; break;
    case 11: //Rainbow colors
      targetPalette = RainbowColors_p; break;
    case 12: //Rainbow stripe colors
      targetPalette = RainbowStripeColors_p; break;
    default: //progmem palettes
      targetPalette = gGradientPalettes[constrain(SEGMENT.palette -13, 0, gGradientPaletteCount -1)];
  }

  if (singleSegmentMode && paletteFade) //only blend if just one segment uses FastLED mode
  {
    nblendPaletteTowardPalette(currentPalette, targetPalette, 48);
  } else
  {
    currentPalette = targetPalette;
  }
}
*/