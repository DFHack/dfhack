.. _quickstart:

Quickstart guide
================

Welcome to DFHack! This guide will help get you oriented with the DFHack system
and teach you how to find and use the tools productively. If you're reading this
in `dfhack-quickstart-guide`, hit the right arrow key or click on the hotkey
hint in the lower right corner of the window to go to the next page.

What is DFHack?
---------------

DFHack is a framework for Dwarf Fortress that provides a unified, cross-platform
environment that enables mods and tools to significantly extend the game. The
default DFHack distribution contains a variety of tools, including bugfixes,
interface improvements, automation agents, design blueprints, modding tools, and
more. Third-party tools (e.g. mods downloaded from Steam Workshop or the forums)
can also seamlessly integrate with the DFHack framework and extend the game far
beyond what can be done by just modding the raws.

DFHack's mission is to provide tools and interfaces for players and modders to:
- expand the bounds of what is possible in Dwarf Fortress
- reduce the impact of game bugs
- give the player more agency and control over the game
- provide alternatives to toilsome or frustrating aspects of gameplay
- overall, make the game more fun

What can I do with DFHack tools?
--------------------------------

DFHack has been around for a long time -- almost as long as Dwarf Fortress
itself. Many of the game's rough edges have been smoothed with DFHack tools.
Here are some common tasks people use DFHack tools to accomplish:

- Automatically chop trees when log stocks are low
- Record blueprint files that allow copy and paste of fort designs
- Import and export lists of manager orders
- Clean contaminants from map squares that dwarves can't reach
- Automatically butcher excess livestock so you don't become overrun with
  animals
- Promote time-sensitive job types (e.g. food hauling) so they are done
  expediently
- Quickly scan the map for visible ores of specific types so you can focus
  your mining

Some tools are one-shot commands. For example, you can run `unforbid all <unforbid>`
to claim all items on the map after a messy siege.

Other tools must be `enabled <enable>` and then they will run in the background.
For example, `enable seedwatch <seedwatch>` will start monitoring your stocks of
seeds and preventing your chefs from cooking seeds that you need for planting.
Enableable tools that affect a fort save their state with the fort and will
remember that they are enabled the next time you load your save.

And still other tools add information to the screen or provide new integrated
functionality via the DFHack `overlay` framework. For example, the `unsuspend`
tool, in addition to its basic function of unsuspending all building construction
jobs, can also overlay a marker on suspended buildings to indicate that they are
suspended (and will use different markers to tell you whether this is a problem).
These overlays can be configured with the `gui/overlay` tool.

How can I figure out which commands to run?
-------------------------------------------

There are several ways to scan DFHack tools and find the one you need right now.

The first place to check is the DFHack logo hover hotspot. It's in the upper
left corner of the screen by default, though you can move it anywhere you want
with the `gui/overlay` tool.

When you hover the mouse over the logo (or hit the :kbd:`Ctrl`:kbd:`Shift`:kbd:`C`
keyboard shortcut) a list of DFHack tools relevant to the current context comes up.
For example, when you have a unit selected, the hotspot will show a list of tools
that inspect units, allow you to edit them, or maybe even teleport them. Next to
each tool, you'll see the global hotkey you can hit to invoke the command without
even opening the hover list.

You can run any DFHack tool from `gui/launcher`, which is always listed first in
the hover list. You can also bring up the launcher by tapping the backtick key
(\`) or hitting :kbd:`Ctrl`:kbd:`Shift`:kbd:`D`. In the launcher, you can quickly
autocomplete any command name by selecting it in the list on the right side of
the window. Commands are ordered by how often you run them, so your favorite
commands will always be on top. You can also pull full commandlines out of your
history with :kbd:`Alt`:kbd:`S` (or by clicking on the "history search" hotkey hint).

Once you have typed (or autocompleted, or searched for) a command name, other
commands related to the one you have selected will appear in the autocomplete list.
Scanning through the related tools is a great way to learn about new tools that
you might find useful. You can also see how commands are grouped by running the
`tags` command.

The bottom panel will show the full help text for the command you are running,
allowing you to refer to the usage documentation and examples when you are typing
your command.

How do DFHack in-game windows work?
-----------------------------------

Many DFHack tools have graphical interfaces that appear in-game. You can tell
which windows belong to DFHack tools because they will have the word "DFHack"
printed across their bottom frame. DFHack provides a custom windowing system
that gives the player a lot of control over where the windows appear and whether
they capture keyboard and mouse input.

The DFHack windowing system allows you to use DFHack tools without interrupting
the game. That is, if the game is unpaused, it will continue to run while a
DFHack window is open. You can also interact with the map, scrolling it with the
keyboard or mouse and selecting units, buildings, and items. Some tools will
force-pause the game if it makes sense to, like `gui/quickfort`, since you cannot
interact with the map normally while placing a blueprint.

DFHack windows are draggable from the title bar or from anywhere on the window
that doesn't have a mouse-clickable widget on it. Many are resizable as well
(if the tool window has components that can be reasonably resized).

DFHack windows close with a right mouse click or keyboard :kbd:`Esc`, but if you
want to keep a DFHack tool open while you interact with the game, you can click the
pin in the upper right corner of the DFHack window or hit :kbd:`Alt`:kbd:`L` so
that the pin turns green. The DFHack window will then ignore right clicks and
:kbd:`Esc` key presses that would otherwise close the window. This is especially
useful for the configuration tool windows for the automation tools. For example,
you can pin the `gui/autochop` window and let it sit there monitoring your
logging industry as you play, using it as a live status window. Note that you can
still right click *on* the DFHack tool window to close it, even when it is pinned.

You can have multiple DFHack tool windows on the screen at the same time. The
one that is receiving keyboard input has a highlighted title bar and will appear
over other windows if dragged over them. Clicking on a DFHack window that is not
currently active will bring it to the foreground and make it the active window.

Where do I go next?
-------------------

To recap:

You can get to popular, relevant tools for the current context by hovering
the mouse over DFHack logo or by hitting :kbd:`Ctrl`:kbd:`Shift`:kbd:`C`.

You can get to the launcher and its integrated autocomplete, history search,
and help text by hitting backtick (\`) or :kbd:`Ctrl`:kbd:`Shift`:kbd:`D`,
or, of course, by running it from the logo hover list.

You can list and start tools that run in the background with the `enable`
command.

You can configure screen overlays with the `gui/overlay` tool.

With those four tools, you have the complete DFHack tool suite at your
fingertips. So what to run first? Here are a few commands to get you started.
You can run them from the launcher.

First, let's import some useful manager orders to keep your fort stocked with
basic necessities. Run ``orders import library/basic``. If you go to your
mangager orders screen, you can see all the orders that have been created for you.

Next, try setting up `autochop` by running the GUI configuration `gui/autochop`.
You can enable it from the GUI, so you don't need to run `enable autochop <enable>`
directly. You can set a target number of logs, and let autochop will manage
your logging industry for you. You can control where your woodsdwarves go to
cut down trees by setting up burrows and configuring autochop to only cut in
those burrows.

Finally, let's set up a water supply for your fort with `gui/quickfort`. Launching
`gui/quickfort` will give you a list of blueprints you can load. Type in ``aquifer_tap``
to filter for just those blueprints. Select the ``aquifer_tap -n /help`` blueprint
to see the instructions for how to build an aquifer tap. Then, go back and load the
``aquifer_tap -n /dig`` blueprint, find some space in a light aquifer layer, and
apply the blueprint there. It was that easy!

There are many more tools to explore. Have fun!
