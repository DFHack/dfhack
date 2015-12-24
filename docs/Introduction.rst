.. _introduction:

#########################
Introduction and Overview
#########################

DFHack is a Dwarf Fortress memory access library, distributed with
a wide variety of useful scripts and plugins.

The project is currently hosted at https://www.github.com/DFHack/dfhack,
and can be downloaded from `the releases page
<http://github.com/DFHack/dfhack/releases>`_.

All new releases are announced in :forums:`the bay12 forums thread <139553>`,
which is also a good place for discussion and questions.

For users, it provides a significant suite of bugfixes and interface
enhancements by default, and more can be enabled.  There are also many tools
(such as `workflow` or `autodump`) which can make life easier.
You can even add third-party scripts and plugins to do almost anything!

For modders, DFHack makes many things possible.  Custom reactions, new
interactions, magic creature abilities, and more can be set through `modtools`
and custom raws.  Non-standard DFHack scripts and inits can be stored in the
raw directory, making raws or saves fully self-contained for distribution -
or for coexistence in a single DF install, even with incompatible components.

For developers, DFHack unites the various ways tools access DF memory and
allows easier development of new tools.  As an open-source project under
`various copyleft licences <license>`, contributions are welcome.


.. contents::


.. _installing:

Installing DFHack
=================
DFHack is available for the SDL version of Dwarf Fortress on Windows,
any modern Linux distribution, and Mac OS X (10.6.8 and later).
It is possible to use Windows DF+DFHack under Wine on Linux or OS X.

Most releases only support the version of DF mentioned in their title - for
example, DFHack 0.40.24-r2 only supports DF 0.40.24 - but some releases
support earlier DF versions as well.  Wherever possible, use the latest version
of DFHack built for the target version of DF.

Installing DFhack involves copying files from a release archive
into your DF folder, so that:

* On Windows, ``SDL.dll`` is replaced
* On Linux or OSX, the ``dfhack`` script is placed in the same folder as the ``df`` script

Uninstalling is basically the same, in reverse:

* On Windows, replace ``SDL.dll`` with ``SDLreal.dll``, then remove the DFHack files.
* On Linux or OSX, remove the DFHack files.

New players may wish to :wiki:`get a pack <Utility:Lazy_Newb_Pack>`
with DFHack preinstalled.


Getting started
===============
DFHack basically extends DF with something similar to the
console found in many PC games.

If DFHack is installed correctly, it will automatically pop up a console
window once DF is started as usual on Windows. Linux and Mac OS X require
running the dfhack script from the terminal, and will use that terminal for
the console.

* Basic interaction with dfhack involves entering commands into the console.
  To learn what commands are available, you can keep reading this documentation
  or skip ahead and use the `ls` and `help` commands.

* Another way to interact with DFHack is to set in-game `keybindings <keybinding>`
  for certain commands.  Many of the newer and user-friendly tools are designed
  to be used this way.

* Commands can also run at startup via `init files <init-files>`,
  on in batches at other times with the `script` command.

* Finally, some commands are persistent once enabled, and will sit in the
  background managing or changing some aspect of the game if you `enable` them.


.. _troubleshooting:

Troubleshooting
===============
Don't panic!  Even if you need this section, it'll be OK :)

If something goes wrong, check the log files in DF's folder
(``stderr.log`` and ``stdout.log``). Looking at these might help you -
or someone else - solve the problem.  Take screenshots of any weird
error messages, and take notes on what you did to cause them.

If  the search function in this documentation isn't enough and
:wiki:`the DF Wiki <>` hasn't helped, try asking in:

- the `#dfhack IRC channel on freenode <https://webchat.freenode.net/?channels=dfhack>`_
- the :forums:`Bay12 DFHack thread <139553>`
- the `/r/dwarffortress <https://dwarffortress.reddit.com>`_ questions thread
- the thread for the mod or Starter Pack you're using (if any)

