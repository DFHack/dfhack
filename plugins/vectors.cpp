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

DFhackCExport command_result df_vectors  (Core * c,
                                          vector <string> & parameters);
DFhackCExport command_result df_clearvec (Core * c,
                                          vector <string> & parameters);

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

    commands.push_back(PluginCommand("clearvec",
               "Set list of vectors to zero length",
                      df_clearvec));
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

static bool mightBeVec(vector<t_memrange> &heap_ranges,
                       t_vecTriplet *vec)
{
    if ((vec->start > vec->end) || (vec->end > vec->alloc_end))
        return false;

    // Vector length might not be a multiple of 4 if, for example,
    // it's a vector of uint8_t or uint16_t.  However, the actual memory
    // allocated to the vector should be 4 byte aligned.
    if ((vec->start % 4 != 0) || (vec->alloc_end % 4 != 0))
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

static bool getHeapRanges(Core * c, std::vector<t_memrange> &heap_ranges)
{
    std::vector<t_memrange> ranges;

    c->p->getMemRanges(ranges);

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
    }

    if (heap_ranges.empty())
    {
        c->con << "No possible heap segments." << std::endl;
        return false;
    }

    return true;
}

////////////////////////////////////////
// COMMAND: vectors
////////////////////////////////////////

static void vectorsUsage(Console &con)
{
    con << "Usage: vectors <start of scan address> <# bytes to scan>"
        << std::endl;
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
        vectorsUsage(con);
        return CR_FAILURE;
    }

    uint32_t start = 0, bytes = 0;

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

    uint32_t end = start + bytes;

    // 4 byte alignment.
    while (start % 4 != 0)
        start++;

    c->Suspend();

    std::vector<t_memrange> heap_ranges;

    if (!getHeapRanges(c, heap_ranges))
    {
        return CR_FAILURE;
        c->Resume();
    }

    bool startInRange = false;
    for (size_t i = 0; i < heap_ranges.size(); i++)
    {
        t_memrange &range = heap_ranges[i];

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
        break;
    } // for (size_t i = 0; i < ranges.size(); i++)

    if (!startInRange)
    {
        con << "Address not in any memory range." << std::endl;
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

////////////////////////////////////////
// COMMAND: clearvec
////////////////////////////////////////

static void clearUsage(Console &con)
{
    con << "Usage: clearvec <vector1 addr> [vector2 addr] ..." << std::endl;
    con << "Address can be either for vector or pointer to vector."
        << std::endl;
}

DFhackCExport command_result df_clearvec (Core * c, vector <string> & parameters)
{
    Console & con = c->con;

    if (parameters.size() == 0)
    {
        clearUsage(con);
        return CR_FAILURE;
    }

    uint32_t start = 0, bytes = 0;

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

    c->Suspend();

    std::vector<t_memrange> heap_ranges;

    if (!getHeapRanges(c, heap_ranges))
    {
        c->Resume();
        return CR_FAILURE;
    }

    for (size_t i = 0; i < parameters.size(); i++)
    {
        std::string addr_str = parameters[i];
        uint32_t addr;
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

        if (!inAnyRange(heap_ranges, addr))
        {
            con << addr_str << " not in any valid address range." << std::endl;
            continue;
        }

        bool          valid = false;
        bool          ptr   = false;
        t_vecTriplet* vec   = (t_vecTriplet*) addr;

        if (mightBeVec(heap_ranges, vec))
            valid = true;
        else
        {
            // Is it a pointer to a vector?
            addr = * ( (uint32_t*) addr);
            vec  = (t_vecTriplet*) addr;

            if (inAnyRange(heap_ranges, addr) && mightBeVec(heap_ranges, vec))
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

    c->Resume();
    return CR_OK;
}
