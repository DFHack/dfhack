#include "../../include/util/ofsub.h"

// sign flag
int8 __SETS__(int x)
{
  if ( sizeof(int) == 1 )
    return int8(x) < 0;
  if ( sizeof(int) == 2 )
    return int16(x) < 0;
  if ( sizeof(int) == 4 )
    return int32(x) < 0;
  return int64(x) < 0;
}

// overflow flag of subtraction (x-y)
int8 __OFSUB__(int x, int y)
{
  int y2 = y;
  int8 sx = __SETS__(x);
  return (sx ^ __SETS__(y2)) & (sx ^ __SETS__(x-y2));
}
