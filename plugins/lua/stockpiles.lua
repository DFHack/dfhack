local _ENV = mkmodule('plugins.stockpiles')

--[[

 Native functions:

 * stockpiles_list_settings(dir_path), list files in directory
 * stockpiles_load(file), with full path
 * stockpiles_save(file), with full path

--]]

local gui = require 'gui'
local script = require 'gui.script'
local persist = require 'persist-table'

function init()
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
    script.start(function()
        local path = persist.GlobalTable.stockpiles['settings_path']
        local list = stockpiles_list_settings(path)
        local tok,i = script.showListPrompt('Stockpile Settings','Load which stockpile?',COLOR_WHITE,tablify(list))
        if tok then
            local filename = list[i];
            stockpiles_load(path..'/'..filename);
        end
    end)
end

function save_settings(stockpile)
    init()
    script.start(function()
        --local sp = dfhack.gui.geSelectedBuilding(true)
        local suggested = stockpile.name
        if #suggested == 0 then
            suggested = 'Stock1'
        end
        local path = persist.GlobalTable.stockpiles['settings_path']
        local sok,filename = script.showInputPrompt('Stockpile Settings', 'Enter stockpile name', COLOR_WHITE, suggested)
        if sok then
            stockpiles_save(path..'/'..filename);
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

function guard()
    if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some/Stockpile') then
        qerror("This script requires a stockpile selected in the 'q' mode")
        return false
    end
    return true
end

function set_path(path)
    init()
    persist.GlobalTable.stockpiles['settings_path'] = path
end

function get_path()
    init()
    return persist.GlobalTable.stockpiles['settings_path']
end

return _ENV
