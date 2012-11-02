-- Utilities for offset scan scripts.

local _ENV = mkmodule('memscan')

local utils = require('utils')

-- A length-checked view on a memory buffer

CheckedArray = CheckedArray or {}

function CheckedArray.new(type,saddr,eaddr)
    local data = df.reinterpret_cast(type,saddr)
    local esize = data:sizeof()
    local count = math.floor((eaddr-saddr)/esize)
    local obj = {
        type = type, start = saddr, size = count*esize,
        esize = esize, data = data, count = count
    }
    setmetatable(obj, CheckedArray)
    return obj
end

function CheckedArray:__len()
    return self.count
end
function CheckedArray:__index(idx)
    if type(idx) == "number" then
        if idx >= self.count then
            error('Index out of bounds: '..tostring(idx))
        end
        return self.data[idx]
    else
        return CheckedArray[idx]
    end
end
function CheckedArray:__newindex(idx, val)
    if idx >= self.count then
        error('Index out of bounds: '..tostring(idx))
    end
    self.data[idx] = val
end
function CheckedArray:addr2idx(addr, round)
    local off = addr - self.start
    if off >= 0 and off < self.size and (round or (off % self.esize) == 0) then
        return math.floor(off / self.esize), off
    end
end
function CheckedArray:idx2addr(idx)
    if idx >= 0 and idx < self.count then
        return self.start + idx*self.esize
    end
end

-- Search methods

function CheckedArray:find(data,sidx,eidx,reverse)
    local dcnt = #data
    sidx = math.max(0, sidx or 0)
    eidx = math.min(self.count, eidx or self.count)
    if (eidx - sidx) >= dcnt and dcnt > 0 then
        return dfhack.with_temp_object(
            df.new(self.type, dcnt),
            function(buffer)
                for i = 1,dcnt do
                    buffer[i-1] = data[i]
                end
                local cnt = eidx - sidx - dcnt
                local step = self.esize
                local sptr = self.start + sidx*step
                local ksize = dcnt*step
                if reverse then
                    local idx, addr = dfhack.internal.memscan(sptr + cnt*step, cnt, -step, buffer, ksize)
                    if idx then
                        return sidx + cnt - idx, addr
                    end
                else
                    local idx, addr = dfhack.internal.memscan(sptr, cnt, step, buffer, ksize)
                    if idx then
                        return sidx + idx, addr
                    end
                end
            end
        )
    end
end
function CheckedArray:find_one(data,sidx,eidx,reverse)
    local idx, addr = self:find(data,sidx,eidx,reverse)
    if idx then
        -- Verify this is the only match
        if reverse then
            eidx = idx+#data-1
        else
            sidx = idx+1
        end
        if self:find(data,sidx,eidx,reverse) then
            return nil
        end
    end
    return idx, addr
end
function CheckedArray:list_changes(old_arr,old_val,new_val,delta)
    if old_arr.type ~= self.type or old_arr.count ~= self.count then
        error('Incompatible arrays')
    end
    local eidx = self.count
    local optr = old_arr.start
    local nptr = self.start
    local esize = self.esize
    local rv
    local sidx = 0
    while true do
        local idx = dfhack.internal.diffscan(optr, nptr, sidx, eidx, esize, old_val, new_val, delta)
        if not idx then
            break
        end
        rv = rv or {}
        rv[#rv+1] = idx
        sidx = idx+1
    end
    return rv
end
function CheckedArray:filter_changes(prev_list,old_arr,old_val,new_val,delta)
    if old_arr.type ~= self.type or old_arr.count ~= self.count then
        error('Incompatible arrays')
    end
    local eidx = self.count
    local optr = old_arr.start
    local nptr = self.start
    local esize = self.esize
    local rv
    for i=1,#prev_list do
        local idx = prev_list[i]
        if idx < 0 or idx >= eidx then
            error('Index out of bounds: '..idx)
        end
        if dfhack.internal.diffscan(optr, nptr, idx, idx+1, esize, old_val, new_val, delta) then
            rv = rv or {}
            rv[#rv+1] = idx
        end
    end
    return rv
end

-- A raw memory area class

MemoryArea = MemoryArea or {}
MemoryArea.__index = MemoryArea

function MemoryArea.new(astart, aend)
    local obj = {
        start_addr = astart, end_addr = aend, size = aend - astart,
        int8_t = CheckedArray.new('int8_t',astart,aend),
        uint8_t = CheckedArray.new('uint8_t',astart,aend),
        int16_t = CheckedArray.new('int16_t',astart,aend),
        uint16_t = CheckedArray.new('uint16_t',astart,aend),
        int32_t = CheckedArray.new('int32_t',astart,aend),
        uint32_t = CheckedArray.new('uint32_t',astart,aend),
        float = CheckedArray.new('float',astart,aend)
    }
    setmetatable(obj, MemoryArea)
    return obj
end

function MemoryArea:__gc()
    df.delete(self.buffer)
end

function MemoryArea:__tostring()
    return string.format('<MemoryArea: %x..%x>', self.start_addr, self.end_addr)
end
function MemoryArea:contains_range(start,size)
    return size >= 0 and start >= self.start_addr and (start+size) <= self.end_addr
end
function MemoryArea:contains_obj(obj,count)
    local size, base = df.sizeof(obj)
    return size and base and self:contains_range(base, size*(count or 1))
end

function MemoryArea:clone()
    local buffer = df.new('int8_t', self.size)
    local _, base = buffer:sizeof()
    local rv = MemoryArea.new(base, base+self.size)
    rv.buffer = buffer
    return rv
end
function MemoryArea:copy_from(area2)
    if area2.size ~= self.size then
        error('Size mismatch')
    end
    dfhack.internal.memmove(self.start_addr, area2.start_addr, self.size)
end
function MemoryArea:delete()
    setmetatable(self, nil)
    df.delete(self.buffer)
    for k,v in pairs(self) do self[k] = nil end
end

-- Static code segment search

function get_code_segment()
    local cstart, cend

    for i,mem in ipairs(dfhack.internal.getMemRanges()) do
        if mem.read and mem.execute
           and (string.match(mem.name,'/dwarfort%.exe$')
             or string.match(mem.name,'/Dwarf_Fortress$')
             or string.match(mem.name,'Dwarf Fortress%.exe'))
        then
            cstart = mem.start_addr
            cend = mem.end_addr
        end
    end
    if cstart and cend then
        return MemoryArea.new(cstart, cend)
    end
end

-- Static data segment search

local function find_data_segment()
    local data_start, data_end

    for i,mem in ipairs(dfhack.internal.getMemRanges()) do
        if data_end then
            if mem.start_addr == data_end and mem.read and mem.write then
                data_end = mem.end_addr
            else
                break
            end
        elseif mem.read and mem.write
           and (string.match(mem.name,'/dwarfort%.exe$')
             or string.match(mem.name,'/Dwarf_Fortress$')
             or string.match(mem.name,'Dwarf Fortress%.exe'))
        then
            data_start = mem.start_addr
            data_end = mem.end_addr
        end
    end

    return data_start, data_end
end

function get_data_segment()
    local s, e = find_data_segment()
    if s and e then
        return MemoryArea.new(s, e)
    end
end

-- Register a found offset, or report an error.

function found_offset(name,val)
    local cval = dfhack.internal.getAddress(name)

    if not val then
        print('Could not find offset '..name)
        if not cval and not utils.prompt_yes_no('Continue with the script?') then
            qerror('User quit')
        end
        return
    end

    if df.isvalid(val) then
        _,val = val:sizeof()
    end

    print(string.format('Found offset %s: %x', name, val))

    if cval then
        if cval ~= val then
            error(string.format('Mismatch with the current value: %x',val))
        end
    else
        dfhack.internal.setAddress(name, val)

        local ival = val - dfhack.internal.getRebaseDelta()
        local entry = string.format("<global-address name='%s' value='0x%x'/>\n", name, ival)

        local ccolor = dfhack.color(COLOR_LIGHTGREEN)
        dfhack.print(entry)
        dfhack.color(ccolor)

        io.stdout:write(entry)
        io.stdout:flush()
    end
end

-- Offset of a field within struct

function field_ref(handle,...)
    local items = table.pack(...)
    for i=1,items.n-1 do
        handle = handle[items[i]]
    end
    return handle:_field(items[items.n])
end

function field_offset(type,...)
    local handle = df.reinterpret_cast(type,1)
    local _,addr = df.sizeof(field_ref(handle,...))
    return addr-1
end

function MemoryArea:object_by_field(addr,type,...)
    if not addr then
        return nil
    end
    local base = addr - field_offset(type,...)
    local obj = df.reinterpret_cast(type, base)
    if not self:contains_obj(obj) then
        obj = nil
    end
    return obj, base
end

-- Validation

function is_valid_vector(ref,size)
    local ints = df.reinterpret_cast('uint32_t', ref)
    return ints[0] <= ints[1] and ints[1] <= ints[2]
       and (size == nil or (ints[1] - ints[0]) % size == 0)
end

-- Difference search helper

DiffSearcher = DiffSearcher or {}
DiffSearcher.__index = DiffSearcher

function DiffSearcher.new(area)
    local obj = { area = area }
    setmetatable(obj, DiffSearcher)
    return obj
end

function DiffSearcher:begin_search(type)
    self.type = type
    self.old_value = nil
    self.search_sets = nil
    if not self.save_area then
        self.save_area = self.area:clone()
    end
end
function DiffSearcher:advance_search(new_value,delta)
    if self.search_sets then
        local nsets = #self.search_sets
        local ovec = self.save_area[self.type]
        local nvec = self.area[self.type]
        local new_set
        if nsets > 0 then
            local last_set = self.search_sets[nsets]
            new_set = nvec:filter_changes(last_set,ovec,self.old_value,new_value,delta)
        else
            new_set = nvec:list_changes(ovec,self.old_value,new_value,delta)
        end
        if new_set then
            self.search_sets[nsets+1] = new_set
            self.old_value = new_value
            self.save_area:copy_from(self.area)
            return #new_set, new_set
        end
    else
        self.old_value = new_value
        self.search_sets = {}
        self.save_area:copy_from(self.area)
        return #self.save_area[self.type], nil
    end
end
function DiffSearcher:reset()
    self.search_sets = nil
    if self.save_area then
        self.save_area:delete()
        self.save_area = nil
    end
end
function DiffSearcher:idx2addr(idx)
    return self.area[self.type]:idx2addr(idx)
end

-- Interactive search utility

function DiffSearcher:find_interactive(prompt,data_type,condition_cb,iter_limit)
    enum = enum or {}

    -- Loop for restarting search from scratch
    while true do
        print('\n'..prompt)

        self:begin_search(data_type)

        local found = false
        local ccursor = 0

        -- Loop through choices
        while true do
            print('')

            if iter_limit and ccursor >= iter_limit then
                dfhack.printerr('  Iteration limit reached without a solution.')
                break
            end

            local ok, value, delta = condition_cb(ccursor)

            ccursor = ccursor + 1

            if not ok then
                break
            end

            -- Search for it in the memory
            local cnt, set = self:advance_search(value, delta)
            if not cnt then
                dfhack.printerr('  Converged to zero candidates; probably a mistake somewhere.')
                break
            elseif set and cnt == 1 then
                -- To confirm, wait for two 1-candidate results in a row
                if found then
                    local addr = self:idx2addr(set[1])
                    print(string.format('  Confirmed address: %x\n', addr))
                    return addr, set[1]
                else
                    found = true
                end
            end

            print('  '..cnt..' candidates remaining.')
        end

        if not utils.prompt_yes_no('\nRetry search from the start?') then
            return nil
        end
    end
end

function DiffSearcher:find_menu_cursor(prompt,data_type,choices,enum)
    enum = enum or {}

    return self:find_interactive(
        prompt, data_type,
        function(ccursor)
            local choice

            -- Select the next value to search for
            if type(choices) == 'function' then
                choice = choices(ccursor)

                if not choice then
                    return false
                end
            else
                choice = choices[(ccursor % #choices) + 1]
            end

            -- Ask the user to select it
            if enum ~= 'noprompt' then
                local cname = enum[choice] or choice
                if type(choice) == 'string' and type(cname) == 'number' then
                    choice, cname = cname, choice
                end
                if cname ~= choice then
                    cname = cname..' ('..choice..')'
                end

                print('  Please select: '..cname)
                if not utils.prompt_yes_no('  Continue?', true) then
                    return false
                end
            end

            return true, choice
        end
    )
end

function DiffSearcher:find_counter(prompt,data_type,delta,action_prompt)
    delta = delta or 1

    return self:find_interactive(
        prompt, data_type,
        function(ccursor)
            if ccursor > 0 then
                print("  "..(action_prompt or 'Please do the action.'))
            end
            if not utils.prompt_yes_no('  Continue?', true) then
                return false
            end
            return true, nil, delta
        end
    )
end

-- Screen size

function get_screen_size()
    -- Use already known globals
    if dfhack.internal.getAddress('init') then
        local d = df.global.init.display
        return d.grid_x, d.grid_y
    end
    if dfhack.internal.getAddress('gps') then
        local g = df.global.gps
        return g.dimx, g.dimy
    end

    -- Parse stdout.log for resize notifications
    io.stdout:flush()

    local w,h = 80,25
    for line in io.lines('stdout.log') do
        local cw, ch = string.match(line, '^Resizing grid to (%d+)x(%d+)$')
        if cw and ch then
            w, h = tonumber(cw), tonumber(ch)
        end
    end
    return w,h
end

return _ENV
