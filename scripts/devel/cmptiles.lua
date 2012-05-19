-- Lists and/or compares two tiletype material groups.
-- Usage: devel/cmptiles material1 [material2]

local nmat1,nmat2=...
local mat1 = df.tiletype_material[nmat1]
local mat2 = df.tiletype_material[nmat2]

local tmat1 = {}
local tmat2 = {}

local attrs = df.tiletype.attrs

for i=df.tiletype._first_item,df.tiletype._last_item do
    local shape = df.tiletype_shape[attrs[i].shape] or ''
    local variant = df.tiletype_variant[attrs[i].variant] or ''
    local special = df.tiletype_special[attrs[i].special] or ''
    local direction = attrs[i].direction or ''
    local code = shape..':'..variant..':'..special..':'..direction

    if attrs[i].material == mat1 then
        tmat1[code] = true
    end
    if attrs[i].material == mat2 then
        tmat2[code] = true
    end
end

local function list_diff(n, t1, t2)
    local lst = {}
    for k,v in pairs(t1) do
        if not t2[k] then
            lst[#lst+1] = k
        end
    end
    table.sort(lst)
    for k,v in ipairs(lst) do
        print(n, v)
    end
end

list_diff(nmat1,tmat1,tmat2)
list_diff(nmat2,tmat2,tmat1)
