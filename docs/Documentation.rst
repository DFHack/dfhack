.. _documentation:

###########################
DFHack Documentation System
###########################


DFHack documentation, like the file you are reading now, is created as ``.rst`` files,
which are in `reStructuredText (reST) <https://www.sphinx-doc.org/rest.html>`_ format.
This is a documentation format common in the Python community. It is very
similar in concept - and in syntax - to Markdown, as found on GitHub and many other
places. However it is more advanced than Markdown, with more features available when
compiled to HTML, such as automatic tables of contents, cross-linking, special
external links (forum, wiki, etc) and more. The documentation is compiled by a
Python tool, `Sphinx <https://www.sphinx-doc.org>`_.

The DFHack build process will compile the documentation, but this is disabled
by default due to the additional Python and Sphinx requirements. You typically
only need to build the docs if you're changing them, or perhaps
if you want a local HTML copy; otherwise, you can read an
`online version hosted by ReadTheDocs <https://dfhack.readthedocs.org>`_.

(Note that even if you do want a local copy, it is certainly not necessary to
compile the documentation in order to read it. Like Markdown, reST documents are
designed to be just as readable in a plain-text editor as they are in HTML format.
The main thing you lose in plain text format is hyperlinking.)

.. contents:: Contents
  :local:

.. _docs-standards:

Documentation standards
=======================

Whether you're adding new code or just fixing old documentation (and there's plenty),
there are a few important standards for completeness and consistent style.  Treat
this section as a guide rather than iron law, match the surrounding text, and you'll
be fine.

Command documentation
---------------------

Each command should have a short (~54 character) help string, which is shown
by the `ls` command.  For scripts, this is a comment on the first line
(the comment marker and whitespace is stripped).  For plugins it's the second
argument to ``PluginCommand``.  Please make this brief but descriptive!

Everything should be documented!  If it's not clear *where* a particular
thing should be documented, ask on IRC or in the DFHack thread on Bay12 -
as well as getting help, you'll be providing valuable feedback that
makes it easier for future readers!

Scripts can use a custom autodoc function, based on the Sphinx ``include``
directive - anything between the tokens is copied into the appropriate scripts
documentation page.  For Ruby, we follow the built-in docstring convention
(``=begin`` and ``=end``).  For Lua, the tokens are ``[====[`` and ``]====]``
- ordinary multi-line strings.  It is highly encouraged to reuse this string
as the in-console documentation by (e.g.) printing it when a ``-help`` argument
is given.

The docs **must** have a heading which exactly matches the command, underlined
with ``=====`` to the same length.  For example, a lua file would have:

.. code-block:: lua

    local helpstr = [====[

    add-thought
    ===========
    Adds a thought or emotion to the selected unit.  Can be used by other scripts,
    or the gui invoked by running ``add-thought gui`` with a unit selected.

    ]====]


.. highlight:: rst

Where the heading for a section is also the name of a command, the spelling
and case should exactly match the command to enter in the DFHack command line.

Try to keep lines within 80-100 characters, so it's readable in plain text
in the terminal - Sphinx (our documentation system) will make sure
paragraphs flow.

Command usage
-------------

If there aren't many options or examples to show, they can go in a paragraph of
text.  Use double-backticks to put commands in monospaced font, like this::

    You can use ``cleanowned scattered x`` to dump tattered or abandoned items.

If the command takes more than three arguments, format the list as a table
called Usage.  The table *only* lists arguments, not full commands.
Input values are specified in angle brackets.  Example::

    Usage:

    :arg1:          A simple argument.
    :arg2 <input>:  Does something based on the input value.
    :Very long argument:
                    Is very specific.

To demonstrate usage - useful mainly when the syntax is complicated, list the
full command with arguments in monospaced font, then indent the next line and
describe the effect::

    ``resume all``
            Resumes all suspended constructions.

Links
-----

If it would be helpful to mention another DFHack command, don't just type the
name - add a hyperlink!  Specify the link target in backticks, and it will be
replaced with the corresponding title and linked:  e.g. ```autolabor```
=> `autolabor`.  Link targets should be equivalent to the command
described (without file extension), and placed above the heading of that
section like this::

    .. _autolabor:

    autolabor
    =========

Add link targets if you need them, but otherwise plain headings are preferred.
Scripts have link targets created automatically.

Note that the DFHack documentation is configured so that single backticks (with
no prefix or suffix) produce links to internal link targets, such as the
``autolabor`` target shown above. This is different from the reStructuredText
default behavior of rendering such text in italics (as a reference to a title).
For alternative link behaviors, see:

- `The reStructuredText documentation on roles <https://docutils.sourceforge.io/docs/ref/rst/roles.html>`__
- `The reStructuredText documentation on external links <https://docutils.sourceforge.io/docs/ref/rst/restructuredtext.html#hyperlink-targets>`__
- `The Sphinx documentation on roles <https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html>`__

    - ``:doc:`` is useful for linking to another document

Required dependencies
=====================

.. highlight:: shell

In order to build the documentation, you must have Python with Sphinx
version |sphinx_min_version| or later. Python 3 is recommended.

When installing Sphinx from OS package managers, be aware that there is
another program called Sphinx, completely unrelated to documentation management.
Be sure you are installing the right Sphinx; it may be called ``python-sphinx``,
for example. To avoid doubt, ``pip`` can be used instead as detailed below.

Once you have installed Sphinx, ``sphinx-build --version`` should report the
version of Sphinx that you have installed. If this works, CMake should also be
able to find Sphinx.

For more detailed platform-specific instructions, see the sections below:

.. contents::
  :local:
  :backlinks: none


Linux
-----
Most Linux distributions will include Python by default. If not, start by
installing Python (preferably Python 3). On Debian-based distros::

  sudo apt install python3

Check your package manager to see if Sphinx |sphinx_min_version| or later is
available. On Debian-based distros, this package is named ``python3-sphinx``.
If this package is new enough, you can install it directly. If not, or if you
want to use a newer Sphinx version (which may result in faster builds), you
can install Sphinx through the ``pip`` package manager instead. On Debian-based
distros, you can install pip with::

  sudo apt install python3-pip

Once pip is available, you can then install Sphinx with::

  pip3 install sphinx

If you run this as an unprivileged user, it may install a local copy of Sphinx
for your user only. The ``sphinx-build`` executable will typically end up in
``~/.local/bin/`` in this case. Alternatively, you can install Sphinx
system-wide by running pip with ``sudo``. In any case, you will need the folder
containing ``sphinx-build`` to be in your ``$PATH``.

macOS
-----
macOS has Python 2.7 installed by default, but it does not have the pip package manager.

You can install Homebrew's Python 3, which includes pip, and then install the
latest Sphinx using pip::

  brew install python3
  pip3 install sphinx

Alternatively, you can simply install Sphinx directly from Homebrew::

  brew install sphinx-doc

This will install Sphinx for macOS's system Python 2.7, without needing pip.

Either method works; if you plan to use Python for other purposes, it might best
to install Homebrew's Python 3 so that you have the latest Python as well as pip.
If not, just installing sphinx-doc for macOS's system Python 2.7 is fine.


Windows
-------
Python for Windows can be downloaded `from python.org <https://www.python.org/downloads/>`_.
The latest version of Python 3 is recommended, as it includes pip already.

You can also install Python and pip through the Chocolatey package manager.
After installing Chocolatey as outlined in the `Windows compilation instructions <compile-windows>`,
run the following command from an elevated (admin) command prompt (e.g. ``cmd.exe``)::

  choco install python pip -y

Once you have pip available, you can install Sphinx with the following command::

  pip install sphinx

Note that this may require opening a new (admin) command prompt if you just
installed pip from the same command prompt.

Building the documentation
==========================

Once the required dependencies are installed, there are multiple ways to run
Sphinx to build the docs:

Using CMake
-----------

Enabling the ``BUILD_DOCS`` CMake option will cause the documentation to be built
whenever it changes as part of the normal DFHack build process. There are several
ways to do this:

* When initially running CMake, add ``-DBUILD_DOCS:bool=ON`` to your ``cmake``
  command. For example::

    cmake .. -DCMAKE_BUILD_TYPE:string=Release -DBUILD_DOCS:bool=ON -DCMAKE_INSTALL_PREFIX=<path to DF>

* If you have already run CMake, you can simply run it again from your build
  folder to update your configuration::

    cmake .. -DBUILD_DOCS:bool=ON

* You can edit the ``BUILD_DOCS`` setting in CMakeCache.txt directly

* You can use the CMake GUI or ``ccmake`` to change the ``BUILD_DOCS`` setting

* On Windows, if you prefer to use the batch scripts, you can run
  ``generate-msvc-gui.bat`` and set ``BUILD_DOCS`` through the GUI. If you are
  running another file, such as ``generate-msvc-all.bat``, you will need to edit
  it to add the flag. You can also run ``cmake`` on the command line, similar to
  other platforms.

The generated documentation will be stored in ``docs/html`` in the root DFHack
folder, and will be installed to ``hack/docs`` when you next install DFHack in a
DF folder.

Running Sphinx manually
-----------------------

You can also build the documentation without running CMake - this is faster if
you only want to rebuild the documentation regardless of any code changes. There
is a ``docs/build.sh`` script provided for Linux and macOS that will run
essentially the same command that CMake runs when building the docs - see the
script for additional options.

To build the documentation with default options, run the following command from
the root DFHack folder::

    sphinx-build . docs/html

The resulting documentation will be stored in ``docs/html`` (you can specify
a different path when running ``sphinx-build`` manually, but be warned that
Sphinx may overwrite existing files in this folder).

Sphinx has many options to enable clean builds, parallel builds, logging, and
more - run ``sphinx-build --help`` for details.

Building a PDF version
----------------------

ReadTheDocs automatically builds a PDF version of the documentation (available
under the "Downloads" section when clicking on the release selector). If you
want to build a PDF version locally, you will need ``pdflatex``, which is part
of a TeX distribution. The following command will then build a PDF, located in
``docs/pdf/latex/DFHack.pdf``, with default options::

  sphinx-build -M latexpdf . docs/pdf

There is a ``docs/build-pdf.sh`` script provided for Linux and macOS that runs
this command for convenience - see the script for additional options.

.. _build-changelog:

Building the changelogs
=======================
If you have Python installed, you can build just the changelogs without building
the rest of the documentation by running the ``docs/gen_changelog.py`` script.
This script provides additional options, including one to build individual
changelogs for all DFHack versions - run ``python docs/gen_changelog.py --help``
for details.

Changelog entries are obtained from ``changelog.txt`` files in multiple repos.
This allows changes to be listed in the same repo where they were made. These
changelogs are combined as part of the changelog build process:

* ``docs/changelog.txt`` for changes in the main ``dfhack`` repo
* ``scripts/changelog.txt`` for changes made to scripts in the ``scripts`` repo
* ``library/xml/changelog.txt`` for changes made in the ``df-structures`` repo

Building the changelogs generates two files: ``docs/_auto/news.rst`` and
``docs/_auto/news-dev.rst``. These correspond to `changelog` and `dev-changelog`
and contain changes organized by stable and development DFHack releases,
respectively. For example, an entry listed under "0.44.05-alpha1" in
changelog.txt will be listed under that version in the development changelog as
well, but under "0.44.05-r1" in the stable changelog (assuming that is the
closest stable release after 0.44.05-alpha1). An entry listed under a stable
release like "0.44.05-r1" in changelog.txt will be listed under that release in
both the stable changelog and the development changelog.


Changelog syntax
----------------

.. include:: /docs/changelog.txt
   :start-after: ===help
   :end-before: ===end

.. _docs-ci:

GitHub Actions
==============

Documentation is built automatically with GitHub Actions (a GitHub-provided
continuous integration service) for all pull requests and commits in the
"dfhack" and "scripts" repositories. These builds run with strict settings, i.e.
warnings are treated as errors. If a build fails, you will see a red "x" next to
the relevant commit or pull request. You can view detailed output from Sphinx in
a few ways:

* Click on the red "x" (or green checkmark), then click "Details" next to
  the "Build / docs" entry
* For pull requests only: navigate to the "Checks" tab, then click on "Build" in
  the sidebar to expand it, then "docs" under it

Sphinx output will be visible under the step named "Build docs". If a different
step failed, or you aren't sure how to interpret the output, leave a comment
on the pull request (or commit).

You can also download the "docs" artifact from the summary page (typically
accessible by clicking "Build") if the build succeeded. This is a way to
visually inspect what the documentation looks like when built without installing
Sphinx locally, although we recommend installing Sphinx if you are planning to
do any significant work on the documentation.
