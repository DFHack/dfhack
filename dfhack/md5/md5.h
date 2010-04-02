/*
 *	This is the C++ implementation of the MD5 Message-Digest
 *	Algorithm desrcipted in RFC 1321.
 *	I translated the C code from this RFC to C++.
 *	There is now warranty.
 *
 *	Feb. 12. 2005
 *	Benjamin Grüdelbach
 */

/*
 * Changed unsigned long int types into uint32_t to make this work on 64bit systems.
 * Sep. 5. 2009
 * Petr Mrázek
 */

/*
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

//----------------------------------------------------------------------
//include protection
#ifndef MD5_H
#define MD5_H

//----------------------------------------------------------------------
//STL includes
#include <string>
#include "../integers.h"
//----------------------------------------------------------------------
//typedefs
typedef unsigned char *POINTER;

/*
 * MD5 context.
 */
typedef struct
{
	uint32_t state[4];   	      /* state (ABCD) */
	uint32_t count[2]; 	      /* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];	      /* input buffer */
} MD5_CTX;

/*
 * MD5 class
 */
class MD5
{

	private:

        void MD5Transform (uint32_t state[4], unsigned char block[64]);
		void Encode (unsigned char*, uint32_t*, unsigned int);
		void Decode (uint32_t*, unsigned char*, unsigned int);
		void MD5_memcpy (POINTER, POINTER, unsigned int);
		void MD5_memset (POINTER, int, unsigned int);

	public:

		void MD5Init (MD5_CTX*);
		void MD5Update (MD5_CTX*, unsigned char*, unsigned int);
		void MD5Final (unsigned char [16], MD5_CTX*);

	MD5(){};
};

//----------------------------------------------------------------------
//End of include protection
#endif

/*
 * EOF
 */
