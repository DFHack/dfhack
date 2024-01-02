-- tests the quickfort ecosystem end-to-end via:
--   .csv -> quickfort/buildingplan/dig-now -> blueprint -> .csv
--
-- test cases are sets of .csv files in the
-- blueprints/library/test/ecosystem/in directory
--
-- test metadata is stored in an associated #notes blueprint:
--   description (required)
--   width (required)
--   height (required)
--   depth (default is 1)
--   start (cursor offset for input blueprints, default is 1,1)
--   extra_fn (the name of a function in the extra_fns table in this file to
--             run after applying all blueprints but before comparing results.
--             this function will get the map coordinate of the upper-left
--             corner of the test area passed to it)
--
-- depends on blueprint, buildingplan, and dig-now plugins (as well as the
-- quickfort script and anything else run in the extra_fns, of course)
--
-- note that this test harness cannot (yet) test #query blueprints that define
-- rooms since furniture is not actually built during the test. It also cannot
-- test blueprints that #build flooring and then #build a workshop on top, again
-- since the flooring is never actually built. We can support these features
-- once we figure out how to programmatically deconstruct buildlings without
-- crashing the game.

config.mode = 'fortress'
config.target = {'quickfort', 'blueprint', 'dig-now', 'tiletypes', 'gui/quantum'}

local argparse = require('argparse')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local utils = require('utils')

local blueprint = require('plugins.blueprint')
local confirm = require('plugins.confirm')

local assign_minecarts = reqscript('assign-minecarts')
local quantum = reqscript('gui/quantum')
local quickfort = reqscript('quickfort')
local quickfort_list = reqscript('internal/quickfort/list')
local quickfort_command = reqscript('internal/quickfort/command')

local blueprints_dir = 'blueprints/'
local input_dir = 'library/test/ecosystem/in/'
local golden_dir = 'library/test/ecosystem/golden/'
local output_dir = 'library/test/ecosystem/out/'

local phase_names = utils.invert(blueprint.valid_phases)

-- clear the output dir before each test run (but not after -- to allow
-- inspection of failed results)
local function test_wrapper(test_fn)
    local outdir = blueprints_dir .. output_dir
    if dfhack.filesystem.exists(outdir) then
        for _, v in ipairs(dfhack.filesystem.listdir_recursive(outdir)) do
            if not v.isdir then
                os.remove(v.path)
            end
        end
    end
    test_fn()
end
config.wrapper = test_wrapper

local function bad_spec(expected, varname, basename, bad_value)
    qerror(('expected %s for %s in "%s" test spec; got "%s"'):
               format(expected, varname, basename, bad_value))
end

local function get_positive_int(numstr, varname, basename)
    local num = tonumber(numstr)
    if not num or num <= 0 or num ~= math.floor(num) then
        bad_spec('positive integer', varname, basename, numstr)
    end
    return num
end

local function os_exists(path)
    return os.rename(path, path) and true or false
end

local function get_blueprint_sets()
    -- find test blueprints with `quickfort list`
    local mock_print = mock.func()
    mock.patch(quickfort_list, 'print', mock_print,
        function()
            dfhack.run_script('quickfort', 'list', input_dir)
        end)

    -- group blueprint sets
    local sets = {}
    for _,args in ipairs(mock_print.call_args) do
        local line = args[1]
        -- phase is the label or, if the label doesn't exist, the mode
        local _,_,listnum,fname,phase = line:find('(%d+)%) (%S+).-[/(]([^ )]+)')
        if listnum then
            local _,_,file_part = fname:find('/([^/]+)$')
            local _,_,basename = file_part:find('^([^-.]+)')
            if not sets[basename] then sets[basename] = {spec={}, phases={}} end
            local golden_path = golden_dir..file_part
            if not os_exists(blueprints_dir..golden_path) then
                golden_path = fname
            end
            sets[basename].phases[phase] = {
                listnum=listnum,
                golden_filepath=golden_path,
                output_filepath=output_dir..file_part}
        end
    end

    -- load test specs
    for basename,set in pairs(sets) do
        local spec, notes = set.spec, set.phases.notes

        -- set defaults
        spec.depth = '1'

        -- read spec
        mock.patch(quickfort_command, 'print',
            function(text)
                for line in text:gmatch('[^\n]*') do
                    local _,_,var,val = line:find('%*?%s*([^=]+)=(.*)')
                    if var then spec[var] = val end
                end
            end,
            function()
                dfhack.run_script('quickfort', 'run', '-q', notes.listnum)
            end)

        -- validate spec and convert number strings to numeric vars
        if not spec.description or spec.description == '' then
            qerror(('missing description in test spec for "%s"'):
                        format(basename))
        end
        spec.width = get_positive_int(spec.width, 'width', basename)
        spec.height = get_positive_int(spec.height, 'height', basename)
        spec.depth = get_positive_int(spec.depth, 'depth', basename)
        if spec.start then
            local start_spec = argparse.numberList(spec.start, basename, 2)
            spec.start = {x=get_positive_int(start_spec[1], 'startx', basename),
                          y=get_positive_int(start_spec[2], 'starty', basename)}
        end
    end

    return sets
end

local function is_usable_test_tile(pos)
    local tiletype = dfhack.maps.getTileType(pos)
    local tileattrs = df.tiletype.attrs[tiletype]
    local good_material = tileattrs.material == df.tiletype_material.STONE or
            tileattrs.material == df.tiletype_material.FEATURE or
            tileattrs.material == df.tiletype_material.MINERAL
    local good_shape = tileattrs.shape == df.tiletype_shape.WALL
    return good_material and good_shape
end

local function get_test_area(area, spec)
    -- return with success if our current area meets or exceeds requirements
    if area.width >= spec.width and area.height >= spec.height and
            area.depth >= spec.depth then
        return true
    end

    -- return with failure if the test requirements cannot ever be satisfied by
    -- the current map
    if spec.width > df.global.world.map.x_count - 2 or
            spec.height > df.global.world.map.y_count - 2 or
            spec.depth > df.global.world.map.z_count then
        print('map too small to accomodate test')
        return false
    end

    -- keep this simple for now. just go down the layers and check the region
    -- starting at the upper left corner of each level.
    local startz = area.pos and area.pos.z or df.global.world.map.z_count-1
    for z_start = startz,0,-1 do
        local z_end = z_start - spec.depth + 1
        if z_end < 1 then return false end
        for z = z_start,z_end,-1 do
            for y = 1,spec.height do
                for x = 1,spec.width do
                    if not is_usable_test_tile(xyz2pos(x, y, z)) then
                        -- next check should start on the z-level below this one
                        z_start = z
                        goto continue
                    end
                end
            end
        end
        do
            area.width, area.height, area.depth =
                    spec.width, spec.height, spec.depth
            area.pos = {x=1, y=1, z=z_start}
            area.endpos = {x=area.width, y=area.height, z=z_start-area.depth+1}
            return true
        end
        ::continue::
    end
end

local function get_cursor_arg(pos, start)
    start = start or {x=1, y=1}
    return ('--cursor=%d,%d,%d'):format(pos.x+start.x-1, pos.y+start.y-1, pos.z)
end

local function quickfort_cmd(cmd, listnum_or_path, pos, start)
    dfhack.run_script('quickfort', cmd, '-q', listnum_or_path,
                      get_cursor_arg(pos, start))
end

local function quickfort_run(listnum_or_path, pos, start)
    quickfort_cmd('run', listnum_or_path, pos, start)
end

local function quickfort_undo(listnum_or_path, pos, start)
    quickfort_cmd('undo', listnum_or_path, pos, start)
end

local function designate_area(pos, spec)
    local endx, endy, endz = pos.x + spec.width - 1, pos.y + spec.height - 1,
            pos.z - spec.depth + 1
    for z = pos.z,endz,-1 do for y = pos.y,endy do for x = pos.x,endx do
        dfhack.maps.getTileFlags(xyz2pos(x, y, z)).dig =
                df.tile_dig_designation.Default
    end end end
end

local function format_pos(pos)
    return ('%s,%s,%s'):format(pos.x, pos.y, pos.z)
end

local function run_dig_now(area)
    dfhack.run_command('dig-now', format_pos(area.pos),
                       format_pos(area.endpos), '--clean')
end

local function get_playback_start_arg(start)
    if not start then return end
    return ('--playback-start=%d,%d'):format(start.x, start.y)
end

local function run_blueprint(basename, spec, pos)
    local args = {tostring(spec.width), tostring(spec.height),
                  tostring(-spec.depth), output_dir..basename,
                  get_cursor_arg(pos), '-tphase'}
    local playback_start_arg = get_playback_start_arg(spec.start)
    if playback_start_arg then
        table.insert(args, playback_start_arg)
    end
    blueprint.run(table.unpack(args))
end

local function reset_area(area, spec)
    -- include the area border tiles that can get unhidden
    local width, height, depth = spec.width+2, spec.height+2, spec.depth
    local commands = {
        'f', 'any', ';',
        'p', 'any', ';',
        'p', 's', 'wall', ';',
        'p', 'sp', 'normal', ';',
        'p', 'h', '1', ';',
        'r', tostring(width), tostring(height), tostring(depth)}
    dfhack.run_command('tiletypes-command', table.unpack(commands))

    local pos = copyall(area.pos)
    -- include the border tiles
    pos.x = pos.x - 1
    pos.y = pos.y - 1
    -- tiletypes goes up z's, so adjust starting zlevel accordingly
    pos.z = pos.z - spec.depth + 1

    dfhack.run_command('tiletypes-here', '--quiet', get_cursor_arg(pos))

    -- patch up tiles where the material couldn't be automatically determined
    -- we don't just set all tiles to 'stone' so we don't obliterate veins
    commands = {
        'f', 's', 'empty', ';',
        'p', 'm', 'stone'}
    dfhack.run_command('tiletypes-command', table.unpack(commands))
    dfhack.run_command('tiletypes-here', '--quiet', get_cursor_arg(pos))

    commands = {'f', 's', 'ramp_top'}
    dfhack.run_command('tiletypes-command', table.unpack(commands))
    dfhack.run_command('tiletypes-here', '--quiet', get_cursor_arg(pos))
end

local function do_phase(phase_data, area, spec)
    quickfort_run(phase_data.listnum, area.pos, spec.start)
end

-- run a #dig blueprint (or just designate the whole block if there is no data)
-- and then run dig-now to materialize the designations
local function do_dig_phase(phase_data, area, spec)
    if phase_data then
        do_phase(phase_data, area, spec)
    else
        designate_area(area.pos, spec)
    end

    -- run dig-now to dig out designated tiles
    run_dig_now(area)
end

local extra_fns = {}

function test.end_to_end()
    -- read in test plan
    local sets = get_blueprint_sets()

    local area = {width=0, height=0, depth=0}
    for basename,set in pairs(sets) do
        local spec = set.spec

        print(('running quickfort ecosystem test: "%s": %s'):
                    format(basename, spec.description))

        -- find an unused area of the map that meets requirements, else skip
        if not get_test_area(area, spec) then
            print(('cannot find unused map area to test set "%s"; skipping'):
                  format(basename))
            goto continue
        end

        local phases = set.phases
        do_dig_phase(phases.dig, area, spec)
        if phases.smooth then do_dig_phase(phases.smooth, area, spec) end
        if phases.carve then do_dig_phase(phases.carve, area, spec) end
        if phases.construct then do_phase(phases.construct, area, spec) end
        if phases.build then do_phase(phases.build, area, spec) end
        if phases.place then do_phase(phases.place, area, spec) end
        if phases.zone then do_phase(phases.zone, area, spec) end
        if phases.query then do_phase(phases.query, area, spec) end
        if phases.rooms then do_phase(phases.rooms, area, spec) end

        -- run any extra commands, if defined by the blueprint spec
        if spec.extra_fn then
            extra_fns[spec.extra_fn](area.pos)
        end

        -- run blueprint to generate files in output dir
        run_blueprint(basename, spec, area.pos)

        -- quickfort undo blueprints (order shouldn't matter)
        for _,phase_name in ipairs(phase_names) do
            if phases[phase_name] then
                quickfort_undo(phases[phase_name].golden_filepath,
                               area.pos, spec.start)
            end
        end

        -- run tiletypes to reset tiles in area to hidden walls
        reset_area(area, set.spec)

        -- compare md5sum of input and output files
        local md5File = dfhack.internal.md5File
        for phase,phase_data in pairs(phases) do
            if phase == 'notes' then goto continue end
            print(('  verifying phase: %s'):format(phase))
            local golden_filepath = blueprints_dir..phase_data.golden_filepath
            local output_filepath = blueprints_dir..phase_data.output_filepath
            local input_hash, input_size = md5File(golden_filepath)
            local output_hash, output_size = md5File(output_filepath)
            expect.eq(input_hash, output_hash,
                      'compare blueprint contents to input: '..output_filepath)
            expect.eq(input_size, output_size,
                      'compare blueprint length to input: '..output_filepath)
            if not output_hash then goto continue end
            if input_hash ~= output_hash or input_size ~= output_size then
                -- show diff
                local input, output =
                    io.open(golden_filepath, 'r'), io.open(output_filepath, 'r')
                local input_lines, output_lines = {}, {}
                for l in input:lines() do table.insert(input_lines, l) end
                for l in output:lines() do table.insert(output_lines, l) end
                input:close()
                output:close()
                expect.table_eq(input_lines, output_lines)
                return nil
            end
            ::continue::
        end

        ::continue::
    end
end

local function send_keys(...)
    local keys = {...}
    for _,key in ipairs(keys) do
        gui.simulateInput(dfhack.gui.getCurViewscreen(true), key)
    end
end

function extra_fns.gui_quantum(pos)
    local vehicles = assign_minecarts.get_free_vehicles()
    --local confirm_state = confirm.isEnabled()
    --local confirm_conf = confirm.get_conf_data()
    local routes = df.global.plotinfo.hauling.routes
    local num_routes = #routes
    local next_order_id = df.global.world.manager_order_next_id

    return dfhack.with_finalize(
        function()
            -- unforbid the minecarts we forbade
            for _,minecart in ipairs(vehicles) do
                local item = df.item.find(minecart.item_id)
                if not item then error('could not find item in list') end
                item.flags.forbid = false
            end

            --[[
            if confirm_state then
                dfhack.run_command('enable confirm')
                for _,c in pairs(confirm_conf) do
                    confirm.set_conf_state(c.id, c.enabled)
                end
            end
            ]]
        end,
        function()
            -- forbid all available minecarts
            for _,minecart in ipairs(vehicles) do
                local item = df.item.find(minecart.item_id)
                if not item then error('could not find item in list') end
                item.flags.forbid = true
            end

            dfhack.run_script('gui/quantum')
            local view = quantum.view
            view:onRender()
            guidm.setCursorPos(pos)
            -- select the feeder stockpile
            send_keys('CURSOR_RIGHT', 'CURSOR_RIGHT', 'SELECT')
            view:onRender()
            -- deselect the feeder stockpile
            send_keys('LEAVESCREEN')
            view:onRender()
            -- reselect the feeder stockpile
            send_keys('SELECT')
            view:onRender()
            -- set a custom name
            send_keys('CUSTOM_N')
            view:onRender()
            view:onInput({_STRING=string.byte('f')})
            view:onInput({_STRING=string.byte('o')})
            view:onInput({_STRING=string.byte('o')})
            send_keys('SELECT')
            -- rotate the dump direction to the south
            send_keys('CUSTOM_D')
            view:onRender()
            -- move the cursor to the dump position
            send_keys('CURSOR_DOWN', 'CURSOR_DOWN', 'CURSOR_DOWN')
            view:onRender()
            -- commit and dismiss the dialog
            send_keys('SELECT', 'SELECT')

            -- verify the created route
            expect.eq(num_routes + 1, #routes)
            local route = routes[#routes-1]
            expect.eq(0, #route.vehicle_ids, 'minecart should not be assigned')
            expect.eq(1, #route.stops, 'should have 1 stop')
            expect.eq(1, #route.stops[0].stockpiles,
                      'should have 1 link')

            -- verify the created order
            expect.eq(next_order_id + 1, df.global.world.manager_order_next_id)
            local orders = df.global.world.manager_orders
            local order = orders[#orders - 1]
            expect.eq(df.job_type.MakeTool, order.job_type)

            -- if confirm is enabled, temporarily disable it so we can remove
            -- the route and manager order via the ui (easier than walking the
            -- structures and carefully deleting all the memory)
            if confirm_state then
                dfhack.run_command('disable confirm')
            end
            -- delete last route
            quickfort.apply_blueprint{mode='config', data='h--x^'}
            -- delete last manager order
            quickfort.apply_blueprint{mode='config', data='jm{Up}r^^'}
        end)
end
