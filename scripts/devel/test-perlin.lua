-- Generates an image using multiple octaves of perlin noise.

local args = {...}
local rng = dfhack.random.new(3)

if #args < 3 then
    qerror('Usage: devel/test-perlin <fname.pgm> <density> <expr> [-xy <xyscale>] [-z <zscale>]')
end

local fname = table.remove(args,1)
local goal = tonumber(table.remove(args,1)) or qerror('Invalid density')
local expr = table.remove(args,1) or qerror('No expression')
local zscale = 2
local xyscale = 1

for i = 1,#args,2 do
    if args[i] == '-xy' then
        xyscale = tonumber(args[i+1]) or qerror('Invalid xyscale')
    end
    if args[i] == '-z' then
        zscale = tonumber(args[i+1]) or qerror('Invalid zscale')
    end
end

local fn_env = copyall(math)

fn_env.rng = rng
fn_env.apow = function(x,y) return math.pow(math.abs(x),y) end
fn_env.spow = function(x,y) return x*math.pow(math.abs(x),y-1) end

-- Noise functions are referenced from expressions
-- as variables of form like "x3a", where:
-- 1) x is one of x/y/z/w independent functions in each octave
-- 2) 3 is the octave number; 0 corresponds to the whole range
-- 3) a is the subtype

-- Independent noise functions: support 4
local ids = { 'x', 'y', 'z', 'w' }
-- Subtype: provides an offset to the coordinates
local subs = {
    [''] = { 0,   0,   0   },
    a =    { 0.5, 0,   0   },
    b =    { 0,   0.5, 0   },
    c =    { 0.5, 0.5, 0   },
    d =    { 0,   0,   0.5 },
    e =    { 0.5, 0,   0.5 },
    f =    { 0,   0.5, 0.5 },
    g =    { 0.5, 0.5, 0.5 },
}

function mkdelta(v)
    if v == 0 then
        return ''
    else
        return '+'..v
    end
end

function mkexpr(expr)
    -- Collect referenced variables
    local max_octave = -1
    local vars = {}

    for var,id,octave,subtype in string.gmatch(expr,'%f[%w](([xyzw])(%d+)(%a*))%f[^%w]') do
        if not vars[var] then
            octave = tonumber(octave)

            if octave > max_octave then
                max_octave = octave
            end

            local sub = subs[subtype] or qerror('Invalid subtype: '..subtype)

            vars[var] = { id = id, octave = octave, subtype = subtype, sub = sub }
        end
    end

    if max_octave < 0 then
        qerror('No noise function references in expression.')
    end

    -- Allocate the noise functions
    local code = ''

    for i = 0,max_octave do
        for j,id in ipairs(ids) do
            code = code .. 'local _fn_'..i..'_'..id..' = rng:perlin(3)\n';
        end
    end

    -- Evaluate variables
    code = code .. 'return function(x,y,z)\n'

    for var,info in pairs(vars) do
        local fn = '_fn_'..info.octave..'_'..info.id
        local mul = math.pow(2,info.octave)
        mul = math.min(48*4, mul)
        code = code .. '    local '..var
                    .. ' = _fn_'..info.octave..'_'..info.id
                    .. '(x*'..mul..mkdelta(info.sub[1])
                    .. ',y*'..mul..mkdelta(info.sub[2])
                    .. ',z*'..mul..mkdelta(info.sub[3])
                    .. ')\n'
    end

    -- Complete and compile the function
    code = code .. '    return ('..expr..')\nend\n'

    local f,err = load(code, '=(expr)', 't', fn_env)
    if not f then
        qerror(err)
    end
    return f()
end

local field_fn = mkexpr(expr)

function render(thresh,file)
    local area = 0
    local line, arr = '', {}

    for zy = 0,1 do
        for y = 0,48*4-1 do
            line = ''
            for zx = 0,1 do
                for x = 0,48*4-1 do
                    local tx = (0.5+x)/(48*4/xyscale)
                    local ty = (0.5+y)/(48*4/xyscale)
                    local tz = 0.3+(zx+zy*2)/(48*4/zscale)
                    local v1 = field_fn(tx, ty, tz)
                    local v = -1
                    if v1 > thresh then
                        v = v1;
                        area = area + 1
                    end
                    if file then
                        local c = math.max(0, math.min(255, v * 127 + 128))
                        arr[2*x+1] = c
                        arr[2*x+2] = c
                    end
                end
                if file then
                    line = line..string.char(table.unpack(arr))
                end
            end
            if file then
                file:write(line,line)
            end
        end
    end

    return area/4/(48*4)/(48*4)
end

function search(fn,min,max,goal,eps)
    local center
    for i = 1,32 do
        center = (max+min)/2
        local cval = fn(center)
        print('At '..center..': '..cval)
        if math.abs(cval-goal) < eps then
            break
        end
        if cval > goal then
            min = center
        else
            max = center
        end
    end
    return center
end

local thresh = search(render, -2, 2, goal, math.min(0.001,goal/20))

local file,err = io.open(fname, 'wb')
if not file then
    print('error: ',err)
    return
end
file:write('P5\n')
file:write('# '..goal..' '..expr..' '..xyscale..' '..zscale..'\n')
file:write('768 768\n255\n')
local area = render(thresh, file)
file:close()

print('Area fraction: '..area)
