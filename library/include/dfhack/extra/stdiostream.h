// stdiostream.hpp
//
// Copyright (c) 2010 Michael Thomas Greer
// Distributed under the Boost Software License, Version 1.0
// (See http://www.boost.org/LICENSE_1_0.txt )
//

#pragma once
#ifndef DUTHOMHAS_STDIOSTREAM_HPP
#define DUTHOMHAS_STDIOSTREAM_HPP

#include <cstdio>
#include <iostream>
#include <streambuf>
#include <string>
#include <vector>
#include <ciso646>

namespace duthomhas
{

/* /////////////////////////////////////////////////////////////////////////
  basic_stdiobuf
///////////////////////////////////////////////////////////////////////// */

template <
typename CharType,
typename CharTraits = std::char_traits <CharType>
>
class basic_stdiobuf: public std::basic_streambuf <CharType, CharTraits>
{
    //------------------------------------------------------------------------
public:
    //------------------------------------------------------------------------
    typedef CharType                                char_type;
    typedef CharTraits                              traits_type;
    typedef typename traits_type::int_type          int_type;
    typedef typename traits_type::pos_type          pos_type;
    typedef typename traits_type::off_type          off_type;

    typedef basic_stdiobuf <char_type, traits_type> this_type;

    //......................................................................
    basic_stdiobuf( FILE* fp = NULL ):
            fp( fp )
    { }

    //......................................................................
//BUG 1: Hey! I never get called! (How is that?)
    ~basic_stdiobuf()
    {
        this->close();
    }

    //......................................................................
    bool is_open() const throw()
    {
        return fp != NULL;
    }

    //......................................................................
    this_type* open( const char* filename, std::ios_base::openmode mode )
    {
        if (is_open()) return NULL;

        // Figure out the open mode flags  . . . . . . . . . . . . . . . . . .
        std::string fmode;

        bool is_ate = mode & std::ios_base::ate;
        bool is_bin = mode & std::ios_base::binary;
        mode &= ~(std::ios_base::ate | std::ios_base::binary);

#define _(flag) std::ios_base::flag
        if      (mode == (         _(in)                    )) fmode = "r";
        else if (mode == (                 _(out) & _(trunc))) fmode = "w";
        else if (mode == (_(app)         & _(out)           )) fmode = "a";
        else if (mode == (         _(in) & _(out)           )) fmode = "r+";
        else if (mode == (         _(in) & _(out) & _(trunc))) fmode = "w+";
        else if (mode == (_(app) & _(in) & _(out)           )) fmode = "a+";
        // I would prefer to throw an exception here,
        // but the standard only wants a NULL result.
        else return NULL;
#undef _
        if (is_bin) fmode.insert( 1, 1, 'b' );

        // Try opening the file  . . . . . . . . . . . . . . . . . . . . . . .
        fp = std::fopen( filename, fmode.c_str() );
        if (!fp) return NULL;

        // Reposition to EOF if wanted . . . . . . . . . . . . . . . . . . . .
        if (is_ate) std::fseek( fp, 0, SEEK_END );

        return this;
    }

    //......................................................................
    this_type* close()
    {
        if (fp)
        {
            std::fclose( fp );
            fp = NULL;
        }
        pushbacks.clear();
        return this;
    }

    //......................................................................
    FILE* stdiofile() const
    {
        return fp;
    }

    //------------------------------------------------------------------------
protected:
    //------------------------------------------------------------------------

    //......................................................................
    // Get the CURRENT character without advancing the file pointer
    virtual int_type underflow()
    {
        // Return anything previously pushed-back
        if (pushbacks.size())
            return pushbacks.back();

        // Else do the right thing
        fpos_t pos;
        if (std::fgetpos( fp, &pos ) != 0)
            return traits_type::eof();

        int c = std::fgetc( fp );
        std::fsetpos( fp, &pos );

        return maybe_eof( c );
    }

    //......................................................................
    // Get the CURRENT character AND advance the file pointer
    virtual int_type uflow()
    {
        // Return anything previously pushed-back
        if (pushbacks.size())
        {
            int_type c = pushbacks.back();
            pushbacks.pop_back();
            return c;
        }

        // Else do the right thing
        return maybe_eof( std::fgetc( fp ) );
    }

    //......................................................................
    virtual int_type pbackfail( int_type c = traits_type::eof() )
    {
        if (!is_open())
            return traits_type::eof();

        // If the argument c is EOF and the file pointer is not at the
        // beginning of the character sequence, it is decremented by one.
        if (traits_type::eq_int_type( c, traits_type::eof() ))
        {
            pushbacks.clear();
            return std::fseek( fp, -1L, SEEK_CUR )
                   ? traits_type::eof()
                   : 0;
        }

        // Otherwise, make the argument the next value to be returned by
        // underflow() or uflow()
        pushbacks.push_back( c );
        return c;
    }

    //......................................................................
    virtual int_type overflow( int_type c = traits_type::eof() )
    {
        pushbacks.clear();

        // Do nothing
        if (traits_type::eq_int_type( c, traits_type::eof() ))
            return 0;

        // Else write a character
        return maybe_eof( std::fputc( c, fp ) );
    }

    //......................................................................
    virtual this_type* setbuf( char* s, std::streamsize n )
    {
        return std::setvbuf( fp, s, (s and n) ? _IOLBF : _IONBF, (size_t)n )
               ? NULL
               : this;
    }

    //......................................................................
    virtual pos_type seekoff(
        off_type                offset,
        std::ios_base::seekdir  direction,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out
    ) {
        pushbacks.clear();
        return std::fseek( fp, offset,
                           (direction == std::ios_base::beg) ? SEEK_SET :
                           (direction == std::ios_base::cur) ? SEEK_CUR :
                           SEEK_END
                         ) ? (-1) : std::ftell( fp );
    }

    //......................................................................
    virtual pos_type seekpos(
        pos_type                position,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out
    ) {
        pushbacks.clear();
        return std::fseek( fp, (long) position, SEEK_SET )
               ? (-1)
               : std::ftell( fp );
    }

    //......................................................................
    virtual int sync()
    {
        pushbacks.clear();
        return std::fflush( fp )
               ? traits_type::eof()
               : 0;
    }

    //------------------------------------------------------------------------
private:
    //------------------------------------------------------------------------
    FILE*                  fp;
    std::vector <int_type> pushbacks;  // we'll treat this like a stack

    //......................................................................
    // Utility function to make sure EOF gets translated to the proper value
    inline int_type maybe_eof( int value ) const
    {
        return (value == EOF) ? traits_type::eof() : value;
    }
};


/* /////////////////////////////////////////////////////////////////////////
  basic_stdiostream
///////////////////////////////////////////////////////////////////////// */

template <
typename CharType,
typename CharTraits = std::char_traits <CharType>
>
struct basic_stdiostream: public std::basic_iostream <CharType, CharTraits>
{
    typedef CharType                                      char_type;
    typedef CharTraits                                    traits_type;

    typedef basic_stdiobuf      <char_type, traits_type>  sbuf_type;
    typedef basic_stdiostream   <char_type, traits_type>  this_type;
    typedef std::basic_iostream <char_type, traits_type>  base_type;

    //......................................................................
    basic_stdiostream( FILE* fp = NULL ):
            base_type( new sbuf_type( fp ) )
    { }

    //......................................................................
    basic_stdiostream( const char* filename, std::ios_base::openmode mode ):
//BUG 2: Oops! This is a potential memory leak!
            base_type( (new sbuf_type)->open( filename, mode ) )
    { }

    //......................................................................
    void open(
        const char*             filename,
        std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out
    ) {
        sbuf_type* buf = static_cast <sbuf_type*> ( this->rdbuf() );
        if (!(buf->open( filename, mode )))
            this->setstate( std::ios_base::badbit );
    }

    //......................................................................
    void close()
    {
        sbuf_type* buf = static_cast <sbuf_type*> ( this->rdbuf() );
        buf->close();
    }
};


/* /////////////////////////////////////////////////////////////////////////
  Useful typedefs
///////////////////////////////////////////////////////////////////////// */

typedef basic_stdiobuf    <char> stdiobuf;
typedef basic_stdiostream <char> stdiostream;

} // namespace duthomhas

#endif

// end stdiostream.hpp
