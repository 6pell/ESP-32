#include "SDUtils.h"

String SDUtils::toString( sdcard_type_t type ) {
  switch( type ){
    case CARD_NONE:     return "none";
    case CARD_MMC:      return "MMC";
    case CARD_SD:       return "SDSC";
    case CARD_SDHC:     return "SDHC";
    default:            return "unknown";
  }
}

