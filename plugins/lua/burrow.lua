local _ENV = mkmodule('plugins.burrow')

--[[

 Native events:

 * onBurrowRename(burrow)
 * onDigComplete(job_type,pos,old_tiletype,new_tiletype)

 Native functions:

 * findByName(name) -> burrow
 * copyUnits(dest,src,enable)
 * copyTiles(dest,src,enable)
 * setTilesByKeyword(dest,kwd,enable) -> success

 'enable' selects between add and remove modes

--]]

local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

---------------------------------
-- BurrowDesignationOverlay
--

local selection_rect = df.global.selection_rect
local if_burrow = df.global.game.main_interface.burrow

local function is_choosing_area(pos)
    return if_burrow.doing_rectangle and
        selection_rect.start_x >= 0 and
        (pos or dfhack.gui.getMousePos())
end

local function reset_selection_rect()
    selection_rect.start_x = -30000
    selection_rect.start_y = -30000
    selection_rect.start_z = -30000
end

local function get_bounds(pos1, pos2)
    pos1 = pos1 or dfhack.gui.getMousePos()
    pos2 = pos2 or xyz2pos(selection_rect.start_x, selection_rect.start_y, selection_rect.start_z)
    local bounds = {
        x1=math.min(pos1.x, pos2.x),
        x2=math.max(pos1.x, pos2.x),
        y1=math.min(pos1.y, pos2.y),
        y2=math.max(pos1.y, pos2.y),
        z1=math.min(pos1.z, pos2.z),
        z2=math.max(pos1.z, pos2.z),
    }

    -- clamp to map edges
    bounds = {
        x1=math.max(0, bounds.x1),
        x2=math.min(df.global.world.map.x_count-1, bounds.x2),
        y1=math.max(0, bounds.y1),
        y2=math.min(df.global.world.map.y_count-1, bounds.y2),
        z1=math.max(0, bounds.z1),
        z2=math.min(df.global.world.map.z_count-1, bounds.z2),
    }

    return bounds
end

local function get_cur_area_dims()
    local bounds = get_bounds()
    return bounds.x2 - bounds.x1 + 1,
            bounds.y2 - bounds.y1 + 1,
            bounds.z2 - bounds.z1 + 1
end

BurrowDesignationOverlay = defclass(BurrowDesignationOverlay, overlay.OverlayWidget)
BurrowDesignationOverlay.ATTRS{
    default_pos={x=6,y=9},
    viewscreens='dwarfmode/Burrow/Paint',
    default_enabled=true,
    frame={w=54, h=1},
}

function BurrowDesignationOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            frame={t=0, l=0},
            subviews={
                widgets.Label{
                    frame={t=0, l=1},
                    text='Double-click to fill.',
                    auto_width=true,
                },
                widgets.Label{
                    frame={t=0, l=25},
                    text_pen=COLOR_DARKGREY,
                    text={
                        'Selected area: ',
                        {text=function() return ('%dx%dx%d'):format(get_cur_area_dims()) end},
                    },
                    visible=is_choosing_area,
                },
            },
        },
    }
end

local function flood_fill(pos, erasing, painting_burrow)
    if erasing then
        burrow_tiles_flood_remove(painting_burrow, pos)
    else
        burrow_tiles_flood_add(painting_burrow, pos)
    end
    reset_selection_rect()
end

local function box_fill(bounds, erasing, painting_burrow)
    if bounds.z1 == bounds.z2 then return end
    local pos1 = xyz2pos(bounds.x1, bounds.y1, bounds.z1)
    local pos2 = xyz2pos(bounds.x2, bounds.y2, bounds.z2)
    if erasing then
        burrow_tiles_box_remove(painting_burrow, pos1, pos2)
    else
        burrow_tiles_box_add(painting_burrow, pos1, pos2)
    end
end

function BurrowDesignationOverlay:onInput(keys)
    if self:inputToSubviews(keys) then
        return true
    -- don't perform burrow modifications immediately -- painting_burrow may not yet
    -- have been initialized. instead, allow clicks to go through so that vanilla
    -- behavior is triggered before we modify the burrow further
    elseif keys._MOUSE_L then
        local pos = dfhack.gui.getMousePos()
        if pos then
            local now_ms = dfhack.getTickCount()
            if not same_xyz(pos, self.saved_pos) then
                self.last_click_ms = now_ms
                self.saved_pos = pos
            else
                if now_ms - self.last_click_ms <= widgets.DOUBLE_CLICK_MS then
                    self.last_click_ms = 0
                    self.pending_fn = curry(flood_fill, pos, if_burrow.erasing)
                    return
                else
                    self.last_click_ms = now_ms
                end
            end
            if is_choosing_area(pos) then
                self.pending_fn = curry(box_fill, get_bounds(pos), if_burrow.erasing)
                return
            end
        end
    end
end

function BurrowDesignationOverlay:onRenderBody(dc)
    BurrowDesignationOverlay.super.onRenderBody(self, dc)
    local pending_fn = self.pending_fn
    self.pending_fn = nil
    if pending_fn and if_burrow.painting_burrow then
        pending_fn(if_burrow.painting_burrow)
    end
end

OVERLAY_WIDGETS = {
    designation=BurrowDesignationOverlay,
}

rawset_default(_ENV, dfhack.burrows)

---------------------------------
-- commandline handling
--

local function set_add_remove(mode, which, params, opts)
    local target_burrow = table.remove(params, 1)
    return _ENV[('burrow_%s_%s'):format(mode, which)](target_burrow, params, opts)
end

local function tiles_box_add_remove(which, params, opts)
    local target_burrow = table.remove(params, 1)
    local pos1 = argparse.coords(params[1] or 'here', 'pos')
    local pos2 = opts.cursor or argparse.coords(params[2] or 'here', 'pos')
    local bounds = get_bounds(pos1, pos2)
    return _ENV['burrow_tiles_box_'..which](target_burrow, bounds, opts)
end

local function tiles_flood_add_remove(which, params, opts)
    local target_burrow = table.remove(params, 1)
    local pos = opts.cursor or argparse.coords('here', 'pos')
    return _ENV['burrow_tiles_flood_'..which](target_burrow, pos, opts)
end

local function run_command(mode, command, params, opts)
    if mode == 'tiles' then
        if command == 'clear' then
            return burrow_tiles_clear(params, opts)
        elseif command == 'set' or command == 'add' or command == 'remove' then
            return set_add_remove('tiles', command, params, opts)
        elseif command == 'box-add' or command == 'box-remove' then
            return tiles_box_add_remove(command:sub(5), params, opts)
        elseif command == 'flood-add' or command == 'flood-remove' then
            return tiles_flood_add_remove(command:sub(7), params, opts)
        else
            return false
        end
    elseif mode == 'units' then
        if command == 'clear' then
            return burrow_units_clear(params)
        elseif command == 'set' or command == 'add' or command == 'remove' then
            return set_add_remove('units', command, params, opts)
        else
            return false
        end
    else
        return false
    end
end

function parse_commandline(...)
    local args, opts = {...}, {}

    if args[1] == 'help' then
        return false
    end

    local positionals = argparse.processArgsGetopt(args, {
            {'c', 'cursor', hasArg=true,
             handler=function(optarg) opts.cursor = argparse.coords(optarg, 'cursor') end},
            {'d', 'dry-run', handler=function() opts.dry_run = true end},
            {'h', 'help', handler=function() opts.help = true end},
            {'z', 'cur-zlevel', handler=function() opts.zlevel = true end},
        })

    if opts.help then
        return false
    end

    local mode = table.remove(positionals, 1)
    local command = table.remove(positionals, 1)
    local ret = run_command(mode, command, positionals, opts)

    if not ret then return false end

    print(('%d %s affected'):format(ret, mode))
    return true
end

return _ENV
