local _ENV = mkmodule('utils')

-- Comparator function
function compare(a,b)
    if a < b then
        return -1
    elseif a > b then
        return 1
    else
        return 0
    end
end

-- Sort strings; compare empty last
function compare_name(a,b)
    if a == '' then
        if b == '' then
            return 0
        else
            return 1
        end
    elseif b == '' then
        return -1
    else
        return compare(a,b)
    end
end

--[[
    Sort items in data according to ordering.

    Each ordering spec is a table with possible fields:

    * key = function(value)
        Computes comparison key from a data value. Not called on nil.
    * key_table = function(data)
        Computes a key table from the data table in one go.
    * compare = function(a,b)
        Comparison function. Defaults to compare above.
        Called on non-nil keys; nil sorts last.

    Returns a table of integer indices into data.
--]]
function make_sort_order(data,ordering)
    -- Compute sort keys and comparators
    local keys = {}
    local cmps = {}

    for i=1,#ordering do
        local order = ordering[i]

        if order.key_table then
            keys[i] = order.key_table(data)
        elseif order.key then
            local kt = {}
            local kf = order.key
            for j=1,#data do
                if data[j] == nil then
                    kt[j] = nil
                else
                    kt[j] = kf(data[j])
                end
            end
            keys[i] = kt
        else
            keys[i] = data
        end

        cmps[i] = order.compare or compare
    end

    -- Make an order table
    local index = {}
    for i=1,#data do
        index[i] = i
    end

    -- Sort the ordering table
    table.sort(index, function(ia,ib)
        for i=1,#keys do
            local ka = keys[i][ia]
            local kb = keys[i][ib]

            -- Sort nil keys to the end
            if ka == nil then
                if kb ~= nil then
                    return false
                end
            elseif kb == nil then
                return true
            else
                local cmpv = cmps[i](ka,kb)
                if cmpv < 0 then
                    return true
                elseif cmpv > 0 then
                    return false
                end
            end
        end
        return ia < ib -- this should ensure stable sort
    end)

    return index
end

return _ENV