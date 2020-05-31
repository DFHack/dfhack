## Kloker (fka Cavern Keeper) 

This is a greatly extended dfhack labor manager plugin.

<img src="https://i.imgur.com/5g0Z34C.png" alt="screenshot1" width="800">
<img src="https://i.imgur.com/Gq3MicB.png" alt="screenshot2" width="600">

It includes these improvements:

* Units characters are summarized in great detail as they are focused in listing.
* Sorting of unit list is more powerful and simple to control.
* Units physical and mental scores, and units conditions (pregnant, noShoes, needs doctor, wounded etc) can be easily browsed.
* DFhack autolabor is integrated, enabling easy assignment of outstanding tasks to selected units.
* Colorful highlighting indicates which jobs and roles each units are especially good or bad at (over and above their plain skill level which is also displayed). 
* A fair cheat feature is included so dwarfs skills can be revised immediately after embark, since they are much easier to examine in kloker than in the embark preparation screens.
* Kloker can also open and display the intresting details of pets, visitors and others, monsters etc. and can also nickname pets and visitors.
* Kloker's UI has multiple refinements over the basic labor manager to make it easier to use often, such as persistent focus and selection, and jumping straight to units from the game map.

### Installing

It is installed by saving one plugin file into dfhacks `/plugins` directory.
It will overwrite the "manipulator" plugin (dfhack labor manager) that lives in there already. Kloker is a much extended version of the old plugin.
It won't work with dfhack versions which it was not compiled specifically for.

Only these latest compiles are feature complete:

#### Compiles for starter packs are located in the following directories:  

Linux DFHack 47.03 beta 1
* [`/linux_dfhack_47.3b1/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df47/linux_dfhack_47.3b1/)

Win64 DFHack 47.04 rel 1
* [`/win64_dfhack_47.04r1/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df47/win64_dfhack_47.04r1/)

#### Also:

Linux Newbie Pack ver. 44.12 r03 (pack [here](http://dffd.bay12games.com/file.php?id=13244))
* [`/linux_lnp_44.12-r03/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/linux_lnp_44.12-r03/)

Linux Newbie Pack ver. 44.12 r1
* [`/linux_dfhack_44.12-r2/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/linux_dfhack_44.12-r2)

Linux Newbie Pack ver. 44.10
* [`/linux_dfhack_44.10/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/linux_dfhack_44_10)

Windows64 Peridix's starter Pack ver. 0.44.12-r04 
* [`/win_peridex_44.12-r1/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/win_peridex_44.12-r1)

Windows64 Peridix's starter Pack ver. 0.44.12-r05 (pack is [here](http://dffd.bay12games.com/file.php?id=7622))
* [`/win_dfhack_44.12-R2/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/win_dfhack_44.12-R2)

For MacOS DFHack ver 44.12-r2
* [`/macos_dfhack_44.12-r2/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/macos_dfhack_44.12-r2)

The plugin file is called `"manipulator.plug.so"` for linux, `"manipulator.plug.dll"` for windows and `"manipulator.plug.dylib"` for MacOS. The file goes into the directory `'../(df_directory)/hack/plugins'` or `'..\dwarf fortress\hack\plugins'` on windows.


### Running

To open Kloker in fortress mode press 'u' (for unitlist) and then 'k' for kloker. It can also be opened on the listing screens for pets and visitors and the dead. It can also open on the map 

It behaves similar to the legacy dfhack labor manager. Adjust 
Klokers extended abilities by pressing the keys displayed in the menu next to 'Mode'. A tooltip appears to help understand setting changes. 

Here is a [intro and discussion](http://www.bay12forums.com/smf/index.php?topic=169329.msg7678623#msg7678623) on the plugin at bay12forum. Feedback there is appreciated.


### Extra configs:

##### Alternative keeper color pallete:

The file [`dfkeeper_pallete.txt`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/dfkeeper_pallete.txt) can be used to customise the theme colors.
It can be saved into the directory `..\[dfhome]\kloker\` and can be edited with a text editor for further tweaking.
 
##### Game's color pallete tweaked for nice text as well as graphics:

[`colors.txt`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/colors.txt) is the default LNP pallet tweaked to balance text readablity and graphics look.
It can be saved on top of `..\[dfhome]\data\init\colors.txt`

##### Keypress Timings:

DF's default keypress timings seem really sluggish to me.
Here are my tweaked keyboard speed settings, they go
near the bottom of `~lnp\df_linux\data\init\init.txt`


```
[KEY_HOLD_MS:250]
[KEY_REPEAT_MS:133]
[KEY_REPEAT_ACCEL_LIMIT:20]
[KEY_REPEAT_ACCEL_START:2]
```

Also the game Zoom setting works nicely with much smaller increments than default setting of 10 ( allows finding better pixel ratios too )
```
[ZOOM_SPEED:3]
```

## Development:

This repository was forked from the main dfhack but I've deleted everything except manipulator.cpp. To make a compile it is necessary to place it into a full dfhack repo and follow DFhack's excellent [compiling instructions](https://docs.dfhack.org/en/stable/docs/Compile.html).

The latest sourecode file is in `/plugins` directory. It has a complete but messy commit history since I worked in and merged a few branches and am learning how to do that better :)

Hopefuly this can go back to dfhack, anytime. It seems quite well tested and stable.
