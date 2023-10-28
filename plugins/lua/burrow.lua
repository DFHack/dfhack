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

local function get_bounds(pos)
    pos = pos or dfhack.gui.getMousePos()
    local bounds = {
        x1=math.min(selection_rect.start_x, pos.x),
        x2=math.max(selection_rect.start_x, pos.x),
        y1=math.min(selection_rect.start_y, pos.y),
        y2=math.max(selection_rect.start_y, pos.y),
        z1=math.min(selection_rect.start_z, pos.z),
        z2=math.max(selection_rect.start_z, pos.z),
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
    default_pos={x=6,y=8},
    viewscreens='dwarfmode/Burrow/Paint',
    default_enabled=true,
    frame={w=30, h=2},
}

function BurrowDesignationOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            frame={t=0, l=0},
            subviews={
                widgets.Label{
                    frame={t=0, l=1},
                    text='Double-click to flood fill.',
                },
            },
        },
        widgets.BannerPanel{
            frame={t=1, l=0},
            subviews={
                widgets.Label{
                    frame={t=0, l=1},
                    text_pen=COLOR_DARKGREY,
                    text={
                        'Selected area: ',
                        {text=function() return ('%dx%dx%d'):format(get_cur_area_dims()) end},
                    },
                },
            },
            visible=is_choosing_area,
        },
    }
end

local function flood_fill(pos, painting_burrow)
    print('flood fill, erasing:', if_burrow.erasing)
    print('origin pos:')
    printall(pos)
    reset_selection_rect()
end

local function box_fill(bounds, painting_burrow)
    print('box fill, erasing:', if_burrow.erasing)
    print('bounds:')
    printall(bounds)
    if bounds.z1 == bounds.z2 then return end
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
                    self.pending_fn = curry(flood_fill, pos)
                    return
                else
                    self.last_click_ms = now_ms
                end
            end
            if is_choosing_area(pos) then
                self.pending_fn = curry(box_fill, get_bounds(pos))
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

return _ENV
