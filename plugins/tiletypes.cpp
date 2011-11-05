//
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <sstream>
using std::vector;
using std::string;
using std::endl;
using std::set;

#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Vegetation.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/modules/Gui.h>
#include <dfhack/TileTypes.h>
#include <dfhack/extra/MapExtras.h>
using namespace MapExtras;
using namespace DFHack;

//zilpin: These two functions were giving me compile errors in VS2008, so I cheated with the C style loop below, just to get it to build.
//Original code is commented out.
void tolower(std::string &str)
{
    //The C++ way...
    //std::transform(str.begin(), str.end(), str.begin(), std::bind2nd(std::ptr_fun(&std::tolower<char> ), std::locale("")));

    //The C way...
    for(char *c=(char *)str.c_str(); *c; ++c)
    {
        *c = tolower(*c);
    }
}

void toupper(std::string &str)
{
    //std::transform(str.begin(), str.end(), str.begin(), std::bind2nd(std::ptr_fun(&std::toupper<char>), std::locale("")));
    for(char *c=(char *)str.c_str(); *c; ++c)
    {
        *c = toupper(*c);
    }
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

struct TileType
{
    DFHack::TileShape shape;
    DFHack::TileMaterial material;
    DFHack::TileSpecial special;
    DFHack::TileVariant variant;
    int hidden;
    int light;
    int subterranean;
    int skyview;

    TileType()
    {
        shape = DFHack::tileshape_invalid;
        material = DFHack::tilematerial_invalid;
        special = DFHack::tilespecial_invalid;
        variant = DFHack::tilevariant_invalid;
        hidden = -1;
        light = -1;
        subterranean = -1;
        skyview = -1;
    }

    bool empty()
    {
        return shape == -1 && material == -1 && special == -1 && variant == -1
            && hidden == -1 && light == -1 && subterranean == -1 && skyview == -1;
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
        needSpace = true;
    }

    if (paint.variant >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << "VAR_" << (paint.variant + 1);
        used = true;
        needSpace = true;
    }

    if (paint.hidden >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.hidden ? "HIDDEN" : "VISIBLE");
        used = true;
        needSpace = true;
    }

    if (paint.light >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.light ? "LIGHT" : "DARK");
        used = true;
        needSpace = true;
    }

    if (paint.subterranean >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.subterranean ? "SUBTERRANEAN" : "ABOVE GROUND");
        used = true;
        needSpace = true;
    }

    if (paint.skyview >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.skyview ? "OUTSIDE" : "INSIDE");
        used = true;
        needSpace = true;
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
        if (valInt >= -1 && valInt < DFHack::tileshape_count)
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
        if (valInt >= -1 && valInt < DFHack::tilespecial_count)
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
    else if (option == "variant" || option == "var" || option == "v")
    {
        if (valInt >= -1 && valInt <= DFHack::VAR_4)
        {
            paint.variant = (DFHack::TileVariant) valInt;
            found = true;
        }
        else
        {
            std::cout << "Unknown tile variant: " << value << std::endl;
        }
    }
    else if (option == "hidden" || option == "h")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.hidden = valInt;
            found = true;
        }
        else
        {
            std::cout << "Unknown hidden flag: " << value << std::endl;
        }
    }
    else if (option == "light" || option == "l")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.light = valInt;
            found = true;
        }
        else
        {
            std::cout << "Unknown light flag: " << value << std::endl;
        }
    }
    else if (option == "subterranean" || option == "st")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.subterranean = valInt;
            found = true;
        }
        else
        {
            std::cout << "Unknown subterranean flag: " << value << std::endl;
        }
    }
    else if (option == "skyview" || option == "sv")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.skyview = valInt;
            found = true;
        }
        else
        {
            std::cout << "Unknown skyview flag: " << value << std::endl;
        }
    }
    else
    {
        std::cout << "Unknown option: '" << option << "'" << std::endl;
    }

    return found;
}

void help( std::ostream & out, const std::string &option)
{
    if (option.empty())
    {
        out << "Commands:" << std::endl
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
            << " Special / sp: set special tile information" << std::endl
            << " Variant / var / v: set variant tile information" << std::endl
            << " Hidden / h: set hidden flag" << std::endl
            << " Light / l: set light flag" << std::endl
            << " Subterranean / st: set subterranean flag" << std::endl
            << " Skyview / sv: set skyview flag" << std::endl
            << "See help [option] for more information" << std::endl;
    }
    else if (option == "shape" || option == "s" ||option == "sh")
    {
        out << "Available shapes:" << std::endl
            << " ANY" << std::endl;
        for (int i = 0; i < DFHack::tileshape_count; i++)
        {
            out << " " << DFHack::TileShapeString[i] << std::endl;
        }
    }
    else if (option == "material"|| option == "mat" ||option == "m")
    {
        out << "Available materials:" << std::endl
            << " ANY" << std::endl;
        for (int i = 0; i < DFHack::tilematerial_count; i++)
        {
            out << " " << DFHack::TileMaterialString[i] << std::endl;
        }
    }
    else if (option == "special" || option == "sp")
    {
        out << "Available specials:" << std::endl
            << " ANY" << std::endl;
        for (int i = 0; i < DFHack::tilespecial_count; i++)
        {
            out << " " << DFHack::TileSpecialString[i] << std::endl;
        }
    }
    else if (option == "variant" || option == "var" || option == "v")
    {
        out << "Available variants:" << std::endl
            << " ANY, 0 - " << DFHack::VAR_4 << std::endl;
    }
    else if (option == "hidden" || option == "h")
    {
        out << "Available hidden flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "light" || option == "l")
    {
        out << "Available light flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "subterranean" || option == "st")
    {
        out << "Available subterranean flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "skyview" || option == "sv")
    {
        out << "Available skyview flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
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

CommandHistory tiletypes_hist;

DFhackCExport command_result df_tiletypes (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "tiletypes";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    tiletypes_hist.load("tiletypes.history");
    commands.clear();
    commands.push_back(PluginCommand("tiletypes", "Paint map tiles freely, similar to liquids.", df_tiletypes, true));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    tiletypes_hist.save("tiletypes.history");
    return CR_OK;
}

DFhackCExport command_result df_tiletypes (Core * c, vector <string> & parameters)
{
    uint32_t x_max = 0, y_max = 0, z_max = 0;
    int32_t x = 0, y = 0, z = 0;

    DFHack::Maps *maps;
    DFHack::Gui *gui;
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("This tool allows painting tiles types with a brush, using an optional filter.\n"
                         "The tool is interactive, similarly to the liquids tool.\n"
                         "Further help is available inside.\n"
            );
            return CR_OK;
        }
    }

    TileType filter, paint;
    Brush *brush = new RectangleBrush(1,1);
    bool end = false;
    std::string brushname = "point";
    int width = 1, height = 1, z_levels = 1;
    c->con << "Welcome to the tiletype tool.\nType 'help' or '?' for a list of available commands, 'q' to quit.\nPress return after a command to confirm." << std::endl;
    c->con.printerr("THIS TOOL CAN BE DANGEROUS. YOU'VE BEEN WARNED.\n");
    while (!end)
    {
        c->con << "Filter: " << filter    << std::endl
               << "Paint: "  << paint     << std::endl
               << "Brush: "  << brushname << std::endl;

        std::string input = "";
        std::string command = "";
        std::string option = "";
        std::string value = "";

        c->con.lineedit("tiletypes> ",input,tiletypes_hist);
        tiletypes_hist.add(input);
        std::istringstream ss(input);
        ss >> command >> option >> value;
        tolower(command);
        tolower(option);

        if (command == "help" || command == "?")
        {
            help(c->con,option);
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
            std::stringstream ss;
            CommandHistory hist;
            ss << "Set range width <" << width << "> ";
            c->con.lineedit(ss.str(),command,hist);
            width = command == "" ? width : toint(command);
            if (width < 1) width = 1;

            ss.str("");
            ss << "Set range height <" << height << "> ";
            c->con.lineedit(ss.str(),command,hist);
            height = command == "" ? height : toint(command);
            if (height < 1) height = 1;

            ss.str("");
            ss << "Set range z-levels <" << z_levels << "> ";
            c->con.lineedit(ss.str(),command,hist);
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
                c->con.printerr("Set the paint first.\n");
                continue;
            }

            c->Suspend();
            maps = c->getMaps();
            gui = c->getGui();
            if (!maps->Start())
            {
                c->con.printerr("Cannot get map info!\n");
                c->Resume();
                return CR_FAILURE;
            }
            maps->getSize(x_max, y_max, z_max);

            if (!(gui->Start() && gui->getCursorCoords(x,y,z)))
            {
                c->con.printerr("Can't get cursor coords! Make sure you have a cursor active in DF.\n");
                c->Resume();
                return CR_FAILURE;
            }
            c->con.print("Cursor coords: (%d, %d, %d)\n",x,y,z);

            DFHack::DFCoord cursor(x,y,z);
            MapExtras::MapCache map(maps);
            coord_vec all_tiles = brush->points(map, cursor);
            c->con.print("working...\n");

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

                DFHack::TileVariant variant = paint.variant;
                if (variant < 0)
                {
                    variant = source->variant;
                }

                // Remove direction from directionless tiles
                DFHack::TileDirection direction = source->direction;
                if (!(shape == DFHack::RIVER_BED || shape == DFHack::BROOK_BED || shape == DFHack::WALL && (material == DFHack::CONSTRUCTED || special == DFHack::TILE_SMOOTH))) {
                    direction.whole = 0;
                }

                int32_t type = DFHack::findTileType(shape, material, variant, special, direction);
                // hack for empty space
                if (shape == DFHack::EMPTY && material == DFHack::AIR && variant == DFHack::VAR_1 && special == DFHack::TILE_NORMAL && direction.whole == 0) {
                    type = 32;
                }
                // make sure it's not invalid
                if(type != -1)
                    map.setTiletypeAt(*iter, type);

                DFHack::t_designation des = map.designationAt(*iter);

                if (paint.hidden > -1)
                {
                    des.bits.hidden = paint.hidden;
                }

                if (paint.light > -1)
                {
                    des.bits.light = paint.light;
                }

                if (paint.subterranean > -1)
                {
                    des.bits.subterranean = paint.subterranean;
                }

                if (paint.skyview > -1)
                {
                    des.bits.skyview = paint.skyview;
                }

                // Remove liquid from walls, etc
                if (type != -1 && !DFHack::FlowPassable(type))
                {
                    des.bits.flow_size = 0;
                    //des.bits.liquid_type = DFHack::liquid_water;
                    //des.bits.water_table = 0;
                    des.bits.flow_forbid = 0;
                    //des.bits.liquid_static = 0;
                    //des.bits.water_stagnant = 0;
                    //des.bits.water_salt = 0;
                }

                map.setDesignationAt(*iter, des);
            }

            if (map.WriteAll())
            {
                c->con.print("OK\n");
            }
            else
            {
                c->con.printerr("Something failed horribly! RUN!\n");
            }
            maps->Finish();
            c->Resume();
        }
    }
    return CR_OK;
}
