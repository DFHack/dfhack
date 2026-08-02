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
contact maintainers. If you are interested in addressing an issue reported on the
:issue:`issue tracker <>`, you can start a discussion there if you prefer.

The sections below cover some guidelines that contributions should follow:

.. contents::
  :local:

General contribution guidelines
-------------------------------
* If convenient, compile on multiple platforms when changing anything that
  compiles. Our CI should catch anything that fails to build, but checking in
  advance can sometimes let you know of any issues sooner.
* Update documentation when applicable - see `docs-standards` for details.
* Update ``docs/changelog.txt`` and ``docs/about/Authors.rst`` when applicable. See
  `build-changelog` for more information on the changelog format.
* Submit ideas and bug reports as :issue:`issues on GitHub <>`.
  Posts in the forum thread or on Discord can easily get missed or forgotten.
* Work on :issue:`reported problems <?q=is:open+-label:idea>`
  will take priority over ideas or suggestions.

Code format
-----------
* Four space indents for C++. Never use tabs for indentation in any language.
* LF (Unix style) line terminators
* No trailing whitespace
* UTF-8 encoding
* For C++:

  * Opening and closing braces on their own lines or opening brace at the end of the previous line
  * Braces placed at original indent level if on their own lines
  * ``#include`` directives should be sorted: C++ libraries first, then DFHack modules, then ``df/`` headers,
    then local includes. Within each category they should be sorted alphabetically.

General C++ code guidelines
---------------------------
* This project is currently built at the C++20 feature level, and C++20 features should be used when appropriate. C++23 features will be allowed once all of our build platforms support them.
* NEVER use ``using namespace`` in a header file. In source files, do not use ``using namespace std``; instead, import each STL identifier you need specifically (e.g. ``using std::string;``).
* Avoid platform specific code as much as possible.
* Avoid including ``Windows.h``; if you must, ensure that ``NOMINMAX`` and ``WIN32_LEAN_AND_MEAN`` are defined before including it.
* Do not include C headers (e.g. ``<stdio.h>``); use the C++ versions (e.g. ``<cstdio>``) instead.
* Do not use ``std::string`` (or ``char *``) for path names; always use ``std::filesystem::path``. This avoids issues with encoding, especially on the Windows platform, which is roughly 80% of our user base.
* Do not use ``printf`` or similar functions for formatting strings; use C++ streams or ``fmt::format`` instead. We use the `fmt library <https://fmt.dev/latest/index.html>`__ for formatting strings; this dependency is automatically fetched by our build system.
* Avoid out parameters; prefer returning a struct, pair, or tuple, or using ``std::optional`` instead.
* Prefer range for loops to traditional for loops when iterating over a container.
* Avoid macros when possible; prefer ``constexpr`` variables for constants and functions or templates for code generation.

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

* Our continuous integration (CI) will perform certain automatic checks,
  ensuring that your code conforms to the code format described above. It is
  recommended to install `pre-commit <https://pre-commit.com/>`__ (e.g. using
  your distribution's package manager, if on Linux, or using ``pip``) and enable
  it by running ``pre-commit install`` from the top-level of any repository from
  which you plan to create pull requests. This will perform those checks when
  you create the commit locally, allowing you to fix any style issues before
  creating the actual pull request.

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

* Avoid using force pushes to your pull request branch after it has been reviewed,
  as this can make it difficult for reviewers to see what has changed since their
  last review. If you need to make changes, consider creating a new commit instead
  of amending or rebasing. We neither enforce nor recommend a "single commit" rule; if you do
  choose to squash your commits, please ensure that the commit message is clear and descriptive of the changes made.
  If your pull request has an unusually large number of commits, a maintainer may
  request that you squash your commits into a smaller number of commits before merging.

* All pull requests must be accompanied by a description of the changes made, and
  any relevant information for reviewers. If your pull request addresses an
  issue, please include a reference to that issue in the description (e.g.
  "Fixes #1234"). If your pull request is related to another pull request, please
  include a reference to that pull request in the description (e.g. "Related to
  #1234").

* All pull requests which have user facing changes, including all new features, bug fixes, or
  changes to existing functionality, must include an entry in the "Future" section of
  the changelog for the relevant repository. If your pull request is merged, this entry
  will be added to the appropriate changelog. These entries are used when preparing the release
  notes for each release, so please be sure to include a clear and concise description
  of the changes made. See `build-changelog` for more information on the changelog format.
  Changes that do not require a changelog entry are mainly those that are purely internal,
  such as refactoring not intended to change semantics, code cleanup, changes to CI implementation
  or to documentation, or changes directly related to the release process. When in doubt,
  assume a changelog entry will be required.

* Pull requests that add or modify tools must include a corresponding update to the documentation
  for that tool. Similarly, pull requests that add or modify either the C++ or Lua APIs
  must include a corresponding update to the appropriate API documentation.
  See `docs-standards` for details.

Other ways to help
==================
DFHack is a software project, but there's a lot more to it than programming.
If you're not comfortable programming, you can help by:

* reporting bugs and incomplete documentation
* improving the documentation (C++ api is rife)
* finding third-party scripts to add
* writing tutorials for newbies

All those things are crucial, and often under-represented.  So if that's
your thing, go get started!
