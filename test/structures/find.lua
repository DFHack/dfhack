config.mode = 'title' -- not safe to run when a world is loaded
config.target = 'core'

local function clean_vec(vec)
    while #vec > 0 do
        if vec[0] then
            expect.true_(vec[0]:delete())
        end
        vec:erase(0)
    end
end

local function with_clean_vec(cls, callback)
    local vec = cls.get_vector()
    dfhack.call_with_finalizer(1, true, clean_vec, vec, callback, vec)
end

function test.image_set()
    -- todo: expand to other types?
    with_clean_vec(df.image_set, function(vec)
        vec:insert('#', {new = df.image_set, id = 1})
        vec:insert('#', {new = df.image_set, id = 2})
        vec:insert('#', {new = df.image_set, id = 4})
        expect.eq(df.image_set.find(1).id, 1)
        expect.eq(df.image_set.find(2).id, 2)
        expect.eq(df.image_set.find(4).id, 4)
        expect.nil_(df.image_set.find(3))
        expect.nil_(df.image_set.find(5))
        expect.nil_(df.image_set.find(-1))
    end)
end
