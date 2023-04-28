local _ENV = mkmodule('plugins.design')

function draw_shape(arr)
    design_draw_shape(arr)
end

function draw_points(points_obj)
    design_draw_points(points_obj)
end

function clear_shape(arr)
    design_clear_shape(arr)
end

return _ENV
