.. _contributing:

###########################
How to contribute to DFHack
###########################

.. contents:: Contents
  :local:

.. _contributing-code:

Contributing Code
=================
Several things should be kept in mind when contributing code to DFHack.

Code Format
-----------
* Four space indents for C++. Never use tabs for indentation in any language.
* LF (Unix style) line terminators
* Avoid trailing whitespace
* UTF-8 encoding
* For C++:

  * Opening and closing braces on their own lines or opening brace at the end of the previous line
  * Braces placed at original indent level if on their own lines
  * #includes should be sorted. C++ libraries first, then dfhack modules, then df structures,
    then local includes. Within each category they should be sorted alphabetically.

How to get new code into DFHack
-------------------------------
* Submit pull requests to the ``develop`` branch, not the ``master`` branch.
  (The ``master`` branch always points at the most recent release)
* Use a new branch for each feature or bugfix so that your changes can be merged independently
  (i.e. not the master or develop branch of your fork).
* If possible, compile on multiple platforms when changing anything that compiles
* It must pass CI - run ``python travis/all.py`` to check this.
* Update documentation when applicable - see `docs-standards` for details.
* Update ``changelog.txt`` and ``docs/Authors.rst`` when applicable. See
  `build-changelog` for more information on the changelog format.
* Create a GitHub pull request once finished
* Submit ideas and bug reports as :issue:`issues on GitHub <>`.
  Posts in the forum thread can easily get missed or forgotten.
* Work on :issue:`reported problems <?q=is:open+-label:idea>`
  will take priority over ideas or suggestions.


Other ways to help
==================
DFHack is a software project, but there's a lot more to it than programming.
If you're not comfortable programming, you can help by:

* reporting bugs and incomplete documentation
* improving the documentation
* finding third-party scripts to add
* writing tutorials for newbies

All those things are crucial, and often under-represented.  So if that's
your thing, go get started!

