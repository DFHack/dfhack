#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/PluginManager.h>
#include <dfhack/Process.h>
#include <dfhack/extra/stopwatch.h>
#include <string>
#include <vector>
#include <sstream>

using std::vector;
using std::string;
using namespace DFHack;

DFhackCExport command_result memview (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "memview";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
	commands.push_back(PluginCommand("memview","Shows memory in real time. Params: adrr length refresh_rate.",memview));
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
bool isAddr(uint32_t *trg,vector<t_memrange> & ranges)
{
	if(trg[0]%4==0)
		for(size_t i=0;i<ranges.size();i++)
			if(ranges[i].isInRange(trg[0]))
				return true;

	return false;
}
void outputHex(uint8_t *buf,uint8_t *lbuf,size_t len,size_t start,Console &con,vector<t_memrange> & ranges)
{	
	
	con.clear();
	const size_t page_size=16;
	for(size_t i=0;i<len;i+=page_size)
	{
		con.print("%8x  ",i+start);
		for(size_t j=0;(j<page_size) && (i+j<len);j++)
			{
				if(j%4==0)
				{
					con.reset_color();
					if(isAddr((uint32_t *)(buf+j+i),ranges))
						con.color(Console::COLOR_LIGHTRED);
				}
				if(lbuf[j+i]!=buf[j+i])
					con.print("*%2x",buf[j+i]);
				else
					con.print(" %2x",buf[j+i]);
			}
		con.reset_color();
		con.print(" | ");
		for(size_t j=0;(j<page_size) && (i+j<len);j++)
			if(buf[j+i]>20)
				con.print("%c",buf[j+i]);
			else
				con.print(".");
		con.print("\n");
	}
}
DFhackCExport command_result memview (Core * c, vector <string> & parameters)
{
	size_t addr=convert(parameters[0],true);
	size_t len;
	if(parameters.size()>1)
		len=convert(parameters[1]);
	else
		len=20*16;
	size_t refresh;
	if(parameters.size()>2)
		refresh=convert(parameters[2]);
	else
		refresh=0;
	Console &con=c->con;
	uint8_t *buf,*lbuf;
	buf=new uint8_t[len];
	lbuf=new uint8_t[len];
	uint64_t timeLast=0;
	vector<t_memrange> ranges;
	c->p->getMemRanges(ranges);
	while(true)//TODO add some sort of way to exit loop??!!
	{
		uint64_t time2 = GetTimeMs64();
		uint64_t delta = time2-timeLast;
		if(refresh!=0)
		if(delta<refresh)
			continue;
		timeLast = time2;

		c->p->read(addr,len,buf);
		outputHex(buf,lbuf,len,addr,con,ranges);
		if(refresh==0)
			break;
        memcpy(lbuf, buf, len);
	}
	delete[] buf;
	delete[] lbuf;
}
