Introduction
------------

DFHack is a Dwarf Fortress memory access library and a set of basic tools using 
this library. The library is a work in progress, so things might change as more
tools are written for it.

It is an attempt to unite the various ways tools access DF memory and allow for
easier development of new tools.


Getting DFHack
----------------
The project is currently hosted on github:
  http://github.com/peterix/dfhack

There's an SVN repository at sourceforge, but will only be updated for major releases:
  https://sourceforge.net/projects/dfhack/
* subversion access: 
  svn co https://dfhack.svn.sourceforge.net/svnroot/dfhack/trunk dfhack 

Compatibility
-------------

DFHack works on Windows XP, Vista, 7 or any modern Linux distribution.

Windows 2000 is currently *not supported* due to missing OS functionality.
If you know how to easily suspend processes, you can fix it :)

OSX is also not supported due to lack of developers with a Mac.

Currently supported Dwarf Fortress versions:
* Windows
  40d
  40d9 - 40d18

* Linux
  40d9 - 40d18


Using the library
-----------------

The library is compilable under Linux with GCC and under Windows with MinGW32
and MSVC compilers. It is using the cmake build system. See COMPILE for details.

DFHack is using the zlib/libpng license. This makes it easy to link to it, use
it in-source or add your own extensions. Contributing back to the dfhack
repository is welcome and the right thing to do :)

At the time of writing there's no API reference or documentation. The code does
have a lot of comments though.


Using DFHack Tools
------------------

The project comes with a special extra library you should add to your DF
installation. It's used to boost the transfer speed between DF and DFHack, and
provide data consistency and synchronization. DFHack will work without the
library, but at suboptimal speeds and the consistency of data written back
to DF is questionable.

!!!     on Windows this currently only works with DF 40d15 - 40d18      !!!
     On Linux, it works with the whole range of supported DF versions.

!!! use the pre-compiled library intended for your OS and version of DF !!!
               You can find them in the 'precompiled' folder.
               

 ** Installing on Windows
 - Open your DF folder, locate SDL.dll and rename it to SDLreal.dll (making
   a backup of it is a good idea)
 - Copy the right DFHack SDL.dll into your DF folder.
 - Restart DF if it is running

 ** Unistalling on Windows
 - Open your DF folder, locate SDL.dll and delete it
 - Rename SDLreal.dll to SDL.dll
 - Restart DF if it is running


 ** Installing on Linux
 - Open your DF folder and the libs folder within it
 - copy DFHack libdfconnect.so to the libs folder
 - copy the df startup script, name it dfhacked
 - open the new dfhacked startup script and add this line:
     export LD_PRELOAD="./libs/libdfconnect.so" # Hack DF!
   just before the line that launches DF

   Here's an example how the file can look after the change:
#!/bin/sh
DF_DIR=$(dirname "$0")
cd "${DF_DIR}"
export SDL_DISABLE_LOCK_KEYS=1 # Work around for bug in Debian/Ubuntu SDL patch.
#export SDL_VIDEO_CENTERED=1 # Centre the screen.  Messes up resizing.
ldd dwarfort.exe | grep SDL_image | grep -qv "not found$"
if [ $? -eq 0 ]; then
 mkdir unused_libs
 mv libs/libSDL* unused_libs/
fi
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"./libs" # Update library search path.
export LD_PRELOAD="./libs/libdfconnect.so" # Hack DF!
./dwarfort.exe $* # Go, go, go! :)

 - Use this new startup script to start DF

 ** Uninstalling on Linux
 - Open your DF and DF/libs folders
 - Delete libdfconnect.so and the dfhacked startup script
 - Go back to using the df startup script


Tools
-----

All the DFHack tools are terminal programs. This might seem strange to Windows
users, but these are meant mostly as examples for developers. Still, they can
be useful and are cross-platform just like the library itself.

If the tool writes back to DF's memory, make sure you are using the shared
memory interface mentioned in the previous section!

* reveal     - plain old reveal tool. It reveals all the map blocks already
               initialized by DF.

* prospector - scans the map for minerals. by default it only scans only visible
               veins. You can make it show hidden things with '-a' and base rock
               and soil layers with '-b'. These can be combined ('-ab')

* cleanmap   - cleans mud, vomit, snow and all kinds of mess from the map.
               It will clean your irrigated farms too, so consider yourself
               warned.

* incremental - incremental search utility.

* bauxite - converts all mechanisms into bauxite mechanisms.

* itemdesignator - Allows mass-designating items by type and material - dump,
                   forbid, melt and set on fire ;)
* digger         - allows designating tiles for digging/cutting/ramp removal

  A list of accepted tile classes:
        1  = WALL
        2  = PILLAR
        3  = FORTIFICATION
        
        4  = STAIR_UP
        5  = STAIR_DOWN
        6  = STAIR_UPDOWN
        
        7  = RAMP
        
        8  = FLOOR
        9  = TREE_DEAD
        10 = TREE_OK
        11 = SAPLING_DEAD
        12 = SAPLING_OK
        13 = SHRUB_DEAD
        14 = SHRUB_OK
        15 = BOULDER
        16 = PEBBLES
  
  Example : dfdigger -o 100,100,15 -t 9,10 -m 10
  This will start looking for trees at coords 100,100,15 and designate ten of them for cutting.

 
Memory offset definitions
-------------------------

The file with memory offset definitions used by dfhack can be found in the
output folder. 

~ EOF ~
