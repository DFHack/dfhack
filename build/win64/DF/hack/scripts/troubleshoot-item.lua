-- troubleshoot-item.lua
--@ module = true
--[====[

troubleshoot-item
=================
Print various properties of the selected item. Sometimes useful for
troubleshooting issues such as why dwarves won't pick up a certain item.

]====]

local function coord_to_str(coord)
    local out = {}
    for k, v in pairs(coord) do
        -- handle 2D and 3D coordinates
        if k == 'x' then out[1] = v end
        if k == 'y' then out[2] = v end
        if k == 'z' then out[3] = v end
    end
    return '(' .. table.concat(out, ', ') .. ')'
end

function troubleshoot_item(item, out)
    local outstr = nil --as:string
    if out == nil then
        outstr = ''
        out = function(s, level) outstr = outstr .. s .. '\n' end
    end
    local function warn(s) out('WARNING: ' .. s) end
    assert(df.item:is_instance(item), 'not an item')
    if item.id < 0 then warn('Invalid ID: ' .. item.id) end
    if not df.item.find(item.id) then warn('Could not locate item in item lists') end

    -- Print the item name, and its ID as a header
    out("Item details:")
    out("Description: " .. dfhack.items.getDescription(item, item:getType(), false), 1)
    out("ID: " .. item.id, 1)

    if item.flags.forbid then out('Forbidden') end
    if item.flags.melt then out('Melt-designated') end
    if item.flags.dump then out('Dump-designated') end
    if item.flags.hidden then out('Hidden') end
    local unit_holder = dfhack.items.getGeneralRef(item, df.general_ref_type.UNIT_HOLDER)
    if unit_holder then
        if unit_holder.unit_id then
            local unit = df.unit.find(unit_holder.unit_id)
            if unit then
                local unit_details = string.format("%s - %s", dfhack.TranslateName(dfhack.units.getVisibleName(unit)), dfhack.units.getProfessionName(unit))
                out('Held by unit: ' .. unit_details)
            else
                warn('Could not find unit with unit_id: ' .. unit_holder.unit_id)
            end
        else
            warn('Could not find unit_id in unit_holder')
        end
    end
    local contained_in = dfhack.items.getGeneralRef(item, df.general_ref_type.CONTAINED_IN_ITEM)
    if contained_in then
        if contained_in.item_id then
            local contained_in_item = df.item.find(contained_in.item_id)
            if contained_in_item then
                out('Stored in item of type: ' .. df.item_type[contained_in_item:getType()])
            else
                warn('Could not find item with item_id: ' .. contained_in.item_id)
            end
        else
            warn('Could not find item_id in contained_in_item')
        end
    end
    local building_holder = dfhack.items.getGeneralRef(item, df.general_ref_type.BUILDING_HOLDER)
    if building_holder then
        if building_holder.building_id then
            local building = df.building.find(building_holder.building_id)
            if building then
                out('Inside building of type: ' .. df.building_type[building:getType()])
            else
                warn('Could not find building with building_id: ' .. building_holder.building_id)
            end
        else
            warn('Could not find building_id in building_holder')
        end
    end
    if item.flags.container then
        local count = 0
        local item_types = {}
        for _, v in pairs(item.general_refs) do
            if v:getType() == df.general_ref_type.CONTAINS_ITEM then
                count = count + 1
                local contained_item = df.item.find(v.item_id)
                local item_desc = dfhack.items.getDescription(contained_item, contained_item:getType(), false)
                item_types[item_desc] = item_types[item_desc] and item_types[item_desc] + 1 or 1
            end
        end
        local item_string = count == 1 and 'item' or 'items'
        if count > 0 then item_string = item_string .. ':' end
        out(string.format('Is a container, holding %s %s', count, item_string))
        for item_desc, item_count in pairs(item_types) do
            out(string.format('%s : %s', item_desc, item_count), 1)
        end
    end
    if item.flags.on_fire then out('On fire') end
    if item.flags.rotten then out('Rotten') end
    if item.flags.trader then out('Trade good') end
    if item.flags.owned then out('Owned') end
    if item.flags.foreign then out('Foreign') end
    if item.flags.encased then out('Encased in ice') end
    if item.flags.garbage_collect then warn('Marked for garbage collection') end
    if item.flags.construction then out('Used for construction') end
    if item.flags.in_building then out('Used for building') end
    if item.flags.artifact then
        out(item.flags.artifact_mood and "Artifact (from strange mood)" or "Artifact")
    end
    if item.flags.in_job then
        out('In job')
        local ref = dfhack.items.getSpecificRef(item, df.specific_ref_type.JOB)
        if ref then
            out('Job type: ' .. df.job_type[ref.data.job.job_type], 1)
            out('Job position: ' .. coord_to_str(ref.data.job.pos), 1)
            local found_job_item = false
            for i, job_item_ref in pairs(ref.data.job.items) do
                if item == job_item_ref.item then found_job_item = true end
            end
            if not found_job_item then warn('Item not attached to job') end
        else
            warn('No job specific_ref found')
        end
    end

    for i, ref in pairs(item.specific_refs) do
        if ref.type ~= df.specific_ref_type.JOB then
            out('Unhandled specific_ref: ' .. df.specific_ref_type[ref.type])
        end
    end

    if outstr then return outstr end
end

function main(args)
    local item = dfhack.gui.getSelectedItem(true)

    if item then
        local out_table = {}
        local out_function = function(s, level)
            table.insert(out_table, {s, level and level or 0})
        end

        troubleshoot_item(item, out_function)

        for _, row in pairs(out_table) do
            local out_line = row[1]
            local indent_amount = row[2]
            local indent = ''
            for i = 1, indent_amount do
                indent = indent .. '  '
            end
            -- Output log lines, heading lines (indent_amount=0) are prefixed with '-', other lines are indented
            print(dfhack.df2console(string.format("%s%s%s", indent_amount == 0 and '- ' or '', indent, out_line)))
        end
    else
        qerror('No item found')
    end
end

if not moduleMode then main({...}) end
