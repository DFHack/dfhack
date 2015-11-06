##################################
Welcome to DFHack's documentation!
##################################

Introduction
============
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


User Manual
===========

.. toctree::
   :maxdepth: 2

   /docs/Introduction
   /docs/Core
   /docs/Plugins
   /docs/Scripts

Other Contents
==============

.. toctree::
   :maxdepth: 1

   /docs/Authors
   /LICENSE
   /NEWS

For Developers
==============

.. toctree::
   :maxdepth: 1

   /docs/Contributing
   /docs/Compile
   /docs/Lua API
   /library/xml/SYNTAX
   /library/xml/how-to-update
   /docs/Binpatches
