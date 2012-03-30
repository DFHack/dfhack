/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Internal.h"
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
using namespace std;

#include <md5wrapper.h>
#include "MemAccess.h"
#include "VersionInfoFactory.h"
#include "VersionInfo.h"
#include "Error.h"
#include <string.h>
using namespace DFHack;

Process::Process(VersionInfoFactory * known_versions)
{
    const char * dir_name = "/proc/self/";
    const char * exe_link_name = "/proc/self/exe";
    const char * cwd_name = "/proc/self/cwd";
    const char * cmdline_name = "/proc/self/cmdline";
    int target_result;

    identified = false;
    my_descriptor = 0;

    md5wrapper md5;
    uint32_t length;
    uint8_t first_kb [1024];
    memset(first_kb, 0, sizeof(first_kb));
    // get hash of the running DF process
    string hash = md5.getHashFromFile(exe_link_name, length, (char *) first_kb);
    // create linux process, add it to the vector
    VersionInfo * vinfo = known_versions->getVersionInfoByMD5(hash);
    if(vinfo)
    {
        my_descriptor = new VersionInfo(*vinfo);
        identified = true;
    }
    else
    {
        char * wd = getcwd(NULL, 0);
        cerr << "Unable to retrieve version information.\n";
        cerr << "File: " << exe_link_name << endl;
        cerr << "MD5: " << hash << endl;
        cerr << "working dir: " << wd << endl;
        cerr << "length:" << length << endl;
        cerr << "1KB hexdump follows:" << endl;
        for(int i = 0; i < 64; i++)
        {
            fprintf(stderr, "%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
                    first_kb[i*16],
                    first_kb[i*16+1],
                    first_kb[i*16+2],
                    first_kb[i*16+3],
                    first_kb[i*16+4],
                    first_kb[i*16+5],
                    first_kb[i*16+6],
                    first_kb[i*16+7],
                    first_kb[i*16+8],
                    first_kb[i*16+9],
                    first_kb[i*16+10],
                    first_kb[i*16+11],
                    first_kb[i*16+12],
                    first_kb[i*16+13],
                    first_kb[i*16+14],
                    first_kb[i*16+15]
                   );
        }
        free(wd);
    }
}

Process::~Process()
{
    // destroy our copy of the memory descriptor
    delete my_descriptor;
}

string Process::doReadClassName (void * vptr)
{
    //FIXME: BAD!!!!!
    char * typeinfo = Process::readPtr(((char *)vptr - 0x4));
    char * typestring = Process::readPtr(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start);
}

//FIXME: cross-reference with ELF segment entries?
void Process::getMemRanges( vector<t_memrange> & ranges )
{
    char buffer[1024];
    char permissions[5]; // r/-, w/-, x/-, p/s, 0

    FILE *mapFile = ::fopen("/proc/self/maps", "r");
    size_t start, end, offset, device1, device2, node;

    while (fgets(buffer, 1024, mapFile))
    {
        t_memrange temp;
        temp.name[0] = 0;
        sscanf(buffer, "%zx-%zx %s %zx %2zx:%2zx %zu %[^\n]",
               &start,
               &end,
               (char*)&permissions,
               &offset, &device1, &device2, &node,
               (char*)temp.name);
        temp.start = (void *) start;
        temp.end = (void *) end;
        temp.read = permissions[0] == 'r';
        temp.write = permissions[1] == 'w';
        temp.execute = permissions[2] == 'x';
        temp.shared = permissions[3] == 's';
        temp.valid = true;
        ranges.push_back(temp);
    }
}

uint32_t Process::getBase()
{
    return 0;
}

static int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
    files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

bool Process::getThreadIDs(vector<uint32_t> & threads )
{
    stringstream ss;
    vector<string> subdirs;
    if(getdir("/proc/self/task/",subdirs) != 0)
    {
        //FIXME: needs exceptions. this is a fatal error
        cerr << "unable to enumerate threads. This is BAD!" << endl;
        return false;
    }
    threads.clear();
    for(size_t i = 0; i < subdirs.size();i++)
    {
        uint32_t tid;
        if(sscanf(subdirs[i].c_str(),"%d", &tid))
        {
            threads.push_back(tid);
        }
    }
    return true;
}

string Process::getPath()
{
    const char * cwd_name = "/proc/self/cwd";
    char target_name[1024];
    int target_result;
    target_result = readlink(cwd_name, target_name, sizeof(target_name));
    target_name[target_result] = '\0';
    return(string(target_name));
}

int Process::getPID()
{
    return getpid();
}

bool Process::setPermisions(const t_memrange & range,const t_memrange &trgrange)
{
    int result;
    int protect=0;
    if(trgrange.read)protect|=PROT_READ;
    if(trgrange.write)protect|=PROT_WRITE;
    if(trgrange.execute)protect|=PROT_EXEC;
    result=mprotect((void *)range.start, (size_t)range.end-(size_t)range.start,protect);

    return result==0;
}