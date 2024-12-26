#include "Console.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "MiscUtils.h"
#include <mutex>
#include <string>
#include <vector>
#include <sstream>

#include "memutils.h"

using std::vector;
using std::string;
using namespace DFHack;

uint64_t timeLast=0;
static std::mutex* mymutex=0;

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

struct memory_data
{
    void * addr;
    size_t len;
    size_t refresh;
    int state;
    uint8_t *buf,*lbuf;
    vector<t_memrange> ranges;
}memdata;
enum HEXVIEW_STATES
{
    STATE_OFF,STATE_ON
};
command_result memview (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("memview");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("memview","Shows DF memory in real time.",memview,false,"Shows memory in real time.\nParams: adrr length refresh_rate. If addr==0 then stop viewing."));
    memdata.state=STATE_OFF;
    mymutex=new std::mutex;
    return CR_OK;
}
size_t convert(const std::string& p,bool ishex=false)
{
    size_t ret;
    std::stringstream conv;
    if(ishex)
        conv<<std::hex;
    conv<<p;
    conv>>ret;
    return ret;
}
bool isAddr(void *trg, vector<t_memrange> &ranges)
{
    for (auto &r : ranges)
        if (r.isInRange(trg))
            return true;
    return false;
}
bool isAddrAt(uintptr_t *trg, vector<t_memrange> &ranges)
{
    if(trg[0]%4==0)
        for(size_t i=0;i<ranges.size();i++)
            if(ranges[i].isInRange((void *)trg[0]))
                return true;

    return false;
}
void outputHex(uint8_t *buf,uint8_t *lbuf,size_t len,size_t start,color_ostream &con,vector<t_memrange> & ranges)
{
    const size_t page_size=16;

    for(size_t i=0;i<len;i+=page_size)
    {
        //con.gotoxy(1,i/page_size+1);
        con.print("0x%08zX ",i+start);
        for(size_t j=0;(j<page_size) && (i+j<len);j++)
            {
                if(j%sizeof(void*)==0)
                {
                    con.reset_color();

                    if(isAddrAt((uintptr_t *)(buf+j+i),ranges))
                        con.color(COLOR_LIGHTRED); //coloring in the middle does not work
                    //TODO make something better?
                }
                if(lbuf[j+i]!=buf[j+i])
                    con.print("*%02X",buf[j+i]); //if modfied show a star
                else
                    con.print(" %02X",buf[j+i]);
            }
        con.reset_color();
        con.print(" | ");
        for(size_t j=0;(j<page_size) && (i+j<len);j++)
            if((buf[j+i]>31)&&(buf[j+i]<128)) //only printable ascii
                con.print("%c",buf[j+i]);
            else
                con.print(".");
        con.print("\n");
    }
    con.print("\n");
}
void Deinit()
{
    if(memdata.state==STATE_ON)
    {
        is_enabled = false;
        memdata.state=STATE_OFF;
        delete [] memdata.buf;
        delete [] memdata.lbuf;
    }
}

size_t detect_size(void *addr) {
    size_t *size = (size_t*)((char*)addr - 16);
    uint32_t *tag = (uint32_t*)((char*)addr - 8);
    if (isAddr(size, memdata.ranges) && (*tag == 0x11223344 || *tag == 0xdfdf4ac8)) {
        return *size;
    }
    // default
    return 20 * 16;
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{

    mymutex->lock();
    if(memdata.state==STATE_OFF)
    {
        mymutex->unlock();
        return CR_OK;
    }
    //Console &con=out;
    uint64_t time2 = GetTimeMs64();
    uint64_t delta = time2-timeLast;

    if(memdata.refresh!=0)
    if(delta<memdata.refresh)
    {
        mymutex->unlock();
        return CR_OK;
    }
    timeLast = time2;

    Core::getInstance().p->read(memdata.addr,memdata.len,memdata.buf);
    outputHex(memdata.buf,memdata.lbuf,memdata.len,(size_t)memdata.addr,out,memdata.ranges);
    memcpy(memdata.lbuf, memdata.buf, memdata.len);
    if(memdata.refresh==0)
        Deinit();
    mymutex->unlock();
    return CR_OK;

}
command_result memview (color_ostream &out, vector <string> & parameters)
{
    mymutex->lock();
    Core::getInstance().p->getMemRanges(memdata.ranges);
    if (parameters.empty())
    {
        memdata.addr = 0;
    }
    else if (toLower_cp437(parameters[0].substr(0, 2)) == "0x")
    {
        memdata.addr = (void *)convert(parameters[0],true);
    }
    else
    {
        memdata.addr = memutils::lua_expr_to_addr(parameters[0].c_str());
    }

    if(memdata.addr==0)
    {
        Deinit();
        memdata.state=STATE_OFF;
        is_enabled = false;
        mymutex->unlock();
        return CR_OK;
    }
    else
    {
        Deinit();
        bool isValid=false;
        for(size_t i=0;i<memdata.ranges.size();i++)
            if(memdata.ranges[i].isInRange(memdata.addr))
                isValid=true;
        if(!isValid)
        {
            out.printerr("Invalid address: %p\n",memdata.addr);
            mymutex->unlock();
            return CR_OK;
        }
        is_enabled = true;
        memdata.state=STATE_ON;
    }
    if (vector_get(parameters, 1, string("a")).substr(0, 1) == "a")
        memdata.len = detect_size(memdata.addr);
    else if (parameters.size()>1)
        memdata.len=convert(parameters[1]);
    else
        memdata.len=20*16;

    if(parameters.size()>2)
        memdata.refresh=convert(parameters[2]);
    else
        memdata.refresh=0;

    memdata.buf=new uint8_t[memdata.len];
    memdata.lbuf=new uint8_t[memdata.len];
    Core::getInstance().p->getMemRanges(memdata.ranges);
    mymutex->unlock();
    return CR_OK;
}
DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    mymutex->lock();
    Deinit();
    mymutex->unlock();
    delete mymutex;
    mymutex = nullptr;
    return CR_OK;
}
