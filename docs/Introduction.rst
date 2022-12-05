.. _introduction:

#########################
Introduction and overview
#########################

DFHack is a Dwarf Fortress memory access library, distributed with
a wide variety of useful scripts and plugins.

The project is currently hosted `on GitHub <https://www.github.com/DFHack/dfhack>`_,
and can be downloaded from `the releases page <https://github.com/DFHack/dfhack/releases>`_
- see `installing` for installation instructions. This is also where the
`DFHack bug tracker <https://www.github.com/DFHack/dfhack>`_ is hosted.

All new releases are announced in `the Bay12 forums thread <https://dfhack.org/bay12>`_,
which is also a good place for discussion and questions.

For users, DFHack provides a significant suite of bugfixes and interface
enhancements by default, and more can be enabled.  There are also many tools
(such as `workflow` or `autodump`) which can make life easier.
You can even add third-party scripts and plugins to do almost anything!

For modders, DFHack makes many things possible.  Custom reactions, new
interactions, magic creature abilities, and more can be set through `tools`
and custom raws.  Non-standard DFHack scripts and inits can be stored in the
raw directory, making raws or saves fully self-contained for distribution -
or for coexistence in a single DF install, even with incompatible components.

For developers, DFHack unites the various ways tools access DF memory and
allows easier development of new tools.  As an open-source project under
`various open-source licenses <license>`, contributions are welcome.


.. contents:: Contents
  :local:


Getting started
===============
See `installing` for details on installing DFHack.

Once DFHack is installed, it extends DF with a console that can be used to run
commands. On Windows, this console will open automatically when DF is started.
On Linux and macOS, you will need to run the ``dfhack`` script from a terminal
(instead of the ``df`` script included with DF), and that terminal will be
used by the DFHack console.

* Basic interaction with DFHack involves entering commands into the console. To
  learn what commands are available, you can keep reading this documentation or
  skip ahead and use the `ls` and `help` commands.

* Another way to interact with DFHack is to set in-game `keybindings <keybinding>`
  for certain commands.  Many of the newer and user-friendly tools are designed
  to be used this way.

* Commands can also run at startup via `init files <init-files>`,
  or in batches at other times with the `script` command.

* Finally, some commands are persistent once enabled, and will sit in the
  background managing or changing some aspect of the game if you `enable` them.

.. note::
    In order to avoid user confusion, as a matter of policy all GUI tools
    display the word :guilabel:`DFHack` on the screen somewhere while active.

    When that is not appropriate because they merely add keybinding hints to
    existing DF screens, they deliberately use red instead of green for the key.

.. _support:

Getting help
============

DFHack has several ways to get help online, including:

- The `DFHack Discord server <https://dfhack.org/discord>`__
- The ``#dfhack`` IRC channel on `Libera <https://libera.chat/>`__
- GitHub:
    - for bugs, use the :issue:`issue tracker <>`
    - for more open-ended questions, use the `discussion board
      <https://github.com/DFHack/dfhack/discussions>`__. Note that this is a
      relatively-new feature as of 2021, but maintainers should still be
      notified of any discussions here.
- The `DFHack thread on the Bay 12 Forum <https://dfhack.org/bay12>`__

Some additional, but less DFHack-specific, places where questions may be answered include:

- The `/r/dwarffortress <https://dwarffortress.reddit.com>`_ questions thread on Reddit
- If you are using a starter pack, the relevant thread on the `Bay 12 Forum <http://www.bay12forums.com/smf/index.php?board=2.0>`__ -
  see the :wiki:`DF Wiki <Utility:Lazy_Newb_Pack>` for a list of these threads

When reaching out to any support channels regarding problems with DFHack, please
remember to provide enough details for others to identify the issue. For
instance, specific error messages (copied text or screenshots) are helpful, as
well as any steps you can follow to reproduce the problem. Sometimes, log output
from ``stderr.log`` in the DF folder can point to the cause of issues as well.

Some common questions may also be answered in documentation, including:

- This documentation (`online here <https://dfhack.readthedocs.io>`__; search functionality available `here <search>`)
- :wiki:`The DF wiki <>`
