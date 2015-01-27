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

* Four space indents
* LF (Unix style) line terminators
* UTF-8 encoding
* For C++:

  * Opening and closing braces on their own lines
  * Braces placed at original indent level

-------------------------------
How to get new code into DFHack
-------------------------------

* Code against the develop branch
* Use new branches for each feature/fix
* If possible, compile on multiple platforms
* Do update NEWS/Contributors.rst
* Do **NOT** run fix-texts.sh or update .html files (unless to locally test changes to .rst files) 
* Create a Github Pull Request once finished
* Work done against `issues <http://github.com/DFHack/dfhack/issues>`_ that are tagged "bug report" gets priority

  * If you have an idea or bug, go ahead and create an issue for it

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