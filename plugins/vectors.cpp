// Lists embeded STL vectors and pointers to STL vectors found in the given
// memory range.
//
// Linux only, enabled with BUILD_VECTORS cmake option.

#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/Process.h>
#include <vector>
#include <string>

#include <stdio.h>

using std::vector;
using std::string;
using namespace DFHack;

struct t_vecTriplet
{
    uint32_t start;
    uint32_t end;
    uint32_t alloc_end;
};

DFhackCExport command_result df_vectors (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "vectors";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("vectors",
               "Scan memory for vectors.\
\n                1st param: start of scan\
\n                2nd param: number of bytes to scan",
                      df_vectors));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

static bool hexOrDec(string &str, uint32_t &value)
{
    if (str.find("0x") == 0 && sscanf(str.c_str(), "%x", &value) == 1)
        return true;
    else if (sscanf(str.c_str(), "%u", &value) == 1)
        return true;

    return false;
}

static void usage(Console &con)
{
    con << "Usage: vectors <start of scan address> <# bytes to scan>"
        << std::endl;
}

static bool mightBeVec(vector<t_memrange> &heap_ranges,
                       t_vecTriplet *vec)
{
    if ((vec->start > vec->end) || (vec->end > vec->alloc_end))
        return false;

    if ((vec->end - vec->start) % 4 != 0)
        return false;

    for (size_t i = 0; i < heap_ranges.size(); i++)
    {
        t_memrange &range = heap_ranges[i];

        if (range.isInRange(vec->start) && range.isInRange(vec->alloc_end))
            return true;
    }

    return false;
}

static bool inAnyRange(vector<t_memrange> &ranges, uint32_t ptr)
{
    for (size_t i = 0; i < ranges.size(); i++)
    {
        if (ranges[i].isInRange(ptr))
            return true;
    }

    return false;
}

static void printVec(Console &con, const char* msg, t_vecTriplet *vec,
                     uint32_t start, uint32_t pos)
{
    uint32_t length = vec->end - vec->start;
    uint32_t offset = pos - start;

    con.print("%8s offset %06p, addr %010p, start %010p, length %u\n",
              msg, offset, pos, vec->start, length);
}

DFhackCExport command_result df_vectors (Core * c, vector <string> & parameters)
{
    Console & con = c->con;

    if (parameters.size() != 2)
    {
        usage(con);
        return CR_FAILURE;
    }

    uint32_t start = 0, bytes = 0;

    if (!hexOrDec(parameters[0], start))
    {
        usage(con);
        return CR_FAILURE;
    }
    
    if (!hexOrDec(parameters[1], bytes))
    {
        usage(con);
        return CR_FAILURE;
    }

    uint32_t end = start + bytes;

    // 4 byte alignment.
    while (start % 4 != 0)
        start++;

    c->Suspend();

    std::vector<t_memrange> ranges;
    std::vector<t_memrange> heap_ranges;
    c->p->getMemRanges(ranges);

    bool startInRange = false;

    for (size_t i = 0; i < ranges.size(); i++)
    {
        t_memrange &range = ranges[i];

        // Some kernels don't report [heap], and the heap can consist of
        // more segments than just the one labeled with [heap], so include
        // all segments which *might* be part of the heap.
        if (range.read && range.write && !range.shared)
        {
            if (strlen(range.name) == 0 || strcmp(range.name, "[heap]") == 0)
                heap_ranges.push_back(range);
        }

        if (!range.isInRange(start))
            continue;

        // Found the range containing the start
        if (!range.isInRange(end))
        {
            con.print("Scanning %u bytes would read past end of memory "
                      "range.\n", bytes);
            uint32_t diff = end - range.end;
            con.print("Cutting bytes down by %u.\n", diff);

            end = (uint32_t) range.end;
        }
        startInRange = true;
    } // for (size_t i = 0; i < ranges.size(); i++)

    if (!startInRange)
    {
        con << "Address not in any memory range." << std::endl;
        c->Resume();
        return CR_FAILURE;
    }

    if (heap_ranges.empty())
    {
        con << "No possible heap segments." << std::endl;
        c->Resume();
        return CR_FAILURE;
    }

    uint32_t pos = start;

    const uint32_t ptr_size = sizeof(void*);

    for (uint32_t pos = start; pos < end; pos += ptr_size)
    {
        // Is it an embeded vector?
        if (pos <= ( end - sizeof(t_vecTriplet) ))
        {
            t_vecTriplet* vec = (t_vecTriplet*) pos;

            if (mightBeVec(heap_ranges, vec))
            {
                printVec(con, "VEC:", vec, start, pos);
                // Skip over rest of vector.
                pos += sizeof(t_vecTriplet) - ptr_size;
                continue;
            }
        }

        // Is it a vector pointer?
        if (pos <= (end - ptr_size))
        {
            uint32_t ptr = * ( (uint32_t*) pos);

            if (inAnyRange(heap_ranges, ptr))
            {
                t_vecTriplet* vec = (t_vecTriplet*) ptr;

                if (mightBeVec(heap_ranges, vec))
                {
                    printVec(con, "VEC PTR:", vec, start, pos);
                    continue;
                }
            }
        }
    } // for (uint32_t pos = start; pos < end; pos += ptr_size)

    c->Resume();
    return CR_OK;
}
