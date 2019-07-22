## Cavern Keeper 

This is an extended dfhack labor manager plugin with these improvements:

* Units characters are summarized in great detail and conviently as they are focused.
* Sorting of list of units is more powerful and simple to control.
* Two extra column modes are included to reveal units physical and mental scores, and units conditions (pregnant, noShoes, needs doctor, wounded etc)
* Colorful highlighting can be enabled to indicate which jobs and roles each units stats make them especially good or bad at (over and above their plain skill level which is displayed). 
* A fair cheat feature is included so dwarfs skills can be revised immediately after embark since they are much easier to examine in keeper than in the embark preparation screens.
* Keeper can also open and display the intresting details of the listings of pets, visitors and others, monsters etc. and can also nickname pets and visitors.
* Keeper's UI has multiple refinements over the basic labor manager to make it easier to use often, such as persistent focus and selection.

### Installing

It is installed by saving one plugin file into dfhacks `/plugins` directory.
It will overwrite the "manipulator" plugin (dfhack labor manager) that lives in there already. Cavern Keeper is a much extended version of the old plugin.
It won't work with dfhack versions which it was not compiled specifically for.
Only these latest compiles are stable and feature complete.

#### Compiles for starter packs are located in the following directories:  

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

#### And finally:

For MacOS DFHack ver 44.12-r2
* [`/macos_dfhack_44.12-r2/`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/macos_dfhack_44.12-r2)

The plugin file is called `"manipulator.plug.so"` for linux, `"manipulator.plug.dll"` for windows and `"manipulator.plug.dylib"` for MacOS. The file goes into the directory `'../(df_directory)/hack/plugins'` or `'..\dwarf fortress\hack\plugins'` on windows.


### Running

To open Cavern Keeper in fortress mode press 'u' (for unitlist) and then 'k' for keeper. It can also be opened on the listing screens for pets and visitors and the dead.

It behaves similar to the legacy dfhack labor manager. Toggle 
Cavern Keepers extended abilities by pressing the keys
displayed in the footer next to 'Mode'.

Here is a [guide and discussion](http://www.bay12forums.com/smf/index.php?topic=169329.msg7678623#msg7678623) on the plugin at bay12forum. Feedback there is appreciated.


### Extra configs:

##### Alternative keeper pallete:

The file [`dfkeeper_pallete.txt`](https://github.com/strainer/dfhack/tree/master/build/compile_archive_df44/dfkeeper_pallete.txt) can be used to customise the theme colors.
It can be saved into the directory `..\[dfhome]\dfkeeper\` and can be edited with a text editor for further tweaking.
 
##### Game pallete tweaked for nice text as well as graphics:

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

This repository was forked from the main dfhack but I've deleted everything except manipulator.cpp so to make a compile it is necessary to place it into a full dfhack repo.

The latest sourecode file is in `/plugins` directory. It has a complete but messy commit history since I worked in and merged a few branches and am learning how to do that better :)

Hopefuly this can go back to dfhack, anytime. It seems quite well tested and stable.