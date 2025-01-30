// Lists embeded STL vectors and pointers to STL vectors found in the given
// memory range.
//
// Linux and OS X only

#include <vector>
#include <string>

#include <stdio.h>

#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "VersionInfo.h"

using std::vector;
using std::string;
using namespace DFHack;

struct t_vecTriplet
{
    void * start;
    void * end;
    void * alloc_end;
};

command_result df_vectors  (color_ostream &out, vector <string> & parameters);
command_result df_clearvec (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("vectors");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("vectors",
               "Scan memory for vectors.\
\n                1st param: start of scan\
\n                2nd param: number of bytes to scan",
                      df_vectors));

    commands.push_back(PluginCommand("clearvec",
               "Set list of vectors to zero length",
                      df_clearvec));
     return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static bool hexOrDec(string &str, uintptr_t &value)
{
    std::stringstream ss;
    ss << str;
    if (str.find("0x") == 0 && (ss >> std::hex >> value))
        return true;
    else if (ss >> std::dec >> value)
        return true;

    return false;
}

static bool mightBeVec(vector<t_memrange> &ranges,
                       t_vecTriplet *vec)
{
    if ((vec->start > vec->end) || (vec->end > vec->alloc_end))
        return false;

    // Vector length might not be a multiple of 4 if, for example,
    // it's a vector of uint8_t or uint16_t.  However, the actual memory
    // allocated to the vector should be 4 byte aligned.
    if (((intptr_t)vec->start % 4 != 0) || ((intptr_t)vec->alloc_end % 4 != 0))
        return false;

    for (size_t i = 0; i < ranges.size(); i++)
    {
        t_memrange &range = ranges[i];

        if (range.isInRange(vec->start) && range.isInRange(vec->alloc_end))
            return true;
    }

    return false;
}

static bool inAnyRange(vector<t_memrange> &ranges, void * ptr)
{
    for (size_t i = 0; i < ranges.size(); i++)
    {
        if (ranges[i].isInRange(ptr))
            return true;
    }

    return false;
}

static bool getRanges(color_ostream &out, std::vector<t_memrange> &out_ranges)
{
    std::vector<t_memrange> ranges;

    Core::getInstance().p->getMemRanges(ranges);

    for (size_t i = 0; i < ranges.size(); i++)
    {
        t_memrange &range = ranges[i];

        if (range.read && range.write && !range.shared)
            out_ranges.push_back(range);
    }

    if (out_ranges.empty())
    {
        out << "No readable segments." << std::endl;
        return false;
    }

    return true;
}

////////////////////////////////////////
// COMMAND: vectors
////////////////////////////////////////

static void vectorsUsage(color_ostream &con)
{
    con << "Usage: vectors <start of scan address> <# bytes to scan>"
        << std::endl;
}

static void printVec(color_ostream &con, const char* msg, t_vecTriplet *vec,
                     uintptr_t start, uintptr_t pos, std::vector<t_memrange> &ranges)
{
    uintptr_t length = (intptr_t)vec->end - (intptr_t)vec->start;
    uintptr_t offset = pos - start;

    con.print("%8s offset 0x%06zx, addr 0x%01zx, start 0x%01zx, length %zi",
              msg, offset, pos, intptr_t(vec->start), length);
    if (length >= 4 && length % 4 == 0)
    {
        void *ptr = vec->start;
        for (int level = 0; level < 2; level++)
        {
            if (inAnyRange(ranges, ptr))
                ptr = *(void**)ptr;
        }
        std::string classname;
        if (Core::getInstance().vinfo->getVTableName(ptr, classname))
            con.print(", 1st item: %s", classname.c_str());
    }
    con.print("\n");
}

command_result df_vectors (color_ostream &con, vector <string> & parameters)
{
    if (parameters.size() != 2)
    {
        vectorsUsage(con);
        return CR_FAILURE;
    }

    uintptr_t start = 0, bytes = 0;

    if (!hexOrDec(parameters[0], start))
    {
        vectorsUsage(con);
        return CR_FAILURE;
    }

    if (!hexOrDec(parameters[1], bytes))
    {
        vectorsUsage(con);
        return CR_FAILURE;
    }

    uintptr_t end = start + bytes;

    // 4 byte alignment.
    while (start % 4 != 0)
        start++;

    std::vector<t_memrange> ranges;

    if (!getRanges(con, ranges))
    {
        return CR_FAILURE;
    }

    bool startInRange = false;
    for (size_t i = 0; i < ranges.size(); i++)
    {
        t_memrange &range = ranges[i];

        if (!range.isInRange((void *)start))
            continue;

        // Found the range containing the start
        if (!range.isInRange((void *)end))
        {
            con.print("Scanning %zi bytes would read past end of memory "
                      "range.\n", bytes);
            size_t diff = end - (intptr_t)range.end;
            con.print("Cutting bytes down by %zi.\n", diff);

            end = (uintptr_t) range.end;
        }
        startInRange = true;
        break;
    } // for (size_t i = 0; i < ranges.size(); i++)

    if (!startInRange)
    {
        con << "Address not in any memory range." << std::endl;
        return CR_FAILURE;
    }

    const size_t ptr_size = sizeof(void*);

    for (uintptr_t pos = start; pos < end; pos += ptr_size)
    {
        // Is it an embeded vector?
        if (pos <= ( end - sizeof(t_vecTriplet) ))
        {
            t_vecTriplet* vec = (t_vecTriplet*) pos;

            if (mightBeVec(ranges, vec))
            {
                printVec(con, "VEC:", vec, start, pos, ranges);
                // Skip over rest of vector.
                pos += sizeof(t_vecTriplet) - ptr_size;
                continue;
            }
        }

        // Is it a vector pointer?
        if (pos <= (end - ptr_size))
        {
            uintptr_t ptr = * ( (uintptr_t*) pos);

            if (inAnyRange(ranges, (void *) ptr))
            {
                t_vecTriplet* vec = (t_vecTriplet*) ptr;

                if (mightBeVec(ranges, vec))
                {
                    printVec(con, "VEC PTR:", vec, start, pos, ranges);
                    continue;
                }
            }
        }
    } // for (uint32_t pos = start; pos < end; pos += ptr_size)

    return CR_OK;
}

////////////////////////////////////////
// COMMAND: clearvec
////////////////////////////////////////

static void clearUsage(color_ostream &con)
{
    con << "Usage: clearvec <vector1 addr> [vector2 addr] ..." << std::endl;
    con << "Address can be either for vector or pointer to vector."
        << std::endl;
}

command_result df_clearvec (color_ostream &con, vector <string> & parameters)
{
    if (parameters.size() == 0)
    {
        clearUsage(con);
        return CR_FAILURE;
    }

    uintptr_t start = 0, bytes = 0;

    if (!hexOrDec(parameters[0], start))
    {
        vectorsUsage(con);
        return CR_FAILURE;
    }

    if (!hexOrDec(parameters[1], bytes))
    {
        vectorsUsage(con);
        return CR_FAILURE;
    }

    std::vector<t_memrange> ranges;

    if (!getRanges(con, ranges))
    {
        return CR_FAILURE;
    }

    for (size_t i = 0; i < parameters.size(); i++)
    {
        std::string addr_str = parameters[i];
        uintptr_t addr;
        if (!hexOrDec(addr_str, addr))
        {
            con << "'" << addr_str << "' not a number." << std::endl;
            continue;
        }

        if (addr % 4 != 0)
        {
            con << addr_str << " not 4 byte aligned." << std::endl;
            continue;
        }

        if (!inAnyRange(ranges, (void *) addr))
        {
            con << addr_str << " not in any valid address range." << std::endl;
            continue;
        }

        bool          valid = false;
        bool          ptr   = false;
        t_vecTriplet* vec   = (t_vecTriplet*) addr;

        if (mightBeVec(ranges, vec))
            valid = true;
        else
        {
            // Is it a pointer to a vector?
            addr = * ( (uintptr_t*) addr);
            vec  = (t_vecTriplet*) addr;

            if (inAnyRange(ranges, (void *) addr) && mightBeVec(ranges, vec))
            {
                valid = true;
                ptr   = true;
            }
        }

        if (!valid)
        {
            con << addr_str << " is not a vector." << std::endl;
            continue;
        }

        vec->end = vec->start;

        if (ptr)
            con << "Vector pointer ";
        else
            con << "Vector         ";

        con << addr_str << " set to zero length." << std::endl;
    } // for (size_t i = 0; i < parameters.size(); i++)

    return CR_OK;
}
