/*
 *	This is part of my wrapper-class to create
 *	a MD5 Hash from a string and a file.
 *
 *	This code is completly free, you
 *	can copy it, modify it, or do
 *	what ever you want with it.
 *
 *	Feb. 2005
 *	Benjamin Grüdelbach
 */

/*
 * Changed unsigned long int types into uint32_t to make this work on 64bit systems.
 * Sep. 5. 2009
 * Petr Mrázek
 */

//----------------------------------------------------------------------
//basic includes
#include <fstream>
#include <iostream>

//my includes
#include "md5wrapper.h"
#include "md5.h"

//---------privates--------------------------

/*
 * internal hash function, calling
 * the basic methods from md5.h
 */
std::string md5wrapper::hashit(std::string text)
{
	MD5_CTX ctx;

	//init md5
	md5->MD5Init(&ctx);
	//update with our string
	md5->MD5Update(&ctx,
		 (unsigned char*)text.c_str(),
		 text.length());

	//create the hash
	unsigned char buff[16] = "";
	md5->MD5Final((unsigned char*)buff,&ctx);

	//converte the hash to a string and return it
	return convToString(buff);
}

/*
 * converts the numeric hash to
 * a valid std::string.
 * (based on Jim Howard's code;
 * http://www.codeproject.com/cpp/cmd5.asp)
 */
std::string md5wrapper::convToString(unsigned char *bytes)
{
	char asciihash[33];

	int p = 0;
	for(int i=0; i<16; i++)
	{
		::sprintf(&asciihash[p],"%02x",bytes[i]);
		p += 2;
	}
	asciihash[32] = '\0';
	return std::string(asciihash);
}

//---------publics--------------------------

//constructor
md5wrapper::md5wrapper()
{
	md5 = new MD5();
}


//destructor
md5wrapper::~md5wrapper()
{
	delete md5;
}

/*
 * creates a MD5 hash from
 * "text" and returns it as
 * string
 */
std::string md5wrapper::getHashFromString(std::string text)
{
	return this->hashit(text);
}


/*
 * creates a MD5 hash from
 * a file specified in "filename" and
 * returns it as string
 * (based on Ronald L. Rivest's code
 * from RFC1321 "The MD5 Message-Digest Algorithm")
 */
std::string md5wrapper::getHashFromFile(std::string filename)
{
	FILE *file;
  	MD5_CTX context;

	int len;
  	unsigned char buffer[1024], digest[16];

	//open file
  	if ((file = fopen (filename.c_str(), "rb")) == NULL)
	{
		return "-1";
	}

	//init md5
 	md5->MD5Init (&context);

	//read the filecontent
	while ( (len = fread (buffer, 1, 1024, file)) )
   	{
		md5->MD5Update (&context, buffer, len);
	}

	/*
	generate hash, close the file and return the
	hash as std::string
	*/
	md5->MD5Final (digest, &context);
 	fclose (file);
	return convToString(digest);
 }

/*
 * EOF
 */
