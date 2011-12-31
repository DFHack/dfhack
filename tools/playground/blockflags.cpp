// Invert/toggle all block flags, to see what they do.
// Seems like they don't do anything...

#include <iostream>
#include <iomanip>

using namespace std;
#include <DFHack.h>
#include <extra/MapExtras.h>
#include <extra/termutil.h>


int main(int argc, char *argv[])
{
    bool temporary_terminal = TemporaryTerminal();

    uint32_t x_max = 0, y_max = 0, z_max = 0;
    DFHack::ContextManager manager("Memory.xml");

    DFHack::Context *context = manager.getSingleContext();
    if (!context->Attach())
    {
        std::cerr << "Unable to attach to DF!" << std::endl;
        if(temporary_terminal)
            std::cin.ignore();
        return 1;
    }

    DFHack::Maps *maps = context->getMaps();
    if (!maps->Start())
    {
        std::cerr << "Cannot get map info!" << std::endl;
        context->Detach();
        if(temporary_terminal)
            std::cin.ignore();
        return 1;
    }
    maps->getSize(x_max, y_max, z_max);
    MapExtras::MapCache map(maps);

    for(uint32_t z = 0; z < z_max; z++)
    {
        for(uint32_t b_y = 0; b_y < y_max; b_y++)
        {
            for(uint32_t b_x = 0; b_x < x_max; b_x++)
            {
                // Get the map block
                DFHack::DFCoord blockCoord(b_x, b_y);
                MapExtras::Block *b = map.BlockAt(DFHack::DFCoord(b_x, b_y, z));
                if (!b || !b->valid)
                {
                    continue;
                }

                DFHack::t_blockflags flags = b->BlockFlags();
                flags.whole = flags.whole ^ 0xFFFFFFFF;
                b->setBlockFlags(flags);
                b->Write();
            } // block x
        } // block y
    } // z

    maps->Finish();
    context->Detach();
    return 0;
}
