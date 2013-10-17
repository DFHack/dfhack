-- Generates an image using multiple octaves of perlin noise.

local args = {...}
local rng = dfhack.random.new(3)

if #args < 3 then
    qerror('Usage: devel/test-perlin <fname.pgm> <density> <coeff|expr...x[...y[...z]]>...')
end

local fname = table.remove(args,1)
local goal = tonumber(table.remove(args,1)) or qerror('Invalid density')

local zscale = 6

local coeffs = {}
local fns = {}

local env = copyall(math)
env.apow = function(x,y) return math.pow(math.abs(x),y) end

for i = 1,#args do
    local fn = rng:perlin(3);
    local fn2 = rng:perlin(3);
    local fn3 = rng:perlin(3);
    local fn4 = rng:perlin(3);

    fns[i] = fn;

    local val = string.match(args[i],'^([+-]?[%d.]+)$')
    if val then
        coeffs[i] = function(x) return x * val end
    else
        local argstr = 'x'
        if string.match(args[i], '%f[%w]w%f[^%w]') then
            argstr = 'x,y,z,w'
            fns[i] = function(x,y,z)
                return fn(x,y,z), fn2(x,y,z), fn3(x+0.5,y+0.5,z+0.5), fn4(x+0.5,y+0.5,z+0.5)
            end
        elseif string.match(args[i], '%f[%w]z%f[^%w]') then
            argstr = 'x,y,z'
            fns[i] = function(x,y,z)
                return fn(x,y,z), fn2(x,y,z), fn3(x+0.5,y+0.5,z+0.5)
            end
        elseif string.match(args[i], '%f[%w]y%f[^%w]') then
            argstr = 'x,y'
            fns[i] = function(x,y,z)
                return fn(x,y,z), fn2(x,y,z)
            end
        end

        local f,err = load(
            'return function('..argstr..') return ('..args[i]..') end',
            '=(expr)', 't', env
        )
        if not f then
            qerror(err)
        end
        coeffs[i] = f()
    end
end

function render(thresh,file)
    local area = 0
    local line, arr = '', {}

    for zy = 0,1 do
        for y = 0,48*4-1 do
            line = ''
            for zx = 0,1 do
                for x = 0,48*4-1 do
                    local tx = (0.5+x)/(48*2)
                    local ty = (0.5+y)/(48*2)
                    local tz = 0.5+(zx+zy*2)/(48*2/zscale)
                    local v1 = 0
                    for i = 1,#coeffs do
                        v1 = v1 + coeffs[i](fns[i](tx,ty,tz))
                        tx = tx*2
                        ty = ty*2
                        tz = tz*2
                    end
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
    while true do
        local center = (max+min)/2
        local cval = fn(center)
        print('At '..center..': '..cval)
        if math.abs(cval-goal) < eps then
            return center
        end
        if cval > goal then
            min = center
        else
            max = center
        end
    end
end

local thresh = search(render, -1, 1, goal, 0.001)

local file,err = io.open(fname, 'wb')
if not file then
    print('error: ',err)
    return
end
file:write('P5\n768 768\n255\n')
local area = render(thresh, file)
file:close()

print('Area fraction: '..area)
