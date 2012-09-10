-- Find some offsets for linux.

local utils = require 'utils'
local ms = require 'memscan'

local is_known = dfhack.internal.getAddress

local os_type = dfhack.getOSType()

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

Finding the first few globals requires this script to be
started immediately after loading the game, WITHOUT
first loading a world. The rest expect a loaded save,
not a fresh embark. Finding current_weather requires
a special save previously processed with devel/prepare-save
on a DF version with working dfhack.

The script expects vanilla game configuration, without
any custom tilesets or init file changes. Never unpause
the game unless instructed. When done, quit the game
without saving using 'die'.
]]

if not utils.prompt_yes_no('Proceed?') then
    return
end

-- Data segment location

local data = ms.get_data_segment()
if not data then
    qerror('Could not find data segment')
end

print('\nData section: '..tostring(data))
if data.size < 5000000 then
    qerror('Data segment too short.')
end

local searcher = ms.DiffSearcher.new(data)

local function validate_offset(name,validator,addr,tname,...)
    local obj = data:object_by_field(addr,tname,...)
    if obj and not validator(obj) then
        obj = nil
    end
    ms.found_offset(name,obj)
end

local function zoomed_searcher(startn, end_or_sz)
    if force_scan.nozoom then
        return nil
    end
    local sv = is_known(startn)
    if not sv then
        return nil
    end
    local ev
    if type(end_or_sz) == 'number' then
        ev = sv + end_or_sz
        if end_or_sz < 0 then
            sv, ev = ev, sv
        end
    else
        ev = is_known(end_or_sz)
        if not ev then
            return nil
        end
    end
    sv = sv - (sv % 4)
    ev = ev + 3
    ev = ev - (ev % 4)
    if data:contains_range(sv, ev-sv) then
        return ms.DiffSearcher.new(ms.MemoryArea.new(sv,ev))
    end
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
                qerror('Quit')
            end
        end
    else
        print('Already known: '..table.concat(names,', '))
    end
end

local ordinal_names = {
    [0] = '1st entry',
    [1] = '2nd entry',
    [2] = '3rd entry'
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

--
-- enabler
--

local function is_valid_enabler(e)
    if not ms.is_valid_vector(e.textures.raws, 4)
    or not ms.is_valid_vector(e.text_system, 4)
    then
        dfhack.printerr('Vector layout check failed.')
        return false
    end

    return true
end

local function find_enabler()
    -- Data from data/init/colors.txt
    local colors = {
        0, 0, 0,       0, 0, 128,      0, 128, 0,
        0, 128, 128,   128, 0, 0,      128, 0, 128,
        128, 128, 0,   192, 192, 192,  128, 128, 128,
        0, 0, 255,     0, 255, 0,      0, 255, 255,
        255, 0, 0,     255, 0, 255,    255, 255, 0,
        255, 255, 255
    }

    for i = 1,#colors do colors[i] = colors[i]/255 end

    local idx, addr = data.float:find_one(colors)
    if idx then
        validate_offset('enabler', is_valid_enabler, addr, df.enabler, 'ccolor')
        return
    end

    dfhack.printerr('Could not find enabler')
end

--
-- gps
--

local function is_valid_gps(g)
    if g.clipx[0] < 0 or g.clipx[0] > g.clipx[1] or g.clipx[1] >= g.dimx then
        dfhack.printerr('Invalid clipx: ', g.clipx[0], g.clipx[1], g.dimx)
    end
    if g.clipy[0] < 0 or g.clipy[0] > g.clipy[1] or g.clipy[1] >= g.dimy then
        dfhack.printerr('Invalid clipy: ', g.clipy[0], g.clipy[1], g.dimy)
    end

    return true
end

local function find_gps()
    print('\nPlease ensure the mouse cursor is not over the game window.')
    if not utils.prompt_yes_no('Proceed?', true) then
        return
    end

    local zone
    if os_type == 'windows' or os_type == 'linux' then
        zone = zoomed_searcher('cursor', 0x1000)
    elseif os_type == 'darwin' then
        zone = zoomed_searcher('enabler', 0x1000)
    end
    zone = zone or searcher

    local w,h = ms.get_screen_size()

    local idx, addr = zone.area.int32_t:find_one{w, h, -1, -1}
    if idx then
        validate_offset('gps', is_valid_gps, addr, df.graphic, 'dimx')
        return
    end

    dfhack.printerr('Could not find gps')
end

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
Searching for ui_sidebar_menus. Please switch to 'q' mode,
select a Mason, Craftsdwarfs, or Carpenters workshop, open
the Add Job menu, and move the cursor within:]],
        'int32_t',
        { 0, 1, 2, 3, 4, 5, 6 },
        ordinal_names
    )
    validate_offset('ui_sidebar_menus', is_valid_ui_sidebar_menus,
                    addr, df.ui_sidebar_menus, 'workshop_job', 'cursor')
end

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

--
-- init
--

local function is_valid_init(i)
    -- derived from curses_*.png image sizes presumably
    if i.font.small_font_dispx ~= 8 or i.font.small_font_dispy ~= 12 or
       i.font.large_font_dispx ~= 10 or i.font.large_font_dispy ~= 12 then
        print('Unexpected font sizes: ',
              i.font.small_font_dispx, i.font.small_font_dispy,
              i.font.large_font_dispx, i.font.large_font_dispy)
        if not utils.prompt_yes_no('Ignore?') then
            return false
        end
    end

    return true
end

local function find_init()
    local zone
    if os_type == 'windows' then
        zone = zoomed_searcher('ui_build_selector', 0x3000)
    elseif os_type == 'linux' or os_type == 'darwin' then
        zone = zoomed_searcher('d_init', -0x2000)
    end
    zone = zone or searcher

    local idx, addr = zone.area.int32_t:find_one{250, 150, 15, 0}
    if idx then
        validate_offset('init', is_valid_init, addr, df.init, 'input', 'hold_time')
        return
    end

    local w,h = ms.get_screen_size()

    local idx, addr = zone.area.int32_t:find_one{w, h}
    if idx then
        validate_offset('init', is_valid_init, addr, df.init, 'display', 'grid_x')
        return
    end

    dfhack.printerr('Could not find init')
end

--
-- current_weather
--

local function find_current_weather()
    print('\nPlease load the save previously processed with prepare-save.')
    if not utils.prompt_yes_no('Proceed?', true) then
        return
    end

    local zone
    if os_type == 'windows' then
        zone = zoomed_searcher('crime_next_id', 512)
    elseif os_type == 'darwin' then
        zone = zoomed_searcher('cursor', -64)
    elseif os_type == 'linux' then
        zone = zoomed_searcher('ui_building_assign_type', -512)
    end
    zone = zone or searcher

    local wbytes = {
        2, 1, 0, 2, 0,
        1, 2, 1, 0, 0,
        2, 0, 2, 1, 2,
        1, 2, 0, 1, 1,
        2, 0, 1, 0, 2
    }

    local idx, addr = zone.area.int8_t:find_one(wbytes)
    if idx then
        ms.found_offset('current_weather', addr)
        return
    end

    dfhack.printerr('Could not find current_weather - must be a wrong save.')
end

--
-- ui_menu_width
--

local function find_ui_menu_width()
    local addr = searcher:find_menu_cursor([[
Searching for ui_menu_width. Please exit to the main
dwarfmode menu, then use Tab to do as instructed below:]],
        'int8_t',
        { 2, 3, 1 },
        { [2] = 'switch to the most usual [mapmap][menu] layout',
          [3] = 'hide the menu completely',
          [1] = 'switch to the default [map][menu][map] layout' }
    )
    ms.found_offset('ui_menu_width', addr)

    -- NOTE: Assume that the vars are adjacent, as always
    ms.found_offset('ui_area_map_width', addr+1)
end

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

--
-- ui_workshop_in_add
--

local function find_ui_workshop_in_add()
    local addr = searcher:find_menu_cursor([[
Searching for ui_workshop_in_add. Please activate the 'q'
mode, find a workshop without jobs (or delete jobs),
and do as instructed below.

NOTE: If not done after first 3-4 steps, resize the game window.]],
        'int8_t',
        { 1, 0 },
        { [1] = 'enter the add job menu',
          [0] = 'add job, thus exiting the menu' }
    )
    ms.found_offset('ui_workshop_in_add', addr)
end

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

--
-- ui_building_in_assign
--

local function find_ui_building_in_assign()
    local addr = searcher:find_menu_cursor([[
Searching for ui_building_in_assign. Please activate
the 'q' mode, select a room building (e.g. a bedroom)
and do as instructed below.

NOTE: If not done after first 3-4 steps, resize the game window.]],
        'int8_t',
        { 1, 0 },
        { [1] = 'enter the Assign owner menu',
          [0] = 'press Esc to exit assign' }
    )
    ms.found_offset('ui_building_in_assign', addr)
end

--
-- ui_building_in_resize
--

local function find_ui_building_in_resize()
    local addr = searcher:find_menu_cursor([[
Searching for ui_building_in_resize. Please activate
the 'q' mode, select a room building (e.g. a bedroom)
and do as instructed below.

NOTE: If not done after first 3-4 steps, resize the game window.]],
        'int8_t',
        { 1, 0 },
        { [1] = 'enter the Resize room mode',
          [0] = 'press Esc to exit resize' }
    )
    ms.found_offset('ui_building_in_resize', addr)
end

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

--
-- window_z
--

local function find_window_z()
    local addr = searcher:find_counter([[
Searching for window_z. Please exit to main dwarfmode menu,
scroll to a Z level near surface, then do as instructed below.

NOTE: If not done after first 3-4 steps, resize the game window.]],
        'int32_t', -1,
        "Please press '>' to scroll one Z level down."
    )
    ms.found_offset('window_z', addr)
end

--
-- cur_year
--

local function find_cur_year()
    local zone
    if os_type == 'windows' then
        zone = zoomed_searcher('formation_next_id', 32)
    elseif os_type == 'darwin' then
        zone = zoomed_searcher('cursor', -32)
    elseif os_type == 'linux' then
        zone = zoomed_searcher('ui_building_assign_type', -512)
    end
    if not zone then
        dfhack.printerr('Cannot search for cur_year - prerequisites missing.')
        return
    end

    local yvalue = utils.prompt_input('Please enter current in-game year: ', utils.check_number)
    local idx, addr = zone.area.int32_t:find_one{yvalue}
    if idx then
        ms.found_offset('cur_year', addr)
        return
    end

    dfhack.printerr('Could not find cur_year')
end

--
-- cur_year_tick
--

local function find_cur_year_tick()
    local zone
    if os_type == 'windows' then
        zone = zoomed_searcher('artifact_next_id', -32)
    else
        zone = zoomed_searcher('cur_year', 128)
    end
    if not zone then
        dfhack.printerr('Cannot search for cur_year_tick - prerequisites missing.')
        return
    end

    local addr = zone:find_counter([[
Searching for cur_year_tick. Please exit to main dwarfmode
menu, then do as instructed below:]],
        'int32_t', 1,
        "Please press '.' to step the game one frame."
    )
    ms.found_offset('cur_year_tick', addr)
end

--
-- process_jobs
--

local function get_process_zone()
    if os_type == 'windows' then
        return zoomed_searcher('ui_workshop_job_cursor', 'ui_building_in_resize')
    elseif os_type == 'linux' or os_type == 'darwin' then
        return zoomed_searcher('cur_year', 'cur_year_tick')
    end
end

local function find_process_jobs()
    local zone = get_process_zone() or searcher

    local addr = zone:find_menu_cursor([[
Searching for process_jobs. Please do as instructed below:]],
        'int8_t',
        { 1, 0 },
        { [1] = 'designate a building to be constructed, e.g a bed',
          [0] = 'step or unpause the game to reset the flag' }
    )
    ms.found_offset('process_jobs', addr)
end

--
-- process_dig
--

local function find_process_dig()
    local zone = get_process_zone() or searcher

    local addr = zone:find_menu_cursor([[
Searching for process_dig. Please do as instructed below:]],
        'int8_t',
        { 1, 0 },
        { [1] = 'designate a tile to be mined out',
          [0] = 'step or unpause the game to reset the flag' }
    )
    ms.found_offset('process_dig', addr)
end

--
-- pause_state
--

local function find_pause_state()
    local zone
    if os_type == 'linux' or os_type == 'darwin' then
        zone = zoomed_searcher('ui_look_cursor', 32)
    elseif os_type == 'windows' then
        zone = zoomed_searcher('ui_workshop_job_cursor', 80)
    end
    zone = zone or searcher

    local addr = zone:find_menu_cursor([[
Searching for pause_state. Please do as instructed below:]],
        'int8_t',
        { 1, 0 },
        { [1] = 'PAUSE the game',
          [0] = 'UNPAUSE the game' }
    )
    ms.found_offset('pause_state', addr)
end

--
-- MAIN FLOW
--

print('\nInitial globals (need title screen):\n')

exec_finder(find_cursor, { 'cursor', 'selection_rect', 'gamemode', 'gametype' })
exec_finder(find_announcements, 'announcements')
exec_finder(find_d_init, 'd_init')
exec_finder(find_gview, 'gview')
exec_finder(find_enabler, 'enabler')
exec_finder(find_gps, 'gps')

print('\nCompound globals (need loaded world):\n')

exec_finder(find_world, 'world')
exec_finder(find_ui, 'ui')
exec_finder(find_ui_sidebar_menus, 'ui_sidebar_menus')
exec_finder(find_ui_build_selector, 'ui_build_selector')
exec_finder(find_init, 'init')

print('\nPrimitive globals:\n')

exec_finder(find_current_weather, 'current_weather')
exec_finder(find_ui_menu_width, { 'ui_menu_width', 'ui_area_map_width' })
exec_finder(find_ui_selected_unit, 'ui_selected_unit')
exec_finder(find_ui_unit_view_mode, 'ui_unit_view_mode')
exec_finder(find_ui_look_cursor, 'ui_look_cursor')
exec_finder(find_ui_building_item_cursor, 'ui_building_item_cursor')
exec_finder(find_ui_workshop_in_add, 'ui_workshop_in_add')
exec_finder(find_ui_workshop_job_cursor, 'ui_workshop_job_cursor')
exec_finder(find_ui_building_in_assign, 'ui_building_in_assign')
exec_finder(find_ui_building_in_resize, 'ui_building_in_resize')
exec_finder(find_window_x, 'window_x')
exec_finder(find_window_y, 'window_y')
exec_finder(find_window_z, 'window_z')

print('\nUnpausing globals:\n')

exec_finder(find_cur_year, 'cur_year')
exec_finder(find_cur_year_tick, 'cur_year_tick')
exec_finder(find_process_jobs, 'process_jobs')
exec_finder(find_process_dig, 'process_dig')
exec_finder(find_pause_state, 'pause_state')

print('\nDone. Now add newly-found globals to symbols.xml.')
searcher:reset()
