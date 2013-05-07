local _ENV = mkmodule('plugins.workflow')

local utils = require 'utils'

--[[

 Native functions:

 * isEnabled()
 * setEnabled(enable)
 * listConstraints([job[,with_history] ]) -> {{...},...}
 * findConstraint(token) -> {...} or nil
 * setConstraint(token[, by_count, goal, gap]) -> {...}
 * deleteConstraint(token) -> true/false
 * getCountHistory(token) -> {{...},...} or nil

--]]

local reaction_id_cache = nil

if dfhack.is_core_context then
    dfhack.onStateChange[_ENV] = function(code)
        if code == SC_MAP_LOADED then
            reaction_id_cache = nil
        end
    end
end

local function get_reaction(name)
    if not reaction_id_cache then
        reaction_id_cache = {}
        for i,v in ipairs(df.global.world.raws.reactions) do
            reaction_id_cache[v.code] = i
        end
    end
    local id = reaction_id_cache[name] or -1
    return id, df.reaction.find(id)
end

local job_outputs = {}

function job_outputs.CustomReaction(callback, job)
    local rid, r = get_reaction(job.reaction_name)

    if not r then
        return
    end

    for i,prod in ipairs(r.products) do
        if df.reaction_product_itemst:is_instance(prod) then
            local mat_type, mat_index = prod.mat_type, prod.mat_index
            local mat_mask

            local get_mat_prod = prod.flags.GET_MATERIAL_PRODUCT
            if get_mat_prod or prod.flags.GET_MATERIAL_SAME then
                local reagent_code = prod.get_material.reagent_code
                local reagent_idx, src = utils.linear_index(r.reagents, reagent_code, 'code')
                if not reagent_idx then goto continue end

                local item_idx, jitem = utils.linear_index(job.job_items, reagent_idx, 'reagent_index')
                if jitem then
                    mat_type, mat_index = jitem.mat_type, jitem.mat_index
                else
                    if not df.reaction_reagent_itemst:is_instance(src) then goto continue end
                    mat_type, mat_index = src.mat_type, src.mat_index
                end

                if get_mat_prod then
                    local p_code = prod.get_material.product_code
                    local mat = dfhack.matinfo.decode(mat_type, mat_index)

                    mat_type, mat_index = -1, -1

                    if mat then
                        local rp = mat.material.reaction_product
                        local idx = utils.linear_index(rp.id, p_code, 'value')
                        if idx then
                            mat_type, mat_index = rp.material.mat_type[idx], rp.material.mat_index[idx]
                        end
                    else
                        if p_code == "SOAP_MAT" then
                            mat_mask = { soap = true }
                        end
                    end
                end
            end

            callback{
                is_craft = prod.flags.CRAFTS,
                item_type = prod.item_type, item_subtype = prod.item_subtype,
                mat_type = mat_type, mat_index = mat_index, mat_mask = mat_mask
            }
        end
        ::continue::
    end
end

local function guess_job_material(job)
    if job.job_type == df.job_type.PrepareMeal then
        return -1, -1, nil
    end

    local mat_type, mat_index = job.mat_type, job.mat_index
    local mask_whole = job.material_category.whole
    local mat_mask

    local jmat = df.job_type.attrs[job.job_type].material
    if jmat then
        mat_type, mat_index = df.builtin_mats[jmat] or -1, -1
        if mat_type < 0 and df.dfhack_material_category[jmat] then
            mat_mask = { [jmat] = true }
        end
    end

    if not mat_mask and mask_whole ~= 0 then
        mat_mask = utils.parse_bitfield_int(mask_whole, df.job_material_category)
        if mat_mask.wood2 then
            mat_mask.wood = true
            mat_mask.wood2 = nil
        end
    end

    if mat_type < 0 and #job.job_items > 0 then
        local item0 = job.job_items[0]
        if #job.job_items == 1 or item0.item_type == df.item_type.PLANT then
            mat_type, mat_index = item0.mat_type, item0.mat_index

            if item0.item_type == df.item_type.WOOD then
                mat_mask = mat_mask or {}
                mat_mask.wood = true
            end
        end
    end

    return mat_type, mat_index, mat_mask
end

function default_output(callback, job, mat_type, mat_index, mat_mask)
    local itype = df.job_type.attrs[job.job_type].item
    if itype >= 0 then
        local subtype = nil
        if df.item_type.attrs[itype].is_rawable then
            subtype = job.item_subtype
        end
        callback{
            item_type = itype, item_subtype = subtype,
            mat_type = mat_type, mat_index = mat_index, mat_mask = mat_mask
        }
    end
end

function job_outputs.SmeltOre(callback, job)
    local mat = dfhack.matinfo.decode(job.job_items[0])
    if mat and mat.inorganic then
        for i,v in ipairs(mat.inorganic.metal_ore.mat_index) do
            callback{ item_type = df.item_type.BAR, mat_type = 0, mat_index = v }
        end
    else
        callback{ item_type = df.item_type.BAR, mat_type = 0 }
    end
end

function job_outputs.ExtractMetalStrands(callback, job)
    local mat = dfhack.matinfo.decode(job.job_items[0])
    if mat and mat.inorganic then
        for i,v in ipairs(mat.inorganic.thread_metal.mat_index) do
            callback{ item_type = df.item_type.THREAD, mat_type = 0, mat_index = v }
        end
    else
        callback{ item_type = df.item_type.THREAD, mat_type = 0 }
    end
end

function job_outputs.PrepareMeal(callback, job)
    if job.mat_type ~= -1 then
        for i,v in ipairs(df.global.world.raws.itemdefs.food) do
            if v.level == job.mat_type then
                callback{ item_type = df.item_type.FOOD, item_subtype = i }
            end
        end
    else
        callback{ item_type = df.item_type.FOOD }
    end
end

function job_outputs.MakeCrafts(callback, job)
    local mat_type, mat_index, mat_mask = guess_job_material(job)
    callback{ is_craft = true, mat_type = mat_type, mat_index = mat_index, mat_mask = mat_mask }
end

local plant_products = {
    BrewDrink = 'DRINK',
    MillPlants = 'MILL',
    ProcessPlants = 'THREAD',
    ProcessPlantsBag = 'LEAVES',
    ProcessPlantsBarrel = 'EXTRACT_BARREL',
    ProcessPlantsVial = 'EXTRACT_VIAL',
    ExtractFromPlants = 'EXTRACT_STILL_VIAL',
}

for job,flag in pairs(plant_products) do
    local ttag = 'type_'..string.lower(flag)
    local itag = 'idx_'..string.lower(flag)
    job_outputs[job] = function(callback, job)
        local mat_type, mat_index = -1, -1
        local mat = dfhack.matinfo.decode(job.job_items[0])
        if mat and mat.plant and mat.plant.flags[flag] then
            mat_type = mat.plant.material_defs[ttag]
            mat_index = mat.plant.material_defs[itag]
        end
        default_output(callback, job, mat_type, mat_index)
    end
end

local function enum_job_outputs(callback, job)
    local handler = job_outputs[df.job_type[job.job_type]]
    if handler then
        handler(callback, job)
    else
        default_output(callback, job, guess_job_material(job))
    end
end

function doEnumJobOutputs(native_cb, job)
    local function cb(info)
        native_cb(
            info.item_type, info.item_subtype,
            info.mat_mask, info.mat_type, info.mat_index,
            info.is_craft
        )
    end

    enum_job_outputs(cb, job)
end

function listJobOutputs(job)
    local res = {}
    enum_job_outputs(curry(table.insert, res), job)
    return res
end

function constraintToToken(cspec)
    local token
    if cspec.is_craft then
        token = 'CRAFTS'
    else
        token = df.item_type[cspec.item_type] or error('invalid item type: '..cspec.item_type)
        if cspec.item_subtype and cspec.item_subtype >= 0 then
            local def = dfhack.items.getSubtypeDef(cspec.item_type, cspec.item_subtype)
            if def then
                token = token..':'..def.id
            else
                error('invalid subtype '..cspec.item_subtype..' of '..token)
            end
        end
    end
    local mask_part
    if cspec.mat_mask then
        mask_part = string.upper(table.concat(utils.list_bitfield_flags(cspec.mat_mask), ','))
    end
    local mat_part
    if cspec.mat_type and cspec.mat_type >= 0 then
        local mat = dfhack.matinfo.decode(cspec.mat_type, cspec.mat_index or -1)
        if mat then
            mat_part = mat:getToken()
        else
            error('invalid material: '..cspec.mat_type..':'..(cspec.mat_index or -1))
        end
    end
    local qlist = {}
    if cspec.is_local then
        table.insert(qlist, "LOCAL")
    end
    if cspec.min_quality and cspec.min_quality > 0 then
        local qn = df.item_quality[cspec.min_quality] or error('invalid quality: '..cspec.min_quality)
        table.insert(qlist, qn)
    end
    local qpart
    if #qlist > 0 then
        qpart = table.concat(qlist, ',')
    end

    if mask_part or mat_part or qpart then
        token = token .. '/' .. (mask_part or '')
        if mat_part or qpart then
            token = token .. '/' .. (mat_part or '')
            if qpart then
                token = token .. '/' .. (qpart or '')
            end
        end
    end

    return token
end

function listWeakenedConstraints(outputs)
    local variants = {}
    local known = {}
    local function register(cons)
        cons.token = constraintToToken(cons)
        if not known[cons.token] then
            known[cons.token] = true
            table.insert(variants, cons)
        end
    end

    local generic = {}
    local anymat = {}
    for i,cons in ipairs(outputs) do
        local mask = cons.mat_mask
        if (cons.mat_type or -1) >= 0 then
            cons.mat_mask = nil
            local info = dfhack.matinfo.decode(cons)
            if info then
                for i,flag in ipairs(df.dfhack_material_category) do
                    if flag and flag ~= 'wood2' and info:matches{[flag]=true} then
                        mask = mask or {}
                        mask[flag] = true
                    end
                end
            end
        end
        register(cons)
        if mask then
            for k,v in pairs(mask) do
                table.insert(generic, {
                    item_type = cons.item_type,
                    item_subtype = cons.item_subtype,
                    is_craft = cons.is_craft,
                    mat_mask = { [k] = v }
                })
            end
        end
        table.insert(anymat, {
            item_type = cons.item_type,
            item_subtype = cons.item_subtype,
            is_craft = cons.is_craft
        })
    end
    for i,cons in ipairs(generic) do register(cons) end
    for i,cons in ipairs(anymat) do register(cons) end

    return variants
end

return _ENV
