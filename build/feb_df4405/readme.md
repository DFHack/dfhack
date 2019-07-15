## Cavern Keeper 

Here is the extended dfhack labor manager plugin.

### Installing

It is installed by saving one plugin file into dfhacks plugins directory.
It will overwrite the "manipulator" plugin (dfhack labor manager) that lives in there already. Cavern Keeper is a much extended version of the old plugin.

#### Compiles for starter packs are located in the following directories:  

Linux Newbie Pack ver. 44.12 r03 (pack [here](http://dffd.bay12games.com/file.php?id=13244))
* [`/linux_lnp_44.12-r03/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/linux_lnp_44.12-r03/)

Linux Newbie Pack ver. 44.12 r1
* [`/linux_dfhack_44.12-r1/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/linux_dfhack_44.12-r1) OR updated/untested : [`/linux_dfhack_44.12-r2/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/linux_dfhack_44.12-r2)

Linux Newbie Pack ver. 44.10
* [`/linux_dfhack_44.10/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/linux_dfhack_44_10)

Windows64 Peridix's starter Pack ver. 0.44.12-r04 
* [`/win_peridex_44.12-r1/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/win_peridex_44.12-r1)

Windows64 Peridix's starter Pack ver. 0.44.12-r05 (pack is [here](http://dffd.bay12games.com/file.php?id=7622))
* [`/win_dfhack_44.12-R2/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/win_dfhack_44.12-R2)

#### And finally:

For MacOS DFHack ver 44.12-r2
*[`/macos_dfhack_44.12-r2/`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/macos_dfhack_44.12-r2)

The plugin file is called `"manipulator.plug.so"` for linux, `"manipulator.plug.dll"` for windows and `"manipulator.plug.dylib"` for MacOS. The file goes into the directory `'../(df_directory)/hack/plugins'` or `'..\dwarf fortress\hack\plugins'` on windows.


### Running

To open Cavern Keeper in fortress mode press 'u' (for unitlist) and then 'k' for keeper. It can also be opened on the listing screens for pets and visitors and the dead.

It behaves similar to the default labor manager. Toggle 
Cavern Keepers extended abilities by pressing the keys
displayed in the footer next to 'Mode'.

[Here is bay12forum](http://www.bay12forums.com/smf/index.php?topic=169329.msg7678623#msg7678623) thread on the plugin. Feedback there is appreciated.


### Extra configs:

##### Alternative keeper pallete:

The file [`dfkeeper_pallete.txt`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/dfkeeper_pallete.txt) can be used to customise the theme colors.
It can be saved into the directory `..\[dfhome]\dfkeeper\` and can be edited with a text editor for further tweaking.
 
##### Game pallete tweaked for nice text as well as graphics:

[`colors.txt`](https://github.com/strainer/dfhack/tree/develop/build/feb_df4405/colors.txt) is the default LNP pallet tweaked to balance text readablity and graphics look.
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

## Develupment:

Theres no point cloning this repo as it doesnt compile since its not in sync with df-hack. I paste manipulator.cpp into the latest dfhack every time I compile rather than figuring out git to sync it properly somehow... This repo just records my developements to manipulator.cpp and hosts compiles for various versions.

My development history is mainly recorded in [this branch](https://github.com/strainer/dfhack/commits/manipu_remix) called "manipu_remix".

This readme file is in the "develop" branch, compiles for various versions are in directories and have src file that compiles for that version.

Latest version of manipulator.cpp is usually the one in, https://github.com/strainer/dfhack/commits/manipu_remix.