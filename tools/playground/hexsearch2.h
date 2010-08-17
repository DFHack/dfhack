/*
 * Author: Silas Dunsmore aka 0x517A5D                          vim:ts=4:sw=4
 *
 * Released under the MIT X11 license; feel free to use some or all of this
 * code, as long as you include the copyright and license statement (below)
 * in all copies of the source code.  In fact, I truly encourage reuse.
 *
 * If you do use large portions of this code, I suggest but do not require
 * that you keep this code in a seperate file (such as this hexsearch.h file)
 * so that it is clear that the terms of the license do not also apply to
 * your code.
 *
 * Should you make fundamental changes, or bugfixes, to this code, I would
 * appreciate it if you would give me a copy of your changes.
 *
 *
 * Copyright (C) 2007-2008 Silas Dunsmore
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * See also:    http://www.opensource.org/licenses/mit-license.php
 * and          http://en.wikipedia.org/wiki/MIT_License
 */


#ifndef HEXSEARCH_H
#define HEXSEARCH_H
#ifdef __cplusplus
extern "C"
{
#endif


extern DWORD df_memory_base, df_memory_start, df_memory_end;
extern HANDLE df_h_process, df_h_thread;
extern DWORD df_pid, df_main_win_tid;

extern DWORD here[16];
extern DWORD target[16];

// ============================================================================
// export these in case you need to work with them directly.
//extern HANDLE df_h_process, df_h_thread;
//extern DWORD df_pid, df_main_win_tid;
extern char *errormessage;


// ============================================================================
// send info to KERNEL32:OutputDebugString(), useful for non-console programs.
// (you can watch this with SysInternals' DebugView.)
// http://www.microsoft.com/technet/sysinternals/Miscellaneous/DebugView.mspx
void d_printf(const char *format, ...);
// it turned out that dprintf() is semi-standard to printf to a file descriptor.


// ============================================================================
// encapsulate the housekeeping.
BOOL open_dwarf_fortress(void);
void close_dwarf_fortress(void);        // is not (normally) necessary because
                                        // it is done at exit time.


// ============================================================================
// memory reads and writes (peeks and pokes, in old BASIC terminology)
BOOL isvalidaddress(DWORD ea);
BOOL peekarb(DWORD ea, OUT void *data, DWORD len);
BYTE peekb(DWORD ea);
WORD peekw(DWORD ea);
DWORD peekd(DWORD ea);
char *peekstr(DWORD ea, OUT char *data, DWORD maxlen);
char *peekustr(DWORD ea, OUT char *data, DWORD maxlen);
BOOL pokearb(DWORD ea, const void *data, DWORD len);
BOOL pokeb(DWORD ea, BYTE data);
BOOL pokew(DWORD ea, WORD data);
BOOL poked(DWORD ea, DWORD data);
BOOL pokestr(DWORD ea, const BYTE *data);


// ============================================================================
// The name has changed because the API HAS CHANGED!
//
// Now instead of returning HERE, it always returns the start of the match.
//
// However, there is an array, here[], that is filled with the locations of
//      the HERE tokens.
//      (Starting at 1.  here[0] is a copy of the start of the match.)
//
// The here[] array, starting at 1, is filled with the locations of the
//      HERE token.
// here[0] is a copy of the start of the match.
//
// Also, the target[] array, starting at 1, is filled with the target
//      addresses of all CALL, JMP, JZ, JNZ, and JCC tokens.
//
// Finally, you no longer pass it a search length.  Instead, each set of
//      search terms must end with an EOL.
//
//
//
// Okay, I admit this one is complicated.  Treat it as a black box.
//
// Search Dwarf Fortress's code and initialized data segments for a pattern.
//      (Does not search stack, heap, or thread-local memory.)
//
// Parameters: any number of search tokens, all unsigned ints.
//      The last token must be EOL.
//
//          0x00 - 0xFF: Match this byte.
//          EOL: End-of-list.  The match succeeds when this token is reached.
//          ANYBYTE: Match any byte.
//          DWORD_: Followed by a dword.  Exact-match the dword.
//              Equivalent to 4 match-this-byte tokens.
//          ANYDWORD: Match any dword.  Equivalant to 4 ANYBYTEs.
//          HERE: Put the current address into the here[] array.
//          SKIP_UP_TO, nn: Allow up to nn bytes between the previous match
//              and the next match.  The next token must be a match-this-byte
//              token.  There is sweet, sweet backtracking.
//          EITHER: Accept either of the next two tokens as a match.
//              Both must be match-this-byte tokens.
//          RANGE_LO, nn: Set low byte for a range comparison.  DEPRECATED.
//          RANGE_HI, nn: Set high byte for a range comparison, and do the
//          comparison.  Should immediately follow a RANGE_LO.  DEPRECATED.
//          BYTERANGE, nn, mm: followed by two bytes, the low and high limits
//          of the range.  This is the new way to do ranges.
//          DWORDRANGE, nnnnnnnn, mmmmmmmm: works like BYTERANGE.
//          ADDRESS: Accept any legal address in the program's text.
//              DOES NOT accept pointers into heap space and such.
//          CALL: Match a near call instruction to a reasonable address.
//          JUMP: Match a short or near unconditional jump to a reasonable
//              address.
//          JZ: Match a short or long jz (jump if zero) instruction.
//          JNZ: Match a short or long jnz (jump if not zero) instruction.
//          JCC: Match any short or long conditional jump instruction.
//          More tokens can easily be added.
//
// Returns the offset in Dwarf Fortress of the first match of the pattern.
//      Also sets global variables here[] and target[].
//
// Note: starting a pattern with ANYBYTE, ANYDWORD, SKIP_UP_TO, or EOL
//          is explicitly illegal.
//
#define EOL         0x100
#define ANYBYTE     0x101
//#define FF_OR_00  0x102               // deprecated
#define HERE        0x103
#define EITHER      0x104
#define SKIP_UP_TO  0x105
#define RANGE_LO    0x106               // deprecated
#define RANGE_HI    0x107               // deprecated
#define DWORD_      0x108
#define ANYDWORD    0x109
#define ADDRESS     0x10A
#define BYTERANGE   0x10B
#define DWORDRANGE  0x10C
#define JZ          0x174
#define JNZ         0x175
#define JCC         0x170
#define CALL        0x1E8
#define JUMP        0x1E9


DWORD hexsearch2(DWORD token1, ...);

// You can use this to limit the search to a particular part of memory.
// Use 0 for start to search from the very start of Dwarf Fortress.
// Use 0 for end to stop searching at the very end of Dwarf Fortress.
void set_hexsearch2_limits(DWORD start, DWORD end);


// ============================================================================
// patch2() and verify2() support a modified subset of hex_search2() tokens:
//  The names changed because the API HAS CHANGED!
//
//          0x00 - 0xFF: pokes the byte.
//          EOL: End-of-list.  Allows you to specify count as 10000 or so 
//              and terminate (with success) when this token is reached.
//          DWORD_: Followed by a DWORD.  Pokes the DWORD.
//          CALL: given an _address_; pokes near call with the proper _delta_.
//          JUMP: given an _address_; pokes near jump with the proper _delta_.
//          JZ: given an _address_; assembles a near (not short) jz & delta.
//          JNZ: given an _address_; assembles a near jnz & delta.
//
//  Particularly note that, unlike hex_search(), CALL, JUMP, JZ, and JNZ
//      are followed by a dword-sized target address.
//
//  Note that patch2() does its own verify(), so you don't have to.

// TODO: is verify() useful enough to maintain?

// Make an offset in Dwarf Fortress have certain bytes.
BOOL patch2(DWORD offset, ...);
// Check that an offset in Dwarf Fortress has certain bytes.
BOOL verify2(DWORD offset, ...);



#ifdef __cplusplus
}
#endif
#endif // HEXSEARCH_H