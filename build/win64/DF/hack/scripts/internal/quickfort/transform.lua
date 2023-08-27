-- logic for euclidean transformations of blueprints for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

-- y coords are inverted since the DF map y axis is inverted
unit_vectors = {
    north={x=0, y=-1},
    east={x=1, y=0},
    south={x=0, y=1},
    west={x=-1, y=0}
}

unit_vectors_revmap = {
    north='x=0, y=-1',
    east='x=1, y=0',
    south='x=0, y=1',
    west='x=-1, y=0'
}

function resolve_vector(transformed_vector, revmap)
    local serialized = ('x=%d, y=%d')
            :format(transformed_vector.x, transformed_vector.y)
    return revmap[serialized]
end

-- the revmap maps serialized vector strings to a string. for example:
-- local bridge_revmap = {
--     [unit_vectors_revmap.north]='gw',
--     [unit_vectors_revmap.east]='gd',
--     [unit_vectors_revmap.south]='gx',
--     [unit_vectors_revmap.west]='ga'
-- }
function resolve_transformed_vector(ctx, vector, revmap)
    return resolve_vector(ctx.transform_fn(vector, true), revmap)
end

local function make_transform_fn(tfn)
    return function(pos, origin_pos)
        -- shift pos to treat origin_pos as the origin
        local pos = xy2pos(pos.x-origin_pos.x, pos.y-origin_pos.y)
        -- apply transformation
        pos = tfn(pos)
        -- undo origin shift
        return xy2pos(pos.x+origin_pos.x, pos.y+origin_pos.y)
    end
end

-- the y axis is reversed in DF so the normal logic for cw and ccw is reversed.
local function transform_cw(tpos) return xy2pos(-tpos.y, tpos.x) end
local function transform_ccw(tpos) return xy2pos(tpos.y, -tpos.x) end
local function transform_fliph(tpos) return xy2pos(-tpos.x, tpos.y) end
local function transform_flipv(tpos) return xy2pos(tpos.x, -tpos.y) end

function make_transform_fn_from_name(name)
    if name == 'rotcw' or name == 'cw' then
        return make_transform_fn(transform_cw)
    elseif name == 'rotccw' or name == 'ccw' then
        return make_transform_fn(transform_ccw)
    elseif name == 'fliph' then
        return make_transform_fn(transform_fliph)
    elseif name == 'flipv' then
        return make_transform_fn(transform_flipv)
    else
        qerror('invalid transformation name: '..name)
    end
end
