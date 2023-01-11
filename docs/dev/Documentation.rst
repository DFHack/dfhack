.. _documentation:

###########################
DFHack documentation system
###########################


DFHack documentation, like the file you are reading now, is created as a set of
``.rst`` files in `reStructuredText (reST) <https://www.sphinx-doc.org/rest.html>`_
format. This is a documentation format common in the Python community. It is very
similar in concept -- and in syntax -- to Markdown, as found on GitHub and many other
places. However it is more advanced than Markdown, with more features available when
compiled to HTML, such as automatic tables of contents, cross-linking, special
external links (forum, wiki, etc) and more. The documentation is compiled by a
Python tool named `Sphinx <https://www.sphinx-doc.org>`_.

The DFHack build process will compile and install the documentation so it can be
displayed in-game by the `help` and `ls` commands (and any other command or GUI that
displays help text), but documentation compilation is disabled by default due to the
additional Python and Sphinx requirements. If you already have a version of the docs
installed (say from a downloaded release binary), then you only need to build the docs
if you're changing them and want to see the changes reflected in your game.

You can also build the docs if you just want a local HTML- or text-rendered copy, though
you can always read the `online version <https://dfhack.readthedocs.org>`_ too.
The active development version of the documentation is tagged with ``latest`` and
is available `here <https://docs.dfhack.org/en/latest/index.html>`_

Note that even if you do want a local copy, it is certainly not necessary to
compile the documentation in order to read it. Like Markdown, reST documents are
designed to be just as readable in a plain-text editor as they are in HTML format.
The main thing you lose in plain text format is hyperlinking.

.. contents:: Contents
  :local:

Concepts and general guidance
=============================

The source ``.rst`` files are compiled to HTML for viewing in a browser and to text
format for viewing in-game. For in-game help, the help text is read from its installed
location in ``hack/docs`` under the DF directory.

When writing documentation, remember that everything should be documented! If it's not
clear *where* a particular thing should be documented, ask on Discord or in the DFHack
thread on Bay12 -- you'll not only be getting help, you'll also be providing valuable
feedback that makes it easier for future contributors to find documentation on how to
write the documentation!

Try to keep lines within 80-100 characters so it's readable in plain text in the
terminal - Sphinx (our documentation system) will make sure paragraphs flow.

Short descriptions
------------------

Each command that a user can run -- as well as every plugin -- needs to have a
short (~54 character) descriptive string associated with it. This description text is:

- used in-game by the `ls` command and DFHack UI screens that list commands
- used in the generated index entries in the HTML docs

Tags
----

To make it easier for players to find related commands, all plugins and commands are marked
with relevant tags. These are used to compile indices and generate cross-links between the
commands, both in the HTML documents and in-game. See the list of available `tag-list` and
think about which categories your new tool belongs in.

Links
-----

If it would be helpful to mention another DFHack command, don't just type the
name - add a hyperlink!  Specify the link target in backticks, and it will be
replaced with the corresponding title and linked:  e.g. ```autolabor```
=> `autolabor`. Scripts and plugins have link targets that match their names
created for you automatically.

If you want to link to a heading in your own page, you can specify it like this::

    `Heading text exactly as written`_

Note that the DFHack documentation is configured so that single backticks (with
no prefix or suffix) produce links to internal link targets, such as the
``autolabor`` target shown above. This is different from the reStructuredText
default behavior of rendering such text in italics (as a reference to a title).
For alternative link behaviors, see:

- `The reStructuredText documentation on roles <https://docutils.sourceforge.io/docs/ref/rst/roles.html>`__
- `The reStructuredText documentation on external links <https://docutils.sourceforge.io/docs/ref/rst/restructuredtext.html#hyperlink-targets>`__
- `The Sphinx documentation on roles <https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html>`__
    - ``:doc:`` is useful for linking to another document outside of DFHack.

.. _docs-standards:

Documentation standards
=======================

.. highlight:: rst

Whether you're adding new code or just fixing old documentation (and there's plenty),
there are a few important standards for completeness and consistent style.  Treat
this section as a guide rather than iron law, match the surrounding text, and you'll
be fine.

Where do I add the help text?
-----------------------------

For scripts and plugins that are distributed as part of DFHack, documentation files
should be added to the :source-scripts:`scripts/docs <docs>` and :source:`docs/plugins` directories,
respectively, in a file named after the script or plugin. For example, a script named
``gui/foobar.lua`` (which provides the ``gui/foobar`` command) should be documented
in a file named ``docs/gui/foobar.rst`` in the scripts repo. Similarly, a plugin named
``foobaz`` should be documented in a file named ``docs/plugins/foobaz.rst`` in the dfhack repo.
For plugins, all commands provided by that plugin should be documented in that same file.

Short descriptions (the ~54 character short help) for scripts and plugins are taken from
the ``summary`` attribute of the ``dfhack-tool`` directive that each tool help document must
have (see the `Header format`_ section below). Please make this brief but descriptive!

Short descriptions for commands provided by plugins are taken from the ``description``
parameter passed to the ``PluginCommand`` constructor used when the command is registered
in the plugin source file.

Header format
-------------

The docs **must** begin with a heading which exactly matches the script or plugin name, underlined
with ``=====`` to the same length. This must be followed by a ``.. dfhack-tool:`` directive with
at least the following parameters:

* ``:summary:`` - a short, single-sentence description of the tool
* ``:tags:`` - a space-separated list of `tags <tag-list>` that apply to the tool

By default, ``dfhack-tool`` generates both a description of a tool and a command
with the same name. For tools (specifically plugins) that do not provide exactly
1 command with the same name as the tool, pass the ``:no-command:`` parameter (with
no content after it) to prevent the command block from being generated.

For tools that provide multiple commands, or a command by the same name but with
significantly different functionality (e.g. a plugin that can be both enabled
and invoked as a command for different results), use the ``.. dfhack-command:``
directive for each command. This takes only a ``:summary:`` argument, with the
same meaning as above.

For example, documentation for the ``build-now`` script might look like::

    build-now
    =========

    .. dfhack-tool::
        :summary: Instantly completes unsuspended building construction jobs.
        :tags: fort armok buildings

    By default, all buildings on the map are completed, but the area of effect is configurable.

And documentation for the ``autodump`` plugin might look like::

    autodump
    ========

    .. dfhack-tool::
        :summary: Automatically set items in a stockpile to be dumped.
        :tags: fort armok fps productivity items stockpiles
        :no-command:

    .. dfhack-command:: autodump
        :summary: Teleports items marked for dumping to the cursor position.

    .. dfhack-command:: autodump-destroy-here
        :summary: Destroy items marked for dumping under the cursor.

    .. dfhack-command:: autodump-destroy-item
        :summary: Destroys the selected item.

    When `enabled <enable>`, this plugin adds an option to the :kbd:`q` menu for
    stockpiles.

    When invoked as a command, it can instantly move all unforbidden items marked
    for dumping to the tile under the cursor.

Usage help
----------

The first section after the header and introductory text should be the usage section. You can
choose between two formats, based on whatever is cleaner or clearer for your syntax. The first
option is to show usage formats together, with an explanation following the block::

    Usage
    -----

    ::

        build-now [<options>]
        build-now here [<options>]
        build-now [<pos> [<pos>]] [<options>]

    Where the optional ``<pos>`` pair can be used to specify the
    coordinate bounds within which ``build-now`` will operate. If
    they are not specified, ``build-now`` will scan the entire map.
    If only one ``<pos>`` is specified, only the building at that
    coordinate is built.

    The ``<pos>`` parameters can either be an ``<x>,<y>,<z>`` triple
    (e.g. ``35,12,150``) or the string ``here``, which means the
    position of the active game cursor.

The second option is to arrange the usage options in a list, with the full command
and arguments in monospaced font. Then indent the next line and describe the effect::

    Usage
    -----

    ``build-now [<options>]``
        Scan the entire map and build all unsuspended constructions
        and buildings.
    ``build-now here [<options>]``
        Build the unsuspended construction or building under the
        cursor.
    ``build-now [<pos> [<pos>]] [<options>]``
        Build all unsuspended constructions within the specified
        coordinate box.

    The ``<pos>`` parameters are specified as...

Note that in both options, the entire commandline syntax is written, including the command itself.
Literal text is written as-is (e.g. the word ``here`` in the above example), and text that
describes the kind of parameter that is being passed (e.g. ``pos`` or ``options``) is enclosed in
angle brackets (``<`` and ``>``). Optional elements are enclosed in square brackets (``[`` and ``]``).
If the command takes an arbitrary number of elements, use ``...``, for example::

    prioritize [<options>] <job type> [<job type> ...]
    quickfort <command>[,<command>...] <list_id>[,<list_id>...] [<options>]

Examples
--------

If the only way to run the command is to type the command itself, then this section is not necessary.
Otherwise, please consider adding a section that shows some real, practical usage examples. For
many users, this will be the **only** section they will read. It is so important that it is a good
idea to include the ``Examples`` section **before** you describe any extended options your command
might take. Write examples for what you expect the popular use cases will be. Also be sure to write
examples showing specific, practical values being used for any parameter that takes a value or has
tricky formatting.

Examples should go in their own subheading. The examples themselves should be organized as in
option 2 for Usage above. Here is an example ``Examples`` section::

    Examples
    --------

    ``build-now``
        Completes all unsuspended construction jobs on the map.
    ``build-now 37,20,154 here``
        Builds the unsuspended, unconstructed buildings in the box
        bounded by the coordinate x=37,y=20,z=154 and the cursor.

Options
-------

The options header should follow the examples, with each option in the same format as the
examples::

    Options
    -------

    ``-h``, ``--help``
        Show help text.
    ``-l``, ``--quality <level>``
        Set the quality of the architecture for built architected
        builtings.
    ``-q``, ``--quiet``
        Suppress informational output (error messages are still
        printed).

Note that for parameters that have both short and long forms, any values that those options
take only need to be specified once (e.g. ``<level>``).

External scripts and plugins
============================

Scripts and plugins distributed separately from DFHack's release packages don't have the
opportunity to add their documentation to the rendered HTML or text output. However, these
scripts and plugins can use a different mechanism to at least make their help text available
in-game.

Note that since help text for external scripts and plugins is not rendered by Sphinx,
it should be written in plain text. Any reStructuredText markup will not be processed and,
if present, will be shown verbatim to the player (which is probably not what you want).

For external scripts, the short description comes from a comment on the first line
(the comment marker and extra whitespace is stripped):

.. code-block:: lua

    -- A short description of my cool script.

The main help text for an external script needs to appear between two markers -- ``[====[``
and ``]====]``. The documentation standards above still apply to external tools, but there is
no need to include backticks for links or monospaced fonts. Here is an example for an
entire script header::

    -- Inventory management for adventurers.
    -- [====[
    gui/adv-inventory
    =================

    Tags: adventure | items

    Allows you to quickly move items between containers. This
    includes yourself and any followers you have.

    Usage
    -----

        gui/adv-inventory [<options>]

    Examples
    --------

    gui/adv-inventory
        Opens the GUI with nothing preselected

    gui/adv-inventory take-all
        Opens the GUI with all container items already selected and
        ready to move into the adventurer's inventory.

    Options
    -------

    take-all
        Starts the GUI with container items pre-selected

    give-all
        Starts the GUI with your own items pre-selected
    ]====]

For external plugins, help text for provided commands can be passed as the ``usage``
parameter when registering the commands with the ``PluginCommand`` constructor. There
is currently no way for associating help text with the plugin itself, so any
information about what the plugin does when enabled should be combined into the command
help.

Required dependencies
=====================

.. highlight:: shell

In order to build the documentation, you must have Python with Sphinx
version |sphinx_min_version| or later and Python 3.

When installing Sphinx from OS package managers, be aware that there is
another program called "Sphinx", completely unrelated to documentation management.
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
installing Python 3. On Debian-based distros::

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

Windows
-------
Python for Windows can be downloaded `from python.org <https://www.python.org/downloads/>`_.
The latest version of Python 3 includes pip already.

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

See our page on `build options <building-documentation>`

Running Sphinx manually
-----------------------

You can also build the documentation without running CMake - this is faster if
you only want to rebuild the documentation regardless of any code changes. The
``docs/build.py`` script will build the documentation in any specified formats
(HTML only by default) using the same command that CMake runs when building the
docs. Run the script with ``--help`` to see additional options.

Examples:

* ``docs/build.py``
    Build just the HTML docs

* ``docs/build.py html text``
    Build both the HTML and text docs

* ``docs/build.py --clean``
    Build HTML and force a clean build (all source files are re-read)

The resulting documentation will be stored in ``docs/html`` and/or ``docs/text``.

Alternatively, you can run Sphinx manually with::

    sphinx-build . docs/html

or, to build plain-text output::

    sphinx-build -b text . docs/text

Sphinx has many options to enable clean builds, parallel builds, logging, and
more - run ``sphinx-build --help`` for details. If you specify a different
output path, be warned that Sphinx may overwrite existing files in the output
folder. Also be aware that when running ``sphinx-build`` directly, the
``docs/html`` folder may be polluted with intermediate build files that normally
get written in the cmake ``build`` directory.

Building a PDF version
----------------------

ReadTheDocs automatically builds a PDF version of the documentation (available
under the "Downloads" section when clicking on the release selector). If you
want to build a PDF version locally, you will need ``pdflatex``, which is part
of a TeX distribution. The following command will then build a PDF, located in
``docs/pdf/latex/DFHack.pdf``, with default options::

  docs/build.py pdf

Alternatively, you can run Sphinx manually with::

  sphinx-build -M latexpdf . docs/pdf

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

Building the changelogs generates two files: ``docs/changelogs/news.rst`` and
``docs/changelogs/news-dev.rst``. These correspond to `changelog` and
`dev-changelog` and contain changes organized by stable and development DFHack
releases, respectively. For example, an entry listed under "0.44.05-alpha1" in
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
