local _ENV = mkmodule('plugins.stockpiles')

--[[

 Native functions:

 * stockpiles_list_settings(dir_path), list files in directory
 * stockpiles_load(file), with full path
 * stockpiles_save(file), with full path

--]]
--
function safe_require(module)
    local status, module = pcall(require, module)
    return status and module or nil
end


local gui = require 'gui'
local script = require 'gui.script'
local persist = safe_require('persist-table')

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
    script.start(function()
        local tok,i = script.showListPrompt('Stockpile Settings','Load which stockpile?',COLOR_WHITE,tablify(list))
        if tok then
            local filename = list[i];
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
        local path = get_path()
        local sok,filename = script.showInputPrompt('Stockpile Settings', 'Enter stockpile name', COLOR_WHITE, suggested)
        if sok then
            if filename == nil or filename == '' then
                script.showMessage('Stockpile Settings', 'Invalid File Name', COLOR_RED)
            else
                print("saving...", path..'/'..filename)
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
