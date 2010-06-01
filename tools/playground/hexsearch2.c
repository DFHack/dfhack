/*
 * Author: Silas Dunsmore aka 0x517A5D                          vim:ts=4:sw=4
 *
 * Released under the MIT X11 license; feel free to use some or all of this
 * code, as long as you include the copyright and license statement (below)
 * in all copies of the source code.  In fact, I truly encourage reuse.
 *
 * If you do use large portions of this code, I suggest but do not require
 * that you keep this code in a seperate file (such as this hexsearch.c file)
 * so that it is clear that the terms of the license do not also apply to
 * your code.
 *
 * Should you make fundamental changes, or bugfixes, to this code, I would
 * appreciate it if you would give me a copy of your changes.
 *
 *
 * Be advised that I use several advanced idioms of the C language:
 * macro expansion, stringification, and variable argument functions.
 * You do not need to understand them.  Usage should be obvious.
 *
 *
 * Lots of logging output is sent to OutputDebugString().
 * The Sysinternals' DebugView program is very useful in monitering this.
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

#include <stdarg.h>                     // va_list and friends
#include <stdio.h>                      // vsnprintf()
#include <stdlib.h>                     // atexit()
#include <signal.h>
#define WINVER 0x0500                   // OpenThread()
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include "hexsearch2.h"


#define     SKIPME  0xFEDCBA98          // token for internal use only.


// exported globals
HANDLE df_h_process, df_h_thread;
DWORD df_pid, df_main_win_tid;
DWORD here[16];
DWORD target[16];
char *errormessage;
DWORD df_memory_base, df_memory_start, df_memory_end;


// local globals
static BOOL change_page_permissions;
static BOOL suspended;
static BYTE *copy_of_df_memory;
static int nexthere;
static int nexttarget;
static DWORD searchmemstart, searchmemend;


#define dump(x) d_printf("%-32s == %08X\n", #x, (x));
#define dumps(x) d_printf("%-32s == '%s'\n", #x, (x));


// ============================================================================
// send info to KERNEL32:OutputDebugString(), useful for non-console programs.
// you can watch this with SysInternals' DebugView.
// http://www.microsoft.com/technet/sysinternals/Miscellaneous/DebugView.mspx
//
void d_printf(const char *format, ...)
{
    va_list va;
    char debugstring[4096];             // debug strings can be up to 4K.

    va_start(va, format);
    _vsnprintf(debugstring, sizeof(debugstring) - 2, format, va);
    va_end(va);

    OutputDebugString(debugstring);
}


// ============================================================================
BOOL isvalidaddress(DWORD address)
{
    MEMORY_BASIC_INFORMATION mbi;
    return (VirtualQueryEx(df_h_process, (void *)address, &mbi, sizeof(mbi))
            == sizeof(mbi) && mbi.State == MEM_COMMIT);
}


// ----------------------------------------------------------------------------
BOOL peekarb(DWORD address, OUT void *data, DWORD len)
{
    BOOL ok = FALSE;
    DWORD succ = 0;
    DWORD OldProtect;
    MEMORY_BASIC_INFORMATION mbi = {0, 0, 0, 0, 0, 0, 0};

    errormessage = "";

    // do {} while(0) is a construct that lets you abort a piece of code
    // in the middle without using gotos.

    do
    {

        if ((succ = VirtualQueryEx(df_h_process, (BYTE *)address + len - 1,
                                   &mbi, sizeof(mbi))) != sizeof(mbi))
        {
            dump(address + len - 1);
            dump(succ);
            errormessage = "peekarb(): VirtualQueryEx() on end address failed";
            break;
        }
        if (mbi.State != MEM_COMMIT)
        {
            dump(mbi.State);
            errormessage = "peekarb(): VirtualQueryEx() says end address "
                           "is not MEM_COMMIT";
            break;
        }

        if ((succ = VirtualQueryEx(df_h_process, (void *)address, &mbi,
                                   sizeof(mbi))) != sizeof(mbi))
        {
            dump(address);
            dump(succ);
            errormessage ="peekarb(): VirtualQueryEx() on start address failed";
            break;
        }
        if (mbi.State != MEM_COMMIT)
        {
            dump(mbi.State);
            errormessage = "peekarb(): VirtualQueryEx() says start address is "
                           "not MEM_COMMIT";
            break;
        }

        if (change_page_permissions)
        {
            if (!VirtualProtectEx(df_h_process, mbi.AllocationBase,
                                  mbi.RegionSize, PAGE_READWRITE, &OldProtect))
            {
                errormessage = "peekarb(): VirtualProtectEx() failed";
                break;
            }
        }

        if (!ReadProcessMemory(df_h_process, (void *)address, data, len, &succ))
        {
            errormessage = "peekarb(): ReadProcessMemory() failed";

            // note that we do NOT break here, as we want to restore the
            // page protection.
        }
        else if (len != succ)
        {
            errormessage = "peekarb(): ReadProcessMemory() returned "
                           "partial read";
        }
        else ok = TRUE;

        if (change_page_permissions)
        {
            if (!VirtualProtectEx(df_h_process, mbi.AllocationBase,
                                  mbi.RegionSize, OldProtect, &OldProtect))
            {
                errormessage = "peekarb(): undo VirtualProtectEx() failed";
                break;
            }
        }
    } while (0);

    if (errormessage != NULL && strlen(errormessage) != 0)
    {
        d_printf("%s\n", errormessage);
        dump(address);
        //dump(len);
        //dump(succ);
        //dump(mbi.AllocationBase);
    }

    return(ok);
}


// ----------------------------------------------------------------------------
BYTE peekb(DWORD address)
{
    BYTE data;
    return(peekarb(address, &data, sizeof(data)) ? data : 0);
    // pop quiz: why don't we set errormessage?
}


// ----------------------------------------------------------------------------
WORD peekw(DWORD address)
{
    WORD data;
    return(peekarb(address, &data, sizeof(data)) ? data : 0);
}


// ----------------------------------------------------------------------------
DWORD peekd(DWORD address)
{
    DWORD data;
    return(peekarb(address, &data, sizeof(data)) ? data : 0);
}


// ----------------------------------------------------------------------------
char *peekstr(DWORD address, OUT char *data, DWORD maxlen)
{
    BYTE c;
    int i = 0;

    data[0] = '\0';
    if (!isvalidaddress(address)) return(data);

    while (--maxlen && (c = peekb(address++)) >= ' ' && c <= '~')
    {
        if (!isvalidaddress(address)) return(data);
        data[i++] = c;
        data[i] = '\0';
    }
    // for convenience
    return(data);
}


// ----------------------------------------------------------------------------
char *peekwstr(DWORD address, OUT char *data, DWORD maxlen)
{
    BYTE c;
    int i = 0;

    data[0] = '\0';
    if (!isvalidaddress(address)) return(data);

    while (--maxlen && (c = peekb(address++)) >= ' ' && c <= '~' && peekb(address++) == 0)
    {
        if (!isvalidaddress(address))
        {
            return(data);
        }
        data[i++] = c;
        data[i] = '\0';
    }
    // for convenience
    return(data);
}


// ----------------------------------------------------------------------------
BOOL pokearb(DWORD address, const void *data, DWORD len)
{
    BOOL ok = FALSE;
    DWORD succ = 0;
    DWORD OldProtect;
    MEMORY_BASIC_INFORMATION mbi;

    errormessage = "";

    do
    {
        if (!isvalidaddress(address))
        {
            errormessage = "pokearb() failed: invalid address";
            break;
        }

        if (!isvalidaddress(address + len - 1))
        {
            errormessage = "pokearb() failed: invalid end address";
            break;
        }

        if (change_page_permissions)
        {
            if (VirtualQueryEx(df_h_process, (void *)address, &mbi, sizeof(mbi)) != sizeof(mbi))
            {
                errormessage = "pokearb(): VirtualQueryEx() failed";
                break;
            }

            if (!VirtualProtectEx(df_h_process, mbi.AllocationBase, mbi.RegionSize, PAGE_READWRITE, &OldProtect))
            {
                errormessage = "pokearb(): VirtualProtectEx() failed";
                break;
            }
        }

        if (!WriteProcessMemory(df_h_process, (void *)address, data, len, &succ))
        {
            errormessage = "pokearb(): WriteProcessMemory() failed";
            // note that we do NOT break here, as we want to restore the
            // page protection.
        }
        else if (len != succ)
        {
            errormessage = "pokearb(): WriteProcessMemory() did partial write";
        }
        else
        {
            ok = TRUE;
        }

        if (change_page_permissions)
        {
            if (!VirtualProtectEx(df_h_process, mbi.AllocationBase, mbi.RegionSize, OldProtect, &OldProtect))
            {
                errormessage = "pokearb(): undo VirtualProtectEx() failed";
                break;
            }
        }
    } while (0);

    if (errormessage != NULL && strlen(errormessage) != 0)
    {
        d_printf("%s\n", errormessage);
        dump(address);
        //dump(len);
        //dump(succ);
        //dump(mbi.AllocationBase);
    }

    return(ok);
}


// ----------------------------------------------------------------------------
BOOL pokeb(DWORD address, BYTE data)
{
    return(pokearb(address, &data, sizeof(data)));
}


// ----------------------------------------------------------------------------
BOOL pokew(DWORD address, WORD data)
{
    return(pokearb(address, &data, sizeof(data)));
}


// ----------------------------------------------------------------------------
BOOL poked(DWORD address, DWORD data)
{
    return(pokearb(address, &data, sizeof(data)));
}


// ----------------------------------------------------------------------------
BOOL pokestr(DWORD address, const BYTE *data)
{
// can't include a "\x00" in the string, obviously.
    return(pokearb(address, data, strlen((const char *)data)));
}


// ----------------------------------------------------------------------------
// helper function for hexsearch.  recursive, with backtracking.
// this checks if a particular memory offset matches the given pattern.
// returns location of start of match (in the cached copy of DF memory).
// returns NULL on mismatch.
//
// TODO: there is a harmless bug in the recursion related to the very first
//  recursive call, probably on each level of recursion.
static BYTE *hexsearch_match2(BYTE *p, DWORD token1, va_list va)
{
    static DWORD recursion_level = 0;
    DWORD tokensprocessed = 0;
    DWORD token = 0x4DECADE5, b1, b2, lo, hi;
    BYTE *retval = p;
    BOOL ok = FALSE, lookedahead;
    int savenexthere = nexthere;
    int savenexttarget = nexttarget;

    // TODO token is being used without being inited on recursion.

    //if (recursion_level) dump(recursion_level);
    //if (recursion_level) dump(p);
    //if (recursion_level) dump(tokensprocessed);

    if (token1 != SKIPME)
    {
        lookedahead = TRUE;
        token = token1;
        tokensprocessed = 1;
    }

    while (1)
    {
        // if the previous argument looked ahead, token is already set.
        // peekahead is currently unused.
        if (!lookedahead)
        {
            token = va_arg(va, unsigned int);
            tokensprocessed++;
        }
        lookedahead = FALSE;

        // exact-match a byte, advance.
        if (token <= 0xFF)
        {
            if (token != *p++) break;
        }

        // the remaining tokens (the metas) ought to be a switch.
        // but that would make it hard to break out of the while(1).

        // if we hit an EOL, the match succeeded.
        else if (token == EOL)
        {
            ok = TRUE;
            break;
        }

        // match any byte, advance.
        else if (token == ANYBYTE)
        {
            p++;
        }

        // return the address of the next matching byte instead of the
        // address of the start of the pattern.  don't advance.
        else if (token == HERE)
        {
            retval = p;
            here[nexthere++] = (DWORD)p;        // needs postprocessing.
        }

        // accept either of the next two parameters as a match. advance.
        // note that this does not count as peeking.
        else if (token == EITHER)
        {
            if ((b1 = va_arg(va, unsigned int)) > 0xFF)
            {
                d_printf("EITHER: not followed by a legal token (byte1): %08X\n", b1);
                break;
            }
            tokensprocessed++;
            if ((b2 = va_arg(va, unsigned int)) > 0xFF)
            {
                d_printf("EITHER: not followed by a legal token (byte2): %08X\n", b2);
                break;
            }
            tokensprocessed++;
            if (!(*p == b1 || *p == b2)) break;
            p++;
        }

        #ifdef FF_OR_00 //DEPRECATED
        // accept either 0x00 or 0xFF.  advance.
        else if (token == FF_OR_00)
        {
            if (!(*p == 0x00 || *p == 0xFF))
            {
                break;
            }
            p++;
        }
        #endif

        // set low value for range comparison.  don't advance.  DEPRECATED.
        else if (token == RANGE_LO)
        {
            if ((lo = va_arg(va, unsigned int)) > 0xFF)
            {
                d_printf("RANGE_LO: not followed by a legal token: %08X\n", lo);
                break;
            }
            tokensprocessed++;
            // Q: peek here to ensure next token is RANGE_HI ?
        }

        // set high value for range comparison, and do comparison.  advance.
        // DEPRECATED.
        else if (token == RANGE_HI)
        {
            if ((hi = va_arg(va, unsigned int)) > 0xFF) {
                d_printf("RANGE_HI: not followed by a legal token: %08X\n", hi);
                break;
            }
            if (*p < lo || *p > hi)
            {
                break;
            }
            p++;
            tokensprocessed++;
        }

        // do a byte-size range comparison
        else if (token == BYTERANGE)
        {
            if ((lo = va_arg(va, unsigned int)) > 0xFF)
            {
                d_printf("BYTERANGE: not followed by a legal token: %08X\n", lo);
                break;
            }
            tokensprocessed++;

            if ((hi = va_arg(va, unsigned int)) > 0xFF)
            {
                d_printf("BYTERANGE: not followed by a legal token: %08X\n", hi);
                break;
            }
            if (*p < lo || *p > hi)
            {
                break;
            }
            p++;
            tokensprocessed++;
        }

        // do a dword-size range comparison
        else if (token == DWORDRANGE)
        {
            lo = va_arg(va, unsigned int);
            tokensprocessed++;

            hi = va_arg(va, unsigned int);
            if (*(DWORD *)p < lo || *(DWORD *)p > hi)
            {
                break;
            }
            p++;
            tokensprocessed++;
        }

        // this is the fun one.  this is where we recurse.
        else if (token == SKIP_UP_TO)
        {
            DWORD len;
            BYTE * subretval;

            len = va_arg(va, unsigned int) + 1;
            tokensprocessed++;

            while (len)
            {
                // um.  This is a kludge.  It ought to work to not set any
                // heres or targets if we're in a recursion.  (not tested)
                int savenexthere;
                int savenexttarget;

                // I think it's not technically legal to copy va_lists;
                // but it should work on any stack-based machine, which is
                // everything except old Crays and weird dataflow processors.

                savenexthere = nexthere;
                savenexttarget = nexttarget;
                //dump(tokensprocessed);
                //dump(recursion_level);
                //dumps("-->");
                recursion_level++;
                subretval = hexsearch_match2(p, SKIPME, va);
                recursion_level--;
                //dumps("<--");
                //dump(recursion_level);
                //dump(tokensprocessed);
                nexthere = savenexthere;
                nexttarget = savenexttarget;
                if (subretval != NULL) {
                    // okay, we now know that the bytes starting at p
                    // match the remainder of the pattern.
                    // Nonetheless, for ease of programming, we will
                    // go through the motions instead of trying to
                    // early-out.  (Basically done for HERE tokens.)
                    break;
                }

                p++;
                len--;
            }
            if (subretval != NULL)
            {
                continue;
            }
            // no match within nn bytes, abort.
            break;
        }

        // exact-match a dword.  advance 4.
        else if (token == DWORD_)
        {
            DWORD d = va_arg(va, unsigned int);
            if (*(DWORD *)p != d) break;
            p += 4;
            tokensprocessed++;
        }

        // match any dword.  advance 4.
        else if (token == ANYDWORD)
        {
            p += 4;
        }

        // match any legal address in
        else if (token == ADDRESS)
        {
            // program text.  advance 4.
            if (*(DWORD *)p < df_memory_start)
            {
                break;
            }
            if (*(DWORD *)p > df_memory_end)
            {
                break;
            }
            p += 4;
        }

        // match any call.  advance 5.
        else if (token == CALL)
        {
            if (*p++ != 0xE8)
            {
                break;
            }
            target[nexttarget++] = *(DWORD *)p + (DWORD)p + 4;
#if 0
            if (*(DWORD *)p > DF_CODE_SIZE
                    && *(DWORD *)p < (DWORD)-DF_CODE_SIZE)
                break;
#endif
            p += 4;
        }

        // match any short or near jump.
        else if (token == JUMP)
        {
            // advance 2 or 5 respectively.
            if (*p == 0xEB)
            {
                target[nexttarget++] = *(signed char *)(p+1) + (DWORD)p + 1;
                p += 2;
                continue;
            }

            if (*p++ != 0xE9)
            {
                break;
            }

            target[nexttarget++] = *(DWORD *)p + (DWORD)p + 4;
#if 0
            if (*(DWORD *)p > DF_CODE_SIZE
                    && *(DWORD *)p < (DWORD)-DF_CODE_SIZE)
                break;
#endif
            p += 4;
        }

        // match a JZ instruction
        else if (token == JZ)
        {
            if (*p == 0x74)
            {
                target[nexttarget++] = *(signed char *)(p+1) + (DWORD)p + 2;
                p += 2;
                continue;
            }
            else if (*p == 0x0F && *(p+1) == 0x84
                     && (*(p+4) == 0x00 || *(p+4) == 0xFF) // assume dist < 64k
                     && (*(p+5) == 0x00 || *(p+5) == 0xFF))
            {
                target[nexttarget++] = *(DWORD *)(p+2) + (DWORD)p + 6;
                p += 6;
                continue;
            }
            else break;
        }

        // match a JNZ instruction
        else if (token == JNZ)
        {
            if (*p == 0x75) {
                target[nexttarget++] = *(signed char *)(p+1) + (DWORD)p + 2;
                p += 2;
                continue;
            }
            else if (*p == 0x0F && *(p+1) == 0x85
                     && (*(p+4) == 0x00 || *(p+4) == 0xFF) // assume dist < 64k
                     && (*(p+5) == 0x00 || *(p+5) == 0xFF)
                    ) {
                target[nexttarget++] = *(DWORD *)(p+2) + (DWORD)p + 6;
                p += 6;
                continue;
            }
            else break;
        }

        // match any conditional jump
        else if (token == JCC)
        {
            if (*p >= 0x70 && *p <= 0x7F)
            {
                target[nexttarget++] = *(signed char *)(p+1) + (DWORD)p + 2;
                p += 2;
                continue;
            }
            else if (*p == 0x0F && *(p+1) >= 0x80 && *(p+1) <= 0x8F
                     && (*(p+4) == 0x00 || *(p+4) == 0xFF) // assume dist < 64k
                     && (*(p+5) == 0x00 || *(p+5) == 0xFF))
            {
                target[nexttarget++] = *(DWORD *)(p+2) + (DWORD)p + 6;
                p += 6;
                continue;
            }
            else break;
        }

        // unknown token, abort
        else
        {
            d_printf("unknown token: %08X\n", token);
            dump(p);
            dump(recursion_level);
            break;
        }   // end of huge if-else

    }   // end of while(1)

    if (!ok)
    {
        retval = NULL;
        nexthere = savenexthere;
        nexttarget = savenexttarget;
    }
    return (retval);
}


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
//
// implementation detail: search() uses a cached copy of the memory, so
// poke()s and patch()s will not be reflected in the searches.
//
// implementation detail: Q: why don't we discard count and always use EOL?
// A: because we need a fixed parameter to start the va_list.
//          (Hmm, could we grab address of a local variable, walk the stack
//          until we see &hexsearch, and use that address to init va_list?)
//          (No, dummy, because &hexsearch is not on the stack.  The stack
//          holds the return address.)
//          (Could we grab the EBP register and treat it as a stack frame?)
//          (Maybe, but the compiler decides if EBP really is a stack frame,
//          so that would be an implementation dependency.  Don't do it.)
//
// TODO: should be a way to repeat a search in case we want the second or
//          eleventh occurance.
//
// TODO: allow EITHER to accept arbitrary (sets of) tokens.
//          (How would that work?  Have two sub-patterns, each terminating
//          with EOL?)
//
DWORD hexsearch2(DWORD token1, ...)
{
    DWORD size;
    BYTE *nextoffset, *foundit;
    DWORD token;
    int i;
    va_list va;

    for (i = 0; i < 16; i++) here[i] = 0;
    for (i = 0; i < 16; i++) target[i] = 0;

    nexthere = 1;
    nexttarget = 1;

    if (   token1 == ANYBYTE
         || token1 == ANYDWORD
         || token1 == SKIP_UP_TO
         || token1 == HERE
         || token1 == EOL        )
    {
        return 0;
    }

    dump(searchmemstart);
    dump(searchmemend);
    dump(df_memory_start);
    dump(copy_of_df_memory);
    nextoffset = copy_of_df_memory;
    nextoffset += (searchmemstart - df_memory_start);
    dump(nextoffset);
    size = searchmemend - searchmemstart;
    dump(size);

    while (1)
    {
        // for speed, if we start with a raw byte, use a builtin function
        // to skip ahead to the next actual occurance.
        if (token1 <= 0xFF)
        {
            nextoffset = (BYTE *)memchr(nextoffset, token1,
                                        size - (nextoffset - copy_of_df_memory));
            if (nextoffset == NULL) break;
        }

        va_start(va, token1);
        foundit = hexsearch_match2(nextoffset, token1, va);
        va_end(va);

        if (foundit)
        {
            break;
        }

        if ((DWORD)(++nextoffset - copy_of_df_memory - (searchmemstart - df_memory_start)) >= size)
        {
            break;
        }

    }

    if (!foundit)
    {
        d_printf("hexsearch2(%X", token1);
        i = 0;
        va_start(va, token1);
        do
        {
            d_printf(",%X", token = va_arg(va, DWORD));
        } while (EOL != token && i++ < 64);
        va_end(va);
        if (i >= 64) d_printf("...");
        d_printf(")\n");
        d_printf("search failed!\n");
        for (i = 0; i < 16; i++)
        {
            here[i] = 0;
            target[i] = 0;
        }
        return 0;
    }

    here[0] = (DWORD)foundit;
    for (i = 0; i < 16; i++)
    {
        if (i < nexthere && here[i] != 0)
        {
            here[i] += df_memory_start - (DWORD)copy_of_df_memory;
        }
        else here[i] = 0;
    }
    for (i = 0; i < 16; i++)
    {
        if (i < nexttarget && target[i] != 0)
        {
            target[i] += df_memory_start - (DWORD)copy_of_df_memory;
        }
        else target[i] = 0;
    }

    return df_memory_start + (foundit - copy_of_df_memory);
}


// ----------------------------------------------------------------------------
void set_hexsearch2_limits(DWORD start, DWORD end)
{
    if (end < start) end = start;

    searchmemstart = start >= df_memory_start ? start : df_memory_start;
    searchmemend = end >= df_memory_start && end <= df_memory_end ?
                   end : df_memory_end;
}


// ============================================================================
// helper function for patch and verify.  does the actual work.
static BOOL vpatchengine(BOOL mode, DWORD address, va_list va)
{
    // mode: TRUE == patch, FALSE == verify
    DWORD next = address, a, c;
    // a is token, c is helper value, b is asm instruction byte to use.
    BYTE b;
    BOOL ok = FALSE;

    while (1)
    {
        //if (length == 0) { ok = TRUE; break; }
        //length--;
        a = va_arg(va, unsigned int);

        if (a == EOL)
        {
            ok = TRUE;
            break;
        }

        if (a == DWORD_)
        {
            //if (length-- == 0) break;
            c = va_arg(va, unsigned int);
            if (mode ? !poked(next, c) : c != peekd(next)) break;
            next += 4;
            continue;
        }

        if (a == CALL || a == JUMP)
        {
            b = (a == CALL ? 0xE8 : 0xE9);
            //if (length-- == 0) break;
            c = va_arg(va, unsigned int) - (next + 5);
            if (mode ? !pokeb(next, b) : b != peekb(next)) break;
            // do NOT merge the next++ into the previous if statement.
            next++;
            if (mode ? !poked(next, c) : c != peekd(next)) break;
            next += 4;
            continue;
        }

        if (a == JZ || a == JNZ)
        {
            b = (a == JZ ? 0x84 : 0x85);
            //if (length-- == 0) break;
            c = va_arg(va, unsigned int) - (next + 6);
            if (mode ? !pokeb(next, 0x0F) : 0x0F != peekb(next)) break;
            next++;
            if (mode ? !pokeb(next, b) : b != peekb(next)) break;
            next++;
            if (mode ? !poked(next, c) : c != peekd(next)) break;
            next += 4;
            continue;
        }

        if (a <= 0xFF)
        {
            if (mode ? !pokeb(next, a) : a != peekb(next)) break;
            next++;
            continue;
        }

        d_printf("vpatchengine: unsupported token: %08X\n", a);
        break;                          // unsupported token
    }

//  d_printf("vpatchengine returning %d, length is %d, next is %08X\n",
//          ok, length, next);
    return(ok);
}


// ----------------------------------------------------------------------------
// patch() and verify() support a modified subset of hex_search() tokens:
//
//          0x00 - 0xFF: poke the byte.
//          EOL: end-of-list.  terminate when this token is reached.
//          DWORD_: Followed by a DWORD, not a byte.  Poke the DWORD.
//          CALL: given an _address_; pokes near call with the proper _delta_.
//          JUMP: given an _address_; pokes near jump with the proper _delta_.
//          JZ: given an _address_; assembles a near (not short) jz & delta.
//          JNZ: given an _address_; assembles a near jnz & delta.
//
//  Particularly note that, unlike hex_search(), CALL, JUMP, JZ, and JNZ
//      are followed by a dword-sized target address.
//
//  Note that patch() does its own verify(), so you don't have to.
//
//  TODO: doing so many individual pokes and peeks is slow.  Consider
//          building a copy, then doing a pokearb/peekarb.

// Make an offset in Dwarf Fortress have certain bytes.
BOOL patch2(DWORD address, ...)
{
    va_list va;
    BOOL ok;

    va_start(va, address);
    ok = vpatchengine(TRUE, address, va);
    va_end(va);

    va_start(va, address);
    if (ok) ok = vpatchengine(FALSE, address, va);
    va_end(va);
    return(ok);
}


// ----------------------------------------------------------------------------
// Check that an offset in Dwarf Fortress has certain bytes.
//
// See patch() documentation.
BOOL verify2(DWORD address, ...)
{
    BOOL ok;
    va_list va;
    va_start(va, address);
    ok = vpatchengine(FALSE, address, va);
    va_end(va);
    return(ok);
}

// ----------------------------------------------------------------------------
// Get the exe name of the DF executable
static DWORD get_exe_base(DWORD pid)
{
    HANDLE snap = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 process = {0};
    MODULEENTRY32 module = {0};

    do
    {
        snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE)
        {
            errormessage = "CreateToolhelp32Snapshot(Process) failed.";
            break;
        }

        // Get the process exe name
        process.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(snap, &process))
        {
            errormessage = "Process32First(snapshot) failed.";
            break;
        }
        do
        {
            if (process.th32ProcessID == pid)
            {
                break;
            }
            else
            {
                //d_printf("Process: %d \"%.*s\"", process.th32ProcessID, MAX_PATH, process.szExeFile);
            }
        } while (Process32Next(snap, &process));
        if (process.th32ProcessID != pid)
        {
            errormessage = "Process32List(snapshot) Couldn't find the target process in the snapshot?";
            break;
        }

        d_printf("Target Process: %d \"%.*s\"", process.th32ProcessID, MAX_PATH, process.szExeFile);
        CloseHandle(snap);

        snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
        if (snap == INVALID_HANDLE_VALUE)
        {
            errormessage = "CreateToolhelp32Snapshot(Modules) failed.";
            break;
        }

        // Now find the module
        module.dwSize = sizeof(MODULEENTRY32);
        if (!Module32First(snap, &module))
        {
            errormessage = "Module32First(snapshot) failed.";
            break;
        }
        do
        {
            if (!stricmp(module.szModule, process.szExeFile))
            {
                break;
            }
            else
            {
                //d_printf("Module: %08X \"%.*s\" \"%.*s\"", (DWORD)(SIZE_T)module.modBaseAddr, MAX_MODULE_NAME32 + 1, module.szModule, MAX_PATH, module.szExePath);
            }
        } while (Module32Next(snap, &module));
        if (stricmp(module.szModule, process.szExeFile))
        {
            errormessage = "Module32List(snapshot) Couldn't find the target module in the snapshot?";
            break;
        }
        d_printf("Target Module: %08X \"%.*s\" \"%.*s\"", (DWORD)(SIZE_T)module.modBaseAddr, MAX_MODULE_NAME32 + 1, module.szModule, MAX_PATH, module.szExePath);
    } while (0);

    if (snap != INVALID_HANDLE_VALUE)
    {
        CloseHandle(snap);
    }

    if (errormessage != NULL && strlen(errormessage) != 0)
    {
        d_printf("%s\n", errormessage);
        return 0;
    }

    return (DWORD)(SIZE_T)module.modBaseAddr;
}

// ============================================================================
void cleanup(void)
{

    if (copy_of_df_memory != NULL)
    {
        GlobalFree(copy_of_df_memory);
        copy_of_df_memory = NULL;
    }

    if (suspended)
    {
        if ((DWORD)-1 == ResumeThread(df_h_thread))
        {
            // do what?  apologise?
        }
        else suspended = FALSE;
    }
#if 0
    // do this incase earlier runs crashed without executing the atexit handler.
    // TODO: trap exceptions?
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
    ResumeThread(df_h_thread);
#endif

    if (df_h_thread != NULL)
    {
        CloseHandle(df_h_thread);
        df_h_thread = NULL;
    }
    if (df_h_process != NULL)
    {
        CloseHandle(df_h_process);
        df_h_process = NULL;
    }
}


// ----------------------------------------------------------------------------
BOOL open_dwarf_fortress(void)
{
    HANDLE df_main_wh;
    char *state;

    change_page_permissions = FALSE;

    atexit(cleanup);

    df_memory_base = 0x400000;          // executables start here unless
    //   explicitly relocated.
    errormessage = "";


    // trigger a GPF
    //*(char *)(0) = '!';

    // trigger an invalid instruction (or DEP on newer processors).
    //   0F 0B is the UD1 pseudo-instruction, explicitly reserved as an invalid
    //   instruction to trigger faults.
    //asm(".fill 1, 1, 0x0F; .fill 1, 1, 0x0B");

    state = "calling FindWindow()... ";
    d_printf(state);
    df_main_wh = FindWindow("OpenGL","Dwarf Fortress");
    d_printf("done\n");
    dump(df_main_wh);
    if (df_main_wh == 0)
    {
        df_main_wh = FindWindow("SDL_app","Dwarf Fortress");
        if (df_main_wh == 0)
        {
            errormessage = "FindWindow(Dwarf Fortress) failed.\nIs the game running?";
            return FALSE;
        }
    }

    state = "calling GetWindowThreadProcessId()... ";
    d_printf(state);
    df_main_win_tid = GetWindowThreadProcessId(df_main_wh, &df_pid);
    d_printf("done\n");
    dump(df_pid);
    dump(df_main_win_tid);

    if (df_main_win_tid == 0) {
        errormessage =
            "GetWindowThreadProcessId(Dwarf Fortress) failed.\n"
            "That should not have happened!";
        return FALSE;
    }

    state = "calling OpenProcess()... ";
    d_printf(state);
    df_h_process = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_SUSPEND_RESUME | PROCESS_QUERY_INFORMATION, FALSE, df_pid);
    d_printf("done\n");
    dump(df_h_process);
    if (df_h_process == NULL)
    {
        errormessage = "OpenProcess(Dwarf Fortress) failed.\n"
                       "Are you the same user that started the game?";
        return(FALSE);
    }

    state = "calling OpenThread()... ";
    d_printf(state);
    df_h_thread = OpenThread(THREAD_ALL_ACCESS, FALSE, df_main_win_tid);
    d_printf("done\n");
    dump(df_h_thread);
    if (df_h_thread == NULL)
    {
        errormessage = "OpenThread(Dwarf Fortress) failed.";
        return(FALSE);
    }

    // TODO enum and suspend all threads?
    state = "calling SuspendThread()... ";
    d_printf(state);
    if ((DWORD)-1 == SuspendThread(df_h_thread))
    {
        errormessage = "SuspendThread(Dwarf Fortress) failed.";
        return(FALSE);
    }
    d_printf("done\n");
    suspended = TRUE;

    // get the base of the df executable
    df_memory_base = get_exe_base(df_pid);
    if (!df_memory_base)
    {
        errormessage = "GetModuleBase(Dwarf Fortress) failed.";
        return(FALSE);
    }
    dump(df_memory_base);

    // we could get this from the PE header, but why bother?
    df_memory_start = df_memory_base + 0x1000;
    dump(df_memory_start);

    // df_memory_end is based on the SizeOfImage field from the in-memory
    // copy of the PE header.
    df_memory_end = df_memory_base + peekd(df_memory_base+peekd(df_memory_base+0x3C)+0x50)-1;
    dump(df_memory_end);

    state = "calling GlobalAlloc(huge)... ";
    d_printf(state);
    dump(df_memory_end - df_memory_start + 0x100);
    if (NULL == (copy_of_df_memory = (BYTE *)GlobalAlloc(GPTR, df_memory_end - df_memory_start + 0x100)))
    {
        errormessage = "GlobalAlloc() of copy_of_df_memory failed";
        return FALSE;
    }
    d_printf("done\n");

    state = "copying memory... ";
    if (!peekarb(df_memory_start, copy_of_df_memory, df_memory_end-df_memory_start))
    {
        d_printf("peekarb(entire program) for search()'s use failed\n");
        return FALSE;
    }

    set_hexsearch2_limits(0, 0);

    return(TRUE);
}


// not (normally) necessary because it is done at exit time by atexit().
// however you can call this to resume DF before putting up a dialog box.
void close_dwarf_fortress(void)
{
    cleanup();
}

