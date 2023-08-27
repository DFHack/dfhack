-- quickstart visual help manual

local gui = require('gui')
local widgets = require('gui.widgets')

local GUIDE_FILE = 'hack/docs/docs/Quickstart.txt'

local function add_section_widget(sections, section)
    if #section == 0 then return end
    local section_text = table.concat(section, NEWLINE)
    local widget = widgets.WrappedLabel{
        frame={t=0},
        text_to_wrap=section_text,
    }
    table.insert(sections, widget)
end
local function get_sections()
    local sections, section = {}, {}
    local ok, lines = pcall(io.lines, GUIDE_FILE)
    if not ok then
        table.insert(section, 'Guide text not found!')
        add_section_widget(sections, section)
        return sections
    end
    local prev_line = nil
    for line in lines do
        if line:match('^[=-]+$') then
            add_section_widget(sections, section)
            section = {}
        end
        table.insert(section, prev_line)
        prev_line = line
    end
    table.insert(section, prev_line)
    add_section_widget(sections, section)
    return sections
end

Quickstart = defclass(Quickstart, widgets.Window)
Quickstart.ATTRS {
    frame_title='DFHack Quickstart Guide',
    frame={w=50, h=45},
    resizable=true,
    resize_min={w=43, h=20},
}

function Quickstart:init()
    local is_not_first_page_fn = function()
        return self.subviews.pages:getSelected() > 1
    end
    local is_not_last_page_fn = function()
        return self.subviews.pages:getSelected() < #self.subviews.pages.subviews
    end
    local prev_page_fn = function()
        local pages = self.subviews.pages
        pages:setSelected(pages:getSelected() - 1)
    end
    local next_page_fn = function()
        local pages = self.subviews.pages
        pages:setSelected(pages:getSelected() + 1)
    end

    self:addviews{
      widgets.Pages{
          view_id='pages',
          frame={t=0, l=0, r=0, b=2},
          subviews=get_sections(),
      },
      widgets.HotkeyLabel{
          frame={l=0, b=0},
          label='Back',
          auto_width=true,
          key='KEYBOARD_CURSOR_LEFT',
          on_activate=prev_page_fn,
          active=is_not_first_page_fn,
          enabled=is_not_first_page_fn,
      },
      widgets.Label{
          frame={l=0, b=0, w=1},
          text_pen=COLOR_LIGHTGREEN,
          text=string.char(27),
      },
      widgets.Label{
          frame={b=0},
          auto_width=true,
          text={
              'Page ',
              {text=function() return (self.subviews.pages:getSelected()) end},
              ' of ',
              {text=function() return #self.subviews.pages.subviews end}
          },
      },
      widgets.HotkeyLabel{
          frame={r=0, b=0},
          label='Next',
          auto_width=true,
          key='KEYBOARD_CURSOR_RIGHT',
          on_activate=next_page_fn,
          active=is_not_last_page_fn,
          enabled=is_not_last_page_fn,
      },
      widgets.Label{
          frame={r=6, b=0, w=1},
          text_pen=COLOR_LIGHTGREEN,
          text=string.char(26),
      },
    }
end

QuickstartScreen = defclass(QuickstartScreen, gui.ZScreen)
QuickstartScreen.ATTRS {
    focus_path='quickstart',
}

function QuickstartScreen:init()
    self:addviews{Quickstart{}}
end

function QuickstartScreen:onDismiss()
    view = nil
end

view = view and view:raise() or QuickstartScreen{}:show()
