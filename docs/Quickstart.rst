.. _quickstart:

Quickstart guide
================

Welcome to DFHack! This guide will help get you oriented with the DFHack system
and teach you how to find and use the tools productively. If you're reading this
in the in-game `quickstart-guide` reader, hit the right arrow key or click on
the hotkey hint in the lower right corner of the window to go to the next page.

What is DFHack?
---------------

DFHack is an add-on for Dwarf Fortress that enables mods and tools to
significantly extend the game. The default DFHack distribution contains a wide
variety of tools, including bugfixes, interface improvements, automation agents,
design blueprints, modding building blocks, and more. Third-party tools (e.g.
mods downloaded from Steam Workshop or the forums) can also seamlessly integrate
with the DFHack framework and extend the game far beyond what can be done by
just modding the raws.

DFHack's mission is to provide tools and interfaces for players and modders to:

- expand the bounds of what is possible in Dwarf Fortress
- reduce the impact of game bugs
- give the player more agency and control over the game
- provide alternatives to toilsome or frustrating aspects of gameplay
- **make the game more fun**

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
  your mining efforts

Some tools are one-shot commands. For example, you can run
`unforbid all <unforbid>` to claim all (reachable) items on the map after a
messy siege.

Other tools must be `enabled <enable>` once and then they will run in the
background. For example, once enabled, `seedwatch` will start monitoring your
stocks of seeds and prevent your chefs from cooking seeds that you need for
planting. Tools that are enabled in the context of a fort will save their state
with that fort, and they will remember that they are enabled the next time you
load your save.

A third class of tools adds information to the screen or provides new integrated
functionality via the DFHack `overlay` framework. For example, the `sort` tool
adds widgets to the squad member selection screen that allow you to search,
sort, and filter the list of military candidates. You don't have to run any
command to get the benefits of the tool, it appears automatically when you're
on the relevant screen.

How can I figure out which commands to run?
-------------------------------------------

There are several ways to scan DFHack tools and find the ones you need right
now.

The first place to check is the DFHack logo menu. It's in the upper left corner
of the screen by default, though you can move it anywhere you want with the
`gui/overlay` configuration UI.

When you click on the logo (or hit the Ctrl-Shift-C keyboard shortcut), a short
list of popular, relevant DFHack tools comes up. These are the tools that have
been assigned hotkeys that are active in the current context. For example, when
you're looking at a fort map, the list will contain fortress design tools like
`gui/quickfort` and `gui/design`. You can click on the tools in the list, or
note the hotkeys listed next to them and maybe use them to launch the tool next
time without even opening the logo menu.

The second place to check is the DFHack control panel: `gui/control-panel`. It
will give you an overview of which tools are currently enabled, and will allow
you to toggle them on or off, see help text for them, or launch their dedicated
configuration UIs. You can open the control panel from anywhere with the
Ctrl-Shift-E hotkey or by selecting it from the logo menu list.

In the control panel, you can also select which tools you'd like to be
automatically enabled and popular commands you'd like to run when you start a
new fort. On the "Preferences" tab, there are settings you can change, like
whether you want to limit DFHack functionality to interface improvements,
bugfixes, and productivity tools, hiding the god-mode tools ("mortal mode") or
whether you want DFHack windows to pause the game when they come up.

Finally, you can explore the full extent of the DFHack catalog in
`gui/launcher`, which is always listed first in the DFHack logo menu list. You
can also bring up the launcher by tapping the backtick key (\`) or hitting
Ctrl-Shift-D. In the launcher, you can quickly autocomplete any command name by
selecting it in the list on the right side of the window. Commands are ordered
by how often you run them, so your favorite commands will always be on top. You
can also pull full commandlines out of your history with Alt-S or by clicking
on the "history search" hotkey hint.

Once you have typed (or autocompleted, or searched for) a command, other
commands related to the one you have selected will appear in the right-hand
panel. Scanning through that list is a great way to learn about new tools that
you might find useful. You can also see how commands are grouped by running the
`tags` command.

The bottom panel will show the full help text for the command you are running,
allowing you to refer to the usage documentation and examples when you are
typing your command. After you run a command, the bottom panel switches to
command output mode, but you can get back to the help text by hitting Ctrl-T or
clicking on the ``Help`` tab.

How do DFHack in-game windows work?
-----------------------------------

Many DFHack tools have graphical interfaces that appear in-game. You can tell
which windows belong to DFHack tools because they will have the word "DFHack"
printed across their bottom frame edge. DFHack provides an advanced windowing
system that gives the player a lot of control over where the windows appear and
whether they capture keyboard and mouse input.

The DFHack windowing system allows multiple overlapping windows to be active at
once. The one with the highlighted title bar has focus and will receive anything
you type at the keyboard. Hit Esc or right click to close the window or cancel
the current action. You can click anywhere on the screen that is not a DFHack
window to unfocus the window and let it just sit in the background. It won't
respond to key presses or mouse clicks until you click on it again to give it
focus. If no DFHack windows are focused, you can right click directly on a
window to close it without left clicking to focus it first.

DFHack windows are draggable from the title bar or from anywhere on the window
that doesn't have a mouse-clickable widget on it. Many are resizable as well
(if the tool window has components that can reasonably be resized).

You can generally use DFHack tools without interrupting the game. That is, if
the game is unpaused, it can continue to run while a DFHack window is open. If
configured to do so in `gui/control-panel`, tools will initially pause the game
to let you focus on the task at hand, but you can unpause like normal if you
want. You can also interact with the map, scrolling it with the keyboard or
mouse and selecting units, buildings, and items. Some tools will intercept all
mouse clicks to allow you to select regions of the map. When these tools have
focus, you will not be able to use the mouse to interact with map elements or
pause/unpause the game. Therefore, these tools will pause the game when they
open, regardless of your settings in `gui/control-panel`. You can still unpause
with the keyboard (spacebar by default), though.

Where do I go next?
-------------------

To recap:

You can get to popular, relevant tools for the current context by clicking on
the DFHack logo or by hitting Ctrl-Shift-C.

You can enable DFHack tools and configure settings with `gui/control-panel`,
which you can open from the DFHack logo or access directly with the
Ctrl-Shift-E hotkey.

You can get to the launcher and its integrated autocomplete, history search,
and help text by hitting backtick (\`) or Ctrl-Shift-D, or, of course, by
running it from the logo menu list.

With those three interfaces, you have the complete DFHack tool suite at your
fingertips. So what to run first? Here are a few examples to get you started.

First, let's import some useful manager orders to keep your fort stocked with
basic necessities. Run ``orders import library/basic``. If you go to your
manager orders screen, you can see all the orders that have been created for
you. Note that you could have imported the orders directly from this screen as
well, using the DFHack `overlay` widget at the bottom of the manager orders
panel.

Next, try setting up `autochop` to automatically designate trees for chopping
when you get low on usable logs. Run `gui/control-panel` and select
``autochop`` in the ``Fort`` list. Click on the button to the left of the name
or hit Enter to enable it. You can then click on the configure button (the gear
icon) to launch `gui/autochop` if you'd like to customize its settings. If you
have the extra screen space, you can go ahead and set the `gui/autochop` window
to minimal mode (click on the hint near the upper right corner of the window or
hit Alt-M) and click on the map so the window loses keyboard focus. As you play
the game, you can glance at the live status panel to check on your stocks of
wood.

Finally, let's do some fort design copy-pasting. Go to some bedrooms that you
have set up in your fort. Run `gui/blueprint`, set a name for your blueprint by
clicking on the name field (or hitting the 'n' hotkey), typing "rooms" (or
whatever) and hitting Enter to set. Then draw a box around the target area by
clicking with the mouse. When you select the second corner, the blueprint will
be saved to your ``dfhack-config/blueprints`` subfolder.

Now open up `gui/quickfort`. You can search for the blueprint you just created
by typing its name, but it should be up near the top already. If you copied a
dug-out area with furniture in it, your blueprint will have two labels: "/dig"
and "/build". Click on the "/dig" blueprint or select it with the keyboard
arrow keys and hit Enter. You can rotate or flip the blueprint around if you
need to with the transform hotkeys. You'll see a preview of where the blueprint
will be applied as you move the mouse cursor around the map. Red outlines mean
that the blueprint may fail to fully apply at that location, so be sure to
choose a spot where all the preview tiles are shown with green diamonds. Click
the mouse or hit Enter to apply the blueprint and designate the tiles for
digging. Your dwarves will come and dig it out as if you had designated the
tiles yourself.

Once the area is dug out, run `gui/quickfort` again and select your "/build"
blueprint this time. Hit ``o`` to generate manager orders for the required
furniture. Apply the blueprint in the dug-out area, and your furniture will be
designated. It's just that easy! Note that `quickfort` uses `buildingplan` to
place buildings, so you don't even need to have the relevant furniture or
building materials in stock yet. The planned furniture/buildings will get built
whenever you are able to produce the building materials.

There are many, many more tools to explore. Have fun!
