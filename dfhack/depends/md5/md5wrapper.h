/*
 *	This is my wrapper-class to create
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

//include protection
#ifndef MD5WRAPPER_H
#define MD5WRAPPER_H

//basic includes
#include <string>

//forwards
class MD5;

class md5wrapper
{
	private:
		MD5 *md5;

		/*
		 * internal hash function, calling
		 * the basic methods from md5.h
		 */
		std::string hashit(std::string text);

		/*
		 * converts the numeric giets to
		 * a valid std::string
		 */
		std::string convToString(unsigned char *bytes);
	public:
		//constructor
		md5wrapper();

		//destructor
		~md5wrapper();

		/*
		 * creates a MD5 hash from
		 * "text" and returns it as
		 * string
		 */
		std::string getHashFromString(std::string text);

		/*
		 * creates a MD5 hash from
		 * a file specified in "filename" and
		 * returns it as string
		 */
		std::string getHashFromFile(std::string filename);
};


//include protection
#endif

/*
 * EOF
 */
