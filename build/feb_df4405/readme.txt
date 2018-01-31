From: https://github.com/strainer/dfhack/tree/develop/build/feb_df4405

Cavern Keeper for LinuxLNP 44.05.RC1

Cavern Keeper is an extended dfhack labor manager plugin.
To install just save this manipuator.plug.so file on top of
the one already present in ~lnp/df_linux/hack/plugins.
(It will keep saved professions if you use those)

Open cavern keeper in fortress mode by pressing 'u' (for unitlist) and then 'k' (for 'keeper') .

It behaves similar to the default labor manager. Turn on 
Cavern Keepers extended abilities by pressing the keys
displayed in the footer next to 'Mode' 

Here is latest bay12forum thread. Feedback is appreciated.

http://www.bay12forums.com/smf/index.php?topic=169329.msg7678623#msg7678623

Enjoy, dont over do it now :)

I need help getting windows compiles, if you make one please let me know:

andrewinput@gmail.com


The included colors.txt is the default LNP pallet tweaked to
balances text readablity and graphics look.
It is configured for TWBT rendering. 
It can go on top of ~lnp\df_linux\data\init\colors.txt

 
The default keypress timings are sluggish on my own system
so here are my tweaked keyboard speed settings which go
near the bottom of ~lnp\df_linux\data\init\init.txt
probably.

[KEY_HOLD_MS:250]

This controls the number of milliseconds that must pass before a held key sends a repeat press to the game after the repeat process has begun.

[KEY_REPEAT_MS:125]

If you set KEY_REPEAT_ACCEL_LIMIT above one, then after KEY_REPEAT_ACCEL_START repetitions the repetition delay will smoothly decrease until repetition is this number of times faster than at the start.

[KEY_REPEAT_ACCEL_LIMIT:19]
[KEY_REPEAT_ACCEL_START:2]
