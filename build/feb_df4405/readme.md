## Cavern Keeper 

Here is an extended dfhack labor manager plugin compiled for the latest Linux LNP releases:

The plugin file `manipuator.plug.so` in this directory is for LNP44.05.r01 (latest). The file of the same name in the directory `/LinuxLNP4405rc1` is for the previous pack release.

To install just save `manipuator.plug.so` ontop of
the one already present in `~lnp/df_linux/hack/plugins`.

Open cavern keeper in fortress mode by pressing 'u' (for unitlist) and then 'k' (for 'Keeper') .

It behaves similar to the default labor manager. Turn on 
Cavern Keepers extended abilities by pressing the keys
displayed in the footer next to 'Mode'

Here is latest bay12forum thread. Feedback is appreciated.

http://www.bay12forums.com/smf/index.php?topic=169329.msg7678623#msg7678623

## Windows compiles

Maybe in a coupel of weeks..

### Extra config files:

`colors.txt` is the default LNP pallet tweaked to
balance text readablity and graphics look.
It is configured for TWBT rendering. 
It can go on top of `~lnp\df_linux\data\init\colors.txt`

 
DF's default keypress timings are sluggish on my own system
so here are tweaked keyboard speed settings which go
near the bottom of `~lnp\df_linux\data\init\init.txt`

[KEY_HOLD_MS:250]

This controls the number of milliseconds that must pass before a held key sends a repeat press to the game after the repeat process has begun.

[KEY_REPEAT_MS:133]

If you set KEY_REPEAT_ACCEL_LIMIT above one, then after KEY_REPEAT_ACCEL_START repetitions the repetition delay will smoothly decrease until repetition is this number of times faster than at the start.

[KEY_REPEAT_ACCEL_LIMIT:20]
[KEY_REPEAT_ACCEL_START:2]
