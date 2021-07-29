#include <ArduinoLog.h>
#include "Events.h"
#include "LogManagerModule.h"
#include "Options.h"

/* Public */

LogManagerModule::LogManagerModule() {
  properties.tick_100mS_required = true;

  // Pre-allocate buffer lines
  for( int i = 0; i < Config::LOG_MAX_LINES; i++ ) {
    buffer.push_front( new BufferLine() );
  }
  // Init Arduino-Log library with log level and log output.
  Log.begin( getByteOption( "LeveL", Config::LOG_LEVEL ), this );
}


LogManagerModule::~LogManagerModule() {
  int size = buffer.size();
  for( int i = 0; i < size; i++ ) {
    delete buffer[i];
  }
  buffer.clear();
}

void LogManagerModule::tick_100mS( uint8_t phase ) {
  if( modified ) {
    modified = false;
    // Inform subscribers about new log line appearance.
    Bus.notify( (LogUpdateEvent) {} );
  }
}

size_t LogManagerModule::write( uint8_t symbol ) {
  if( symbol != '\r' && symbol != '\0' ) {      // ignore these symbols
    if( symbol == '\n' ) {
      newline = true;
      modified = true;
    } else {
      if( newline ) {
        // A new line should be started at front.
        newline = false;
        BufferLine* line = buffer.back();
        buffer.pop_back();
        line->set( symbol );
        buffer.push_front( line );
      } else {
        // Continue to fill the front line.
        BufferLine* line = buffer.front();
        line->append( symbol );
      }
    }
    Serial.print( (char)symbol );
  }
  return 1;
}

String LogManagerModule::getLogString() {
  // Calc the total length of log lines.
  int total = 0, size = buffer.size();
  for( int i = size-1; i >= 0; i-- ) {
    BufferLine* line = buffer[i];
    if( line->size > 0 ) {
      total += line->size + 1;
    }
  }
  // Build the resulting string.
  String result;
  if( total > 0 ) {
    result.reserve( total );
    for( int i = size-1; i >= 0; i-- ) {
      BufferLine* line = buffer[i];
      if( line->size > 0 ) {
        line->terminate();
        if( result.length() > 0 ) result += '\n';
        result += String( line->buffer );
      }
    }
  }
  return result;
}

void LogManagerModule::setLogLevel( int level ) {
  Log.begin( level, this );
}