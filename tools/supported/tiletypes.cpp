//

#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include <locale>
#include <functional>

using namespace std;
#include <DFHack.h>
#include <dfhack/extra/MapExtras.h>
#include <dfhack/extra/termutil.h>

void tolower(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), std::bind2nd(std::ptr_fun(&std::tolower<char>), std::locale("")));
}

void toupper(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), std::bind2nd(std::ptr_fun(&std::toupper<char>), std::locale("")));
}

int toint(const std::string &str, int failValue = 0)
{
    std::istringstream ss(str);
    int valInt;
    ss >> valInt;
    if (ss.fail())
    {
        return failValue;
    }
    return valInt;
}

struct TileType {
    DFHack::TileShape shape;
    DFHack::TileMaterial material;
    DFHack::TileSpecial special;

    TileType()
    {
        shape = DFHack::tileshape_invalid;
        material = DFHack::tilematerial_invalid;
        special = DFHack::tilespecial_invalid;
    }

    bool empty()
    {
        return shape == -1 && material == -1 && special == -1;
    }
};

std::ostream &operator<<(std::ostream &stream, const TileType &paint)
{
    bool used = false;
    bool needSpace = false;

    if (paint.special >= 0)
    {
        stream << DFHack::TileSpecialString[paint.special];
        used = true;
        needSpace = true;
    }

    if (paint.material >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << DFHack::TileMaterialString[paint.material];
        used = true;
        needSpace = true;
    }

    if (paint.shape >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << DFHack::TileShapeString[paint.shape];
        used = true;
    }

    if (!used)
    {
        stream << "any";
    }

    return stream;
}

bool processTileType(TileType &paint, const std::string &option, const std::string &value)
{
    std::string val = value;
    toupper(val);
    int valInt;
    if (val == "ANY")
    {
        valInt = -1;
    }
    else
    {
        valInt = toint(value, -2);
    }
    bool found = false;

    if (option == "shape" || option == "sh" || option == "s")
    {
        if (valInt >= -1 && valInt << DFHack::tileshape_count)
        {
            paint.shape = (DFHack::TileShape) valInt;
            found = true;
        }
        else
        {
            for (int i = 0; i < DFHack::tileshape_count; i++)
            {
                if (val == DFHack::TileShapeString[i])
                {
                    paint.shape = (DFHack::TileShape) i;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "Unknown tile shape: " << value << std::endl;
            }
        }
    }
    else if (option == "material" || option == "mat" || option == "m")
    {
        if (valInt >= -1 && valInt < DFHack::tilematerial_count)
        {
            paint.material = (DFHack::TileMaterial) valInt;
            found = true;
        }
        else
        {
            for (int i = 0; i < DFHack::tilematerial_count; i++)
            {
                if (val == DFHack::TileMaterialString[i])
                {
                    paint.material = (DFHack::TileMaterial) i;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "Unknown tile material: " << value << std::endl;
            }
        }
    }
    else if (option == "special" || option == "sp")
    {
        if (valInt >= -1 && valInt << DFHack::tilespecial_count)
        {
            paint.special = (DFHack::TileSpecial) valInt;
            found = true;
        }
        else
        {
            for (int i = 0; i < DFHack::tilespecial_count; i++)
            {
                if (val == DFHack::TileSpecialString[i])
                {
                    paint.special = (DFHack::TileSpecial) i;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "Unknown tile special: " << value << std::endl;
            }
        }
    }
    else
    {
        std::cout << "Unknown option: '" << option << "'" << std::endl;
    }

    return found;
}

void help(const std::string &option)
{
    if (option.empty())
    {
        std::cout << "Commands:" << std::endl
                  << " quit / q              : quit" << std::endl
                  << " filter / f [options]  : change filter options" << std::endl
                  << " paint / p [options]   : change paint options" << std::endl
                  << " point / p             : set point brush" << std::endl
                  << " range / r             : set range brush" << std::endl
                  << " block                 : set block brush" << std::endl
                  << " column                : set column brush" << std::endl
                  << std::endl
                  << "Filter/paint options:" << std::endl
                  << " Shape / sh / s: set tile shape information" << std::endl
                  << " Material / mat / m: set tile material information" << std::endl
                  << " Special / s: set special tile information" << std::endl
                  << "See help [option] for more information" << std::endl;
    }
    else if (option == "shape")
    {
        std::cout << "Available shapes:" << std::endl
                  << " ANY" << std::endl;
        for (int i = 0; i < DFHack::tileshape_count; i++)
        {
            std::cout << " " << DFHack::TileShapeString[i] << std::endl;
        }
    }
    else if (option == "material")
    {
        std::cout << "Available materials:" << std::endl
                  << " ANY" << std::endl;
        for (int i = 0; i < DFHack::tilematerial_count; i++)
        {
            std::cout << " " << DFHack::TileMaterialString[i] << std::endl;
        }
    }
    else if (option == "special")
    {
        std::cout << "Available specials:" << std::endl
                  << " ANY" << std::endl;
        for (int i = 0; i < DFHack::tilespecial_count; i++)
        {
            std::cout << " " << DFHack::TileSpecialString[i] << std::endl;
        }
    }
}

typedef std::vector<DFHack::DFCoord> coord_vec;

class Brush
{
public:
    virtual ~Brush() {};
    virtual coord_vec points(MapExtras::MapCache &mc, DFHack::DFCoord start) = 0;
};
/**
 * generic 3D rectangle brush. you can specify the dimensions of
 * the rectangle and optionally which tile is its 'center'
 */
class RectangleBrush : public Brush
{
    int x_, y_, z_;
    int cx_, cy_, cz_;

public:
    RectangleBrush(int x, int y, int z = 1, int centerx = -1, int centery = -1, int centerz = -1)
    {
        if (centerx == -1)
        {
            cx_ = x/2;
        }
        else
        {
            cx_ = centerx;
        }

        if (centery == -1)
        {
            cy_ = y/2;
        }
        else
        {
            cy_ = centery;
        }

        if (centerz == -1)
        {
            cz_ = z/2;
        }
        else
        {
            cz_ = centerz;
        }

        x_ = x;
        y_ = y;
        z_ = z;
    };

    coord_vec points(MapExtras::MapCache &mc, DFHack::DFCoord start)
    {
        coord_vec v;
        DFHack::DFCoord iterstart(start.x - cx_, start.y - cy_, start.z - cz_);
        DFHack::DFCoord iter = iterstart;
        for (int xi = 0; xi < x_; xi++)
        {
            for (int yi = 0; yi < y_; yi++)
            {
                for (int zi = 0; zi < z_; zi++)
                {
                    if(mc.testCoord(iter))
                        v.push_back(iter);

                    iter.z++;
                }

                iter.z = iterstart.z;
                iter.y++;
            }

            iter.y = iterstart.y;
            iter.x ++;
        }

        return v;
    };

    ~RectangleBrush(){};
};

/**
 * stupid block brush, legacy. use when you want to apply something to a whole DF map block.
 */
class BlockBrush : public Brush
{
public:
    BlockBrush() {};
    ~BlockBrush() {};

    coord_vec points(MapExtras::MapCache &mc, DFHack::DFCoord start)
    {
        coord_vec v;
        DFHack::DFCoord blockc = start % 16;
        DFHack::DFCoord iterc = blockc * 16;
        if (!mc.testCoord(start))
            return v;

        for (int xi = 0; xi < 16; xi++)
        {
            for (int yi = 0; yi < 16; yi++)
            {
                v.push_back(iterc);
                iterc.y++;
            }
            iterc.x++;
        }

        return v;
    };
};

/**
 * Column from a position through open space tiles
 * example: create a column of magma
 */
class ColumnBrush : public Brush
{
public:
    ColumnBrush(){};

    ~ColumnBrush(){};

    coord_vec points(MapExtras::MapCache &mc, DFHack::DFCoord start)
    {
        coord_vec v;
        bool juststarted = true;
        while (mc.testCoord(start))
        {
            uint16_t tt = mc.tiletypeAt(start);
            if(DFHack::LowPassable(tt) || juststarted && DFHack::HighPassable(tt))
            {
                v.push_back(start);
                juststarted = false;
                start.z++;
            }
            else break;
        }
        return v;
    };
};

int main(int argc, char *argv[])
{
    bool temporary_terminal = TemporaryTerminal();
    uint32_t x_max = 0, y_max = 0, z_max = 0;
    int32_t x = 0, y = 0, z = 0;
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
    DFHack::Gui *gui = context->getGui();

    TileType filter, paint;
    Brush *brush = new RectangleBrush(1,1);
    bool end = false;
    std::string brushname = "point";
    int width = 1, height = 1, z_levels = 1;

    context->Resume();
    while (!end)
    {
        std::cout << std::endl;
        std::cout << "Filter: " << filter << std::endl;
        std::cout << "Paint: " << paint << std::endl;
        std::cout << "Brush: " << brushname << std::endl;

        std::string input = "";
        std::string command = "";
        std::string option = "";
        std::string value = "";

        std::cout << "Command > ";
        std::getline(std::cin, input);
        if (std::cin.eof())
        {
            command = "q";
            std::cout << std::endl; // No newline from the user here!
        }
        std::istringstream ss(input);
        ss >> command >> option >> value;
        tolower(command);
        tolower(option);

        if (command == "help" || command == "?")
        {
            help(option);
        }
        else if (command == "quit" || command == "q")
        {
            end = true;
        }
        else if (command == "filter" || command == "f")
        {
            processTileType(filter, option, value);
        }
        else if (command == "paint" || (command == "p" && !option.empty()))
        {
            processTileType(paint, option, value);
        }
        else if (command == "point" || command == "p")
        {
            delete brush;
            brushname = "point";
            brush = new RectangleBrush(1,1);
        }
        else if (command == "range" || command == "r")
        {
            std::cout << "\tSet range width <" << width << "> ";
            std::getline(std::cin, command);
            width = command == "" ? width : toint(command);
            if (width < 1) width = 1;

            std::cout << "\tSet range height <" << height << "> ";
            std::getline(std::cin, command);
            height = command == "" ? height : toint(command);
            if (height < 1) height = 1;

            std::cout << "\tSet range z-levels <" << z_levels << "> ";
            std::getline(std::cin, command);
            z_levels = command == "" ? z_levels : toint(command);
            if (z_levels < 1) z_levels = 1;

            delete brush;
            if (width == 1 && height == 1 && z_levels == 1)
            {
                brushname = "point";
            }
            else
            {
                brushname = "range";
            }
            brush = new RectangleBrush(width, height, z_levels, 0, 0, 0);
        }
        else if (command == "block")
        {
            delete brush;
            brushname = "block";
            brush = new BlockBrush();
        }
        else if (command == "column")
        {
            delete brush;
            brushname = "column";
            brush = new ColumnBrush();
        }
        else if (command.empty())
        {
            if (paint.empty())
            {
                std::cout << "No paint!" << std::endl;
                continue;
            }

            context->Suspend();

            if (!maps->Start())
            {
                std::cerr << "Cannot get map info!" << std::endl;
                context->Detach();
                if(temporary_terminal)
                    std::cin.ignore();
                return 1;
            }
            maps->getSize(x_max, y_max, z_max);

            if (!(gui->Start() && gui->getCursorCoords(x,y,z)))
            {
                cout << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
                break;
            }
            std::cout << "Cursor coords: (" << x << ", " << y << ", " << z << ")" << std::endl;

            DFHack::DFCoord cursor(x,y,z);
            MapExtras::MapCache map(maps);
            coord_vec all_tiles = brush->points(map, cursor);
            std::cout << "working..." << std::endl;

            for (coord_vec::iterator iter = all_tiles.begin(); iter != all_tiles.end(); ++iter)
            {
                const DFHack::TileRow *source = DFHack::getTileRow(map.tiletypeAt(*iter));

                if ((filter.shape > -1 && filter.shape != source->shape)
                        || (filter.material > -1 && filter.material != source->material)
                        || (filter.special > -1 && filter.special != source->special))
                {
                    continue;
                }

                DFHack::TileShape shape = paint.shape;
                if (shape < 0)
                {
                    shape = source->shape;
                }

                DFHack::TileMaterial material = paint.material;
                if (material < 0)
                {
                    material = source->material;
                }

                DFHack::TileSpecial special = paint.special;
                if (special < 0)
                {
                    special = source->special;
                }

                int32_t type = DFHack::findTileType(shape, material, source->variant, special, source->direction);
                map.setTiletypeAt(*iter, type);

                // Remove liquid from walls, etc
                if (!DFHack::FlowPassable(shape))
                {
                    DFHack::t_designation des = map.designationAt(*iter);
                    des.bits.flow_size = 0;
                    map.setDesignationAt(*iter, des);
                }
            }

            if (map.WriteAll())
            {
                std::cout << "OK" << std::endl;
            }
            else
            {
                std::cout << "Something failed horribly! RUN!" << std::endl;
            }
            maps->Finish();
            context->Resume();
        }
    }

    context->Detach();
    if(temporary_terminal)
    {
        std::cout << "Press any key to finish.";
        std::cin.ignore();
    }
    std::cout << std::endl;
    return 0;
}
