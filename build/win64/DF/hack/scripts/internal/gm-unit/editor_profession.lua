-- Profession editor module for gui/gm-unit.
--@ module = true

local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")

Editor_Prof = defclass(Editor_Prof, base_editor.Editor)
Editor_Prof.ATTRS{
    frame_title = "Profession editor"
}

function Editor_Prof:init()
    local u = self.target_unit
    local opts = {}
    local craw = df.creature_raw.find(u.race)
    for i in ipairs(df.profession) do
        if i ~= df.profession.NONE then
            local attrs = df.profession.attrs[i]
            local caption = attrs.caption or '?'
            local tile = string.char(attrs.military and craw.creature_soldier_tile ~= 0 and
                craw.creature_soldier_tile or craw.creature_tile)
            table.insert(opts, {
                text = {
                    (i == u.profession and '*' or ' ') .. ' ',
                    {text = tile, pen = dfhack.units.getCasteProfessionColor(u.race, u.caste, i)},
                    ' ' .. caption
                },
                profession = i,
                search_key = caption:lower(),
            })
        end
    end

    self:addviews{
        widgets.FilteredList{
            frame = {t=0, l=0, b=0},
            choices = opts,
            view_id = 'professions',
            on_submit = self:callback('save_profession'),
        },
    }
end

function Editor_Prof:save_profession(_, choice)
    self.target_unit.profession = choice.profession
    self.target_unit.profession2 = choice.profession
end

function Editor_Prof:onOpen()
    self.subviews[1].edit:setFocus(true)
end
