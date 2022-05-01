local _ENV = mkmodule('plugins.cxxrandom')

function MakeNewEngine(seed)
    if type(seed) == 'number' then
        if seed == 0 then
            print(":WARNING: Seeds equal to 0 are used if no seed is provided. This indicates to cxxrandom.plug.dll that the engine needs to be seeded with the current time.\nRecommendation: use a non-zero value for your seed, or don't provide a seed to use the time since epoch(1969~).")
        end
        return GenerateEngine(seed)
    elseif type(seed) == 'nil' then
        return GenerateEngine(0)
    else
        error("Argument `seed` must be a number, or nil.")
    end
end

--Class: crng
-------------
crng = {}
function crng:new(engineID, destroyEngineOnDestruction, distrib)
    local o = {}
    self.__index = self
    local idtype = type(engineID)
    local flagtype = type(destroyEngineOnDestruction)

    if idtype == 'number' then
        o.rngID = engineID
    elseif idtype == 'nil' then
        o.rngID = GenerateEngine(0)
    else
        error("Invalid argument type (engineID): " .. tostring(engineID))
    end

    if flagtype ~= 'nil' and flagtype == 'boolean' then
        o.destroyid = destroyEngineOnDestruction
    elseif flagtype == 'nil' then
        o.destroyid = true
    else
        error("Invalid arugment type (destroyEngineOnDestruction): " .. tostring(destroyEngineOnDestruction))
    end

    if type(distrib) ~= 'nil' then
        if type(distrib) == 'table' and type(distrib.next) == 'function' then
            o.distrib = distrib
            o.distrib.rngID = o.rngID
        else
            error("Invalid distribution used as an argument. Cannot set this as the number distribution.")
        end
    end
    setmetatable(o,self)
    return o
end
--crng destructor - we may need to destroy the engine, the user may be doing it manually though
function crng:__gc()
    if self.destroyid then
        DestroyEngine(self.rngID)
    end
end
function crng:changeSeed(seed)
    if type(seed) == 'number' then
        if seed == 0 then
            print(":WARNING: Seeds equal to 0 are used if no seed is provided. This indicates to cxxrandom.plug.dll that the engine needs to be seeded with the current time.\nRecommendation: use a non-zero value for your seed, or don't provide a seed to use the time since epoch(1969~).")
        end
        return NewSeed(self.rngID, seed)
    elseif type(seed) == 'nil' then
        return NewSeed(self.rngID, 0)
    else
        error("Argument `seed` must be a number, or nil.")
    end
end
function crng:setNumDistrib(distrib)
    if type(distrib) == 'table' and type(distrib.next) == 'function' then
        self.distrib = distrib
        self.distrib.rngID = self.rngID
    else
        error("Invalid distribution used as an argument. Cannot set this as the number distribution.")
    end
end
function crng:next()
    if type(self.distrib) == 'table' and type(self.distrib.next) == 'function' then
        return self.distrib:next(self.rngID)
    else
        error("crng object does not have a valid number distribution set")
    end
end
function crng:shuffle()
    if type(self.distrib) == 'table' and type(self.distrib.shuffle) == 'function' then
        self.distrib:shuffle(self.rngID)
    else
        print(":WARNING: No self.distrib.shuffle not found.")
        changeSeed(0)
    end
end

--Class: normal_distribution
----------------------------
normal_distribution = {}
function normal_distribution:new(avg, stddev)
    local o = {}
    self.__index = self
    if type(avg) ~= 'number' or type(stddev) ~= 'number' then
        error("Invalid arguments in normal_distribution construction. Average and standard deviation must be numbers.")
    end
    o.average = avg
    o.std_deviation = stddev
    setmetatable(o,self)
    return o
end
function normal_distribution:next(id)
    return rollNormal(id, self.average, self.std_deviation)
end

--Class: real_distribution
----------------------------
real_distribution = {}
function real_distribution:new(min, max)
    local o = {}
    self.__index = self
    if type(min) ~= 'number' or type(max) ~= 'number' then
        error("Invalid arguments in real_distribution construction. min and max must be numbers.")
    end
    o.min = min
    o.max = max
    setmetatable(o,self)
    return o
end
function real_distribution:next(id)
    return rollDouble(id, self.min, self.max)
end

--Class: int_distribution
----------------------------
int_distribution = {}
function int_distribution:new(min, max)
    local o = {}
    self.__index = self
    if type(min) ~= 'number' or type(max) ~= 'number' then
        error("Invalid arguments in int_distribution construction. min and max must be numbers.")
    end
    o.min = min
    o.max = max
    setmetatable(o,self)
    return o
end
function int_distribution:next(id)
    return rollInt(id, self.min, self.max)
end

--Class: bool_distribution
----------------------------
bool_distribution = {}
function bool_distribution:new(chance)
    local o = {}
    self.__index = self
    if type(chance) ~= 'number' or chance < 0 or chance > 1 then
        error("Invalid arguments in bool_distribution construction. chance must be a number between 0.0 and 1.0 (both included).")
    end
    o.p = chance
    setmetatable(o,self)
    return o
end
function bool_distribution:next(id)
    return rollBool(id, self.p)
end

--Class: num_sequence
----------------------------
num_sequence = {}
function num_sequence:new(a,b)
    local o = {}
    self.__index = self
    local btype = type(b)
    local atype = type(a)
    if atype == 'number' and btype == 'number' then
        if a == b then
            print(":WARNING: You've provided two equal arguments to initialize your sequence with. This is the mechanism used to indicate to cxxrandom.plug.dll that an empty sequence is desired.\nRecommendation: provide no arguments if you wish for an empty sequence.")
        end
        o.seqID = MakeNumSequence(a, b)
    elseif atype == 'table' then
        o.seqID = MakeNumSequence(0,0)
        for _,v in pairs(a) do
            if type(v) ~= 'number' then
                error("num_sequence can only be initialized using numbers. " .. tostring(v) .. " is not a number.")
            end
            AddToSequence(o.seqID, v)
        end
    elseif atype == "nil" and btype == "nil" then
        o.seqID = MakeNumSequence(0,0)
    else
        error("Invalid arguments - a: " .. tostring(a) .. " and b: " .. tostring(b))
    end
    --print("seqID:"..o.seqID)
    setmetatable(o,self)
    return o
end
function num_sequence:__gc()
    DestroyNumSequence(self.seqID)
end
function num_sequence:add(x)
    if type(x) ~= 'number' then
        error("Cannot add non-number to num_sequence.")
    end
    AddToSequence(self.seqID, x)
end
function num_sequence:next()
    return NextInSequence(self.seqID)
end
function num_sequence:shuffle()
    if self.rngID == 'nil' then
        error("Add num_sequence object to crng as distribution, before attempting to shuffle.")
    end
    ShuffleSequence(self.seqID, self.rngID)
end

return _ENV
