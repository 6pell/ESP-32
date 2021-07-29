#pragma once
#include <algorithm>
#include <array>
#include <deque>
#include "Config.h"
#include "Module.h"

class LogManagerModule : public Module, public Print {

/* BufferLine */

struct BufferLine {
  char buffer[Config::JSON_MESSAGE_SIZE+1];
  uint16_t size = 0;

  void append( char c ) {
    if( size < Config::JSON_MESSAGE_SIZE ) {
      buffer[size] = c;
      size++;
    }
  }

  void set( char c ) {
    buffer[size] = c;
    size = 1;
  }

  void set( const String& s ) {
    unsigned int len = std::min( s.length(), Config::JSON_MESSAGE_SIZE );
    strncpy( buffer, s.c_str(), len );
    size = len;
  }

  void terminate() {
    buffer[size] = '\0';
  }
};

/* LogManagerModule */

private:
  std::deque<BufferLine*> buffer;
  bool newline;
  bool modified;

public:
  LogManagerModule();
  virtual ~LogManagerModule();
  virtual void tick_100mS( uint8_t phase );
  virtual const char* getId()    { return LOG_MODULE; }
  virtual const char* getName()  { return "Log Manager"; }

  virtual size_t write( uint8_t symbol );

  String getLogString();
  void setListenCommandEvents( bool value );
  void setLogLevel( int level );
};