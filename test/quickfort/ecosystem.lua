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
--
-- depends on blueprint, buildingplan, and dig-now plugins (as well as the
-- quickfort script, of course)
--
-- note that this test harness cannot (yet) test #query blueprints that define
-- rooms since furniture is not actually built during the test. It also cannot
-- test blueprints that #build flooring and then #build a workshop on top, again
-- since the flooring is never actually built.

config = {
    mode = 'fortress',
}

local blueprint = require('plugins.blueprint')
local quickfort_list = reqscript('internal/quickfort/list')
local quickfort_command = reqscript('internal/quickfort/command')

local blueprints_dir = 'blueprints/'
local input_dir = 'library/test/ecosystem/in/'
local output_dir = 'library/test/ecosystem/out/'

local mode_names = {'dig', 'build', 'place', 'query'}

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

local function get_blueprint_sets()
    -- find test blueprints with `quickfort list`
    local mock_print = mock.func()
    mock.patch(quickfort_list, 'print', mock_print,
        function()
            dfhack.run_script('quickfort', 'list', '-l', input_dir)
        end)

    -- group blueprint sets
    local sets = {}
    for _,args in ipairs(mock_print.call_args) do
        local line = args[1]
        local _,_,listnum,fname,mode = line:find('(%d+)%) (%S+) %((%S+)%)$')
        if listnum then
            local _,_,file_part = fname:find('/([^/]+)$')
            local _,_,basename = file_part:find('^([^-.]+)')
            if not sets[basename] then sets[basename] = {spec={}, modes={}} end
            sets[basename].modes[mode] = {
                listnum=listnum,
                input_filepath=blueprints_dir..fname,
                output_filepath=blueprints_dir..output_dir..file_part}
        end
    end

    -- load test specs
    for basename,set in pairs(sets) do
        local spec, notes = set.spec, set.modes.notes

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

        -- validate spec and convert numbers to numeric vars
        if not spec.description or spec.description == '' then
            qerror(('missing description in test spec for "%s"'):
                        format(basename))
        end
        spec.width = get_positive_int(spec.width, 'width', basename)
        spec.height = get_positive_int(spec.height, 'height', basename)
        spec.depth = get_positive_int(spec.depth, 'depth', basename)
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
            return true
        end
        ::continue::
    end
end

local function get_cursor_arg(pos)
    return ('--cursor=%d,%d,%d'):format(pos.x, pos.y, pos.z)
end

local function quickfort_cmd(cmd, listnum, pos)
    dfhack.run_script('quickfort', cmd, '-q', listnum, get_cursor_arg(pos))
end

local function quickfort_run(listnum, pos)
    quickfort_cmd('run', listnum, pos)
end

local function quickfort_undo(listnum, pos)
    quickfort_cmd('undo', listnum, pos)
end

local function designate_area(pos, spec)
    local endx, endy, endz = pos.x + spec.width - 1, pos.y + spec.height - 1,
            pos.z - spec.depth + 1
    for z = pos.z,endz,-1 do for y = pos.y,endy do for x = pos.x,endx do
        dfhack.maps.getTileFlags(xyz2pos(x, y, z)).dig =
                df.tile_dig_designation.Default
    end end end
end

local function run_blueprint(basename, set, pos)
    local blueprint_args = {tostring(set.spec.width),
                            tostring(set.spec.height),
                            tostring(-set.spec.depth),
                            output_dir..basename, get_cursor_arg(pos)}
    for _,mode_name in pairs(mode_names) do
        if set.modes[mode_name] then table.insert(blueprint_args, mode_name) end
    end
    blueprint.run(table.unpack(blueprint_args))
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
end

function test.end_to_end()
    -- read in test plan
    local sets = get_blueprint_sets()

    local area = {width=0, height=0, depth=0}
    for basename,set in pairs(sets) do
        print(('running quickfort test: "%s": %s'):
                    format(basename, set.spec.description))

        -- find an unused area of the map that meets requirements, else skip
        if not get_test_area(area, set.spec) then
            print(('cannot find unused map area to test set "%s"; skipping'):
                  format(basename))
            goto continue
        end

        -- quickfort run #dig blueprint (or just designate the whole block if
        -- there is no #dig blueprint)
        local modes = set.modes
        if modes.dig then
            quickfort_run(modes.dig.listnum, area.pos)
        else
            designate_area(area.pos, set.spec)
        end

        -- run dig-now to dig out designated tiles
        dfhack.run_command('dig-now')

        -- quickfort run remaining blueprints
        for _,mode_name in pairs(mode_names) do
            if mode_name ~= 'dig' and modes[mode_name] then
                quickfort_run(modes[mode_name].listnum, area.pos)
            end
        end

        -- run blueprint to generate files in output dir
        run_blueprint(basename, set, area.pos)

        -- quickfort undo blueprints
        for _,mode_name in pairs(mode_names) do
            if modes[mode_name] then
                quickfort_undo(modes[mode_name].listnum, area.pos)
            end
        end

        -- run tiletypes to reset tiles in area to hidden walls
        reset_area(area, set.spec)

        -- compare md5sum of input and output files
        local md5File = dfhack.internal.md5File
        for mode,mode_data in pairs(modes) do
            if mode == 'notes' then goto continue end
            local input_filepath = mode_data.input_filepath
            local output_filepath = mode_data.output_filepath
            local input_hash, input_size = md5File(input_filepath)
            local output_hash, output_size = md5File(output_filepath)
            expect.eq(input_hash, output_hash,
                      'compare blueprint contents to input: '..output_filepath)
            expect.eq(input_size, output_size,
                      'compare blueprint length to input: '..output_filepath)
            if not output_hash then goto continue end
            if input_hash ~= output_hash or input_size ~= output_size then
                -- show diff
                local input, output =
                    io.open(input_filepath, 'r'), io.open(output_filepath, 'r')
                local input_lines, output_lines = {}, {}
                for l in input:lines() do table.insert(input_lines, l) end
                for l in output:lines() do table.insert(output_lines, l) end
                input:close()
                output:close()
                expect.table_eq(input_lines, output_lines)
            end
            ::continue::
        end

        ::continue::
    end
end
