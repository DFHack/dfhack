local _ENV = mkmodule('plugins.design')

view2 = {design_window = { name = "hello"}}

function getPen(hi)
    design_getPen(hi)
end

return _ENV
