## Cavern Keeper 

Here is an extended dfhack labor manager plugin.

### Installing

It is installed by saving one plugin file into dfhacks plugins directory.
It will overwrite the "manipulator" plugin (dfhack labor manager) that lives in there already. Cavern Keeper is a much extended version of the old plugin.

Compiles for starter packs are located in the following directories:
* Windows64 Peridix's starter Pack ver. 0.44.05-r03 (pack is [here](http://dffd.bay12games.com/file.php?id=7622))
  * [`./win_peridex_44r3/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/win_peridex_44r3)
* Linux Newbie Pack ver. 44.05.r02 (pack [here](http://dffd.bay12games.com/file.php?id=13244))
  * [`./linux_lnp_44r2/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/linux_lnp_44r2)

Compiles for slightly older packs:
* Windows64 Peridix's starter Pack ver. 0.44.05-r02 (has dfhack-r1)
  * [`./win_peridex_44r2/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/win_peridex_44r2)
* Linux Newbie Pack ver. 44.05.r01 
  * [`./linux_lnp_44r1/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/linux_lnp_44r1)
* Linux Newbie Pack ver. 44.05.rc1 (older)
  * [`./linux_lnp_44rc1/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/linux_lnp_44rc1)

The plugin file is called `manipulator.plug.so` for linux and `manipulator.plug.dll` for windows. The file goes into the directory `'../df_linux/hack/plugins'` or `'..\dwarf fortress\hack\plugins'` on windows.


### Running

To open Cavern Keeper in fortress mode press 'u' (for unitlist) and then 'k' (for 'Keeper'). It can also be opened on the listing screens for pets and visitors and the dead.

It behaves similar to the default labor manager. Turn on 
Cavern Keepers extended abilities by pressing the keys
displayed in the footer next to 'Mode'.

[Here is bay12forum](http://www.bay12forums.com/smf/index.php?topic=169329.msg7678623#msg7678623) thread on the plugin. Feedback there is appreciated.


### Extra configs:

##### Alternative keeper pallete:

With the windows plugin, the file [`dfkeeper_pallete.txt`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/Win64Perix4405rc2/dfkeeper_pallete.txt) can be used to alter keepers theme to be more readable with the default windows pack font which seems to render background colors much more vividly than default on Linux.
It can be saved into the directory `..\[dfhome]\dfkeeper\` and can be edited with a text editor for further tweaking.
 
##### Text-color tweaked game pallete:

[`colors.txt`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/colors.txt) is the default LNP pallet tweaked to
balance text readablity and graphics look.
It can be saved on top of `..\[dfhome]\data\init\colors.txt`

##### Keypress Timings:

DF's default keypress timings are sluggish on my own system
so here are my tweaked keyboard speed settings, they go
near the bottom of `~lnp\df_linux\data\init\init.txt`

```
[KEY_HOLD_MS:250]
[KEY_REPEAT_MS:133]
[KEY_REPEAT_ACCEL_LIMIT:20]
[KEY_REPEAT_ACCEL_START:2]
```

Also the game Zoom setting works nicely with much smaller increments than default setting of 10 ( lets you find better pixel ratios too ):
```
[ZOOM_SPEED:3]
```

## Development:

My very ad-hoc development commits are in [this branch](https://github.com/strainer/dfhack/commits/manipu_remix).

Manipulator.cpp has seen almost 10 years of open development and was a little large and disorganised (already) when I found it. It is now three times the size and contains some rather unweildy code, so it probably won't make it back into DFHack without skilled work to make it publicly maintainable. If anyone would like to help with that let me know - otherwise, the plugin is very efficient, UI is polished and I have put all the features in it that I wished for, so I may just update to keep it working with the latest major DF releases.