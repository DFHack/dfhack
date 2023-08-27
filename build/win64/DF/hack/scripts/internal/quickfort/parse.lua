-- file parsing logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local argparse = require('argparse')
local utils = require('utils')
local quickfort_reader = reqscript('internal/quickfort/reader')
local quickfort_transform = reqscript('internal/quickfort/transform')

valid_modes = utils.invert({
    'dig',
    'build',
    'place',
    'zone',
    'meta',
    'notes',
    'ignore',
    'aliases',
})

-- returns a tuple of {keys, extent} where keys is a string and extent is of the
-- format: {width, height, specified}, where width and height are numbers and
-- specified is true when an extent was explicitly specified. this function
-- assumes that text has been trimmed of leading and trailing spaces.
-- if the extent is specified, it will be transformed according ctx.transform_fn
-- so that the extent extends in the intended direction.
function parse_cell(ctx, text)
    -- first try to match expansion syntax
    local _, _, keys, width, height =
            text:find('^([^(][^(]-)%s*%(%s*(%-?%d+)%s*x%s*(%-?%d+)%s*%)$')
    local specified = nil
    if keys then
        width, height = tonumber(width), tonumber(height)
        specified = width and height and true
    else
        _, _, keys = text:find('(.+)')
    end
    if not specified then
        width, height = 1, 1
    else
        if width == 0 then
            qerror('invalid expansion syntax: width cannot be 0: ' .. text)
        elseif height == 0 then
            qerror('invalid expansion syntax: height cannot be 0: ' .. text)
        end
        local transformed = ctx.transform_fn(xy2pos(width, height), true)
        width, height = transformed.x, transformed.y
    end
    return keys, {width=width, height=height, specified=specified}
end

-- sorts by row (y), then column (x)
local function coord2d_lt(cell_a, cell_b)
    return cell_a.y < cell_b.y or
            (cell_a.y == cell_b.y and cell_a.x < cell_b.x)
end

-- returns a list of {x, y, cell, text} tuples in order of ascending y, then
-- ascending x
function get_ordered_grid_cells(grid)
    local cells = {}
    for y, row in pairs(grid) do
        for x, cell_and_text in pairs(row) do
            local cell, text = cell_and_text.cell, cell_and_text.text
            table.insert(cells, {y=y, x=x, cell=cell, text=text})
        end
    end
    table.sort(cells, coord2d_lt)
    return cells
end

-- sheet names can contain (or even start with or even be completely composed
-- of) spaces. labels, however, cannot contain spaces.
-- if there are characters in the input that follow the matched sheet name and
-- label, they are trimmed and returned in the third return value.
function parse_section_name(section_name)
    local sheet_name, label, remaining = nil, nil, nil
    if section_name and #section_name > 0 then
        local endpos
        _, endpos, sheet_name, label = section_name:find('^([^/]*)/?(%S*)')
        if #sheet_name == 0 then sheet_name = nil end
        if #label == 0 then label = nil end
        remaining = section_name:sub(endpos+1):trim()
    end
    return sheet_name, label, remaining
end

function parse_preserve_engravings(input, want_error_traceback)
    input = tonumber(input) or input
    if input == 'None' or input == -1 then
        return nil
    end
    if df.item_quality[input] then
        return tonumber(input) or df.item_quality[input]
    end
    (want_error_traceback and error or qerror)(
        ('unknown engraving quality: "%s"'):format(input))
end

function quote_if_has_spaces(str)
    if str:find(' ') then return '"' .. str .. '"' end
    return str
end

-- returns a string like:
--   orders library/dreamfort.csv -n /apartments2
--   run "some file.csv"
function format_command(command, blueprint_name, section_name, dry_run)
    local command_str = ''
    if command then
        command_str = ('%s '):format(command)
    end
    local section_name_str = ''
    if section_name then
        section_name_str = (' -n %s'):format(quote_if_has_spaces(section_name))
    end
    local dry_run_str = ''
    if dry_run then
        dry_run_str = ' --dry-run'
    end
    return ('%s%s%s%s'):format(command_str, quote_if_has_spaces(blueprint_name),
                               section_name_str, dry_run_str)
end

-- returns the next token, the current (possibly reassembed multiline) line, and
-- the start position of the following token in the line
-- or nil if there are no more tokens
local function get_next_csv_token(line, pos, get_next_line_fn, sep)
    local sep = sep or ','
    local c = string.sub(line, pos, pos)
    if c == '' then return nil end
    if c == '"' then
        -- quoted value (ignore separators within)
        local txt = ''
        repeat
            local startp, endp = string.find(line, '^%b""', pos)
            while not startp do
                -- handle multi-line quoted string
                local next_line = get_next_line_fn()
                if not next_line then
                    -- don't put quote characters around %s because it ends up
                    -- being confusing
                    dfhack.printerr(string.format(
                            'unterminated quoted string: %s', line))
                    return nil
                end
                line = line .. '\n' .. next_line
                startp, endp = string.find(line, '^%b""', pos)
            end
            txt = txt .. string.sub(line, startp+1, endp-1)
            pos = endp + 1
            -- check first char AFTER quoted string, if it is another
            -- quoted string without separator, then append it
            -- this is the way to "escape" the quote char in a quote.
            -- example: "blub""blip""boing" -> blub"blip"boing
            c = string.sub(line, pos, pos)
            if (c == '"') then txt = txt .. '"' end
        until c ~= '"'
        if line:sub(pos, pos) == sep then pos = pos + 1 end
        return txt, line, pos
    end
    -- no quotes used, just look for the first separator
    local startp, endp = string.find(line, sep, pos)
    if startp then
        return string.sub(line, pos, startp-1), line, startp+1
    end
    -- no separator found -> use rest of string
    return string.sub(line, pos), line, #line+1
end

-- adapted from example on http://lua-users.org/wiki/LuaCsv
-- returns a list of strings corresponding to the text in the cells in the row
local function tokenize_next_csv_line(get_next_line_fn, max_cols)
    local line = get_next_line_fn()
    if not line then return nil end
    local tokens = {}
    local sep, token, pos = ',', nil, nil
    token, line, pos = get_next_csv_token(line, 1, get_next_line_fn, sep)
    while token do
        table.insert(tokens, token)
        if max_cols > 0 and #tokens >= max_cols then break end
        token, line, pos = get_next_csv_token(line, pos, get_next_line_fn)
    end
    return tokens
end

-- returns the character position after the marker and the trimmed marker body
-- or nil if the specified marker is not found at the indicated start_pos
local function get_marker_body(text, start_pos, marker_name)
    local _, marker_end, marker_body = string.find(
            text, '^%s*'..marker_name..'%s*(%b())%s*', start_pos)
    if not marker_body then return nil end
    local _, _, inner_body = string.find(marker_body, '^%(%s*(.-)%s*%)$')
    return marker_end + 1, inner_body
end

local function parse_label(modeline, start_pos, filename, marker_values)
    local next_start_pos, label = get_marker_body(modeline, start_pos, 'label')
    if not label then return false, start_pos end
    if label:match('^%a[-_%w]*$') then
        marker_values.label = label
    else
        dfhack.printerr(string.format(
            'error while parsing "%s": labels must start with a letter and' ..
            ' consist only of letters, numbers, dashes, and underscores: "%s"',
            filename, modeline))
    end
    return true, next_start_pos
end

local function parse_start(modeline, start_pos, filename, marker_values)
    local next_start_pos, start_str =
            get_marker_body(modeline, start_pos, 'start')
    if not start_str then return false, start_pos end
    local _, _, startx, starty, start_comment =
            start_str:find('^(%d+)%s-[;, ]%s-(%d+)%s-[;, ]?%s*(.*)')
    if startx and starty then
        marker_values.startx = startx
        marker_values.starty = starty
        marker_values.start_comment = #start_comment>0 and start_comment or nil
    elseif #start_str > 0 then
        -- the whole thing is a comment
        marker_values.start_comment = start_str
    else
        dfhack.printerr(string.format(
            'error while parsing "%s": start() markers must either have both' ..
            ' an x and a y value or else have a non-empty comment: "%s"',
            filename, modeline))
    end
    return true, next_start_pos
end

local function parse_hidden(modeline, start_pos, filename, marker_values)
    local next_start_pos, exist = get_marker_body(modeline, start_pos, 'hidden')
    if not exist then return false, start_pos end
    marker_values.hidden = true
    return true, next_start_pos
end

local function parse_message(modeline, start_pos, filename, marker_values)
    local next_start_pos, message =
            get_marker_body(modeline, start_pos, 'message')
    if not message then return false, start_pos end
    if #message > 0 then marker_values.message = message end
    return true, next_start_pos
end

-- uses given marker_fns to parse markers in the given text in any order
-- returns table of marker values and next unmatched text position
local function parse_markers(text, start_pos, filename, marker_fns, marker_data)
    marker_data = marker_data or {}
    local remaining_marker_fns = copyall(marker_fns)
    while #remaining_marker_fns > 0 do
        local matched = false
        for i,marker_fn in ipairs(remaining_marker_fns) do
            matched, start_pos = marker_fn(text, start_pos, filename,
                                           marker_data)
            if matched then
                table.remove(remaining_marker_fns, i)
                break
            end
        end
        -- no more matched text, start_pos contains the next text pos
        if not matched then break end
    end
    return marker_data, start_pos
end

local modeline_marker_fns = {
    parse_label,
    parse_start,
    parse_hidden,
    parse_message,
}

--[[
parses a modeline
example: '#dig label(dig1) start(4;4;center of stairs) dining hall'
where all elements other than the initial #mode are optional. If a label is not
specified, the modeline_id is used as the label.
returns a table that includes at least the mode and the label, plus a comment if
one is specified, plus any marker-related data (see above parsers for details).
returns nil if the modeline is invalid.
]]
local function parse_modeline(modeline, filename, modeline_id)
    if not modeline then return nil end
    local _, mode_end, mode = string.find(modeline, '^#([%l]+)')
    -- ignore no-longer-supported blueprint modes
    if mode == 'query' or mode == 'config' then
        mode = 'ignore'
    end
    if not mode or not valid_modes[mode] then return nil end
    local modeline_data, comment_start =
            parse_markers(modeline, mode_end+1, filename, modeline_marker_fns)
    local _, _, comment = string.find(modeline, '^%s*(.-)%s*$', comment_start)
    modeline_data.mode = mode
    modeline_data.label = modeline_data.label or tostring(modeline_id)
    modeline_data.comment = #comment > 0 and comment or nil
    return modeline_data
end

-- meta markers

function parse_repeat_params(text, modifiers)
    local _, _, direction, count = text:find('^%s*([%a<>]*)%s*,?%s*(%d*)')
    direction = direction:lower()
    local zoff = 0
    if direction == 'up' or direction == '<' then zoff = 1
    elseif direction == 'down' or direction == '>' then zoff = -1
    else qerror('unknown repeat direction: '..direction) end
    modifiers.repeat_zoff, modifiers.repeat_count = zoff, tonumber(count) or 1
end

local function parse_repeat(text, start_pos, _, modifiers)
    local next_start_pos, params = get_marker_body(text, start_pos, 'repeat')
    if not params then return false, start_pos end
    parse_repeat_params(params, modifiers)
    return true, next_start_pos
end

local function make_shift_fn(xoff, yoff)
    return function(pos)
        return xy2pos(pos.x+xoff, pos.y+yoff)
    end
end

function parse_shift_params(text, modifiers)
    local _, _, xstr, ystr, zstr = text:find('^(%-?%d+)%s*[;,]?%s*(%-?%d*)')
    local x, y = tonumber(xstr), tonumber(ystr) or 0
    if not x then qerror('invalid x offset in: '..text) end
    table.insert(modifiers.shift_fn_stack, make_shift_fn(x, y))
end

local function parse_shift(text, start_pos, _, modifiers)
    local next_start_pos, params = get_marker_body(text, start_pos, 'shift')
    if not params then return false, start_pos end
    parse_shift_params(params, modifiers)
    return true, next_start_pos
end

local function get_next_transform_name(text, start_pos)
    local _, end_pos, name = text:find('^(%a+)%s*[;,]?%s*', start_pos)
    return end_pos and end_pos+1 or nil, name
end

function parse_transform_params(text, modifiers)
    local next_pos, name = get_next_transform_name(text, 1)
    if not name then qerror('invalid transformation list: '..text) end
    while name do
        table.insert(modifiers.transform_fn_stack,
                     quickfort_transform.make_transform_fn_from_name(name))
        next_pos, name = get_next_transform_name(text, next_pos)
    end
end

local function parse_transform(text, start_pos, _, modifiers)
    local next_start_pos, params = get_marker_body(text, start_pos, 'transform')
    if not params then return false, start_pos end
    parse_transform_params(params, modifiers)
    return true, next_start_pos
end

local meta_marker_fns = {
    parse_repeat,
    parse_shift,
    parse_transform,
}

function get_modifiers_defaults()
    return {
        repeat_count=1,
        repeat_zoff=0,
        transform_fn_stack = {},
        shift_fn_stack = {},
    }
end

function get_meta_modifiers(text, filename)
    local marker_data, extra_start =
            parse_markers(text, 1, filename, meta_marker_fns,
                          get_modifiers_defaults())
    if extra_start ~= #text+1 then
        dfhack.printerr(('extra unparsed text at the end of "%s": "%s"'):
                        format(text, text:sub(extra_start)))
    end
    return marker_data
end

local function get_col_name(col)
  if col <= 26 then
    return string.char(string.byte('A') + col - 1)
  end
  local div, mod = math.floor(col / 26), math.floor(col % 26)
  if mod == 0 then
      mod = 26
      div = div - 1
  end
  return get_col_name(div) .. get_col_name(mod)
end

local function make_cell_label(col_num, row_num)
    return get_col_name(col_num) .. tostring(math.floor(row_num))
end

-- removes spaces from the beginning and end of the given string
local function trim_token(token)
    _, _, token = token:find('^%s*(.-)%s*$')
    return token
end

-- returns a grid representation of the current level, the number of rows
-- read from the input, and the next z-level modifier, if any. See
-- process_section() for grid format.
local function process_level(reader, start_line_num, start_coord, transform_fn)
    local grid = {}
    local y = start_coord.y
    while true do
        local row_tokens = reader:get_next_row()
        if not row_tokens then return grid, y-start_coord.y end
        for i, v in ipairs(row_tokens) do
            v = trim_token(v)
            if i == 1 then
                local _, _, zchar, zcount = v:find('^#([<>])%s*,?%s*(%d*)')
                if zchar then
                    zcount = tonumber(zcount) or 1
                    local zdir = (zchar == '<') and 1 or -1
                    return grid, y-start_coord.y, zcount*zdir
                end
                if parse_modeline(v, reader.filepath) then
                    return grid, y-start_coord.y
                end
            end
            if v:match('^#$') then break end
            if not v:match('^[`~%s]*$') and not v:match('^%s*#') then
                -- cell has actual content, not just comments or chalkline chars
                local x = start_coord.x + i - 1
                local pos_t = transform_fn(xy2pos(x, y))
                if not grid[pos_t.y] then grid[pos_t.y] = {} end
                local line_num = start_line_num + y - start_coord.y
                grid[pos_t.y][pos_t.x] = {cell=make_cell_label(i, line_num),
                                          text=v}
            end
        end
        y = y + 1
    end
end

local function process_levels(reader, label, start_cursor_coord, transform_fn)
    local section_data_list = {}
    -- scan down to the target label
    local cur_line_num, modeline_id = 1, 1
    local row_tokens, modeline = nil, nil
    local first_line = true
    while not modeline or (label and modeline.label ~= label) do
        row_tokens = reader:get_next_row()
        if not row_tokens then
            local label_str = 'no data found'
            if label then
                label_str = string.format('label "%s" not found', label)
            end
            qerror(string.format(
                    "%s in %s", label_str, reader.filepath))
        end
        modeline = parse_modeline(row_tokens[1], reader.filepath,
                                  modeline_id)
        if first_line then
            if not modeline then
                modeline = {mode='dig', label='1'}
                reader:redo() -- we need to reread the first line
                cur_line_num = cur_line_num - 1
            end
            first_line = false
        end
        if modeline then
            if modeline.mode == 'ignore' or modeline.mode == 'aliases' then
                modeline = nil
            else
                modeline_id = modeline_id + 1
            end
        end
        cur_line_num = cur_line_num + 1
    end
    local x = start_cursor_coord.x - (modeline.startx or 1) + 1
    local y = start_cursor_coord.y - (modeline.starty or 1) + 1
    local z = start_cursor_coord.z
    while true do
        local grid, num_section_rows, zmod =
                process_level(reader, cur_line_num, xy2pos(x, y), transform_fn)
        table.insert(section_data_list,
                     {modeline=modeline, zlevel=z, grid=grid})
        if zmod == nil then break end
        cur_line_num = cur_line_num + num_section_rows + 1
        z = z + zmod
    end
    return section_data_list
end

local alias_pattern = '[%w-_][%w-_]+'

-- validates the format of aliasname (as per parse_alias_combined() below)
-- if the format is valid, adds the alias to the aliases table and returns true.
-- otherwise returns false.
local function parse_alias_separate(aliasname, definition, aliases)
    if aliasname and definition and
            aliasname:find('^('..alias_pattern..')$') and #definition > 0 then
        aliases[aliasname] = definition
        return true
    end
    return false
end

-- parses aliases of the form
--   aliasname: value
-- where aliasname must be at least two alphanumerics long (plus - and _) to
-- distinguish them from regular keystrokes
-- if the format is matched, the alias is added to the aliases table and the
-- function returns true. if the format is not matched, the function returns
-- false.
function parse_alias_combined(combined_str, aliases)
    if not combined_str then return false end
    local _, _, aliasname, definition = combined_str:find('^([^:]+):%s*(.*)')
    return parse_alias_separate(aliasname, definition, aliases)
end

local function get_sheet_metadata(reader)
    local modelines, aliases = {}, {}
    local row_tokens = reader:get_next_row()
    local first_line, alias_mode = true, false
    while row_tokens do
        local modeline = nil
        local first_tok = row_tokens[1]
        if first_tok then
            modeline = parse_modeline(first_tok, reader.filepath, #modelines+1)
        end
        if first_line then
            if not modeline then
                modeline = {mode='dig', label="1"}
            end
            first_line = false
        end
        if modeline then
            alias_mode = false
            if modeline.mode == 'aliases' then
                alias_mode = true
            elseif modeline.mode ~= 'ignore' then
                table.insert(modelines, modeline)
            end
        elseif alias_mode and first_tok and first_tok:match('^%s*[^#]') then
            if not parse_alias_combined(first_tok, aliases) then
                if not parse_alias_separate(first_tok, row_tokens[2],
                                            aliases) then
                    dfhack.printerr(string.format(
                            'invalid alias in #aliases section: "%s"',
                            first_tok))
                end
            end
        end
        row_tokens = reader:get_next_row()
    end
    return modelines, aliases
end

local function new_reader(filepath, sheet_name, max_cols)
    if string.find(filepath:lower(), '[.]xlsx$') then
        return quickfort_reader.XlsxReader{
            filepath=filepath,
            max_cols=max_cols,
            sheet_name=sheet_name,
        }
    else
        return quickfort_reader.CsvReader{
            filepath=filepath,
            max_cols=max_cols,
            row_tokenizer=tokenize_next_csv_line,
        }
    end
end

-- returns a list of modeline tables and a map of discovered aliases
function get_metadata(filepath, sheet_name)
    local reader = new_reader(filepath, sheet_name, 2)
    return dfhack.with_finalize(
        function() reader:cleanup() end,
        function() return get_sheet_metadata(reader) end
    )
end

--[[
returns a list of {modeline, zlevel, grid} tables
Where the structure of modeline is defined as per parse_modeline and grid is a:
  map of target y coordinate ->
    map of target map x coordinate ->
      {cell=spreadsheet cell, text=text from spreadsheet cell}
Map keys are numbers, and the keyspace is sparse -- only cells that have content
are non-nil.
]]
function process_section(filepath, sheet_name, label, start_cursor_coord,
                         transform_fn)
    transform_fn = transform_fn or function(pos) return pos end
    local reader = new_reader(filepath, sheet_name)
    return dfhack.with_finalize(
        function() reader:cleanup() end,
        function()
            return process_levels(reader, label, start_cursor_coord,
                                  transform_fn)
        end
    )
end

-- throws on invalid token
-- returns the contents of the extended token
local function get_extended_token(text, startpos)
    -- extended token must start with a '{' and be at least 3 characters long
    if text:sub(startpos, startpos) ~= '{' or #text < startpos + 2 then
        qerror(
            string.format('invalid extended token: "%s"', text:sub(startpos)))
    end
    -- construct a modified version of the string so we can search for the
    -- matching closing brace, ignoring initial '{' and '}' characters that
    -- should be treated as literals
    local etoken_mod = ' {' .. text:sub(startpos + 2)
    local b, e = etoken_mod:find(' %b{}')
    if not b then
        qerror(string.format(
                'invalid extended token: "%s"; did you mean "{{}"?',
                text:sub(startpos)))
    end
    return text:sub(startpos + b - 1, startpos + e - 1)
end

-- returns the token (i.e. the alias name or keycode) and the start position
-- of the next element in the etoken
local function get_token(etoken)
    local _, e, token = etoken:find('^{(.[^}%s]*)%s*')
    if token == 'Numpad' then
        local num = nil
        _, e, num = etoken:find('^{Numpad%s+(%d)%s*')
        if not num then
            qerror(string.format(
                    'invalid extended token: "%s"; missing Numpad number?',
                    etoken))
        end
        token = string.format('%s %d', token, num)
    end
    return token, e + 1
end

-- param names follow the same naming rules as aliases
local function get_next_param(etoken, start)
    local _, pos, name = etoken:find('%s*('..alias_pattern..')=.', start)
    return name, pos
end

-- returns the specified param map and the start position of the next element
-- in the etoken
local function get_params(etoken, start)
    -- trim off the last '}' from the etoken so the csv parser doesn't read it
    etoken = etoken:sub(1, -2)
    local params, next_pos = {}, start
    local name, pos = get_next_param(etoken, start)
    while name do
        local val, next_param_start = nil, nil
        if etoken:sub(pos, pos) == '{' then
            _, next_param_start, val = etoken:find('(%b{})', pos)
            next_param_start = next_param_start + 1
        else
            val, _, next_param_start = get_next_csv_token(
                    etoken, pos, function() return nil end, ' ')
        end
        if not val or #val == 0 then
            qerror(string.format(
                    'invalid extended token param: "%s"', etoken:sub(pos)))
        end
        params[name] = val
        next_pos = next_param_start
        name, pos = get_next_param(etoken, next_param_start)
    end
    return params, next_pos
end

-- returns the specified repetitions, or 1 if not specified, and the position
-- of the next element in etoken
local function get_repetitions(etoken, start)
    local _, e, repetitions = etoken:find('%s*(%d+)%s*}', start)
    return tonumber(repetitions) or 1, e or start
end

-- parses the sequence starting with '{' and ending with the matching '}' that
-- delimit an extended token of the form:
--   {token params repetitions}
-- where token is an alias name or a keycode, and both params and repetitions
-- are optional. if the first character of token is '{' or '}', it is treated as
-- a literal and not a syntax element.
-- Examples:
--   param_name=param_val
--   param_name=param_val{othertoken}
--   param_name="param val with spaces"
--   param_name="param val {othertoken} with spaces"
--   param_name="param_val{otherextendedtoken param=val}"
--   param_name="{alias1}{alias2}"
--   param_name={token params repetitions}
-- if the params within the extended token within the params require quotes,
-- .csv-style doubled double quotes are required:
--   param_name="{firstalias}{secondalias var=""with spaces"" 5}"
-- if repetitions is not specified, the value 1 is returned
-- returns token as string, params as map, repetitions as number, start position
-- of the next element after the etoken in text as number
function parse_extended_token(text, startpos)
    startpos = startpos or 1
    local etoken = get_extended_token(text, startpos)
    local token, params_start = get_token(etoken)
    local params, repetitions_start = get_params(etoken, params_start)
    local repetitions, last_pos = get_repetitions(etoken, repetitions_start)
    if last_pos ~= #etoken then
        qerror(string.format('invalid extended token format: "%s"', etoken))
    end
    local next_token_pos = startpos + #etoken
    return token, params, repetitions, next_token_pos
end

-- parses a blueprint key sequence optionally followed by a label. returns a map of {keys=string, label=string}
-- and the start position of the next token. label is either a string of at least length 1 or nil
function parse_token_and_label(text, startpos, token_pattern)
    token_pattern = token_pattern or alias_pattern
    local _, endpos, token, label = text:find('^%s*('..token_pattern..')/('..alias_pattern..')%s*', startpos)
    if not endpos then
        _, endpos, token = text:find('^%s*('..token_pattern..')%s*', startpos)
    end
    if not endpos then
        return nil, startpos
    end
    return {token=token, label=label}, endpos+1
end

-- parses a sequence starting with '{' and ending with the matching '}' that
-- delimit properties that modify a blueprint element. Properties are in the same
-- format as parse_extended_token above.
-- returns params as map, start position of the next token as int
function parse_properties(text, startpos)
    local _, endpos, properties = text:find('^%s*(%b{})%s*', startpos)
    if not properties then return {}, startpos end
    return get_params(properties, 2), endpos+1
end

local stockpile_config_spec_pattern = '[%w_]+'

local function get_next_stockpile_transformation(text, startpos)
    local _, e, op, name = text:find('^%s*([%+%-=])%s*('..stockpile_config_spec_pattern..')%s*', startpos)
    if not e then return nil, startpos end
    local mode = 'set'
    if op == '+' then mode = 'enable'
    elseif op == '-' then mode = 'disable'
    end
    local filters
    if text:sub(e+1, e+1) == '/' then
        local _, filter_end_pos, filter_str = text:find('^([^%+%-=]+)', e+2)
        if filter_end_pos then
            e = filter_end_pos
            filters = argparse.stringList(filter_str)
        end
    end
    return {mode=mode, name=name, filters=filters}, e+1
end

-- returns a list of stockpile transformations and the start position of the next token as int
function parse_stockpile_transformations(text, startpos)
    local transformations = {}
    local _, e = text:find('^%s*:%s*', startpos)
    if not e then return transformations, startpos end
    local transformation, next_token_start_pos = get_next_stockpile_transformation(text, e+1)
    while transformation do
        table.insert(transformations, transformation)
        transformation, next_token_start_pos = get_next_stockpile_transformation(text, next_token_start_pos)
    end
    return transformations, next_token_start_pos
end

if dfhack.internal.IN_TEST then
    unit_test_hooks = {
        parse_cell=parse_cell,
        coord2d_lt=coord2d_lt,
        get_ordered_grid_cells=get_ordered_grid_cells,
        parse_section_name=parse_section_name,
        parse_preserve_engravings=parse_preserve_engravings,
        quote_if_has_spaces=quote_if_has_spaces,
        format_command=format_command,
        get_next_csv_token=get_next_csv_token,
        tokenize_next_csv_line=tokenize_next_csv_line,
        get_marker_body=get_marker_body,
        parse_label=parse_label,
        parse_start=parse_start,
        parse_hidden=parse_hidden,
        parse_message=parse_message,
        modeline_marker_fns=modeline_marker_fns,
        parse_markers=parse_markers,
        parse_modeline=parse_modeline,
        parse_repeat_params=parse_repeat_params,
        parse_repeat=parse_repeat,
        parse_shift_params=parse_shift_params,
        parse_shift=parse_shift,
        parse_transform_params=parse_transform_params,
        parse_transform=parse_transform,
        get_modifiers_defaults=get_modifiers_defaults,
        get_meta_modifiers=get_meta_modifiers,
        get_col_name=get_col_name,
        make_cell_label=make_cell_label,
        trim_token=trim_token,
        process_level=process_level,
        process_levels=process_levels,
        parse_alias_separate=parse_alias_separate,
        parse_alias_combined=parse_alias_combined,
        get_sheet_metadata=get_sheet_metadata,
        get_extended_token=get_extended_token,
        get_token=get_token,
        get_next_param=get_next_param,
        get_params=get_params,
        get_repetitions=get_repetitions,
        parse_extended_token=parse_extended_token,
    }
end
