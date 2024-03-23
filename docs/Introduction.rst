.. _introduction:

#########################
Introduction and overview
#########################

DFHack is a Dwarf Fortress memory access library, distributed with
a wide variety of useful scripts and plugins.

The project is hosted `on GitHub <https://www.github.com/DFHack/dfhack>`__,
and can be downloaded from `the releases page <https://github.com/DFHack/dfhack/releases>`__
-- see `installing` for installation instructions. This is also where the
`DFHack bug tracker <https://www.github.com/DFHack/dfhack>`__ is hosted. If you would like
to download the DFHack documentation for offline viewing, you can do so by clicking
the expansion panel in the lower right corner of our
`online documentation <https://dfhack.org/docs>`__ and selecting your desired format from
the "Downloads" section.

New releases are announced in the
`DF subreddit <https://www.reddit.com/r/dwarffortress/>`__, the
`DFHack Discord <https://dfhack.org/discord>`__, and the
`Bay12 forums thread <https://dfhack.org/bay12>`__. Discussion and questions are also
welcome in each of these venues.

For users, DFHack provides a significant suite of bugfixes and interface
enhancements by default, and more features can be enabled as desired. There are
also many tools (such as `autofarm`) which automate aspects of gameplay many players
find toilsome. You can even add third-party scripts and plugins to do almost anything!

For modders, DFHack makes many things possible. Custom reactions, new
interactions, magic creature abilities, and more can be set through `tools`
and custom raws. 3rd party DFHack scripts can be distributed `in mods <modding-guide>`
via the DF Steam Workshop or on the forums.

For developers, DFHack unites the various ways tools access DF memory and
allows easier development of new tools. As an open-source project under
`various open-source licenses <license>`, contributions are welcome.


.. contents:: Contents
  :local:


Getting started
===============

See `installing` for details about installing DFHack.

Once DFHack is installed, it extends DF with a console that can be used to run
commands. The in-game version of this console is called `gui/launcher`, and you
can bring it up at any time by hitting the backtick (\`) key (on most keyboards
this is the same as the tilde (~) key). There are also external consoles you can
open in a separate window. On Windows, you can show this console with the `show`
command. On Linux and macOS, you will need to run the ``dfhack`` script from a
terminal, and that terminal will be used as the DFHack console.

Basic interaction with DFHack involves entering commands into the console. To
learn what commands are available, you can keep reading this documentation or
skip ahead and use the `ls` and `help` commands. The first command you should
run is likely `gui/control-panel` so you can set up which tools you would like
to enable now and which tools you want automatically started for new games.

Another way to interact with DFHack is to set in-game `keybindings <keybinding>`
to run commands in response to a hotkey. If you have specific commands that you
run frequently and that don't already have default keybindings, this can be a
better option than adding the command to the `gui/quickcmd` list.

Commands can also run at startup via `init files <init-files>`, or in batches
at other times with the `script` command.

Finally, some commands are persistent once enabled, and will sit in the
background managing or changing some aspect of the game if you `enable` them.

.. note::
    In order to avoid user confusion, as a matter of policy all GUI tools
    display the word :guilabel:`DFHack` on the screen somewhere while active.

    When that is not appropriate because they merely add keybinding hints to
    existing DF screens, they surround the added text or clickable buttons in red
    square brackets.

For a more thorough introduction and guide through DFHack's capabilities, please see
the `quickstart`.

.. _support:

Getting help
============

DFHack has several ways to get help online, including:

- The `DFHack Discord server <https://dfhack.org/discord>`__
- GitHub:
    - for bugs, use the :issue:`issue tracker <>`
    - for more open-ended questions, use the `discussion board
      <https://github.com/DFHack/dfhack/discussions>`__. Note that this is a
      relatively-new feature as of 2021, but maintainers should still be
      notified of any discussions here.
- The `DFHack thread on the Bay 12 Forum <https://dfhack.org/bay12>`__
- The `/r/dwarffortress <https://www.reddit.com/r/dwarffortress/>`__ questions thread on Reddit

When reaching out to any support channels regarding problems with DFHack, please
remember to provide enough details for others to identify the issue. For
instance, specific error messages (copied text or screenshots) are helpful, as
well as any steps you can follow to reproduce the problem. Log output from
``stderr.log`` in the DF folder can often help point to the cause of issues.

Some common questions may also be answered in documentation, including:

- This documentation (`online here <https://dfhack.readthedocs.io>`__; search functionality available `here <search>`)
- :wiki:`The DF wiki <>`
