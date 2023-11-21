#include "utils.h"

const char *region_str(enum Region r)
{
  switch(r)
  {
    case REGION_US:
      return "US";
    case REGION_EU:
      return "EU";
    case REGION_JP:
      return "JP";
    default:
      return "??";
  }
}