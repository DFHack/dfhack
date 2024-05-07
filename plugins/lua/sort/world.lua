local _ENV = mkmodule('plugins.sort.world')

local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

-- ----------------------
-- WorldOverlay
--

WorldOverlay = defclass(WorldOverlay, sortoverlay.SortOverlay)
WorldOverlay.ATTRS{
    desc='Adds search functionality to the artifact list on the world raid screen.',
    default_pos={x=-18, y=2},
    viewscreens='world/ARTIFACTS',
    frame={w=40, h=1},
}

local function get_world_artifact_search_key(artifact, rumor)
    local search_key = ('%s %s'):format(dfhack.TranslateName(artifact.name, true),
        dfhack.items.getDescription(artifact.item, 0))
    if rumor then
        local hf = df.historical_figure.find(rumor.hfid)
        if hf then
            search_key = ('%s %s %s'):format(search_key,
                dfhack.TranslateName(hf.name),
                dfhack.TranslateName(hf.name, true))
        end
        local ws = df.world_site.find(rumor.stid)
        if ws then
            search_key = ('%s %s'):format(search_key,
                dfhack.TranslateName(ws.name, true))
        end
    else
        local hf = df.historical_figure.find(artifact.holder_hf)
        if hf then
            local unit = df.unit.find(hf.unit_id)
            if unit then
                search_key = ('%s %s'):format(search_key,
                    dfhack.units.getReadableName(unit))
            end
        end
    end
    return search_key
end

local function cleanup_artifact_vectors(data)
    local vs_world = dfhack.gui.getDFViewscreen(true)
    if not df.viewscreen_worldst:is_instance(vs_world) then return end
    vs_world.artifact:assign(data.saved_original)
    vs_world.artifact:resize(data.saved_original_size)
    vs_world.artifact_arl:assign(data.saved_flags)
    vs_world.artifact_arl:resize(data.saved_flags_size)
end

function WorldOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            frame={l=0, t=0, r=0, h=1},
            visible=self:callback('get_key'),
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=1, t=0, r=1},
                    label_text="Search: ",
                    key='CUSTOM_ALT_S',
                    on_change=function(text) self:do_search(text) end,
                },
            },
        },
    }

    self:register_handler('ARTIFACTS',
        function() return dfhack.gui.getDFViewscreen(true).artifact end,
        curry(sortoverlay.flags_vector_search,
            {
                get_search_key_fn=get_world_artifact_search_key,
                get_elem_id_fn=function(artifact_record) return artifact_record.id end,
            },
            function() return dfhack.gui.getDFViewscreen(true).artifact_arl end),
        cleanup_artifact_vectors)
end

function WorldOverlay:get_key()
    local scr = dfhack.gui.getDFViewscreen(true)
    if scr.view_mode == df.world_view_mode_type.ARTIFACTS then
        return 'ARTIFACTS'
    end
end

return _ENV
