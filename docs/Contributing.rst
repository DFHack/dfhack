.. _contributing:

###########################
How to contribute to DFHack
###########################

.. contents:: Contents
  :local:

.. _contributing-code:

Contributing Code
=================

DFHack's source code is hosted on GitHub. To obtain the code, you do not need an
account - see the `compilation instructions <compile-how-to-get-the-code>` for
details. However, to contribute code to DFHack, you will need a GitHub account
to submit pull requests. DFHack consists of several repositories, so you will
need to fork the repository (or repositories) containing the code you wish to
modify. GitHub has several documentation pages on these topics, including:

* `An overview of forks
  <https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/working-with-forks>`__
* `Proposing changes with pull requests
  <https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/proposing-changes-to-your-work-with-pull-requests>`__
  (note: see `contributing-pr-guidelines` for some DFHack-specific information)

In general, if you are not sure where or how to make a change, or would like
advice before attempting to make a change, please see `support` for ways to
contact maintainers - DFHack-specific channels such as IRC or Bay12 are
preferred. If you are interested in addressing an issue reported on the
:issue:`issue tracker <>`, you can start a discussion there if you prefer.

The sections below cover some guidelines that contributions should follow:

.. contents::
  :local:

Code format
-----------
* Four space indents for C++. Never use tabs for indentation in any language.
* LF (Unix style) line terminators
* Avoid trailing whitespace
* UTF-8 encoding
* For C++:

  * Opening and closing braces on their own lines or opening brace at the end of the previous line
  * Braces placed at original indent level if on their own lines
  * ``#include`` directives should be sorted: C++ libraries first, then DFHack modules, then ``df/`` headers,
    then local includes. Within each category they should be sorted alphabetically.

.. _contributing-pr-guidelines:

Pull request guidelines
-----------------------

* Pull requests should be based on (and submitted to) the default branch of the
  relevant repo, which is the branch you see when you access the repo on GitHub
  or clone the repo without specifying a branch. As of 0.47.04-r1, this is
  ``develop`` for the main DFHack repo and ``master`` for other repos.
* We often leave feedback as comments on pull requests, so be sure that you have
  `notifications turned on <https://github.com/settings/notifications>`__ or
  that you check back for feedback periodically.
* Use a new branch for each feature or bugfix so that your changes can be merged
  independently (i.e. not the ``master`` or ``develop`` branch of your fork).

  * An exception: for a collection of small miscellaneous changes (e.g.
    structures research), one branch instead of many small branches is fine. It
    is still preferred that this branch be dedicated to this purpose, i.e. not
    ``master`` or ``develop``. Your pull request may be merged at any point
    unless you indicate that it isn't ready (see below), but you can continue to
    push to the same branch and open new pull requests as needed.

* Try to keep pull requests relatively small so that they are easier to review
  and merge.

  * If you expect to make a large number of related additions or changes (e.g.
    adding a large new plugin), multiple PRs are preferred, as they allow more
    frequent (and easier) feedback. If development of this feature is expected
    to take a while, we may create a dedicated branch to merge your pull
    requests into instead of the repo's default branch.

* If you plan to make additional changes to your pull request in the near
  future, or if it isn't quite ready to be merged, mark it as a
  `draft pull request <https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/about-pull-requests#draft-pull-requests>`_
  or add "WIP" to the title. Otherwise, your pull request may be reviewed and/or
  merged prematurely.

General contribution guidelines
-------------------------------
* If convenient, compile on multiple platforms when changing anything that
  compiles. Our CI should catch anything that fails to build, but checking in
  advance can sometimes let you know of any issues sooner.
* Update documentation when applicable - see `docs-standards` for details.
* Update ``changelog.txt`` and ``docs/Authors.rst`` when applicable. See
  `build-changelog` for more information on the changelog format.
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
