-- shape definitions for gui/dig
--@ module = true

local Point = reqscript("internal/design/util").Point

if not dfhack_flags.module then
    qerror("this script cannot be called directly")
end

-- Base shape class, should not be used directly
Shape = defclass(Shape)
Shape.ATTRS {
    name = "",
    arr = {}, -- sparse 2D array containing points, if true then the point is in the shape, false or nil otherwise
    options = DEFAULT_NIL,
    invertable = true,
    invert = false,
    width = 1,
    height = 1,
    points = {}, -- Main points that define the shape
    min_points = 2,
    max_points = 2,
    extra_points = {}, -- Extra points like bezeir curve
    drag_corners = { ne = true, nw = true, se = true, sw = true }, -- which corners should show and be draggable,
    num_tiles = 0, -- count of actual num_tiles in the shape
    basic_shape = true -- true if bounded strictly by a rectangle, like the Rectangle, Ellipse, Rows, etc... set to false for complex shapes
}

-- Transform shape so that the top left-most point is at min_x, min_y
-- Returns a 2D array with the shape, does not modify the existing shape
function Shape:transform(min_x, min_y)
    local ret = {}
    local dim_min, dim_max = self:get_true_dims()

    local x_transform = min_x - dim_min.x
    local y_transform = min_y - dim_min.y

    for x = dim_min.x, dim_max.x do
        ret[x + x_transform] = {}
        for y = dim_min.y, dim_max.y do
            ret[x + x_transform][y + y_transform] = self.arr[x][y]
        end
    end

    return ret
end

-- Dims defined by the main points of a shape
function Shape:get_point_dims()

    if #self.points == 0 then return nil end

    local min_x = self.points[1].x
    local max_x = self.points[1].x
    local min_y = self.points[1].y
    local max_y = self.points[1].y

    for _, point in ipairs(self.points) do
        min_x = math.min(min_x, point.x)
        max_x = math.max(max_x, point.x)
        min_y = math.min(min_y, point.y)
        max_y = math.max(max_y, point.y)
    end

    return Point{ x = min_x, y = min_y }, Point{ x = max_x, y = max_y }
end

-- Get dimensions as defined by the array of the shape
function Shape:get_true_dims()
    local min_x, min_y, max_x, max_y
    for x, _ in pairs(self.arr) do
        for y, _ in pairs(self.arr[x]) do
            if not min_x then
                min_x = x
                max_x = x
                min_y = y
                max_y = y
            else
                min_x = math.min(min_x, x)
                max_x = math.max(max_x, x)
                min_y = math.min(min_y, y)
                max_y = math.max(max_y, y)
            end
        end
    end

    return { x = min_x, y = min_y }, { x = max_x, y = max_y }
end

-- Get dimensions taking into account, main points, extra points,
-- and the shape array, anything that needs to be drawn should be
-- within these bounds
-- TODO this probably belongs in dig.lua at this point
function Shape:get_view_dims(extra_points, mirror_point)
    local function update_minmax(point, min_x, min_y, max_x, max_y)
        if not min_x then
            min_x, max_x, min_y, max_y = point.x, point.x, point.y, point.y
        else
            min_x, max_x = math.min(min_x, point.x), math.max(max_x, point.x)
            min_y, max_y = math.min(min_y, point.y), math.max(max_y, point.y)
        end
        return min_x, min_y, max_x, max_y
    end

    local min_x, min_y, max_x, max_y

    for x, y_vals in pairs(self.arr) do
        for y, _ in pairs(y_vals) do
            min_x, min_y, max_x, max_y = update_minmax({ x = x, y = y }, min_x, min_y, max_x, max_y)
        end
    end

    for _, point in pairs(self.points) do
        min_x, min_y, max_x, max_y = update_minmax(point, min_x, min_y, max_x, max_y)
    end

    for _, point in pairs(extra_points) do
        min_x, min_y, max_x, max_y = update_minmax(point, min_x, min_y, max_x, max_y)
    end

    if mirror_point then
        min_x, min_y, max_x, max_y = update_minmax(mirror_point, min_x, min_y, max_x, max_y)
    end

    return { x = min_x, y = min_y }, { x = max_x, y = max_y }
end

function Shape:points_to_string(points)
    local points = points or self.points
    local output = ""
    local sep = ""
    for _, point in ipairs(points) do
        output = output .. sep .. string.format("(%d, %d)", point.x, point.y)
        sep = ", "
    end

    return output
end

function Shape:get_center()
    -- TODO revisit
    -- This code calculates the center based on the points of the shape
    -- It's slower though, and maybe not needed compared to the simpler method
    -- local num_points = 0
    -- local sum_x = 0
    -- local sum_y = 0

    -- for x, row in pairs(self.arr) do
    --   for y, value in pairs(row) do
    --     if value then
    --       num_points = num_points + 1
    --       sum_x = sum_x + x
    --       sum_y = sum_y + y
    --     end
    --   end
    -- end

    -- local center_x = math.floor(sum_x / num_points)
    -- local center_y = math.floor(sum_y / num_points)

    -- return center_x, center_y

    -- Simple way to get the center defined by the point dims
    if #self.points == 0 then return nil, nil end
    local top_left, bot_right = self:get_point_dims()
    return Point{x = math.floor((bot_right.x - top_left.x) / 2) + top_left.x,
        y = math.floor((bot_right.y - top_left.y) / 2) + top_left.y}

end

-- Basic update function that loops over a rectangle from top left to bottom right
-- Can be overridden for more complex shapes
function Shape:update(points)
    self.num_tiles = 0
    self.points = copyall(points)
    self.arr = {}
    if #points < self.min_points then return end
    local top_left, bot_right = self:get_point_dims()
    self.height = bot_right.y - top_left.y
    self.width = bot_right.x - top_left.x

    if #points < self.min_points then return end

    for x = top_left.x, bot_right.x do
        self.arr[x] = {}
        for y = top_left.y, bot_right.y do
            local value = self:has_point(x - top_left.x, y - top_left.y)
            if not self.invert then
                self.arr[x][y] = value
            else
                self.arr[x][y] = not value
            end

            self.num_tiles = self.num_tiles + (self.arr[x][y] and 1 or 0)
        end
    end
end

function Shape:get_point(x, y) if self.arr[x] and self.arr[x][y] then return true else return false end end

function Shape:has_point(x, y)
    -- This class isn't meant to be used directly
    return false
end

-- Shape definitions
-- All should have a string name, and a function has_point(self, x, y) which returns true or false based
-- on if the x,y point is within the shape
-- Also options can be defined in a table, see existing shapes for example

--Ellipse

Ellipse = defclass(Ellipse, Shape)
Ellipse.ATTRS {
    name = "Ellipse",
}

function Ellipse:init()
    self.options = {
        hollow = {
            name = "Hollow",
            type = "bool",
            value = false,
            key = "CUSTOM_H",
        },
        thickness = {
            name = "Thickness",
            type = "plusminus",
            value = 2,
            enabled = { "hollow", true },
            min = 1,
            max = function(shape) if not shape.height or not shape.width then
                    return nil
                else
                    return math.ceil(math.min(shape.height, shape.width) / 2)

                end
            end,
            keys = { "CUSTOM_T", "CUSTOM_SHIFT_T" },
        },
    }
end

function Ellipse:has_point(x, y)
    local center_x, center_y = self.width / 2, self.height / 2
    local point_x, point_y = x - center_x, y - center_y
    local is_inside =
    (point_x / (self.width / 2)) ^ 2 + (point_y / (self.height / 2)) ^ 2 <= 1

    if self.options.hollow.value and is_inside then
        local all_points_inside = true
        for dx = -self.options.thickness.value, self.options.thickness.value do
            for dy = -self.options.thickness.value, self.options.thickness.value do
                if dx ~= 0 or dy ~= 0 then
                    local surrounding_x, surrounding_y = x + dx, y + dy
                    local surrounding_point_x, surrounding_point_y =
                    surrounding_x - center_x,
                        surrounding_y - center_y
                    if (surrounding_point_x / (self.width / 2)) ^ 2 + (surrounding_point_y / (self.height / 2)) ^ 2 >
                        1 then
                        all_points_inside = false
                        break
                    end
                end
            end
            if not all_points_inside then
                break
            end
        end
        return not all_points_inside
    else
        return is_inside
    end
end

Rectangle = defclass(Rectangle, Shape)
Rectangle.ATTRS {
    name = "Rectangle",
}

function Rectangle:init()
    self.options = {
        hollow = {
            name = "Hollow",
            type = "bool",
            value = false,
            key = "CUSTOM_H",
        },
        thickness = {
            name = "Thickness",
            type = "plusminus",
            value = 2,
            enabled = { "hollow", true },
            min = 1,
            max = function(shape) if not shape.height or not shape.width then
                    return nil
                else
                    return math.ceil(math.min(shape.height, shape.width) / 2)
                end
            end,
            keys = { "CUSTOM_T", "CUSTOM_SHIFT_T" },
        },
    }
end

function Rectangle:has_point(x, y)
    if self.options.hollow.value == false then
        if (x >= 0 and x <= self.width) and (y >= 0 and y <= self.height) then
            return true
        end
    else
        if (x >= self.options.thickness.value and x <= self.width - self.options.thickness.value) and
            (y >= self.options.thickness.value and y <= self.height - self.options.thickness.value) then
            return false
        else
            return true
        end
    end
    return false
end

Rows = defclass(Rows, Shape)
Rows.ATTRS {
    name = "Rows",
}

function Rows:init()
    self.options = {
        vertical = {
            name = "Vertical",
            type = "bool",
            value = true,
            key = "CUSTOM_V",
        },
        horizontal = {
            name = "Horizontal",
            type = "bool",
            value = false,
            key = "CUSTOM_H",
        },
        spacing = {
            name = "Spacing",
            type = "plusminus",
            value = 3,
            min = 1,
            keys = { "CUSTOM_T", "CUSTOM_SHIFT_T" },
        },
    }
end

function Rows:has_point(x, y)
    if self.options.vertical.value and x % self.options.spacing.value == 0 or
        self.options.horizontal.value and y % self.options.spacing.value == 0 then
        return true
    else
        return false
    end
end

Diag = defclass(Diag, Shape)
Diag.ATTRS {
    name = "Diagonal",
}

function Diag:init()
    self.options = {
        spacing = {
            name = "Spacing",
            type = "plusminus",
            value = 5,
            min = 1,
            keys = { "CUSTOM_T", "CUSTOM_SHIFT_T" },
        },
        reverse = {
            name = "Reverse",
            type = "bool",
            value = false,
            key = "CUSTOM_R",
        },
    }
end

function Diag:has_point(x, y)
    local mult = 1
    if self.options.reverse.value then
        mult = -1
    end

    if (x + mult * y) % self.options.spacing.value == 0 then
        return true
    else
        return false
    end
end

Line = defclass(Line, Shape)
Line.ATTRS {
    name = "Line",
    extra_points = { { label = "Curve Point" }, { label = "Second Curve Point" } },
    invertable = false, -- Doesn't support invert
    basic_shape = false -- Driven by points, not rectangle bounds
}

function Line:init()
    self.options = {
        thickness = {
            name = "Thickness",
            type = "plusminus",
            value = 1,
            min = 1,
            max = function(shape) if not shape.height or not shape.width then
                    return nil
                else
                    return math.max(shape.height, shape.width)

                end
            end,
            keys = { "CUSTOM_T", "CUSTOM_SHIFT_T" },
        },
    }
end

function Line:plot_bresenham(x0, y0, x1, y1, thickness)
    local dx = math.abs(x1 - x0)
    local dy = math.abs(y1 - y0)
    local sx = x0 < x1 and 1 or -1
    local sy = y0 < y1 and 1 or -1
    local err = dx - dy
    local e2, x, y

    for i = 0, thickness - 1 do
        x = x0
        y = y0 + i
        while true do
            for j = -math.floor(thickness / 2), math.ceil(thickness / 2) - 1 do
                if not self.arr[x + j] then self.arr[x + j] = {} end
                self.arr[x + j][y] = true
                self.num_tiles = self.num_tiles + 1
            end

            if x == x1 and y == y1 + i then
                break
            end

            e2 = 2 * err

            if e2 > -dy then
                err = err - dy
                x = x + sx
            end

            if e2 < dx then
                err = err + dx
                y = y + sy
            end
        end
    end

end

function Line:cubic_bezier(x0, y0, x1, y1, bezier_point1, bezier_point2, thickness)
    local t = 0
    local x2, y2 = bezier_point1.x, bezier_point1.y
    local x3, y3 = bezier_point2.x, bezier_point2.y
    while t <= 1 do
        local x = math.floor(((1 - t) ^ 3 * x0 + 3 * (1 - t) ^ 2 * t * x2 + 3 * (1 - t) * t ^ 2 * x3 + t ^ 3 * x1) +
            0.5)
        local y = math.floor(((1 - t) ^ 3 * y0 + 3 * (1 - t) ^ 2 * t * y2 + 3 * (1 - t) * t ^ 2 * y3 + t ^ 3 * y1) +
            0.5)
        for i = 0, thickness - 1 do
            for j = -math.floor(thickness / 2), math.ceil(thickness / 2) - 1 do
                if not self.arr[x + j] then self.arr[x + j] = {} end
                if not self.arr[x + j][y + i] then
                    self.arr[x + j][y + i] = true
                    self.num_tiles = self.num_tiles + 1
                end
            end
        end
        t = t + 0.01
    end

    -- Get the last point
    local x_end = math.floor(((1 - 1) ^ 3 * x0 + 3 * (1 - 1) ^ 2 * 1 * x2 + 3 * (1 - 1) * 1 ^ 2 * x3 + 1 ^ 3 * x1) +
        0.5)
    local y_end = math.floor(((1 - 1) ^ 3 * y0 + 3 * (1 - 1) ^ 2 * 1 * y2 + 3 * (1 - 1) * 1 ^ 2 * y3 + 1 ^ 3 * y1) +
        0.5)
    for i = 0, thickness - 1 do
        for j = -math.floor(thickness / 2), math.ceil(thickness / 2) - 1 do
            if not self.arr[x_end + j] then self.arr[x_end + j] = {} end
            if not self.arr[x_end + j][y_end + i] then
                self.arr[x_end + j][y_end + i] = true
                self.num_tiles = self.num_tiles + 1
            end
        end
    end
end

function Line:quadratic_bezier(x0, y0, x1, y1, bezier_point1, thickness)
    local t = 0
    local x2, y2 = bezier_point1.x, bezier_point1.y
    while t <= 1 do
        local x = math.floor(((1 - t) ^ 2 * x0 + 2 * (1 - t) * t * x2 + t ^ 2 * x1) + 0.5)
        local y = math.floor(((1 - t) ^ 2 * y0 + 2 * (1 - t) * t * y2 + t ^ 2 * y1) + 0.5)
        for i = 0, thickness - 1 do
            for j = -math.floor(thickness / 2), math.ceil(thickness / 2) - 1 do
                if not self.arr[x + j] then self.arr[x + j] = {} end
                if not self.arr[x + j][y + i] then
                    self.arr[x + j][y + i] = true
                    self.num_tiles = self.num_tiles + 1
                end
            end
        end
        t = t + 0.01
    end
end

function Line:update(points, extra_points)
    self.num_tiles = 0
    self.points = copyall(points)
    local top_left, bot_right = self:get_point_dims()
    if not top_left or not bot_right then return end
    self.arr = {}
    self.height = bot_right.x - top_left.x
    self.width = bot_right.y - top_left.y

    if #points < self.min_points then return end

    local x0, y0 = self.points[1].x, self.points[1].y
    local x1, y1 = self.points[2].x, self.points[2].y

    local thickness = self.options.thickness.value or 1
    local bezier_point1 = (#extra_points > 0) and { x = extra_points[1].x, y = extra_points[1].y } or nil
    local bezier_point2 = (#extra_points > 1) and { x = extra_points[2].x, y = extra_points[2].y } or nil

    if bezier_point1 and bezier_point2 then -- Use Cubic Bezier curve
        self:cubic_bezier(x0, y0, x1, y1, bezier_point1, bezier_point2, thickness)
    elseif bezier_point1 then
        self:quadratic_bezier(x0, y0, x1, y1, bezier_point1, thickness)
    else
        -- Due to how bresenham's breaks ties sometimes lines won't be exactly symmetrical
        -- as a user might expect. By plotting the line both ways, we can ensure symmetry
        self:plot_bresenham(x0, y0, x1, y1, thickness)
        self:plot_bresenham(x1, y1, x0, y0, thickness)
    end
end

FreeForm = defclass(FreeForm, Shape)
FreeForm.ATTRS = {
    name = "FreeForm",
    invertable = false, -- doesn't support invert
    min_points = 1,
    max_points = DEFAULT_NIL,
    basic_shape = false,
    can_mirror = true
}

function FreeForm:init()
    self.options = {
        thickness = {
            name = "Thickness",
            type = "plusminus",
            value = 1,
            min = 1,
            max = function(shape)
                if not shape.height or not shape.width then
                    return nil
                else
                    return math.max(shape.height, shape.width)
                end
            end,
            keys = { "CUSTOM_T", "CUSTOM_SHIFT_T" },
        },
        closed = {
            name = "Closed",
            type = "bool",
            value = true,
            key = "CUSTOM_Y",
        },
        filled = {
            name = "Filled",
            type = "bool",
            value = false,
            key = "CUSTOM_L",
            enabled = { "closed", true },
        },
    }
end

function FreeForm:update(points, extra_points)
    self.num_tiles = 0
    self.points = copyall(points)
    self.arr = {}
    if #points < self.min_points then return end
    local top_left, bot_right = self:get_point_dims()
    self.height = bot_right.x - top_left.x
    self.width = bot_right.y - top_left.y

    local thickness = self.options.thickness.value or 1

    -- Determine the edges of the polygon
    local edges = {}
    for i = 1, #self.points - 1 do
        table.insert(edges, { self.points[i].x, self.points[i].y, self.points[i + 1].x, self.points[i + 1].y })
    end

    if self.options.closed.value then
        -- Connect last point to first point to close the shape
        table.insert(edges,
            { self.points[#self.points].x, self.points[#self.points].y, self.points[1].x, self.points[1].y })
    end

    -- Iterate over each edge and draw a line
    for _, edge in ipairs(edges) do
        local line = { { x = edge[1], y = edge[2] }, { x = edge[3], y = edge[4] } }
        local line_class = Line()
        line_class.options.thickness.value = thickness
        line_class:update(line, {})
        for x, y_row in pairs(line_class.arr) do
            for y, _ in pairs(y_row) do
                if not self.arr[x] then self.arr[x] = {} end
                self.arr[x][y] = true
                self.num_tiles = self.num_tiles + 1
            end
        end
    end

    if self.options.filled.value and self.options.closed.value then
        -- Fill internal points if shape is closed and filled
        for x = top_left.x, bot_right.x do
            for y = top_left.y, bot_right.y do
                if self:point_in_polygon(x, y) then
                    if not self.arr[x] then self.arr[x] = {} end
                    self.arr[x][y] = true
                    self.num_tiles = self.num_tiles + 1
                end
            end
        end
    end
end

-- https://en.wikipedia.org/wiki/Even%E2%80%93odd_rule
function FreeForm:point_in_polygon(x, y)

    local inside = false
    local j = #self.points

    for i = 1, #self.points do
        if (self.points[i].y > y) ~= (self.points[j].y > y) and
            x <
            (self.points[j].x - self.points[i].x) * (y - self.points[i].y) / (self.points[j].y - self.points[i].y) +
            self.points[i].x then
            inside = not inside
        end
        j = i
    end

    return inside
end

-- module users can get shapes through this global, shape option values
-- persist in these as long as the module is loaded
-- idk enough lua to know if this is okay to do or not
all_shapes = { Rectangle {}, Ellipse {}, Rows {}, Diag {}, Line {}, FreeForm {} }
