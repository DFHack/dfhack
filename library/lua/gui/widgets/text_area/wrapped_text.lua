-- This class caches lines of text wrapped to a specified width for performance
-- and readability. It can convert a given text index to (x, y) coordinates in
-- the wrapped text and vice versa.

-- Usage:
-- This class should only be used in the following scenarios.
--  1. When text or text features need to be rendered
--     (wrapped {x, y} coordinates are required).
--  2. When mouse input needs to be converted to the original text position.

-- Using this class in other scenarios may lead to issues with the component's
-- behavior when the text is wrapped.
WrappedText = defclass(WrappedText)

WrappedText.ATTRS{
    text = '',
    wrap_width = math.huge,
}

function WrappedText:init()
    self:update(self.text, self.wrap_width)
end

function WrappedText:update(text, wrap_width)
    self.lines = text:wrap(
        wrap_width,
        {
            return_as_table=true,
            keep_trailing_spaces=true,
            keep_original_newlines=true
        }
    )
end

function WrappedText:coordsToIndex(x, y)
    local offset = 0

    local normalized_y = math.max(
        1,
        math.min(y, #self.lines)
    )

    local line_bonus_length = normalized_y == #self.lines and 1 or 0
    local normalized_x = math.max(
        1,
        math.min(x, #self.lines[normalized_y] + line_bonus_length)
    )

    for i=1, normalized_y - 1 do
      offset = offset + #self.lines[i]
    end

    return offset + normalized_x
end

function WrappedText:indexToCoords(index)
    local offset = index

    for y, line in ipairs(self.lines) do
        local line_bonus_length = y == #self.lines and 1 or 0
        if offset <= #line + line_bonus_length then
            return offset, y
        end
        offset = offset - #line
    end

    return #self.lines[#self.lines] + 1, #self.lines
end

return WrappedText
