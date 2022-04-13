===================================
DF data definitions (DF-structures)
===================================

DFHack's information about DF's data structures is stored in XML files in the
`df-structures repository <https://github.com/DFHack/df-structures>`_. If you
have `obtained a local copy of the DFHack source <compile-how-to-get-the-code>`,
DF-structures is included as a submodule in ``library/xml``.

Data structure layouts are described in files named with the ``df.*.xml``
pattern. This information is transformed by a Perl script (``codegen.pl``) into
C++ headers, as well as metadata for the Lua wrapper. This ultimately allows
DFHack code to access DF data directly, with the same speed and capabilities as
DF itself, which is an  advantage over the older out-of-process approach (used
by debuggers and utilities like Dwarf Therapist). The main disadvantage of this
approach is that any compiled code relying on these layouts will break when DF's
layout changes, and will need to be recompiled for every new DF version.

Addresses of DF global objects and vtables are stored in a separate file,
:file:`symbols.xml`. Since these are only absolute addresses, they do not need
to be compiled in to DFHack code, and are instead loaded at runtime. This makes
fixes and additions to global addresses possible without recompiling DFHack.
In an installed copy of DFHack, this file can be found at the root of the
``hack`` folder.

The following pages contain more detailed information about various aspects
of DF-structures:

.. toctree::
   :titlesonly:

   /library/xml/SYNTAX
   /library/xml/how-to-update
