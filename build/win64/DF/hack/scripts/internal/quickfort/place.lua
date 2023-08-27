-- place-related data and logic for the quickfort script
--@ module = true
--[[
stockpiles data structure:
  list of {type, cells, pos, width, height, extent_grid}
- type: letter from stockpile designation screen
- cells: list of source spreadsheet cell labels (for debugging)
- pos: target map coordinates of upper left corner of extent (or nil if invalid)
- width, height: number between 1 and 31 (could be 0 if pos == nil)
- extent_grid: [x][y] -> boolean where 1 <= x <= width and 1 <= y <= height
]]

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

require('dfhack.buildings') -- loads additional functions into dfhack.buildings
local argparse = require('argparse')
local utils = require('utils')
local stockpiles = require('plugins.stockpiles')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_building = reqscript('internal/quickfort/building')
local quickfort_orders = reqscript('internal/quickfort/orders')
local quickfort_parse = reqscript('internal/quickfort/parse')
local quickfort_set = reqscript('internal/quickfort/set')

local log = quickfort_common.log

local function is_valid_stockpile_tile(pos)
    local flags, occupancy = dfhack.maps.getTileFlags(pos)
    if flags.hidden or occupancy.building ~= 0 then return false end
    local shape = df.tiletype.attrs[dfhack.maps.getTileType(pos)].shape
    return shape == df.tiletype_shape.FLOOR or
            shape == df.tiletype_shape.BOULDER or
            shape == df.tiletype_shape.PEBBLES or
            shape == df.tiletype_shape.STAIR_UP or
            shape == df.tiletype_shape.STAIR_DOWN or
            shape == df.tiletype_shape.STAIR_UPDOWN or
            shape == df.tiletype_shape.RAMP or
            shape == df.tiletype_shape.TWIG or
            shape == df.tiletype_shape.SAPLING or
            shape == df.tiletype_shape.SHRUB
end

local function is_valid_stockpile_extent(s)
    for extent_x, col in ipairs(s.extent_grid) do
        for extent_y, in_extent in ipairs(col) do
            if in_extent then return true end
        end
    end
    return false
end

local function merge_db_entries(self, other)
    if self.label ~= other.label then
        error(('cannot merge db entries of different types: %s != %s'):format(self.label, other.label))
    end
    utils.assign(self.props, other.props)
    utils.assign(self.logistics, other.logistics)
    for adj in pairs(other.adjustments) do
        self.adjustments[adj] = true
    end
    for _, to in ipairs(other.links.give_to) do
        table.insert(self.links.give_to, to)
    end
    for _, from in ipairs(other.links.take_from) do
        table.insert(self.links.take_from, from)
    end
end

local stockpile_template = {
    has_extents=true, min_width=1, max_width=math.huge, min_height=1, max_height=math.huge,
    is_valid_tile_fn = is_valid_stockpile_tile,
    is_valid_extent_fn = is_valid_stockpile_extent,
    merge_fn = merge_db_entries,
}

local stockpile_db_raw = {
    a={label='Animal', categories={'animals'}},
    f={label='Food', categories={'food'}, want_barrels=true},
    u={label='Furniture', categories={'furniture'}},
    n={label='Coins', categories={'coins'}, want_bins=true},
    y={label='Corpses', categories={'corpses'}},
    r={label='Refuse', categories={'refuse'}},
    s={label='Stone', categories={'stone'}, want_wheelbarrows=true},
    w={label='Wood', categories={'wood'}},
    e={label='Gem', categories={'gems'}, want_bins=true},
    b={label='Bars and Blocks', categories={'bars_blocks'}, want_bins=true},
    h={label='Cloth', categories={'cloth'}, want_bins=true},
    l={label='Leather', categories={'leather'}, want_bins=true},
    z={label='Ammo', categories={'ammo'}, want_bins=true},
    S={label='Sheets', categories={'sheets'}, want_bins=true},
    g={label='Finished Goods', categories={'finished_goods'}, want_bins=true},
    p={label='Weapons', categories={'weapons'}, want_bins=true},
    d={label='Armor', categories={'armor'}, want_bins=true},
    c={label='Custom', categories={}}
}
for _, v in pairs(stockpile_db_raw) do utils.assign(v, stockpile_template) end

local place_key_pattern = '%w+'

local function parse_keys(keys)
    local token_and_label, props_start_pos = quickfort_parse.parse_token_and_label(keys, 1, place_key_pattern)
    local props, next_token_pos = quickfort_parse.parse_properties(keys, props_start_pos)
    local adjustments = quickfort_parse.parse_stockpile_transformations(keys, next_token_pos)
    return token_and_label, props, adjustments
end

local function add_resource_digit(cur_val, digit)
    return (cur_val * 10) + digit
end

local function make_db_entry(keys)
    local labels, categories = {}, {}
    local want_bins, want_barrels, want_wheelbarrows = false, false, false
    local num_bins, num_barrels, num_wheelbarrows = nil, nil, nil
    local prev_key, in_digits = nil, false
    for k in keys:gmatch('.') do
        local digit = tonumber(k)
        if digit and prev_key then
            local raw_db_entry = rawget(stockpile_db_raw, prev_key)
            if raw_db_entry.want_bins then
                if not in_digits then num_bins = 0 end
                num_bins = add_resource_digit(num_bins, digit)
            elseif raw_db_entry.want_barrels then
                if not in_digits then num_barrels = 0 end
                num_barrels = add_resource_digit(num_barrels, digit)
            else
                if not in_digits then num_wheelbarrows = 0 end
                num_wheelbarrows = add_resource_digit(num_wheelbarrows, digit)
            end
            in_digits = true
            goto continue
        end
        if not rawget(stockpile_db_raw, k) then return nil end
        table.insert(labels, stockpile_db_raw[k].label)
        table.insert(categories, stockpile_db_raw[k].categories[1])
        want_bins = want_bins or stockpile_db_raw[k].want_bins
        want_barrels = want_barrels or stockpile_db_raw[k].want_barrels
        want_wheelbarrows =
                want_wheelbarrows or stockpile_db_raw[k].want_wheelbarrows
        prev_key = k
        -- flag that we're starting a new (potential) digit sequence and we
        -- should reset the accounting for the relevent resource number
        in_digits = false
        ::continue::
    end
    local db_entry = {
        label=table.concat(labels, '+'),
        categories=categories,
        want_bins=want_bins,
        want_barrels=want_barrels,
        want_wheelbarrows=want_wheelbarrows,
        num_bins=num_bins,
        num_barrels=num_barrels,
        num_wheelbarrows=num_wheelbarrows,
        links={give_to={}, take_from={}},
        props={},
        adjustments={},
        logistics={},
    }
    utils.assign(db_entry, stockpile_template)
    return db_entry
end

local function custom_stockpile(_, keys)
    local token_and_label, props, adjustments = parse_keys(keys)
    local db_entry = make_db_entry(token_and_label.token)
    if not db_entry then return nil end
    if token_and_label.label then
        db_entry.label = ('%s/%s'):format(db_entry.label, token_and_label.label)
    end
    if next(adjustments) then
        db_entry.adjustments[adjustments] = true
    end

    -- logistics properties
    if props.automelt == 'true' then
        db_entry.logistics.automelt = true
        props.automelt = nil
    end
    if props.autotrade == 'true' then
        db_entry.logistics.autotrade = true
        props.autotrade = nil
    end
    if props.autodump == 'true' then
        db_entry.logistics.autodump = true
        props.autodump = nil
    end
    if props.autotrain == 'true' then
        db_entry.logistics.autotrain = true
        props.autotrain = nil
    end

    -- convert from older parsing style to properties
    db_entry.props.max_barrels = db_entry.num_barrels
    db_entry.num_barrels = nil
    db_entry.props.max_bins = db_entry.num_bins
    db_entry.num_bins = nil
    db_entry.props.max_wheelbarrows = db_entry.num_wheelbarrows
    db_entry.num_wheelbarrows = nil

    -- alias properties
    if props.quantum == 'true' then
        props.links_only = 'true'
        props.containers = 0
        props.quantum = nil
    end
    if props.containers then
        props.barrels = props.containers
        props.bins = props.containers
        props.wheelbarrows = props.containers
        props.containers = nil
    end

    -- actual properties
    if props.barrels then
        db_entry.props.max_barrels = tonumber(props.barrels)
        props.barrels = nil
    end
    if props.bins then
        db_entry.props.max_bins = tonumber(props.bins)
        props.bins = nil
    end
    if props.wheelbarrows then
        db_entry.props.max_wheelbarrows = tonumber(props.wheelbarrows)
        props.wheelbarrows = nil
    end
    if props.links_only == 'true' then
        db_entry.props.use_links_only = 1
        props.links_only = nil
    end
    if props.name then
        db_entry.props.name = props.name
        props.name = nil
    end
    if props.take_from then
        db_entry.links.take_from = argparse.stringList(props.take_from)
        props.take_from = nil
    end
    if props.give_to then
        db_entry.links.give_to = argparse.stringList(props.give_to)
        props.give_to = nil
    end

    for k,v in pairs(props) do
        dfhack.printerr(('unhandled property: "%s"="%s"'):format(k, v))
    end

    return db_entry
end

local stockpile_db = {}
setmetatable(stockpile_db, {__index=custom_stockpile})

local function configure_stockpile(bld, db_entry)
    for _,cat in ipairs(db_entry.categories) do
        local name = ('library/cat_%s'):format(cat)
        log('enabling stockpile category: %s', cat)
        stockpiles.import_stockpile(name, {id=bld.id, mode='enable'})
    end
    for adjlist in pairs(db_entry.adjustments or {}) do
        for _,adj in ipairs(adjlist) do
            log('applying stockpile preset: %s %s (filters=)', adj.mode, adj.name, table.concat(adj.filters or {}, ','))
            stockpiles.import_stockpile(adj.name, {id=bld.id, mode=adj.mode, filters=adj.filters})
        end
    end
end

local function init_containers(db_entry, ntiles)
    if db_entry.want_barrels or db_entry.props.max_barrels then
        local max_barrels = db_entry.props.max_barrels or
                quickfort_set.get_setting('stockpiles_max_barrels')
        db_entry.props.max_barrels = (max_barrels < 0 or max_barrels >= ntiles) and ntiles or max_barrels
        log('barrels set to %d', db_entry.props.max_barrels)
    end
    if db_entry.want_bins or db_entry.props.max_bins then
        local max_bins = db_entry.props.max_bins or
                quickfort_set.get_setting('stockpiles_max_bins')
        db_entry.props.max_bins = (max_bins < 0 or max_bins >= ntiles) and ntiles or max_bins
        log('bins set to %d', db_entry.props.max_bins)
    end
    if db_entry.want_wheelbarrows or db_entry.props.max_wheelbarrows then
        local max_wb = db_entry.props.max_wheelbarrows or
                quickfort_set.get_setting('stockpiles_max_wheelbarrows')
        if max_wb < 0 then max_wb = 1 end
        db_entry.props.max_wheelbarrows = (max_wb >= ntiles - 1) and ntiles-1 or max_wb
        log('wheelbarrows set to %d', db_entry.props.max_wheelbarrows)
    end
end

local function create_stockpile(s, link_data, dry_run)
    local db_entry = s.db_entry
    log('creating %s stockpile at map coordinates (%d, %d, %d), defined from' ..
        ' spreadsheet cells: %s',
        db_entry.label, s.pos.x, s.pos.y, s.pos.z, table.concat(s.cells, ', '))
    local extents, ntiles = quickfort_building.make_extents(s, dry_run)
    local fields = {room={x=s.pos.x, y=s.pos.y, width=s.width, height=s.height,
                          extents=extents}}
    init_containers(db_entry, ntiles)
    if dry_run then return ntiles end
    local bld, err = dfhack.buildings.constructBuilding{
        type=df.building_type.Stockpile, abstract=true, pos=s.pos,
        width=s.width, height=s.height, fields=fields}
    if not bld then
        -- this is an error instead of a qerror since our validity checking
        -- is supposed to prevent this from ever happening
        error(string.format('unable to place stockpile: %s', err))
    end
    local props = db_entry.props
    configure_stockpile(bld, db_entry)
    utils.assign(bld, props)
    if props.name then
        table.insert(ensure_key(link_data.piles, props.name), bld)
    end
    for _,recipient in ipairs(db_entry.links.give_to) do
        log('giving to: "%s"', recipient)
        table.insert(link_data.nodes, {from=bld, to=recipient})
    end
    for _,supplier in ipairs(db_entry.links.take_from) do
        log('taking from: "%s"', supplier)
        table.insert(link_data.nodes, {from=supplier, to=bld})
    end
    if next(db_entry.logistics) then
        local logistics_command = {'logistics', 'add', '-s', tostring(bld.stockpile_number)}
        if db_entry.logistics.automelt then
            table.insert(logistics_command, 'melt')
        end
        if db_entry.logistics.autotrade then
            table.insert(logistics_command, 'trade')
        end
        if db_entry.logistics.autodump then
            table.insert(logistics_command, 'dump')
        end
        if db_entry.logistics.autotrain then
            table.insert(logistics_command, 'train')
        end
        log('running logistics command: "%s"', table.concat(logistics_command, ' '))
        dfhack.run_command(logistics_command)
    end
    return ntiles
end

function get_stockpiles_by_name()
    local piles = {}
    for _, pile in ipairs(df.global.world.buildings.other.STOCKPILE) do
        if #pile.name > 0 then
            table.insert(ensure_key(piles, pile.name), pile)
        end
    end
    return piles
end

local function get_workshops_by_name()
    local shops = {}
    for _, shop in ipairs(df.global.world.buildings.other.WORKSHOP_ANY) do
        if #shop.name > 0 then
            table.insert(ensure_key(shops, shop.name), shop)
        end
    end
    return shops
end

local function get_pile_targets(name, peer_piles, all_piles)
    if peer_piles[name] then return peer_piles[name], all_piles end
    all_piles = all_piles or get_stockpiles_by_name()
    return all_piles[name], all_piles
end

local function get_shop_targets(name, all_shops)
    all_shops = all_shops or get_workshops_by_name()
    return all_shops[name], all_shops
end

-- will link to stockpiles created in this blueprint
-- if no match, will search all stockpiles
-- if no match, will search all workshops
local function link_stockpiles(link_data)
    local all_piles, all_shops
    for _,node in ipairs(link_data.nodes) do
        if type(node.from) == 'string' then
            local name = node.from
            node.from, all_piles = get_pile_targets(name, link_data.piles, all_piles)
            if node.from then
                for _,from in ipairs(node.from) do
                    utils.insert_sorted(from.links.give_to_pile, node.to, 'id')
                    utils.insert_sorted(node.to.links.take_from_pile, from, 'id')
                end
            else
                node.from, all_shops = get_shop_targets(name, all_shops)
                if node.from then
                    for _,from in ipairs(node.from) do
                        utils.insert_sorted(from.profile.links.give_to_pile, node.to, 'id')
                        utils.insert_sorted(node.to.links.take_from_workshop, from, 'id')
                    end
                else
                    dfhack.printerr(('cannot find stockpile or workshop named "%s" to take from'):format(name))
                end
            end
        elseif type(node.to) == 'string' then
            local name = node.to
            node.to, all_piles = get_pile_targets(name, link_data.piles, all_piles)
            if node.to then
                for _,to in ipairs(node.to) do
                    utils.insert_sorted(node.from.links.give_to_pile, to, 'id')
                    utils.insert_sorted(to.links.take_from_pile, node.from, 'id')
            end
            else
                node.to, all_shops = get_shop_targets(name, all_shops)
                if node.to then
                    for _,to in ipairs(node.to) do
                        utils.insert_sorted(node.from.links.give_to_workshop, to, 'id')
                        utils.insert_sorted(to.profile.links.take_from_pile, node.from, 'id')
                    end
                else
                    dfhack.printerr(('cannot find stockpile or workshop named "%s" to give to'):format(name))
                end
            end
        end
    end
end

function do_run(zlevel, grid, ctx)
    local stats = ctx.stats
    stats.place_designated = stats.place_designated or
            {label='Stockpiles designated', value=0, always=true}
    stats.place_tiles = stats.place_tiles or
            {label='Stockpile tiles designated', value=0}
    stats.place_occupied = stats.place_occupied or
            {label='Stockpile tiles skipped (tile occupied)', value=0}

    local piles = {}
    stats.invalid_keys.value =
            stats.invalid_keys.value + quickfort_building.init_buildings(
                ctx, zlevel, grid, piles, stockpile_db)
    stats.out_of_bounds.value =
            stats.out_of_bounds.value + quickfort_building.crop_to_bounds(
                ctx, piles, stockpile_db)
    stats.place_occupied.value =
            stats.place_occupied.value +
            quickfort_building.check_tiles_and_extents(
                ctx, piles, stockpile_db)

    local dry_run = ctx.dry_run
    local link_data = {piles={}, nodes={}}
    for _, s in ipairs(piles) do
        if s.pos then
            local ntiles = create_stockpile(s, link_data, dry_run)
            stats.place_tiles.value = stats.place_tiles.value + ntiles
            stats.place_designated.value = stats.place_designated.value + 1
        end
    end
    if dry_run then return end
    link_stockpiles(link_data)
    dfhack.job.checkBuildingsNow()
end

-- enqueues orders only for explicitly requested containers
function do_orders(zlevel, grid, ctx)
    local piles = {}
    quickfort_building.init_buildings(ctx, zlevel, grid, piles, stockpile_db)
    for _, s in ipairs(piles) do
        local db_entry = s.db_entry
        local props = db_entry.props or {}
        quickfort_orders.enqueue_container_orders(ctx,
            props.max_bins, props.max_barrels, props.max_wheelbarrows)
    end
end

function do_undo(zlevel, grid, ctx)
    local stats = ctx.stats
    stats.place_removed = stats.place_removed or
            {label='Stockpiles removed', value=0, always=true}

    local piles = {}
    stats.invalid_keys.value =
            stats.invalid_keys.value + quickfort_building.init_buildings(
                ctx, zlevel, grid, piles, stockpile_db)

    -- ensure we don't delete the currently selected stockpile, which causes crashes.
    local selected_pile = dfhack.gui.getSelectedStockpile(true)

    for _, s in ipairs(piles) do
        for extent_x, col in ipairs(s.extent_grid) do
            for extent_y, in_extent in ipairs(col) do
                if not in_extent then goto continue end
                local pos =
                        xyz2pos(s.pos.x+extent_x-1, s.pos.y+extent_y-1, s.pos.z)
                local bld = dfhack.buildings.findAtTile(pos)
                if bld and bld:getType() == df.building_type.Stockpile then
                    if not ctx.dry_run then
                        if bld == selected_pile then
                            dfhack.printerr('cannot remove actively selected stockpile.')
                            dfhack.printerr('please deselect the stockpile and try again.')
                        else
                            dfhack.buildings.deconstruct(bld)
                        end
                    end
                    stats.place_removed.value = stats.place_removed.value + 1
                end
                ::continue::
            end
        end
    end
end
