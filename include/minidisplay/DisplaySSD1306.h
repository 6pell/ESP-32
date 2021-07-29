#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "SimpleMap.h"

/* Display_SSD1306 */

class Display_SSD1306 : public Adafruit_SSD1306 {
private:
  int16_t textLeftPadding = 0;
  SimpleMap<String, String> templateParams;

public:
  Display_SSD1306( uint8_t w, uint8_t h ) :
    Adafruit_SSD1306( w, h ),
    templateParams( [](String& a, String& b) -> int {
      return b.compareTo( a );
    }) {}

  void clearTemplateParameters()                                       { templateParams.clear(); }
  void removeTemplateParameter( const String& key )                    { templateParams.remove( key ); }
  void setTemplateParameter( const String& key, const String& value )  { templateParams.put( key, value); }
  size_t printLine( const String& s );
  void setLeftPadding( int16_t padding )  { textLeftPadding = padding; }
  virtual size_t write( uint8_t c );


private:
  String resolveTemplateKey( const String& key );
};
