-- Stock dialog for selecting materials

local _ENV = mkmodule('gui.materials')

local gui = require('gui')
local widgets = require('gui.widgets')
local dlg = require('gui.dialogs')

ARROW = string.char(26)

CREATURE_BASE = 19
PLANT_BASE = 419

MaterialDialog = defclass(MaterialDialog, gui.FramedScreen)

MaterialDialog.focus_path = 'MaterialDialog'

MaterialDialog.ATTRS{
    prompt = 'Type or select a material from this list',
    frame_style = gui.GREY_LINE_FRAME,
    frame_inset = 1,
    frame_title = 'Select Material',
    -- new attrs
    none_caption = 'none',
    hide_none = false,
    use_inorganic = true,
    use_creature = true,
    use_plant = true,
    mat_filter = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_cancel = DEFAULT_NIL,
    on_close = DEFAULT_NIL,
}

function MaterialDialog:init(info)
    self:addviews{
        widgets.Label{
            text = {
                self.prompt, '\n\n',
                'Category: ', { text = self:cb_getfield('context_str'), pen = COLOR_CYAN }
            },
            text_pen = COLOR_WHITE,
            frame = { l = 0, t = 0 },
        },
        widgets.HotkeyLabel{
            view_id = 'back',
            visible = false,
            key = 'LEAVESCREEN',
            label = 'Back',
            on_activate = self:callback('onEsc'),
            frame = { r = 0, b = 0 },
            auto_width = true,
        },
        widgets.FilteredList{
            view_id = 'list',
            not_found_label = 'No matching materials',
            frame = { l = 0, r = 0, t = 4, b = 2 },
            icon_width = 2,
            on_submit = self:callback('onSubmitItem'),
            edit_on_char=function(c) return c:match('%l') end,
        },
        widgets.Label{
            text = { {
                key = 'SELECT', text = ': Select',
                disabled = function() return not self.subviews.list:canSubmit() end
            } },
            frame = { l = 0, b = 0 },
        }
    }
    self:initBuiltinMode()
end

function MaterialDialog:getWantedFrameSize(rect)
    return math.max(self.frame_width or 40, #self.prompt), math.min(28, rect.height-8)
end

function MaterialDialog:onDestroy()
    if self.on_close then
        self.on_close()
    end
end

function MaterialDialog:initBuiltinMode()
    local choices = {}
    if not self.hide_none then
        table.insert(choices, { text = self.none_caption, mat_type = -1, mat_index = -1 })
    end

    if self.use_inorganic then
        table.insert(choices, {
            icon = ARROW, text = 'inorganic', key = 'CUSTOM_SHIFT_I',
            cb = self:callback('initInorganicMode')
        })
    end
    if self.use_creature then
        table.insert(choices, {
            icon = ARROW, text = 'creature', key = 'CUSTOM_SHIFT_C',
            cb = self:callback('initCreatureMode')
        })
    end
    if self.use_plant then
        table.insert(choices, {
            icon = ARROW, text = 'plant', key = 'CUSTOM_SHIFT_P',
            cb = self:callback('initPlantMode')
        })
    end

    local table = df.global.world.raws.mat_table.builtin
    for i=0,df.builtin_mats._last_item do
        self:addMaterial(choices, table[i], i, -1, false, nil)
    end

    self:pushContext('Any material', choices)
end

function MaterialDialog:initInorganicMode()
    local choices = {}

    for i,mat in ipairs(df.global.world.raws.inorganics.all) do
        self:addMaterial(choices, mat.material, 0, i, false, mat)
    end

    self:pushContext('Inorganic materials', choices)
end

function MaterialDialog:initCreatureMode()
    local choices = {}

    for i,v in ipairs(df.global.world.raws.creatures.all) do
        self:addObjectChoice(choices, v, v.name[0], CREATURE_BASE, i)
    end

    self:pushContext('Creature materials', choices)
end

function MaterialDialog:initPlantMode()
    local choices = {}

    for i,v in ipairs(df.global.world.raws.plants.all) do
        self:addObjectChoice(choices, v, v.name, PLANT_BASE, i)
    end

    self:pushContext('Plant materials', choices)
end

function MaterialDialog:addObjectChoice(choices, obj, name, typ, index)
    -- Check if any eligible children
    local count = #obj.material
    local idx = 0

    if self.mat_filter then
        count = 0
        for i,v in ipairs(obj.material) do
            if self.mat_filter(v, obj, typ+i, index) then
                count = count + 1
                if count > 1 then break end
                idx = i
            end
        end
    end

    -- Create an entry
    if count < 1 then
        return
    elseif count == 1 then
        self:addMaterial(choices, obj.material[idx], typ+idx, index, true, obj)
    else
        table.insert(choices, {
            icon = ARROW, text = name, mat_type = typ, mat_index = index,
            obj = obj, cb = self:callback('onSelectObj')
        })
    end
end

function MaterialDialog:onSelectObj(item)
    local choices = {}
    for i,v in ipairs(item.obj.material) do
        self:addMaterial(choices, v, item.mat_type+i, item.mat_index, false, item.obj)
    end
    self:pushContext(item.text, choices)
end

function MaterialDialog:addMaterial(choices, mat, typ, idx, pfix, parent)
    if not mat then return end
    -- Check the filter
    if self.mat_filter and not self.mat_filter(mat, parent, typ, idx) then
        return
    end

    -- Find the material name
    local state = 0
    if mat.heat.melting_point <= 10015 then
        state = 1
    end
    local name = mat.state_name[state]
    name = string.gsub(name, '^frozen ','')
    name = string.gsub(name, '^molten ','')
    name = string.gsub(name, '^condensed ','')

    -- Add prefix if requested
    local key
    if pfix and mat.prefix ~= '' then
        name = mat.prefix .. ' ' .. name
        key = mat.prefix
    end

    table.insert(choices, {
        text = name,
        search_key = key,
        material = mat,
        mat_type = typ, mat_index = idx
    })
end

function MaterialDialog:pushContext(name, choices)
    if not self.back_stack then
        self.back_stack = {}
        self.subviews.back.visible = false
    else
        table.insert(self.back_stack, {
            context_str = self.context_str,
            all_choices = self.subviews.list:getChoices(),
            edit_text = self.subviews.list:getFilter(),
            selected = self.subviews.list:getSelected(),
        })
        self.subviews.back.visible = true
    end

    self.context_str = name
    self.subviews.list:setChoices(choices, 1)
end

function MaterialDialog:onEsc()
    if not self.subviews.back.visible then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
        return
    end

    local save = table.remove(self.back_stack)
    self.subviews.back.visible = (#self.back_stack > 0)

    self.context_str = save.context_str
    self.subviews.list:setChoices(save.all_choices)
    self.subviews.list:setFilter(save.edit_text, save.selected)
end

function MaterialDialog:submitMaterial(typ, index)
    self:dismiss()

    if self.on_select then
        self.on_select(typ, index)
    end
end

function MaterialDialog:onSubmitItem(idx, item)
    if item.cb then
        item:cb(idx)
    else
        self:submitMaterial(item.mat_type, item.mat_index)
    end
end

function MaterialDialog:onInput(keys)
    if keys._MOUSE_R then
        self:onEsc()
        return true
    end
    self:inputToSubviews(keys)
    return true
end

function showMaterialPrompt(title, prompt, on_select, on_cancel, mat_filter)
    MaterialDialog{
        frame_title = title,
        prompt = prompt,
        mat_filter = mat_filter,
        on_select = on_select,
        on_cancel = on_cancel,
    }:show()
end

function ItemTypeDialog(args)
    args.text = args.prompt or 'Type or select an item type'
    args.text_pen = COLOR_WHITE
    args.with_filter = true
    args.icon_width = 2

    local choices = {}

    if not args.hide_none then
        table.insert(choices, {
            icon = '?', text = args.none_caption or 'none',
            item_type = -1, item_subtype = -1
        })
    end

    local filter = args.item_filter

    for itype = 0,df.item_type._last_item do
        local attrs = df.item_type.attrs[itype]
        local defcnt = dfhack.items.getSubtypeCount(itype)

        if not filter or filter(itype,-1) then
            local name = attrs.caption or df.item_type[itype]
            local icon
            if defcnt >= 0 then
                name = 'any '..name
                icon = '+'
            end
            table.insert(choices, {
                icon = icon, text = string.lower(name), item_type = itype, item_subtype = -1
            })
        end

        if defcnt > 0 then
            for subtype = 0,defcnt-1 do
                local def = dfhack.items.getSubtypeDef(itype, subtype)
                local success, chain = pcall(function() return def.props.flags.CHAIN_METAL_TEXT end)
                local text = success and chain and " (chain) " or " "
                local success, adjective = pcall(function() return def.adjective end)
                if success and adjective ~= "" then
                    text = text .. adjective .. " " .. def.name
                else
                    text = text .. def.name
                end
                if not filter or filter(itype,subtype,def) then
                    table.insert(choices, {
                        icon = '\x1e', text = text, item_type = itype, item_subtype = subtype
                    })
                end
            end
        end
    end

    args.choices = choices

    if args.on_select then
        local cb = args.on_select
        args.on_select = function(idx, obj)
            return cb(obj.item_type, obj.item_subtype)
        end
    end

    return dlg.ListBox(args)
end

function ItemTraitsDialog(args)
    local job_item_flags_map = {}
    for i = 1, 5 do
        if not df['job_item_flags'..i] then break end
        for _, f in ipairs(df['job_item_flags'..i]) do
            if f then
                job_item_flags_map[f] = 'flags'..i
            end
        end
    end
    local job_item_flags = {}
    for k, _ in pairs(job_item_flags_map) do
        job_item_flags[#job_item_flags + 1] = k
    end
    table.sort(job_item_flags)
    --------------------------------------
    local tool_uses = {}
    for i, _ in ipairs(df.tool_uses) do
        tool_uses[#tool_uses + 1] = df.tool_uses[i]
    end
    local restore_none = false
    if tool_uses[1] == 'NONE' then
        restore_none = true
        table.remove(tool_uses, 1)
    end
    table.sort(tool_uses)
    if restore_none then
        table.insert(tool_uses, 1, 'NONE')
    end
    --------------------------------------
    local set_ore_ix = {}
    for i, raw in ipairs(df.global.world.raws.inorganics.all) do
        for _, ix in ipairs(raw.metal_ore.mat_index) do
            set_ore_ix[ix] = true
        end
    end
    local ores = {}
    for ix in pairs(set_ore_ix) do
        local raw = df.global.world.raws.inorganics.all[ix]
        ores[#ores+1] = {mat_index = ix, name = raw.material.state_name.Solid}
    end
    table.sort(ores, function(a,b) return a.name < b.name end)

    --------------------------------------
    -- CALCIUM_CARBONATE, CAN_GLAZE, FAT, FLUX,
    -- GYPSUM, PAPER_PLANT, PAPER_SLURRY, TALLOW, WAX
    local reaction_classes_set = {}
    for ix,reaction in ipairs(df.global.world.raws.reactions.reactions) do
        if #reaction.reagents > 0 then
            for _, r in ipairs(reaction.reagents) do
                if r.reaction_class and r.reaction_class ~= '' then
                    reaction_classes_set[r.reaction_class] = true
                end
            end
        end --if
    end
    local reaction_classes = {}
    for k in pairs(reaction_classes_set) do
        reaction_classes[#reaction_classes + 1] = k
    end
    table.sort(reaction_classes)
    --------------------------------------
    -- PRESS_LIQUID_MAT, TAN_MAT, BAG_ITEM etc
    local product_materials_set = {}
    for ix,reaction in ipairs(df.global.world.raws.reactions.reactions) do
        if #reaction.products > 0 then
            --for _, p in ipairs(reaction.products) do
            -- in the list in work order conditions there is no SEED_MAT.
            -- I think it's because the game doesn't iterate over all products.
                local p = reaction.products[0]
                local mat = p.get_material
                if mat and mat.product_code ~= '' then
                    product_materials_set[mat.product_code] = true
                end
            --end
        end --if
    end
    local product_materials = {}
    for k in pairs(product_materials_set) do
        product_materials[#product_materials + 1] = k
    end
    table.sort(product_materials)
    --==================================--

    local function set_search_keys(choices)
        for _, choice in ipairs(choices) do
            if not choice.search_key then
                if type(choice.text) == 'table' then
                    local search_key = {}
                    for _, token in ipairs(choice.text) do
                        search_key[#search_key+1] = string.lower(token.text or '')
                    end
                    choice.search_key = table.concat(search_key, ' ')
                elseif choice.text then
                    choice.search_key = string.lower(choice.text)
                end
            end
        end
    end

    args.text = args.prompt or 'Type or select an item trait'
    args.text_pen = COLOR_WHITE
    args.with_filter = true
    args.icon_width = 2
    args.dismiss_on_select = false

    local pen_active = COLOR_LIGHTCYAN
    local pen_active_d = COLOR_CYAN
    local pen_not_active = COLOR_LIGHTRED
    local pen_not_active_d = COLOR_RED
    local pen_action = COLOR_WHITE
    local pen_action_d = COLOR_GREY

    local job_item = args.job_item
    local choices = {}

    local pen_cb = function(args, fnc)
        if not (args and fnc) then
            return COLOR_YELLOW
        end
        return fnc(args) and pen_active or pen_not_active
    end
    local pen_d_cb = function(args, fnc)
        if not (args and fnc) then
            return COLOR_YELLOW
        end
        return fnc(args) and pen_active_d or pen_not_active_d
    end
    local icon_cb = function(args, fnc)
        if not (args and fnc) then
            return '\19' -- ‼
        end
        -- '\251' is a checkmark
        -- '\254' is a square
        return fnc(args) and '\251' or '\254'
    end

    if not args.hide_none then
        table.insert(choices, {
            icon = '!',
            text = {{text = args.none_caption or 'none', pen = pen_action, dpen = pen_action_d}},
            reset_all_traits = true
        })
    end

    local isActiveFlag = function (obj)
        return obj.job_item[obj.ffield][obj.flag]
    end
    table.insert(choices, {
        icon = '!',
        text = {{text = 'unset flags', pen = pen_action, dpen = pen_action_d}},
        reset_flags = true
    })
    for _, flag in ipairs(job_item_flags) do
        local ffield = job_item_flags_map[flag]
        local text = 'is ' .. (value and '' or 'any ') .. string.lower(flag)
        local args = {job_item=job_item, ffield=ffield, flag=flag}
        table.insert(choices, {
            icon = curry(icon_cb, args, isActiveFlag),
            text = {{text = text,
                    pen = curry(pen_cb, args, isActiveFlag),
                    dpen = curry(pen_d_cb, args, isActiveFlag),
            }},
            ffield = ffield, flag = flag
        })
    end

    local isActiveTool = function (args)
        return df.tool_uses[args.tool_use] == args.job_item.has_tool_use
    end
    for _, tool_use in ipairs(tool_uses) do
        if tool_use == 'NONE' then
            table.insert(choices, {
                icon = '!',
                text = {{text = 'unset use', pen = pen_action, dpen = pen_action_d}},
                tool_use = tool_use
            })
        else
            local args = {job_item = job_item, tool_use=tool_use}
            table.insert(choices, {
                icon = ' ',
                text = {{text = 'has use ' .. tool_use,
                        pen = curry(pen_cb, args, isActiveTool),
                        dpen = curry(pen_d_cb, args, isActiveTool),
                }},
                tool_use = tool_use
            })
        end
    end

    local isActiveOre = function(args)
        return (args.job_item.metal_ore == args.mat_index)
    end
    table.insert(choices, {
            icon = '!',
            text = {{text = 'unset ore', pen = pen_action, dpen = pen_action_d}},
            ore_ix = -1
        })
    for _, ore in ipairs(ores) do
        local args = {job_item = job_item, mat_index=ore.mat_index}
        table.insert(choices, {
            icon = ' ',
            text = {{text = 'ore of ' .. ore.name,
                    pen = curry(pen_cb, args, isActiveOre),
                    dpen = curry(pen_d_cb, args, isActiveOre),
            }},
            ore_ix = ore.mat_index
        })
    end

    local isActiveReactionClass = function(args)
        return (args.job_item.reaction_class == args.reaction_class)
    end
    table.insert(choices, {
            icon = '!',
            text = {{text = 'unset reaction class', pen = pen_action, dpen = pen_action_d}},
            reaction_class = ''
        })
    for _, reaction_class in ipairs(reaction_classes) do
        local args = {job_item = job_item, reaction_class=reaction_class}
        table.insert(choices, {
            icon = ' ',
            text = {{text = 'reaction class ' .. reaction_class,
                    pen = curry(pen_cb, args, isActiveReactionClass),
                    dpen = curry(pen_d_cb, args, isActiveReactionClass)
            }},
            reaction_class = reaction_class
        })
    end

    local isActiveProduct = function(args)
        return (args.job_item.has_material_reaction_product == args.product_materials)
    end
    table.insert(choices, {
            icon = '!',
            text = {{text = 'unset producing', pen = pen_action, dpen = pen_action_d}},
            product_materials = ''
        })
    for _, product_materials in ipairs(product_materials) do
        local args = {job_item = job_item, product_materials=product_materials}
        table.insert(choices, {
            icon = ' ',
            text = {{text = product_materials .. '-producing',
                    pen = curry(pen_cb, args, isActiveProduct),
                    dpen = curry(pen_d_cb, args, isActiveProduct)
            }},
            product_materials = product_materials
        })
    end

    set_search_keys(choices)
    args.choices = choices

    if args.on_select then
        local cb = args.on_select
        args.on_select = function(idx, obj)
            return cb(obj)
        end
    else
        local function toggleFlag(obj, ffield, flag)
            local job_item = obj.job_item
            job_item[ffield][flag] = not job_item[ffield][flag]
        end

        local function toggleToolUse(obj, tool_use)
            local job_item = obj.job_item
            tool_use = df.tool_uses[tool_use]
            if job_item.has_tool_use == tool_use then
                job_item.has_tool_use = df.tool_uses.NONE
            else
                job_item.has_tool_use = tool_use
            end
        end

        local function toggleMetalOre(obj, ore_ix)
            local job_item = obj.job_item
            if job_item.metal_ore == ore_ix then
                job_item.metal_ore = -1
            else
                job_item.metal_ore = ore_ix
            end
        end

        function toggleReactionClass(obj, reaction_class)
            local job_item = obj.job_item
            if job_item.reaction_class == reaction_class then
                job_item.reaction_class = ''
            else
                job_item.reaction_class = reaction_class
            end
        end

        local function toggleProductMaterial(obj, product_materials)
            local job_item = obj.job_item
            if job_item.has_material_reaction_product == product_materials then
                job_item.has_material_reaction_product = ''
            else
                job_item.has_material_reaction_product = product_materials
            end
        end

        local function unsetFlags(obj)
            local job_item = obj.job_item
            for flag, ffield in pairs(job_item_flags_map) do
                if job_item[ffield][flag] then
                    toggleFlag(obj, ffield, flag)
                end
            end
        end

        local function setTrait(obj, sel)
            if sel.ffield then
                --print('toggle flag', sel.ffield, sel.flag)
                toggleFlag(obj, sel.ffield, sel.flag)
            elseif sel.reset_flags then
                --print('reset every flag')
                unsetFlags(obj)
            elseif sel.tool_use then
                --print('toggle tool_use', sel.tool_use)
                toggleToolUse(obj, sel.tool_use)
            elseif sel.ore_ix then
                --print('toggle ore', sel.ore_ix)
                toggleMetalOre(obj, sel.ore_ix)
            elseif sel.reaction_class then
                --print('toggle reaction class', sel.reaction_class)
                toggleReactionClass(obj, sel.reaction_class)
            elseif sel.product_materials then
                --print('toggle product materials', sel.product_materials)
                toggleProductMaterial(obj, sel.product_materials)
            elseif sel.reset_all_traits then
                --print('reset every trait')
                -- flags
                unsetFlags(obj)
                -- tool use
                toggleToolUse(obj, 'NONE')
                -- metal ore
                toggleMetalOre(obj, -1)
                -- reaction class
                toggleReactionClass(obj, '')
                -- producing
                toggleProductMaterial(obj, '')
            else
                print('unknown sel')
                printall(sel)
                error('Selected entry in ItemTraitsDialog was of unknown type')
            end
        end

        args.on_select = function(idx, choice)
            setTrait(args, choice)
        end
    end

    return dlg.ListBox(args)
end

return _ENV
