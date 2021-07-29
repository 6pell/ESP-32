#include <Arduino.h>
#include "DisplayDriver.h"

void DisplayDriver::writeCommandSequence( const LcdInitCommand sequence[] ) {
  uint8_t i = 0;
  while( sequence[i].length != 0xFF ) {
    // write command
    writeCommand( sequence[i].cmd );
    // write data
    uint8_t c = sequence[i].length & 0x7F;
    const uint8_t* ptr = sequence[i].data;
    while( c-- ) {
      writeData( *ptr++ );
    }
    // optional delay
    if( sequence[i].length & 0x80 ) {
      delay( 120 );
    }
    // go next command in a sequence
    i++;
  }
}