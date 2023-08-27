-- Utilities for design.lua
--@ module = true

-- Point class used by gui/design
Point = defclass(Point)
Point.ATTRS {
    __is_point = true,
    x = DEFAULT_NIL,
    y = DEFAULT_NIL,
    z = DEFAULT_NIL
}

function Point:init(init_table)
    self.x = init_table.x
    self.y = init_table.y
    if init_table.z then self.z = init_table.z end
end

function Point:check_valid(point)
    point = point or self
    if not point.x or not point.y then error("Invalid Point: x and y values are required") end
    if not type(point.x) == "number" then error("Invalid value for x, must be a number") end
    if not type(point.y) == "number" then error("Invalid value for y, must be a number") end
    if point.z and not type(point.y) == "number" then error("Invalid value for z, must be a number") end
end

function Point:is_mouse_over()
    local pos = dfhack.gui.getMousePos()
    if not pos then return false end

    return Point(pos) == self
end

function Point:__tostring()
    return "("..tostring(self.x)..", "..tostring(self.y)..", "..tostring(self.z)..")"
end

function Point:get_add_sub_table(arg)
    local t_other = { x = 0, y = 0, z = 0 }

    if type(arg) == "number" then
        t_other = Point { x = arg, y = arg } -- As of now we don't want to add anything to z unless explicit
    elseif type(arg) == "table" then
        if not (arg.x ~= nil or arg.y ~= nil or arg.z ~= nil) then
            error("Adding table that doesn't have x, y or z values.")
        end
        if arg.x then
            if type(arg.x) == "number" then
                t_other.x = arg.x
            end
        end
        if arg.y then
            if type(arg.y) == "number" then
                t_other.y = arg.y
            end
        end
        if arg.z then
            if type(arg.z) == "number" then
                t_other.z = arg.z
            end
        end
    end

    return t_other
end

function Point:__add(arg)
    local t_other = self:get_add_sub_table(arg)
    return Point { x = self.x + t_other.x, y = self.y + t_other.y, z = (self.z and t_other.z) and self.z + t_other.z or nil }
end

function Point:__sub(arg)
    local t_other = self:get_add_sub_table(arg)
    return Point { x = self.x - t_other.x, y = self.y - t_other.y, z = (self.z and t_other.z) and self.z - t_other.z or nil}
end

-- For the purposes of gui/design, we only care about x and y being equal, z is only used for determining boundaries and x levels to apply the shape
function Point:__eq(other)
    self:check_valid(other)
    return self.x == other.x and self.y == other.y
end

function getMousePoint()
    local pos = dfhack.gui.getMousePos()
    return pos and Point{x = pos.x, y = pos.y, z = pos.z} or nil
end
