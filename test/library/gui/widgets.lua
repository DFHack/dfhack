local widgets = require('gui.widgets')

function test.togglehotkeylabel()
   local toggle = widgets.ToggleHotkeyLabel{}
   expect.true_(toggle:getOptionValue())
   toggle:cycle()
   expect.false_(toggle:getOptionValue())
   toggle:cycle()
   expect.true_(toggle:getOptionValue())
end

function test.togglehotkeylabel_default_value()
   local toggle = widgets.ToggleHotkeyLabel{initial_option=2}
   expect.false_(toggle:getOptionValue())

   toggle = widgets.ToggleHotkeyLabel{initial_option=false}
   expect.false_(toggle:getOptionValue())
end
