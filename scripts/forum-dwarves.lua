-- scripts/forum-dwarves.lua
-- Save a copy of a text screen for the DF forums.  Use 'forumdwarves help' for more details.
-- original author: Caldfir
-- edited by expwnent, Mchl
--[[=begin

forum-dwarves
=============
Saves a copy of a text screen, formatted in bbcode for posting to the Bay12 Forums.
Use ``forum-dwarves help`` for more information.

=end]]

local args = {...}

if args[1] == 'help' then
 print([[
description:
    This script will attempt to read the current df-screen, and if it is a
    text-viewscreen (such as the dwarf 'thoughts' screen or an item
    'description') then append a marked-up version of this text to the
    target file. Previous entries in the file are not overwritten, so you
    may use the 'forumdwarves' command multiple times to create a single
    document containing the text from multiple screens (eg: text screens
    from several dwarves, or text screens from multiple artifacts/items,
    or some combination).
known screens:
    The screens which have been tested and known to function properly with
    this script are:
        1: dwarf/unit 'thoughts' screen
        2: item/art 'description' screen
        3: individual 'historical item/figure' screens
    There may be other screens to which the script applies.  It should be
    safe to attempt running the script with any screen active, with an
    error message to inform you when the selected screen is not appropriate
    for this script.
target file:
    The target file's name is 'forumdwarves.txt'.  A remider to this effect
    will be displayed if the script is successful.
character encoding:
    The text will likely be using system-default encoding, and as such
    will likely NOT display special characters (eg:È,ı,Á) correctly.  To
    fix this, you need to modify the character set that you are reading
    the document with.  'Notepad++' is a freely available program which
    can do this using the following steps:
        1: open the document in Notepad++
        2: in the menu-bar, select
            Encoding->Character Sets->Western European->OEM-US
        3: copy the text normally to wherever you want to use it
]])
 return
end
local utils = require 'utils'
local gui = require 'gui'
local dialog = require 'gui.dialogs'
local colors_css = {
 [0] = 'black',
 [1] = 'navy',
 [2] = 'green',
 [3] = 'teal',
 [4] = 'maroon',
 [5] = 'purple',
 [6] = 'olive',
 [7] = 'silver',
 [8] = 'gray',
 [9] = 'blue',
 [10] = 'lime',
 [11] = 'cyan',
 [12] = 'red',
 [13] = 'magenta',
 [14] = 'yellow',
 [15] = 'white'
}

local scrn = dfhack.gui.getCurViewscreen()
local flerb = dfhack.gui.getFocusString(scrn)

local function format_for_forum(strin)
 local strout = strin

 local newline_idx = string.find(strout, '[P]', 1, true)
 while newline_idx ~= nil do
  strout = string.sub(strout,1, newline_idx-1)..'\n'..string.sub(strout,newline_idx+3)
  newline_idx = string.find(strout, '[P]', 1, true)
 end

 newline_idx = string.find(strout, '[B]', 1, true)
 while newline_idx ~= nil do
  strout = string.sub(strout,1, newline_idx-1)..'\n'..string.sub(strout,newline_idx+3)
  newline_idx = string.find(strout, '[B]', 1, true)
 end

 newline_idx = string.find(strout, '[R]', 1, true)
 while newline_idx ~= nil do
  strout = string.sub(strout,1, newline_idx-1)..'\n'..string.sub(strout,newline_idx+3)
  newline_idx = string.find(strout, '[R]', 1, true)
 end

 local color_idx = string.find(strout, '[C:', 1, true)
 while color_idx ~= nil do
  local colormatch = (string.byte(strout, color_idx+3)-48)+((string.byte(strout, color_idx+7)-48)*8)
  strout = string.sub(strout,1, color_idx-1)..'[/color][color='..colors_css[colormatch]..']'..string.sub(strout,color_idx+9)
  color_idx = string.find(strout, '[C:', 1, true)
 end

 return strout
end

if flerb == 'textviewer' then
 print(scrn)
 printall(scrn)
 local lines = scrn.src_text
 local line = ""

 if lines ~= nil then
  local log = io.open('forumdwarves.txt', 'a')
  log:write("[color=silver]")
  log:write(scrn.title)
  for n,x in ipairs(lines) do
   print(x)
   printall(x)
   print(x.value)
   printall(x.value)
   if (x ~= nil) and (x.value ~= nil) then
    log:write(format_for_forum(x.value), ' ')
    --log:write(x[0],'\n')
   end
  end
 log:write("[/color]\n")
 log:close()
 end
 print 'data prepared for forum in \"forumdwarves.txt\"'
else
 print 'this is not a textview screen'
end
