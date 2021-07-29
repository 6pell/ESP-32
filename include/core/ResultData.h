#pragma once
#include <WString.h>
#include "Messages.h"

/* Command return code */

enum RetCode {
  RC_UNCHANGED       = 0x00,
  RC_ERROR           = 0x01,
  RC_UNKNOWN_COMMAND = 0x02,
  RC_UNKNOWN_OPTION  = 0x02,
  RC_INVALID_VALUE   = 0x04,
  RC_OK              = 0x10,
  RC_OK_REINIT       = 0x20,
  RC_OK_RESTART      = 0x40
};

/* Command execution or options applying results */

struct ResultData {
  RetCode  code;
  String   details;
};

static const ResultData RESULT_OK       = {RC_OK, Messages::OK};
static const ResultData UNKNOWN_COMMAND = {RC_UNKNOWN_COMMAND, Messages::COMMAND_UNKNOWN};
static const ResultData UNKNOWN_OPTION  = {RC_UNKNOWN_OPTION, Messages::SETTINGS_UNKNOWN_OPTION};
static const ResultData INVALID_VALUE   = {RC_INVALID_VALUE, Messages::COMMAND_INVALID_VALUE};
