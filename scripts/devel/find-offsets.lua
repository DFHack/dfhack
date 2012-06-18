-- Find some offsets for linux.

local utils = require 'utils'
local ms = require 'memscan'

local is_known = dfhack.internal.getAddress

local force_scan = {}
for _,v in ipairs({...}) do
    force_scan[v] = true
end

collectgarbage()

print[[
WARNING: THIS SCRIPT IS STRICTLY FOR DFHACK DEVELOPERS.

Running this script on a new DF version will NOT
MAKE IT RUN CORRECTLY if any data structures
changed, thus possibly leading to CRASHES AND/OR
PERMANENT SAVE CORRUPTION.

This script should be initially started immediately
after loading the game, WITHOUT first loading a world.
It expects vanilla game configuration, without any
custom tilesets or init file changes.
]]

if not utils.prompt_yes_no('Proceed?') then
    return
end

-- Data segment location

local data = ms.get_data_segment()
if not data then
    error('Could not find data segment')
end

print('\nData section: '..tostring(data))
if data.size < 5000000 then
    error('Data segment too short.')
end

local searcher = ms.DiffSearcher.new(data)

local function validate_offset(name,validator,addr,tname,...)
    local obj = data:object_by_field(addr,tname,...)
    if obj and not validator(obj) then
        obj = nil
    end
    ms.found_offset(name,obj)
end

local function exec_finder(finder, names)
    if type(names) ~= 'table' then
        names = { names }
    end
    local search = force_scan['all']
    for _,v in ipairs(names) do
        if force_scan[v] or not is_known(v) then
            search = true
        end
    end
    if search then
        if not dfhack.safecall(finder) then
            if not utils.prompt_yes_no('Proceed with the rest of the script?') then
                searcher:reset()
                error('Quit')
            end
        end
    else
        print('Already known: '..table.concat(names,', '))
    end
end

local ordinal_names = {
    [0] = '1st entry',
    [1] = '2nd entry'
}
setmetatable(ordinal_names, {
    __index = function(self,idx) return (idx+1)..'th entry' end
})

local function list_index_choices(length_func)
    return function(id)
        if id > 0 then
            local ok, len = pcall(length_func)
            if not ok then
                len = 5
            elseif len > 10 then
                len = 10
            end
            return id % len
        else
            return 0
        end
    end
end

--
-- Cursor group
--

local function find_cursor()
    print('\nPlease navigate to the title screen to find cursor.')
    if not utils.prompt_yes_no('Proceed?', true) then
        return false
    end

    -- Unpadded version
    local idx, addr = data.int32_t:find_one{
        -30000, -30000, -30000,
        -30000, -30000, -30000, -30000, -30000, -30000,
        df.game_mode.NONE, df.game_type.NONE
    }
    if idx then
        ms.found_offset('cursor', addr)
        ms.found_offset('selection_rect', addr + 12)
        ms.found_offset('gamemode', addr + 12 + 24)
        ms.found_offset('gametype', addr + 12 + 24 + 4)
        return true
    end

    -- Padded version
    idx, addr = data.int32_t:find_one{
        -30000, -30000, -30000, 0,
        -30000, -30000, -30000, -30000, -30000, -30000, 0, 0,
        df.game_mode.NONE, 0, 0, 0, df.game_type.NONE
    }
    if idx then
        ms.found_offset('cursor', addr)
        ms.found_offset('selection_rect', addr + 0x10)
        ms.found_offset('gamemode', addr + 0x30)
        ms.found_offset('gametype', addr + 0x40)
        return true
    end

    dfhack.printerr('Could not find cursor.')
    return false
end

exec_finder(find_cursor, { 'cursor', 'selection_rect', 'gamemode', 'gametype' })

--
-- Announcements
--

local function find_announcements()
    local idx, addr = data.int32_t:find_one{
        25, 25, 31, 31, 24, 24, 40, 40, 40, 40, 40, 40, 40
    }
    if idx then
        ms.found_offset('announcements', addr)
        return
    end

    dfhack.printerr('Could not find announcements.')
end

exec_finder(find_announcements, 'announcements')

--
-- d_init
--

local function is_valid_d_init(di)
    if di.sky_tile ~= 178 then
        print('Sky tile expected 178, found: '..di.sky_tile)
        if not utils.prompt_yes_no('Ignore?') then
            return false
        end
    end

    local ann = is_known 'announcements'
    local size,ptr = di:sizeof()
    if ann and ptr+size ~= ann then
        print('Announcements not immediately after d_init.')
        if not utils.prompt_yes_no('Ignore?') then
            return false
        end
    end

    return true
end

local function find_d_init()
    local idx, addr = data.int16_t:find_one{
        1,0, 2,0, 5,0, 25,0,   -- path_cost
        4,4,                   -- embark_rect
        20,1000,1000,1000,1000 -- store_dist
    }
    if idx then
        validate_offset('d_init', is_valid_d_init, addr, df.d_init, 'path_cost')
        return
    end

    dfhack.printerr('Could not find d_init')
end

exec_finder(find_d_init, 'd_init')

--
-- gview
--

local function find_gview()
    local vs_vtable = dfhack.internal.getVTable('viewscreenst')
    if not vs_vtable then
        dfhack.printerr('Cannot search for gview - no viewscreenst vtable.')
        return
    end

    local idx, addr = data.uint32_t:find_one{0, vs_vtable}
    if idx then
        ms.found_offset('gview', addr)
        return
    end

    dfhack.printerr('Could not find gview')
end

exec_finder(find_gview, 'gview')

--
-- World
--

local function is_valid_world(world)
    if not ms.is_valid_vector(world.units.all, 4)
    or not ms.is_valid_vector(world.units.bad, 4)
    or not ms.is_valid_vector(world.history.figures, 4)
    or not ms.is_valid_vector(world.cur_savegame.map_features, 4)
    then
        dfhack.printerr('Vector layout check failed.')
        return false
    end

    if #world.units.all == 0 or #world.units.all ~= #world.units.bad then
        print('Different or zero size of units.all and units.bad:'..#world.units.all..' vs '..#world.units.bad)
        if not utils.prompt_yes_no('Ignore?') then
            return false
        end
    end

    return true
end

local function find_world()
    local addr = searcher:find_menu_cursor([[
Searching for world. Please open the stockpile creation
menu, and select different types as instructed below:]],
        'int32_t',
        { 'Corpses', 'Refuse', 'Stone', 'Wood', 'Gems', 'Bars', 'Cloth', 'Leather', 'Ammo', 'Coins' },
        df.stockpile_category
    )
    validate_offset('world', is_valid_world, addr, df.world, 'selected_stockpile_type')
end

exec_finder(find_world, 'world')

--
-- UI
--

local function is_valid_ui(ui)
    if not ms.is_valid_vector(ui.economic_stone, 1)
    or not ms.is_valid_vector(ui.dipscripts, 4)
    then
        dfhack.printerr('Vector layout check failed.')
        return false
    end

    if ui.follow_item ~= -1 or ui.follow_unit ~= -1 then
        print('Invalid follow state: '..ui.follow_item..', '..ui.follow_unit)
        return false
    end

    return true
end

local function find_ui()
    local addr = searcher:find_menu_cursor([[
Searching for ui. Please open the designation
menu, and switch modes as instructed below:]],
        'int16_t',
        { 'DesignateMine', 'DesignateChannel', 'DesignateRemoveRamps', 'DesignateUpStair',
          'DesignateDownStair', 'DesignateUpDownStair', 'DesignateUpRamp', 'DesignateChopTrees' },
        df.ui_sidebar_mode
    )
    validate_offset('ui', is_valid_ui, addr, df.ui, 'main', 'mode')
end

exec_finder(find_ui, 'ui')

--
-- ui_sidebar_menus
--

local function is_valid_ui_sidebar_menus(usm)
    if not ms.is_valid_vector(usm.workshop_job.choices_all, 4)
    or not ms.is_valid_vector(usm.workshop_job.choices_visible, 4)
    then
        dfhack.printerr('Vector layout check failed.')
        return false
    end

    if #usm.workshop_job.choices_all == 0
    or #usm.workshop_job.choices_all ~= #usm.workshop_job.choices_visible then
        print('Different or zero size of visible and all choices:'..
              #usm.workshop_job.choices_all..' vs '..#usm.workshop_job.choices_visible)
        if not utils.prompt_yes_no('Ignore?') then
            return false
        end
    end

    return true
end

local function find_ui_sidebar_menus()
    local addr = searcher:find_menu_cursor([[
Searching for ui_sidebar_menus. Please open the add job
ui of Mason, Craftsdwarfs, or Carpenters workshop, and
select entries in the list:]],
        'int32_t',
        { 0, 1, 2, 3, 4, 5, 6 },
        ordinal_names
    )
    validate_offset('ui_sidebar_menus', is_valid_ui_sidebar_menus,
                    addr, df.ui_sidebar_menus, 'workshop_job', 'cursor')
end

exec_finder(find_ui_sidebar_menus, 'ui_sidebar_menus')

--
-- ui_build_selector
--

local function is_valid_ui_build_selector(ubs)
    if not ms.is_valid_vector(ubs.requirements, 4)
    or not ms.is_valid_vector(ubs.choices, 4)
    then
        dfhack.printerr('Vector layout check failed.')
        return false
    end

    if ubs.building_type ~= df.building_type.Trap
    or ubs.building_subtype ~= df.trap_type.PressurePlate then
        print('Invalid building type and subtype:'..ubs.building_type..','..ubs.building_subtype)
        return false
    end

    return true
end

local function find_ui_build_selector()
    local addr = searcher:find_menu_cursor([[
Searching for ui_build_selector. Please start constructing
a pressure plate, and enable creatures. Then change the min
weight as requested, remembering that the ui truncates the
number, so when it shows "Min (5000df", it means 50000:]],
        'int32_t',
        { 50000, 49000, 48000, 47000, 46000, 45000, 44000 }
    )
    validate_offset('ui_build_selector', is_valid_ui_build_selector,
                    addr, df.ui_build_selector, 'plate_info', 'unit_min')
end

exec_finder(find_ui_build_selector, 'ui_build_selector')

--
-- ui_selected_unit
--

local function find_ui_selected_unit()
    if not is_known 'world' then
        dfhack.printerr('Cannot search for ui_selected_unit: no world')
        return
    end

    for i,unit in ipairs(df.global.world.units.active) do
        dfhack.units.setNickname(unit, i)
    end

    local addr = searcher:find_menu_cursor([[
Searching for ui_selected_unit. Please activate the 'v'
mode, point it at units, and enter their numeric nickname
into the prompts below:]],
        'int32_t',
        function()
            return utils.prompt_input('  Enter index: ', utils.check_number)
        end,
        'noprompt'
    )
    ms.found_offset('ui_selected_unit', addr)
end

exec_finder(find_ui_selected_unit, 'ui_selected_unit')

--
-- ui_unit_view_mode
--

local function find_ui_unit_view_mode()
    local addr = searcher:find_menu_cursor([[
Searching for ui_unit_view_mode. Having selected a unit
with 'v', switch the pages as requested:]],
        'int32_t',
        { 'General', 'Inventory', 'Preferences', 'Wounds' },
        df.ui_unit_view_mode.T_value
    )
    ms.found_offset('ui_unit_view_mode', addr)
end

exec_finder(find_ui_unit_view_mode, 'ui_unit_view_mode')

--
-- ui_look_cursor
--

local function look_item_list_count()
    return #df.global.ui_look_list.items
end

local function find_ui_look_cursor()
    local addr = searcher:find_menu_cursor([[
Searching for ui_look_cursor. Please activate the 'k'
mode, find a tile with many items or units on the ground,
and select list entries as instructed:]],
        'int32_t',
        list_index_choices(look_item_list_count),
        ordinal_names
    )
    ms.found_offset('ui_look_cursor', addr)
end

exec_finder(find_ui_look_cursor, 'ui_look_cursor')

--
-- ui_building_item_cursor
--

local function building_item_list_count()
    return #df.global.world.selected_building.contained_items
end

local function find_ui_building_item_cursor()
    local addr = searcher:find_menu_cursor([[
Searching for ui_building_item_cursor. Please activate the 't'
mode, find a cluttered workshop, trade depot, or other building
with many contained items, and select as instructed:]],
        'int32_t',
        list_index_choices(building_item_list_count),
        ordinal_names
    )
    ms.found_offset('ui_building_item_cursor', addr)
end

exec_finder(find_ui_building_item_cursor, 'ui_building_item_cursor')

--
-- ui_workshop_in_add
--

local function find_ui_workshop_in_add()
    local addr = searcher:find_menu_cursor([[
Searching for ui_workshop_in_add. Please activate the 'q'
mode, find a workshop without jobs (or delete jobs),
and do as instructed below.

NOTE: After first 3 steps resize the game window.]],
        'int8_t',
        { 1, 0 },
        { [1] = 'enter the add job menu',
          [0] = 'add job, thus exiting the menu' }
    )
    ms.found_offset('ui_workshop_in_add', addr)
end

exec_finder(find_ui_workshop_in_add, 'ui_workshop_in_add')

--
-- ui_workshop_job_cursor
--

local function workshop_job_list_count()
    return #df.global.world.selected_building.jobs
end

local function find_ui_workshop_job_cursor()
    local addr = searcher:find_menu_cursor([[
Searching for ui_workshop_job_cursor. Please activate the 'q'
mode, find a workshop with many jobs, and select as instructed:]],
        'int32_t',
        list_index_choices(workshop_job_list_count),
        ordinal_names
    )
    ms.found_offset('ui_workshop_job_cursor', addr)
end

exec_finder(find_ui_workshop_job_cursor, 'ui_workshop_job_cursor')

--
-- ui_building_in_assign
--

local function find_ui_building_in_assign()
    local addr = searcher:find_menu_cursor([[
Searching for ui_building_in_assign. Please activate
the 'q' mode, select a room building (e.g. a bedroom)
and do as instructed below.

NOTE: After first 3 steps resize the game window.]],
        'int8_t',
        { 1, 0 },
        { [1] = 'enter the Assign owner menu',
          [0] = 'press Esc to exit assign' }
    )
    ms.found_offset('ui_building_in_assign', addr)
end

exec_finder(find_ui_building_in_assign, 'ui_building_in_assign')

--
-- ui_building_in_resize
--

local function find_ui_building_in_resize()
    local addr = searcher:find_menu_cursor([[
Searching for ui_building_in_resize. Please activate
the 'q' mode, select a room building (e.g. a bedroom)
and do as instructed below.

NOTE: After first 3 steps resize the game window.]],
        'int8_t',
        { 1, 0 },
        { [1] = 'enter the Resize room mode',
          [0] = 'press Esc to exit resize' }
    )
    ms.found_offset('ui_building_in_resize', addr)
end

exec_finder(find_ui_building_in_resize, 'ui_building_in_resize')


--
-- window_x
--

local function find_window_x()
    local addr = searcher:find_counter([[
Searching for window_x. Please exit to main dwarfmode menu,
scroll to the LEFT edge, then do as instructed:]],
        'int32_t', 10,
        'Please press Right to scroll right one step.'
    )
    ms.found_offset('window_x', addr)
end

exec_finder(find_window_x, 'window_x')

--
-- window_y
--

local function find_window_y()
    local addr = searcher:find_counter([[
Searching for window_y. Please exit to main dwarfmode menu,
scroll to the TOP edge, then do as instructed:]],
        'int32_t', 10,
        'Please press Down to scroll down one step.'
    )
    ms.found_offset('window_y', addr)
end

exec_finder(find_window_y, 'window_y')

--
-- window_z
--

local function find_window_z()
    local addr = searcher:find_counter([[
Searching for window_z. Please exit to main dwarfmode menu,
scroll to ground level, then do as instructed below.

NOTE: After first 3 steps resize the game window.]],
        'int32_t', -1,
        "Please press '>' to scroll one Z level down."
    )
    ms.found_offset('window_z', addr)
end

exec_finder(find_window_z, 'window_z')

--
-- THE END
--

print('Done.')
searcher:reset()
