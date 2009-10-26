/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#ifndef DFCOMMON_H_INCLUDED
#define DFCOMMON_H_INCLUDED

///TODO: separate into extrenal and internal

#include <string>
#include <vector>
#include <map>
//#include <boost/bimap/bimap.hpp>
//using namespace boost::bimaps;

#include <fstream>
using namespace std;
#include "integers.h"
#include <assert.h>
#include <string.h>

#ifdef LINUX_BUILD
#include <sys/types.h>
#include <sys/ptrace.h>
#include <dirent.h>
#define __USE_FILE_OFFSET64
#define _FILE_OFFSET_BITS 64
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#else
#define WINVER 0x0500					// OpenThread(), PSAPI, Toolhelp32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#include <winnt.h>
#include <psapi.h>
#endif

#include "DFTypes.h"
#include "DFDataModel.h"
#include "DFProcessManager.h"
#include "DFMemAccess.h"
#include "DFVector.h"
//#include "DfMap.h"


#endif // DFCOMMON_H_INCLUDED
