-- notes-related logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_parse = reqscript('internal/quickfort/parse')

local log = quickfort_common.log

function do_run(_, grid, ctx)
    local cells = quickfort_parse.get_ordered_grid_cells(grid)
    local line, lines = {}, {}
    local prev_x, prev_y = nil, nil
    for _,cell in ipairs(cells) do
        if prev_y ~= cell.y and #line > 0 then
            table.insert(lines, table.concat(line, '    '))
            for _ = prev_y or cell.y,cell.y-2 do
                table.insert(lines, '')
            end
            line = {}
        end
        for _ = prev_x or cell.x,cell.x-2 do
            table.insert(line, '    ')
        end
        table.insert(line, cell.text)
        prev_x, prev_y = cell.x, cell.y
    end
    if #line > 0 then
        table.insert(lines, table.concat(line, '    '))
    end
    table.insert(ctx.messages, table.concat(lines, '\n'))
end

function do_orders()
    log('nothing to do for blueprints in mode: notes')
end

function do_undo()
    log('nothing to do for blueprints in mode: notes')
end
