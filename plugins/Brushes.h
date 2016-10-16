#pragma once
#include <llimits.h>
#include <sstream>
#include <string>
#include <stack>
#include <set>

typedef vector <df::coord> coord_vec;
class Brush
{
public:
    virtual ~Brush(){};
    virtual coord_vec points(MapExtras::MapCache & mc,DFHack::DFCoord start) = 0;
    virtual std::string str() const {
        return "unknown";
    }
};
/**
 * generic 3D rectangle brush. you can specify the dimensions of
 * the rectangle and optionally which tile is its 'center'
 */
class RectangleBrush : public Brush
{
public:
    RectangleBrush(int x, int y, int z = 1, int centerx = -1, int centery = -1, int centerz = -1)
    {
        if(centerx == -1)
            cx_ = x/2;
        else
            cx_ = centerx;
        if(centery == -1)
            cy_ = y/2;
        else
            cy_ = centery;
        if(centerz == -1)
            cz_ = z/2;
        else
            cz_ = centerz;
        x_ = x;
        y_ = y;
        z_ = z;
    };
    coord_vec points(MapExtras::MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        DFHack::DFCoord iterstart(start.x - cx_, start.y - cy_, start.z - cz_);
        DFHack::DFCoord iter = iterstart;
        for(int xi = 0; xi < x_; xi++)
        {
            for(int yi = 0; yi < y_; yi++)
            {
                for(int zi = 0; zi < z_; zi++)
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
    std::string str() const {
        if (x_ == 1 && y_ == 1 && z_ == 1)
        {
            return "point";
        }
        else
        {
            std::ostringstream ss;
            ss << "rect: " << x_ << "/" << y_ << "/" << z_ << std::endl;
            return ss.str();
        }
    }
private:
    int x_, y_, z_;
    int cx_, cy_, cz_;
};

/**
 * stupid block brush, legacy. use when you want to apply something to a whole DF map block.
 */
class BlockBrush : public Brush
{
public:
    BlockBrush(){};
    ~BlockBrush(){};
    coord_vec points(MapExtras::MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        DFHack::DFCoord blockc = start / 16;
        DFHack::DFCoord iterc = blockc * 16;
        if( !mc.testCoord(start) )
            return v;
        auto starty = iterc.y;
        for(int xi = 0; xi < 16; xi++)
        {
            for(int yi = 0; yi < 16; yi++)
            {
                v.push_back(iterc);
                iterc.y++;
            }
            iterc.y = starty;
            iterc.x ++;
        }
        return v;
    };
    std::string str() const {
        return "block";
    }
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
    coord_vec points(MapExtras::MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        bool juststarted = true;
        while (mc.testCoord(start))
        {
            df::tiletype tt = mc.tiletypeAt(start);
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
    std::string str() const {
        return "column";
    }
};

/**
 * Flood-fill water tiles from cursor (for wclean)
 * example: remove salt flag from a river
 */
class FloodBrush : public Brush
{
public:
    FloodBrush(DFHack::Core *c){c_ = c;};
    ~FloodBrush(){};
    coord_vec points(MapExtras::MapCache & mc, DFHack::DFCoord start)
    {
        using namespace DFHack;
        coord_vec v;

        std::stack<DFCoord> to_flood;
        to_flood.push(start);

        std::set<DFCoord> seen;

        while (!to_flood.empty()) {
            DFCoord xy = to_flood.top();
            to_flood.pop();

                        df::tile_designation des = mc.designationAt(xy);

            if (seen.find(xy) == seen.end()
                            && des.bits.flow_size
                            && des.bits.liquid_type == tile_liquid::Water) {
                v.push_back(xy);
                seen.insert(xy);

                maybeFlood(DFCoord(xy.x - 1, xy.y, xy.z), to_flood, mc);
                maybeFlood(DFCoord(xy.x + 1, xy.y, xy.z), to_flood, mc);
                maybeFlood(DFCoord(xy.x, xy.y - 1, xy.z), to_flood, mc);
                maybeFlood(DFCoord(xy.x, xy.y + 1, xy.z), to_flood, mc);

                df::tiletype tt = mc.tiletypeAt(xy);
                if (LowPassable(tt))
                {
                    maybeFlood(DFCoord(xy.x, xy.y, xy.z - 1), to_flood, mc);
                }
                if (HighPassable(tt))
                {
                    maybeFlood(DFCoord(xy.x, xy.y, xy.z + 1), to_flood, mc);
                }
            }
        }

        return v;
    }
    std::string str() const {
        return "flood";
    }
private:
    void maybeFlood(DFHack::DFCoord c, std::stack<DFHack::DFCoord> &to_flood,
                    MapExtras::MapCache &mc) {
        if (mc.testCoord(c)) {
            to_flood.push(c);
        }
    }
    DFHack::Core *c_;
};

DFHack::command_result parseRectangle(DFHack::color_ostream & out,
                              vector<string>  & input, int start, int end,
                              int & width, int & height, int & zLevels,
                              bool hasConsole = true)
{
    using namespace DFHack;
    int newWidth = 0, newHeight = 0, newZLevels = 0;

    if (end > start + 1)
    {
        newWidth = atoi(input[start++].c_str());
        newHeight = atoi(input[start++].c_str());
        if (end > start) {
            newZLevels = atoi(input[start++].c_str());
        } else {
            newZLevels = 1; // So 'range w h' won't ask for it.
        }
    }

    string command = "";
    std::stringstream str;
    CommandHistory hist;

    if (newWidth < 1) {
        if (hasConsole) {
            Console &con = static_cast<Console&>(out);

            str.str("");
            str << "Set range width <" << width << "> ";
            con.lineedit(str.str(), command, hist);
            hist.add(command);
            newWidth = command.empty() ? width : atoi(command.c_str());
        } else {
            return CR_WRONG_USAGE;
        }
    }

    if (newHeight < 1) {
        if (hasConsole) {
            Console &con = static_cast<Console&>(out);

            str.str("");
            str << "Set range height <" << height << "> ";
            con.lineedit(str.str(), command, hist);
            hist.add(command);
            newHeight = command.empty() ? height : atoi(command.c_str());
        } else {
            return CR_WRONG_USAGE;
        }
    }

    if (newZLevels < 1) {
        if (hasConsole) {
            Console &con = static_cast<Console&>(out);

            str.str("");
            str << "Set range z-levels <" << zLevels << "> ";
            con.lineedit(str.str(), command, hist);
            hist.add(command);
            newZLevels = command.empty() ? zLevels : atoi(command.c_str());
        } else {
            return CR_WRONG_USAGE;
        }
    }

    width = newWidth < 1? 1 : newWidth;
    height = newHeight < 1? 1 : newHeight;
    zLevels = newZLevels < 1? 1 : newZLevels;

    return CR_OK;
}

inline std::ostream &operator<<(std::ostream &stream, const Brush& brush)
{
    stream << brush.str();
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const Brush* brush)
{
    stream << brush->str();
    return stream;
}
