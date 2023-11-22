#### Q: How do I download DFHack?
**A:** Either add to your Steam library from our [Steam page](https://store.steampowered.com/app/2346660/DFHack) or scroll to the latest release on our [GitHub releases page](https://github.com/DFHack/dfhack/releases), expand the "Assets" list, and download the file for your platform (e.g. `dfhack-XX.XX-rX-Windows-64bit.zip`. If you are on Windows and are manually installing from the zip file, please remember to right click on the file after downloading, open the file properties, and select the "Unblock" checkbox. This will prevent issues with Windows antivirus programs.

-------------

This release is compatible with all distributions of Dwarf Fortress: [Steam](https://store.steampowered.com/app/975370/Dwarf_Fortress/), [Itch](https://kitfoxgames.itch.io/dwarf-fortress), and [Classic](https://www.bay12games.com/dwarves/).

- [Install DFHack from Steam](https://store.steampowered.com/app/2346660/DFHack)
- [Manual install](https://docs.dfhack.org/en/stable/docs/Installing.html#installing)
- [Quickstart guide (for players)](https://docs.dfhack.org/en/stable/docs/Quickstart.html#quickstart)
- [Modding guide (for modders)](https://docs.dfhack.org/en/stable/docs/guides/modding-guide.html)

Please report any issues (or feature requests) on the DFHack [GitHub issue tracker](https://github.com/DFHack/dfhack/issues). When reporting issues, please upload a zip file of your savegame and a zip file of your `mods` directory to the cloud and add links to the GitHub issue. Make sure your files are downloadable by "everyone with the link". We need your savegame to reproduce the problem and test the fix, and we need your active mods so we can load your savegame. Issues with savegames and mods attached get fixed first!

Highlights
----------------------------------

<details>
<summary>Highlight 1, Highlight 2</summary>

### Highlight 1

Demo screenshot/vidcap

Text

### Highlight 2

Demo screenshot/vidcap

Text

</details>

Announcements
----------------------------------

<details>
<summary>Annc 1, PSAs</summary>

### Annc 1

Text

### PSAs

As always, remember that, just like the vanilla DF game, DFHack tools can also have bugs. It is a good idea to **save often and keep backups** of the forts that you care about.

Many DFHack tools that worked in previous (pre-Steam) versions of DF have not been updated yet and are marked with the "unavailable" tag in their docs. If you try to run them, they will show a warning and exit immediately. You can run the command again to override the warning (though of course the tools may not work). We make no guarantees of reliability for the tools that are marked as "unavailable".

The in-game interface for running DFHack commands (`gui/launcher`) will not show "unavailable" tools by default. You can still run them if you know their names, or you can turn on dev mode by hitting Ctrl-D while in `gui/launcher` and they will be added to the autocomplete list. Some tools do not compile yet and are not available at all, even when in dev mode.

If you see a tool complaining about the lack of a cursor, know that it's referring to the **keyboard** cursor (which used to be the only real option in Dwarf Fortress). You can enable the keyboard cursor by entering mining mode or selecting the dump/forbid tool and hitting Alt-K (the DFHack keybinding for `toggle-kbd-cursor`). We're working on making DFHack tools more mouse-aware and accessible so this step isn't necessary in the future.

</details>

Changelog
====================

<details>
<summary>New tools, fixes, and improvements</summary>

%RELEASE_NOTES%
</details>
