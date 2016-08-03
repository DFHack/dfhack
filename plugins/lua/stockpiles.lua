local _ENV = mkmodule('plugins.stockpiles')

--[[

 Native functions:

 * stockpiles_list_settings(dir_path), list files in directory
 * stockpiles_load(file), with full path
 * stockpiles_save(file), with full path
 * isEnabled()

--]]
--
function safe_require(module)
    local status, module = pcall(require, module)
    return status and module or nil
end


local gui = require 'gui'
local widgets = require('gui.widgets')
local dlg = require('gui.dialogs')
local script = require 'gui.script'
local persist = safe_require('persist-table')


function ListFilterDialog(args)
    args.text = args.prompt or 'Type or select an option'
    args.text_pen = COLOR_WHITE
    args.with_filter = true
    args.icon_width = 2

    local choices = {}

    if not args.hide_none then
        table.insert(choices, {
            icon = '?', text = args.none_caption or 'none',
            index = -1, name = -1
        })
    end

    local filter = args.item_filter

    for i,v in ipairs(args.items) do
        if not filter or filter(v,-1) then
            local name = v
            local icon
            table.insert(choices, {
                icon = icon, search_key = string.lower(name), text = name, index = i
            })
        end
    end

    args.choices = choices

    if args.on_select then
        local cb = args.on_select
        args.on_select = function(idx, obj)
            return cb(obj.index, args.items[obj.index])
        end
    end

    return dlg.ListBox(args)
end

function showFilterPrompt(title, list, text,item_filter,hide_none)
    ListFilterDialog{
        frame_title=title,
        items=list,
        prompt=text,
        item_filter=item_filter,
        hide_none=hide_none,
        on_select=script.mkresume(true),
        on_cancel=script.mkresume(false),
        on_close=script.qresume(nil)
    }:show()

    return script.wait()
end

function init()
    if persist == nil then return end
    if dfhack.isMapLoaded() then
        if persist.GlobalTable.stockpiles == nil then
            persist.GlobalTable.stockpiles = {}
            persist.GlobalTable.stockpiles['settings_path'] = './stocksettings'
        end
    end
end

function tablify(iterableObject)
    t={}
    for k,v in ipairs(iterableObject) do
        t[k] = v~=nil and v or 'nil'
    end
    return t
end

local filename_invalid_regex = '[^A-Za-z0-9 ._-]'

function valid_filename(filename)
    return not filename:match(filename_invalid_regex)
end

function sanitize_filename(filename)
    local ret = ''
    for i = 1, #filename do
        local ch = filename:sub(i, i)
        if valid_filename(ch) then
            ret = ret .. ch
        else
            ret = ret .. '-'
        end
    end
    return ret
end

FilenameInputBox = defclass(FilenameInputBox, dlg.InputBox)
function FilenameInputBox:onInput(keys)
    if not valid_filename(string.char(keys._STRING or 0)) and not keys.STRING_A000 then
        keys._STRING = nil
    end
    FilenameInputBox.super.onInput(self, keys)
end

function showFilenameInputPrompt(title, text, tcolor, input, min_width)
    FilenameInputBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        input = input,
        frame_width = min_width,
        on_input = script.mkresume(true),
        on_cancel = script.mkresume(false),
        on_close = script.qresume(nil)
    }:show()

    return script.wait()
end

function load_settings()
    init()
    local path = get_path()
    local ok, list = pcall(stockpiles_list_settings, path)
    if not ok then
        show_message_box("Stockpile Settings", "The stockpile settings folder doesn't exist.", true)
        return
    end
    if #list == 0 then
        show_message_box("Stockpile Settings", "There are no saved stockpile settings.", true)
        return
    end

    local choice_list = {}
    for i,v in ipairs(list) do
        choice_list[i] = string.gsub(v, "/", "/ ")
        choice_list[i] = string.gsub(choice_list[i], "-", " - ")
        choice_list[i] = string.gsub(choice_list[i], "_", " ")
    end

    script.start(function()
        local ok2,index,name=showFilterPrompt('Stockpile Settings', choice_list, 'Choose a stockpile', function(item) return true end, true)
        if ok2 then
            local filename = list[index];
            stockpiles_load(path..'/'..filename)
        end
    end)
end

function save_settings(stockpile)
    init()
    script.start(function()
        local suggested = stockpile.name
        if #suggested == 0 then
            suggested = 'Stock1'
        end
        suggested = sanitize_filename(suggested)
        local path = get_path()
        local sok,filename = showFilenameInputPrompt('Stockpile Settings', 'Enter filename', COLOR_WHITE, suggested)
        if sok then
            if filename == nil or filename == '' or not valid_filename(filename) then
                script.showMessage('Stockpile Settings', 'Invalid File Name', COLOR_RED)
            else
                if not dfhack.filesystem.exists(path) then
                    dfhack.filesystem.mkdir(path)
                end
                stockpiles_save(path..'/'..filename)
            end
        end
    end)
end

function manage_settings(sp)
    init()
    if not guard() then return false end
    script.start(function()
        local list = {'Load', 'Save'}
        local tok,i = script.showListPrompt('Stockpile Settings','Load or Save Settings?',COLOR_WHITE,tablify(list))
        if tok then
            if i == 1 then
                load_settings()
            else
                save_settings(sp)
            end
        end
    end)
end

function show_message_box(title, msg, iserror)
    local color = COLOR_WHITE
    if iserror then
        color = COLOR_RED
    end
    script.start(function()
        script.showMessage(title, msg, color)
    end)
end

function guard()
    if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some/Stockpile') then
        qerror("This script requires a stockpile selected in the 'q' mode")
        return false
    end
    return true
end

function set_path(path)
    init()
    if persist == nil then
        qerror("This version of DFHack doesn't support setting the stockpile settings path. Sorry.")
        return
    end
    persist.GlobalTable.stockpiles['settings_path'] = path
end

function get_path()
    init()
    if persist == nil then
        return "stocksettings"
    end
    return persist.GlobalTable.stockpiles['settings_path']
end

return _ENV
