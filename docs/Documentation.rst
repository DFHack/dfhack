.. _documentation:

####################
DFHack Documentation
####################


DFHack documentation, like the file you are reading now, is created as ``.rst`` files,
which are in `reStructuredText (reST) <http://sphinx-doc.org/rest.html>`_ format.
This is a documentation format common in the Python community. It is very
similar in concept - and in syntax - to Markdown, as found on GitHub and many other
places. However it is more advanced than Markdown, with more features available when
compiled to HTML, such as automatic tables of contents, cross-linking, special
external links (forum, wiki, etc) and more. The documentation is compiled by a
Python tool, `Sphinx <http://sphinx-doc.org>`_.

The DFHack build process will compile the documentation, but this is disabled
by default due to the additional Python and Sphinx requirements. You typically
only need to build the docs if you're changing them, or perhaps
if you want a local HTML copy; otherwise, you can read an
`online version hosted by ReadTheDocs <https://dfhack.readthedocs.org>`_.

(Note that even if you do want a local copy, it is certainly not necessary to
compile the documentation in order to read it. Like Markdown, reST documents are
designed to be just as readable in a plain-text editor as they are in HTML format.
The main thing you lose in plain text format is hyperlinking.)

.. contents::


Required dependencies
=====================
In order to build the documentation, you must have Python with Sphinx
version 1.3.1 or later. Both Python 2.x and 3.x are supported.

When installing Sphinx from OS package managers, be aware that there is
another program called Sphinx, completely unrelated to documentation management.
Be sure you are installing the right Sphinx; it may be called ``python-sphinx``,
for example. To avoid doubt, ``pip`` can be used instead as detailed below.

For more detailed platform-specific instructions, see the sections below:


Linux
-----
Most Linux distributions will include Python as standard.

Check your package manager to see if Sphinx 1.3.1 or later is available,
but at the time of writing Ubuntu for example only has 1.2.x.

You can instead install Sphinx with the pip package manager. This may need
to be installed from your OS package manager; this is the case on Ubuntu.
On Ubuntu/Debian, use the following to first install pip::

  sudo apt-get install python-pip

Once pip is available, you can then install the Python Sphinx module with::

  pip install sphinx

If you run this as a normal user it will install a local copy for your user only.
Run it with sudo if you want a system-wide install. Either is fine for DFHack,
however if installing locally do check that ``sphinx-build`` is in your path.
It may be installed in a directory such as ``~/.local/bin/``, so after pip
install, find ``sphinx-build`` and ensure its directory is in your local ``$PATH``.


macOS
-----
macOS has Python 2.7 installed by default, but it does not have the pip package manager.

You can install Homebrew's Python 3, which includes pip, and then install the
latest Sphinx using pip::

  brew install python3
  pip3 install sphinx

Alternatively, you can simply install Sphinx 1.3.x directly from Homebrew::

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

Once you have installed Sphinx, ``sphinx-build --version`` should report the
version of Sphinx that you have installed. If this works, CMake should also be
able to find Sphinx.

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

* You can use the CMake GUI to change ``BUILD_DOCS``

* On Windows, if you prefer to use the batch scripts, you can run
  ``generate-msvc-gui.bat`` and set ``BUILD_DOCS`` through the GUI. If you are
  running another file, such as ``generate-msvc-all.bat``, you will need to edit
  it to add the flag. You can also run ``cmake`` on the command line, similar to
  other platforms.

Running Sphinx manually
-----------------------

You can also build the documentation without going through CMake, which may be
faster. There is a ``docs/build.sh`` script available for Linux and macOS that
will run essentially the same command that CMake runs - see the script for
options.

To build the documentation with default options, run the following command from
the root DFHack folder::

    sphinx-build . docs/html

Sphinx has many options to enable clean builds, parallel builds, logging, and
more - run ``sphinx-build --help`` for details.

.. _build-changelog:

Building the changelogs
=======================
If you have Python installed, but do not want to build all of the documentation,
you can build the changelogs with the ``docs/gen_changelog.py`` script. This
script provides additional options, including one to build individual changelogs
for all DFHack versions - run ``python docs/gen_changelog.py --help`` for details.

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
