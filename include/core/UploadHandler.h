#pragma once
#include "core/ResultData.h"

class UploadHandler {
public:
  virtual ~UploadHandler() {;}
  virtual ResultData begin( const String& action, const int total_size ) = 0;
  virtual ResultData uploadDataBlock( const char* data, const uint16_t data_size ) = 0;
  virtual ResultData end( const bool hasSuccessful ) = 0;
};