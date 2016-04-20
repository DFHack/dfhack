#ifndef OFSUB_H
#define OFSUB_H

/*
********************************************************************************************
* IDA stuff
********************************************************************************************
*/

#if defined(__GNUC__)
typedef long long ll;

#elif defined(_MSC_VER)
typedef __int64 ll;

#endif
typedef char   int8;
typedef short  int16;
typedef int    int32;
typedef ll     int64;



// sign flag
int8 __SETS__(int x);

// overflow flag of subtraction (x-y)
int8 __OFSUB__(int x, int y);


#endif // OFSUB_H
