-- Query is a script useful for finding and reading values of data structure fields. Purposes will likely be exclusive to writing lua script code.
-- Written by Josh Cooper(cppcooper) on 2017-12-21, last modified: 2021-06-13
-- Version: 3.2
--luacheck:skip-entirely
local utils=require('utils')
local validArgs = utils.invert({
 'help',

 'unit',
 'item',
 'plant',
 'building',
 'job',
 'tile',
 'block',
 'script',
 'json',
 'table',
 'getfield',

 'search',
 'findvalue',
 'maxdepth',
 'maxlength',
 'excludetypes',
 'excludekinds',

 'dumb',

 'showpaths',
 'setvalue',
 'alignto',
 'oneline',
 '1',
 'nopointers',
 'disableprint',
 'debug',
 'debugdata'
})
local args = utils.processArgs({...}, validArgs)
local selection = nil
local path_info = nil
local path_info_pattern = nil
local new_value = nil
local find_value = nil
local maxdepth = nil
local cur_depth = -1
local tilex = nil
local tiley = nil
local bToggle = true
local bool_flags = {}

--Section: core logic
function query(table, name, search_term, path, bprinted_parent, parent_table)
    -- Is the search depth too high? (Default: 7)
    -- Is the data capable of being iterated, or does it only have a value?
    -- Is the data recursively indexing (eg. A.B.C.A.*)?

    if bprinted_parent == nil then
        bprinted_parent = false
    end
    if parent_table ~= nil then
        if is_tiledata(parent_table) then
            if setValue(parent_table[tilex], path, tiley, table, new_value) then
                table = new_value
            end
        elseif setValue(parent_table, path, name, table, new_value) then
            table = new_value
        end
    end
    local bprinted = printField(path, name, table, bprinted_parent)
    --print("table/field printed")
    cur_depth = cur_depth + 1
    if cur_depth <= maxdepth then
        -- check that we can search
        if is_searchable(name, table) then
            -- iterate over search space
            function recurse(field, value)
                -- Is the data recursively indexing (eg. A.B.C.A.*)?

                local new_tname = tostring(makeName(name, field))
                if is_tiledata(value) then
                    local indexed_name = string.format("%s[%d][%d]", field,tilex,tiley)
                    query(value[tilex][tiley], indexed_name, search_term, appendField(path, indexed_name), bprinted, value)
                elseif not is_looping(path, new_tname) then
                    query(value, new_tname, search_term, appendField(path, field), bprinted, table)
                else
                    -- I don't know when this prints (if ever)
                    printField(path, field, value, bprinted)
                end
            end
            foreach(table, name, recurse)
        end
    end
    cur_depth = cur_depth - 1
    return bprinted
end

function foreach(table, name, callback)
    -- How can the data be iterated?
    -- Is the iteration count for the data too high? (Default: 257)

    function print_truncation_msg(name,index)
        print("<query on " .. name .. " truncated after " .. index .. ">")
    end
    local index = 0
    if getmetatable(table) and table._kind and (table._kind == "enum-type" or table._kind == "bitfield-type") then
        for idx, value in ipairs(table) do
            if is_exceeding_maxlength(index) then
                print_truncation_msg(name, index)
                return
            end
            callback(idx, value)
            index = index + 1
        end
    elseif table._type and string.find(tostring(table._type), "_list_link") then
        for field, value in utils.listpairs(table) do
            local m = tostring(field):gsub("<.*: ",""):gsub(">.*",""):gsub("%x%x%x%x%x%x","%1 ",1)
            local s = string.format("next{%d}->item", index)
            if is_exceeding_maxlength(index) then
                print_truncation_msg(name, index)
                return
            end
            callback(s, value)
            index = index + 1
        end
    else
        for field, value in safe_pairs(table) do
            if is_exceeding_maxlength(index) then
                print_truncation_msg(name, index)
                return
            end
            callback(field, value)
            index = index + 1
        end
    end
end

function setValue(table, path, field, value, new_value)
    -- Does the data match the search pattern?

    if args.setvalue then
        if not args.search or is_match(path, field, value) then
            if type(table[field]) == type(value) and type(value) == type(new_value) then
                table[field] = new_value
                print(string.format("Set %s value to %s, from %s", path, new_value, value)) --will call tostring
                return true
            end
        end
    end
    return false
end

--Section: entry/initialization
function main()
    if args.help then
        print(dfhack.script_help())
        return
    end
    processArguments()
    selection, path_info = table.unpack{getSelectionData()}
    debugf(0, tostring(selection), path_info)

    if selection == nil then
        qerror(string.format("Selected %s is null. Invalid selection.", path_info))
        return
    end
    debugf(0, string.format("query(%s, %s, %s, %s)", selection, path_info, args.search, path_info))
    query(selection, path_info, args.search, path_info)
end

local eval_env = utils.df_shortcut_env()
function eval(s)
    local f, err = load("return " .. s, "expression", "t", eval_env)
    if err then qerror(err) end
    return f()
end

function getSelectionData()
    local selection = nil
    local path_info = nil
    if args.table then
        debugf(0,"table selection")
        selection = eval(args.table)
        path_info = args.table
        path_info_pattern = escapeSpecialChars(path_info)
    elseif args.json then
        local json = require("json")
        local json_file = string.format("%s/%s", dfhack.getDFPath(), args.json)
        if dfhack.filesystem.isfile(json_file) then
            selection = json.decode_file(json_file)
            path_info = args.json
            path_info_pattern = path_info
        else
            qerror(string.format("File '%s' not found.", json_file))
        end
    elseif args.script then
        selection = reqscript(args.script)
        path_info = args.script
        path_info_pattern = path_info
    elseif args.unit then
        debugf(0,"unit selection")
        selection = dfhack.gui.getSelectedUnit()
        path_info = "unit"
        path_info_pattern = path_info
    elseif args.item then
        debugf(0,"item selection")
        selection = dfhack.gui.getSelectedItem()
        path_info = "item"
        path_info_pattern = path_info
    elseif args.plant then
        debugf(0,"plant selection")
        selection = dfhack.gui.getSelectedPlant()
        path_info = "plant"
        path_info_pattern = path_info
    elseif args.building then
        debugf(0,"item selection")
        selection = dfhack.gui.getSelectedBuilding()
        path_info = "building"
        path_info_pattern = path_info
    elseif args.job then
        debugf(0,"job selection")
        selection = dfhack.gui.getSelectedJob()
        if selection == nil and df.global.cursor.x >= 0 then
            local pos = { x=df.global.cursor.x,
                          y=df.global.cursor.y,
                          z=df.global.cursor.z }
            print("searching for a job at the cursor")
            for _link, job in utils.listpairs(df.global.world.jobs.list) do
                local jp = job.pos
                if jp.x == pos.x and jp.y == pos.y and jp.z == pos.z then
                    if selection == nil then
                        selection = {}
                    end
                    table.insert(selection, job)
                end
            end
        end
        path_info = "job"
        path_info_pattern = path_info
    elseif args.tile then
        debugf(0,"tile selection")
        local pos = copyall(df.global.cursor)
        selection = dfhack.maps.ensureTileBlock(pos.x,pos.y,pos.z)
        bpos = selection.map_pos
        path_info = string.format("tile[%d][%d][%d]",pos.x,pos.y,pos.z)
        path_info_pattern = string.format("tile%%[%d%%]%%[%d%%]%%[%d%%]",pos.x,pos.y,pos.z)
        tilex = pos.x%16
        tiley = pos.y%16
    elseif args.block then
        debugf(0,"block selection")
        local pos = copyall(df.global.cursor)
        selection = dfhack.maps.ensureTileBlock(pos.x,pos.y,pos.z)
        bpos = selection.map_pos
        path_info = string.format("blocks[%d][%d][%d]",bpos.x,bpos.y,bpos.z)
        path_info_pattern = string.format("blocks%%[%d%%]%%[%d%%]%%[%d%%]",bpos.x,bpos.y,bpos.z)
    else
        print(help)
    end
    if args.getfield then
        selection = findPath(selection,args.getfield)
        path_info = path_info .. "." .. args.getfield
        path_info_pattern = path_info_pattern .. "." .. args.getfield
    end
    --print(selection, path_info)
    return selection, path_info
end

function processArguments()
    if args["1"] then
        args.oneline = true
    end
    --Table Recursion
    if args.maxdepth then
        maxdepth = tonumber(args.maxdepth)
        if not maxdepth then
            qerror(string.format("Must provide a number with -depth"))
        end
    elseif args.dumb then
        maxdepth = 25
    else
        maxdepth = 7
    end
    args.maxdepth = maxdepth

    --Table Length
    if args.maxlength then
        args.maxlength = tonumber(args.maxlength)
    elseif args.dumb then
        args.maxlength = 10000
    else
        --2048 was chosen as it was the smallest power of 2 that can query the `df` table entirely, and it seems likely it'll be able to query most structures fully
        args.maxlength = 2048
    end

    new_value = toType(args.setvalue)
    find_value = toType(args.findvalue)

    -- singular or plural
    args.excludetypes = args.excludetypes and args.excludetypes or args.excludetype
    args.excludekinds = args.excludekinds and args.excludekinds or args.excludekind
    -- must be a string
    args.excludetypes = args.excludetypes and args.excludetypes or ""
    args.excludekinds = args.excludekinds and args.excludekinds or ""
    if string.find(args.excludetypes, 'a') then
        bool_flags["boolean"] = true
        bool_flags["function"] = true
        bool_flags["number"] = true
        bool_flags["string"] = true
        bool_flags["table"] = true
        bool_flags["userdata"] = true
    else
        bool_flags["boolean"] = string.find(args.excludetypes, 'b') and true or false
        bool_flags["function"] = string.find(args.excludetypes, 'f') and true or false
        bool_flags["number"] = string.find(args.excludetypes, 'n') and true or false
        bool_flags["string"] = string.find(args.excludetypes, 's') and true or false
        bool_flags["table"] = string.find(args.excludetypes, 't') and true or false
        bool_flags["userdata"] = string.find(args.excludetypes, 'u') and true or false
    end

    if string.find(args.excludekinds, 'a') then
        bool_flags["bitfield-type"] = true
        bool_flags["class-type"] = true
        bool_flags["enum-type"] = true
        bool_flags["struct-type"] = true
    else
        bool_flags["bitfield-type"] = string.find(args.excludekinds, 'b') and true or false
        bool_flags["class-type"] = string.find(args.excludekinds, 'c') and true or false
        bool_flags["enum-type"] = string.find(args.excludekinds, 'e') and true or false
        bool_flags["struct-type"] = string.find(args.excludekinds, 's') and true or false
    end
end

local bRunOnce={}
function runOnce(caller)
    if bRunOnce[caller] == true then
        return false
    end
    bRunOnce[caller] = true
    return true
end

function toType(str)
    if str ~= nil then
        if string.find(str,"true") then
            return true
        elseif string.find(str,"false") then
            return false
        elseif string.find(str,"nil") then
            return nil
        elseif tonumber(str) then
            return tonumber(str)
        else
            return tostring(str)
        end
    end
    return nil
end

--Section: filters
function is_searchable(field, value)
    -- Is the data capable of being iterated, or does it only have a value?

    if not df.isnull(value) then
        debugf(3,string.format("is_searchable( %s ): type: %s, length: %s, count: %s", value,type(value),getTableLength(value), countTableLength(value)))
        if not isEmpty(value) then
            if getmetatable(value) then
                if value._kind == "primitive" then
                    return false
                elseif value._kind == "struct" then
                    if args.safer then
                        return false
                    else
                        return true
                    end
                end
                debugf(3,string.format("_kind: %s, _type: %s", value._kind, value._type))
            end
            for _,_ in safe_pairs(value) do
                return true
            end
        end
    end
    return false
end

function match_searcharg(path,field)
    if type(args.search) == "table" then
        for _,pattern in pairs(args.search) do
            if string.find(tostring(field),pattern) or string.find(path,pattern) then
                return true
            end
        end
    elseif string.find(tostring(field),args.search) or string.find(path,args.search) then
        return true
    end
    return false
end

function is_match(path, field, value)
    debugf(3, string.format("path: %s\nfield: %s\nvalue: %s", path, field, value))
    if not args.search or match_searcharg(path,field) then
        if not args.findvalue or (type(value) == type(find_value) and (value == find_value or (type(value) == "string" and string.find(value,find_value)))) then
            return true
        end
    end
    return false
end

function is_looping(path, field)
    return not args.dumb and string.find(path, tostring(field))
end

function is_tiledata(value)
    if args.tile and string.find(tostring(value),"%[16%]") then
        if type(value) and string.find(tostring(value[tilex]),"%[16%]") then
            return true
        end
    end
    return false
end

function is_excluded(value)
    return bool_flags[type(value)] or not isEmpty(value) and getmetatable(value) and bool_flags[value._kind]
end

function is_exceeding_maxlength(index)
    return args.maxlength and not (index < args.maxlength)
end

--Section: table helpers
function isEmpty(t)
    for _,_ in safe_pairs(t) do
        return false
    end
    return true
end

function countTableLength(t)
    local count = 0
    for _,_ in safe_pairs(t) do
        count = count + 1
    end
    debugf(1,string.format("countTableEntries( %s ) = %d",t,count))
    return count
end

function getTableLength(t)
    if type(t) == "table" then
        local count=#t
        debugf(1,string.format("----getTableLength( %s ) = %d",t,count))
        return count
    end
    return 0
end

function findPath(t, path)
    debugf(0,string.format("findPath(%s, %s)",t, path))
    local curTable = t
    if not curTable then
        error("no table to search")
    end
    local keyParts = string.split(path, '.', true)
    for _,v in pairs(keyParts) do
        if v and curTable[v] ~= nil then
            debugf(1,"found something",v,curTable,curTable[v])
            curTable = curTable[v]
        else
            qerror("Key not found: " .. v)
        end
    end
    --debugf(1,"returning",curTable)
    return curTable
end

function hasMetadata(value)
    if not isEmpty(value) then
        if getmetatable(value) and value._kind and value._kind ~= nil then
            return true
        end
    end
    return false
end

--Section: output helpers
function makeName(tname, field)
    if tonumber(field) then
        return string.format("%s[%s]", tname, field)
    end
    return field
end

function appendField(parent, field)
    newParent=""
    if tonumber(field) then
        newParent=string.format("%s[%s]",parent,field)
    else
        newParent=string.format("%s.%s",parent,field)
    end
    debugf(2, string.format("new parent: %s", newParent))
    return newParent
end

function makeIndentation()
    local base="| "
    local indent=""
    for i=0,(cur_depth) do
        indent=indent .. string.format("%s",base)
    end
    --indent=string.format("%s ",base)
    return indent
end

function presentField(path, field, value)
    local output = tostring(args.showpaths and path or field)
    local leading_indent = not args.showpaths and makeIndentation() or ""
    output = leading_indent .. output
    local align = 40
    if tonumber(args.alignto) then
        align = tonumber(args.alignto)
    end
    output = string.format("%-"..align.."s ", output .. ":")
    if args.debugdata or not args.oneline or bToggle then
        if not args.nopointers or not string.find(tostring(value), "0x%x%x%x%x%x%x") then
            output = string.gsub(output,"  "," ~")
        end
        bToggle = false
    else
        bToggle = true
    end
    return output
end

function presentValue(field, value)
    local output = ""
    if not args.nopointers or not string.find(tostring(value), "0x%x%x%x%x%x%x") then
        if not args.debugdata then
            output = string.format(" %s", value)
        elseif hasMetadata(value) then
            if args.oneline then
                output = string.format(" %s [%s]", value, value._kind)
            else
                local N = math.min(90, string.len(indented_field))
                local newline_indent = string.format("%" .. N .. "s", "")
                output = string.format("%s %s\n%s [has metatable; _kind: %s]", output, value, newline_indent, value._kind)
            end
        else
            output = string.format("%s %s; type(%s) = %s", output, value, field, type(value))
        end
    end
    return output
end

function presentDebugData(presentedField, field, value)
    local output = ""
    if args.debugdata then
        local N = math.min(90, string.len(presentedField))
        local newline_indent = string.format("%" .. N .. "s", "")
        if hasMetadata(value) then
            -- why not search?
            if not args.search and args.oneline then
                output = output .. string.format("\n%s type(%s): %s, _kind: %s, _type: %s",
                        newline_indent, field, type(value), value._kind, value._type)
            else
                output = output .. string.format("\n%s type(%s): %s\n%s _kind: %s\n%s _type: %s",
                        newline_indent, field, type(value), newline_indent, value._kind, newline_indent, value._type)
            end
        end
    end
    return output
end

function printOnce(key, msg)
    if runOnce(key) then
        print(msg)
    end
end

--sometimes used to print fields, always used to print parents of fields
function printParents(path, field, value)
    local value_printed = false
    path = string.gsub(path, path_info_pattern, "")
    field = string.gsub(field, path_info_pattern, "")
    local cd = cur_depth
    cur_depth = 0
    local cur_path = path_info
    local words = {}
    local index = 1
    for word in string.gmatch(path, '([^.]+)') do
        words[index] = word
        index = index + 1
    end
    local last_index = index - 1
    for i,word in ipairs(words) do
        if i ~= last_index then
            cur_path = appendField(cur_path, word)
            printOnce(cur_path, string.format("%s%s", makeIndentation(),word))
        elseif string.find(word,"%a+%[%d+%]%[%d+%]") then
            value_printed = true
            cur_path = appendField(cur_path, word)
            local f = presentField(path, field, value)
            local v = presentValue(field, value)
            print(f .. v .. presentDebugData(f, field, value))
        end
        cur_depth = cur_depth + 1
    end
    cur_depth = cd
    return value_printed
end

function printField(path, field, value, bprinted_parent)
    -- Does the data match the search pattern?

    if runOnce(printField) then
        -- print selection
        printOnce(path,string.format("%s: %s", path, value))
        return
    end
    if not args.disableprint and not is_excluded(value) then
        if not args.search and not args.findvalue or is_match(path, field, value) then
            local bprinted_field = false
            if not args.showpaths and not bprinted_parent then
                bprinted_field = printParents(path, field, value)
            end
            if not bprinted_field then
                if is_tiledata(value) then
                    value = value[tilex][tiley]
                    field = string.format("%s[%d][%d]", field,tilex,tiley)
                end
                local f = presentField(path, field, value)
                local v = presentValue(field, value)
                print(f .. v .. presentDebugData(f, field, value))
            end
            return true
        end
    end
    return false
end

function debugf(level,...)
    if args.debug and level <= tonumber(args.debug) then
        local str=string.format(" #  %s",select(1, ...))
        for i = 2, select('#', ...) do
            str=string.format("%s\t%s",str,select(i, ...))
        end
        print(str)
    end
end

function escapeSpecialChars(str)
    return str:gsub('(['..("%^$()[].*+-?"):gsub("(.)", "%%%1")..'])', "%%%1")
end

main()
print()
