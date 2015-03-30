######################
Contributing to DFHACK
######################

.. contents::

Contributing to DFHack
======================

Several things should be kept in mind when contributing to DFHack.

-----------
Code Format
-----------

* Four space indents for C++. Never use tabs for indentation in any language.
* LF (Unix style) line terminators
* Avoid trailing whitespace
* UTF-8 encoding
* For C++:

  * Opening and closing braces on their own lines or opening brace at the end of the previous line
  * Braces placed at original indent level if on their own lines
  * #includes should be sorted. C++ libraries first, then dfhack modules, then df structures, then local includes. Within each category they should be sorted alphabetically. This policy is currently broken by most C++ files but try to follow it if you can.

-------------------------------
How to get new code into DFHack
-------------------------------

* Submit pull requests to the ``develop`` branch, not the master branch. The master branch always points at the most recent release.
* Use new branches for each feature/fix so that your changes can be merged independently (i.e. not the master or develop branch of your fork).
* If possible, compile on multiple platforms
* Update NEWS and Contributors.rst when applicable
* Do **NOT** run fixTexts.sh or update .html files (except to locally test changes to .rst files)
* Create a Github Pull Request once finished
* Work done against `issues <http://github.com/DFHack/dfhack/issues>`_ that are tagged "bug report" gets priority

  * Submit ideas and bug reports as issues on Github. Posts in the forum thread are also acceptable but can get missed or forgotten more easily.

---------------
Memory research
---------------
If you want to do memory research, you'll need some tools and some knowledge.
In general, you'll need a good memory viewer and optionally something
to look at machine code without getting crazy :)

Good windows tools include:

* Cheat Engine
* IDA Pro 5.0 (freely available for non-commercial use)

Good linux tools:

* angavrilov's df-structures gui (visit us on IRC for details).
* edb (Evan's Debugger)
* IDA Pro 5.0 running under Wine
* Some of the tools residing in the ``legacy`` dfhack branch.

Using publicly known information and analyzing the game's data is preferred.
