#include <vector>
#include <string>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include <VTableInterpose.h>
#include "df/renderer.h"
#include "df/enabler.h"

#include "renderer_opengl.hpp"

using namespace DFHack;
using std::vector;
using std::string;
enum RENDERER_MODE
{
    MODE_DEFAULT,MODE_TRIPPY,MODE_TRUECOLOR
};
RENDERER_MODE current_mode=MODE_DEFAULT;
static command_result rendermax(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("rendermax");


DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "rendermax", "switch rendering engine.", rendermax, false,
        "  rendermax trippy\n"
        "  rendermax truecolor red|green|blue|white\n"
        "  rendermax disable\n"
        ));
    return CR_OK;
}
void removeOld()
{
    if(current_mode!=MODE_DEFAULT)
        delete df::global::enabler->renderer;
    current_mode=MODE_DEFAULT;
}
void installNew(df::renderer* r,RENDERER_MODE newMode)
{
    df::global::enabler->renderer=r;
    current_mode=newMode;
}
static command_result rendermax(color_ostream &out, vector <string> & parameters)
{
    if(parameters.size()==0)
        return CR_WRONG_USAGE;
    string cmd=parameters[0];
    if(cmd=="trippy")
    {
        removeOld();
        installNew(new renderer_trippy(df::global::enabler->renderer),MODE_TRIPPY);
        return CR_OK;
    }
    else if(cmd=="truecolor")
    {
        if(current_mode!=MODE_TRUECOLOR)
        {
            removeOld();
            installNew(new renderer_test(df::global::enabler->renderer),MODE_TRUECOLOR);
        }
        if(current_mode==MODE_TRUECOLOR && parameters.size()==2)
        {
            lightCell red(1,0,0),green(0,1,0),blue(0,0,1),white(1,1,1);
            lightCell cur=white;
            lightCell dim(0.2,0.2,0.2);
            string col=parameters[1];
            if(col=="red")
                cur=red;
            else if(col=="green")
                cur=green;
            else if(col=="blue")
                cur=blue;

            renderer_test* r=reinterpret_cast<renderer_test*>(df::global::enabler->renderer);
            tthread::lock_guard<tthread::fast_mutex> guard(r->dataMutex);
            int h=df::global::gps->dimy;
            int w=df::global::gps->dimx;
            int cx=w/2;
            int cy=h/2;
            int rad=cx;
            if(rad>cy)rad=cy;
            rad/=2;
            int radsq=rad*rad;
            for(size_t i=0;i<r->lightGrid.size();i++)
            {
                r->lightGrid[i]=dim;
            }
            for(int i=-rad;i<rad;i++)
            for(int j=-rad;j<rad;j++)
            {
                if((i*i+j*j)<radsq)
                {
                    float val=(radsq-i*i-j*j)/(float)radsq;
                    r->lightGrid[(cx+i)*h+(cy+j)]=dim+cur*val;
                }
            }
            return CR_OK;
        }
       
        
    }
    else if(cmd=="disable")
    {
        if(current_mode==MODE_DEFAULT)
            out.print("%s\n","Not installed, doing nothing.");
        else
            removeOld();
        
        return CR_OK;
    }
    return CR_WRONG_USAGE;
}