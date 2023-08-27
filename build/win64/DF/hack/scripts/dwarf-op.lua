-- Optimizes dwarves for labor. Very flexible. Very robust. Check the help.
-- written by josh cooper(cppcooper) [created: Dec. 2017 | last modified: 2020-03-01]

print(dfhack.current_script_name() .. " v1.4")
utils ={}
argparse = require('argparse')
utils = require('utils')
json = require('json')
local rng = require('plugins.cxxrandom')
local engineID = rng.MakeNewEngine()
local dorf_tables = reqscript('internal/dwarf-op/dorf_tables')
cloned = {} --assurances I'm sure
cloned = {
    distributions = utils.clone(dorf_tables.job_distributions, true),
    attrib_levels = utils.clone(dorf_tables.attrib_levels, true),
    types = utils.clone(dorf_tables.types, true),
    jobs = utils.clone(dorf_tables.jobs, true),
    professions = utils.clone(dorf_tables.professions, true),
}
protected_dwarf_signals = {'.', ','}
local validArgs = utils.invert({
    'help',
    'debug',
    'show',
    'reset',
    'resetall',

    'list',

    'select', --highlighted --all --named --unnamed --employed --optimized --unoptimized --protected --unprotected --drunks --jobs
    'clean',
    'clear',
    'reroll',
    'optimize',

    'applyjobs',
    'applyprofessions',
    'applytypes',
    'renamejob'
})
local args = utils.processArgs({...}, validArgs)


if args.debug and tonumber(args.debug) >= 0 then print("Debug info [ON]") end
if args.select and args.select == 'optimized' then
    if args.optimize and not args.clear then
        error("Invalid arguments detected. You've selected only optimized dwarves, and are attempting to optimize them without clearing them. This will not work, so I'm warning you about it with this lovely error.")
    end
end

--[[--
The persistent data contains information on current allocations

FileData: {
    Dwarves : {
        id : {
            job : dorf_table.dorf_jobs[job],
            professions : [
                dorf_table.professions[prof],
            ]
        },
    },
    dorf_table.dorf_jobs[job] : {
        count : int,
        profs : {
            dorf_table.professions[prof] : {
                count : int,
                p : float  --intended ratio of profession in job
            },
        }
    }
}
--]]--
function LoadPersistentData()
    local gamePath = dfhack.getDFPath()
    local fortName = dfhack.TranslateName(df.world_site.find(df.global.plotinfo.site_id).name)
    local savePath = dfhack.getSavePath()
    local fileName = fortName .. ".json.dat"
    local file_cur = gamePath .. "/save/current/" .. fileName
    local file_sav = savePath .. "/" .. fileName
    local cur = json.open(file_cur)
    local saved = json.open(file_sav)
    if saved.exists == true and cur.exists == false then
        print("Previous session save data found. [" .. file_sav .. "]")
        cur.data = saved.data
    elseif saved.exists == false then
        print("No session data found. All dwarves will be treated as non-optimized.")
        --saved:write()
    elseif cur.exists == true then
        print("Existing session data found. [" .. file_cur .. "]")
    end
    OpData = cur.data
end

function SavePersistentData()
    local gamePath = dfhack.getDFPath()
    local fortName = dfhack.TranslateName(df.world_site.find(df.global.plotinfo.site_id).name)
    local fileName = fortName .. ".json.dat"
    local cur = json.open(gamePath .. "/save/current/" .. fileName)
    local newDwfTable = {}
    for k,v in pairs(OpData.Dwarves) do
        if v~=nil then
            newDwfTable[k] = v
        end
    end
    OpData.Dwarves = newDwfTable
    cur.data = OpData
    cur:write()
end

function ClearPersistentData(all)
    local gamePath = dfhack.getDFPath()
    local fortName = dfhack.TranslateName(df.world_site.find(df.global.plotinfo.site_id).name)
    local savePath = dfhack.getSavePath()
    local fileName = fortName .. ".json.dat"
    local file_cur = gamePath .. "/save/current/" .. fileName
    local file_sav = savePath .. "/" .. fileName
    local cur = json.open(gamePath .. "/save/current/" .. fileName)
    print("Deleting " .. file_cur)
    cur.data = {}
    cur:write() --can't seem to find a way to fully nuke this file, unless manually done
    if all then
        print("Deleting " .. file_sav)
        os.remove(file_sav)
    end
end

function safecompare(a,b)
    if a == b then
        return 0
    elseif tonumber(a) and tonumber(b) then
        if a < b then
            return -1
        elseif a > b then
            return 1
        end
    elseif tonumber(a) then
        return 1
    else
        return -1
    end
end

function twofield_compare(t,v1,v2,f1,f2,cmp1,cmp2)
    local a = t[v1]
    local b = t[v2]
    local c1 = cmp1(a[f1],b[f1])
    local c2 = cmp2(a[f2],b[f2])
    if c1 == 0 then
        return c2
    end
    return c1
end

--sorted pairs
function spairs(t, cmp)
    -- collect the keys
    local keys = {}
    for k,v in pairs(t) do
        table.insert(keys,k)
    end

    utils.sort_vector(keys, nil, cmp)

    -- return the iterator function
    local i = 0
    return function()
        i = i + 1
        if keys[i] then
            return keys[i], t[keys[i]]
        end
    end
end

--random pairs
function rpairs(t, gen)
    -- collect the keys
    local keys = {}
    for k,v in pairs(t) do
        table.insert(keys,k)
    end

    -- return the iterator function
    return function()
        local i = gen:next()
        if keys[i] then
            return keys[i], t[keys[i]]
        end
    end
end

function GetChar(str,i)
    return string.sub(str,i,i)
end

function DisplayTable(t,recursion)
    if recursion == nil then
        print('###########################')
        print(t)
        print('######')
        recursion = 0
    elseif recursion == 1 then
        print('-------------')
    elseif recursion == 2 then
        print('-------')
    elseif recursion == 3 then
        print('---')
    end
    for i,k in pairs(t) do
        if type(k) ~= "table" then
            print(i,k)
        end
    end
    for i,k in pairs(t) do
        if type(k) == "table" then
            print(i,k)
            DisplayTable(k,recursion+1)
            if recursion >= 2 then
                print('')
            elseif recursion == 0 then
                print('######')
            end
        end
    end
    if recursion == nil then
        print('###########################')
    end
end

function TableToString(t)
    local s = '['
    local n=0
    for k,v in pairs(t) do
        n=n+1
        if n ~= 1 then
            s = s .. ", "
        end
        s = s .. tostring(v)
    end
    s = s .. ']'
    return s
end

function count_this(to_be_counted)
    local count = -1
    local var1 = ""
    while var1 ~= nil do
        count = count + 1
        var1 = (to_be_counted[count])
    end
    count=count-1
    return count
end

function ArrayLength(t)
    local count = 0
    for i,k in pairs(t) do
        if tonumber(i) then
            count = count + 1
        end
    end
    return count
end

function TableLength(table)
    local count = 0 for i,k in pairs(table) do
        count = count + 1
    end
    return count
end

function TableContainsValue(t,value)
    for _,v in pairs(t) do
        if v == value then
            return true
        end
    end
    return false
end

function FindValueKey(t, value, depth)
    if depth == nil then
        depth = 0
    elseif depth == 10 then
        return nil
    end
    for k,v in pairs(t) do
        if v == value then
            return k
        elseif type(v) == 'table' then
            if FindValueKey(v, value, depth + 1) ~= nil then
                return k
            end
        end
    end
    return nil
end

function FindKeyValue(t, key)
    for k,v in pairs(t) do
        if k == key then
            return v
        end
    end
end

function GetRandomTableEntry(gen, t)
    -- iterate over whole table to get all keys
    local keyset = {}
    for k in pairs(t) do
        table.insert(keyset, k)
    end
    -- now you can reliably return a random key
    local N = TableLength(t)
    local i = gen:next()
    local key = keyset[i]
    local R = t[key]
    if args.debug and tonumber(args.debug) >= 3 then print(N,i,key,R) end
    return R
end

local attrib_seq = rng.num_sequence:new(1,TableLength(cloned.attrib_levels))
function GetRandomAttribLevel() --returns a randomly generated value for assigning to an attribute
    local gen = rng.crng:new(engineID,false,attrib_seq)
    gen:shuffle()
    while true do
        local level = GetRandomTableEntry(gen, cloned.attrib_levels)
        if rng.rollBool(engineID, level.p) then
            return level
        end
    end
    return nil
end

function isValidJob(job)
    if job ~= nil and job.req ~= nil then
        local jobName = FindValueKey(cloned.jobs, job)
        local jd = cloned.distributions[jobName]
        if not jd then
            error(string.format("Job distribution not found. Job: %s; jobName: %s",job,jobName))
        end
        if OpData[jobName].count < jd.max then
            return true
        end
    end
    return false
end

--Gets the skill table for a skill id from a particular dwarf
function GetSkillTable(dwf, skill)
    local id = df.job_skill[skill]
    assert(id, "Invalid skill - GetSkillTable(" .. skill .. ")")
    for _,skillTable in pairs(dwf.status.current_soul.skills) do
        if skillTable.id == id then
            return skillTable
        end
    end
    if args.debug and tonumber(args.debug) >= 0 then print("Could not find skill: " .. skill) end
    return nil
end

function GenerateStatValue(stat, atr_lvl)
    atr_lvl = atr_lvl == nil and GetRandomAttribLevel() or cloned.attrib_levels[atr_lvl]
    if args.debug and tonumber(args.debug) >= 4 then print(atr_lvl, atr_lvl[1], atr_lvl[2]) end
    local R = rng.rollNormal(engineID, atr_lvl[1], atr_lvl[2])
    local value = math.floor(R)
    value = value < 0 and 0 or value
    value = value > 5000 and 5000 or value
    stat.value = stat.value < value and value or stat.value
    if args.debug and tonumber(args.debug) >= 3 then print(R, stat.value) end
end

function LoopStatsTable(statsTable, callback)
    for k, v in safe_pairs(statsTable) do
        callback(v)
    end
end

function ApplyType(dwf, dwf_type)
    local type = cloned.types[dwf_type]
    assert(type, "Invalid dwarf type.")
    for attribute, atr_lvl in pairs(type.attribs) do
        if args.debug and tonumber(args.debug) >= 3 then print(attribute, atr_lvl[1]) end
        if
        attribute == 'STRENGTH' or
        attribute == 'AGILITY' or
        attribute == 'TOUGHNESS' or
        attribute == 'ENDURANCE' or
        attribute == 'RECUPERATION' or
        attribute == 'DISEASE_RESISTANCE'
        then
            GenerateStatValue(dwf.body.physical_attrs[attribute], atr_lvl[1])
        elseif
        attribute == 'ANALYTICAL_ABILITY' or
        attribute == 'FOCUS' or
        attribute == 'WILLPOWER' or
        attribute == 'CREATIVITY' or
        attribute == 'INTUITION' or
        attribute == 'PATIENCE' or
        attribute == 'MEMORY' or
        attribute == 'LINGUISTIC_ABILITY' or
        attribute == 'SPATIAL_SENSE' or
        attribute == 'MUSICALITY' or
        attribute == 'KINESTHETIC_SENSE' or
        attribute == 'EMPATHY' or
        attribute == 'SOCIAL_AWARENESS'
        then
            GenerateStatValue(dwf.status.current_soul.mental_attrs[attribute], atr_lvl[1])
        else
            error("Invalid stat:" .. attribute)
        end
    end
    if type.skills ~= nil then
        for skill, skillRange in pairs(type.skills) do
            local sTable = GetSkillTable(dwf, skill)
            if sTable == nil then
                --print("ApplyType()", skill, skillRange)
                utils.insert_or_update(dwf.status.current_soul.skills, { new = true, id = df.job_skill[skill], rating = 0 }, 'id')
                sTable = GetSkillTable(dwf, skill)
            end
            local points = rng.rollInt(engineID, skillRange[1], skillRange[2])
            sTable.rating = sTable.rating < points and points or sTable.rating
            sTable.rating = sTable.rating > 20 and 20 or sTable.rating
            sTable.rating = sTable.rating <= 1 and 1 or sTable.rating
            if args.debug and tonumber(args.debug) >= 2 then print(skill .. ".rating = " .. sTable.rating) end
        end
    end
    return true
end

--Apply only after previously validating
function ApplyProfession(dwf, profession, min, max)
    local prof = cloned.professions[profession]
    --todo: consider counting total dwarves trained in a profession [currently counting total sub-professions, of a job]
    for skill, bonus in pairs(prof.skills) do
        local sTable = GetSkillTable(dwf, skill)
        if sTable == nil then
            utils.insert_or_update(dwf.status.current_soul.skills, { new = true, id = df.job_skill[skill], rating = 0 }, 'id')
            sTable = GetSkillTable(dwf, skill)
        end
        local points = rng.rollInt(engineID, min, max)
        sTable.rating = sTable.rating < points and points or sTable.rating
        --sTable.natural_skill_lvl =
        sTable.rating = sTable.rating + bonus
        sTable.rating = sTable.rating >= 20 and 20 or sTable.rating
        sTable.rating = sTable.rating <= 2 and 2 or sTable.rating
        if args.debug and tonumber(args.debug) >= 2 then print(skill .. ".rating = " .. sTable.rating) end
    end
    return true
end

--Apply only after previously validating
function ApplyJob(dwf, jobName) --job = dorf_jobs[X]
    local jd = cloned.distributions[jobName]
    local job = cloned.jobs[jobName]
    if args.debug and tonumber(args.debug) >= 3 then print(dwf,job,jobName, OpData[jobName]) end
    OpData[jobName].count = OpData[jobName].count + 1
    jd.cur = OpData[jobName].count
    local id = tostring(dwf.id)
    DwarvesData[id] = {}
    DwarvesData[id]['job'] = jobName
    DwarvesData[id]['professions'] = {}
    if not OpData[jobName] then
        OpData[jobName] = {}
    end
    dwf.custom_profession = jobName
    RollStats(dwf, job.types)

    -- Apply required professions
    local bAlreadySetProf2 = false
    local job_req_sequence = rng.num_sequence:new()
    for i=1,ArrayLength(job.req) do
        job_req_sequence:add(i)
    end
    local gen = rng.crng:new(engineID,false,job_req_sequence)
    --two required professions are set as the professional titles for a dwarf [prof1, prof2]
    --so when more than 2 are required it is necessary to randomize the iteration of their application to a dwarf
    --this is done with rpairs and the above RNG code
    gen:shuffle()
    job_req_sequence:add(0) --adding an out of bounds key (ie. 0) to ensure rpairs won't keep going forever
    --[note it is added after shuffling]
    local i = 0
    for _, prof in pairs(job.req) do
        --> Set Profession(s) (by #)
        i = i + 1 --since the key can't tell us what iteration we're on
        if i == 1 then
            dwf.profession = df.profession[prof]
        elseif i == 2 then
            bAlreadySetProf2 = true
            dwf.profession2 = df.profession[prof]
        end
        --These are required professions for this job class
        ApplyProfession(dwf, prof, 11, 17)
    end

    -- Loop tertiary professions
    -- Sort loop (asc)
    local points = 11
    local base_dec = 11 / job.max[1]
    local total = 0
    --We want to loop through professions according to need (ie. count & ratio(ie. p))
    for prof, t in spairs(OpData[jobName].profs,
    function(a,b)
        return twofield_compare(OpData[jobName].profs,
        a, b, 'count', 'p',
        function(f1,f2) return safecompare(f1,f2) end,
        function(f1,f2) return safecompare(f2,f1) end)
    end)
    do
        if total < job.max[1] then
            if args.debug and tonumber(args.debug) >= 1 then print("dwf id:", dwf.id, jobName, prof) end
            local ratio = job[prof]
            if ratio ~= nil then --[[not clear why this was happening, simple fix though
                (tried to reproduce the next day and couldn't,
                must have been a bad table lingering in memory between tests despite resetting persistent data and dwarves)
                --]]
                local max = math.ceil(points)
                local min = math.ceil(points - 5)
                min = min < 0 and 0 or min
                --Firsts are special
                if OpData[jobName].profs[prof].count < (ratio * OpData[jobName].count) and points > 7.7 then
                    ApplyProfession(dwf, prof, min, max)
                    table.insert(DwarvesData[id]['professions'], prof)
                    OpData[jobName].profs[prof].count = OpData[jobName].profs[prof].count + 1
                    if args.debug and tonumber(args.debug) >= 1 then print("count: ", OpData[jobName].profs[prof].count) end

                    if not bAlreadySetProf2 then
                        bAlreadySetProf2 = true
                        dwf.profession2 = df.profession[prof]
                    end
                    points = points - base_dec
                    total = total + 1
                else
                    local p = OpData[jobName].profs[prof].count > 0 and (1 - (ratio / ((ratio*OpData[jobName].count) / OpData[jobName].profs[prof].count))) or ratio
                    p = p < 0 and 0 or p
                    p = p > 1 and 1 or p
                    --p = (p - math.floor(p)) >= 0.5 and math.ceil(p) or math.floor(p)
                    --> proc probability and check points
                    if points >= 1 and rng.rollBool(engineID, p) then
                        ApplyProfession(dwf, prof, min, max)
                        table.insert(DwarvesData[id]['professions'], prof)
                        OpData[jobName].profs[prof].count = OpData[jobName].profs[prof].count + 1
                        if args.debug and tonumber(args.debug) >= 1 then print("dwf id:", dwf.id, "count: ", OpData[jobName].profs[prof].count, jobName, prof) end

                        if not bAlreadySetProf2 then
                            bAlreadySetProf2 = true
                            dwf.profession2 = df.profession[prof]
                        end
                        points = points - base_dec
                        total = total + 1
                    end
                end
            end
        end
    end
    if not bAlreadySetProf2 then
        dwf.profession2 = dwf.profession
    end
    return true
end

function RollStats(dwf, types)
    LoopStatsTable(dwf.body.physical_attrs, GenerateStatValue)
    LoopStatsTable(dwf.status.current_soul.mental_attrs, GenerateStatValue)
    for i, type in pairs(types) do
        if args.debug and tonumber(args.debug) >= 4 then print(i, type) end
        ApplyType(dwf, type)
    end
    for type, table in pairs(cloned.types) do
        local p = table.p
        if p ~= nil then
            if rng.rollBool(engineID, p) then
                ApplyType(dwf, type)
            end
        end
    end
end

--Returns true if a job was found and applied, returns false otherwise
function FindJob(dwf, recursive)

    if isDwarfOptimized(dwf) then
        return false
    end
    for jobName, jd in spairs(cloned.distributions,
    function(a,b)
        return twofield_compare(cloned.distributions,
        a, b, 'cur', 'max',
        function(a,b) return safecompare(a,b) end,
        function(a,b) return safecompare(b,a) end)
   end)
    do
        if args.debug and tonumber(args.debug) >= 4 then print("FindJob() ", jobName) end
        local job = cloned.jobs[jobName]
        if isValidJob(job) then
            if args.debug and tonumber(args.debug) >= 1 then print("Found a job!") end
            ApplyJob(dwf, jobName)
            return true
        end
    end
    --not recursive => not recursively called (yet~)
    if not recursive and TrySecondPassExpansion() then
        return FindJob(dwf, true)
    end
    print(":WARNING: No job found, that is bad?!")
    return false
end

function TrySecondPassExpansion() --Tries to expand distribution maximums
    local curTotal = 0
    for k,v in pairs(cloned.distributions) do
        if v.cur ~= nil then
            curTotal = curTotal + v.cur
        end
    end

    if curTotal < work_force then
        local I = 0
        for i, v in pairs(cloned.distributions.Thresholds) do
            if work_force >= v then
                I = i + 1
            end
        end

        local delta = 0
        for jobName, jd in spairs(cloned.distributions,
        function(a,b)
            return twofield_compare(cloned.distributions,
            a, b, 'max', 'cur',
            function(a,b) return safecompare(a,b) end,
            function(a,b) return safecompare(a,b) end)
        end)
        do
            if cloned.jobs[jobName] then
                if (curTotal + delta) < work_force then
                    delta = delta + jd[I]
                    jd.max = jd.max + jd[I]
                end
            end
        end
        return true
    end
    return false
end

function CleanDwarf(dwf)
    threshold=tonumber(args.clean)
    if threshold then
        N=-1
        for _,_ in ipairs(dwf.status.current_soul.skills) do
            N = N + 1
        end
        utils.sort_vector(dwf.status.current_soul.skills, 'id')
        for i=N,0,-1 do
            v=dwf.status.current_soul.skills[i]
            --print(i)
            --print(v.rating,v.id,df.job_skill[v.id])
            if v.rating <= threshold then
                utils.erase_sorted_key(dwf.status.current_soul.skills, v.id, 'id')
            end
        end
        return true
    end
    error("invalid value given for argument '-clean <value>'")
end

function ZeroDwarf(dwf)
    LoopStatsTable(dwf.body.physical_attrs, function(attribute) attribute.value = 0 end)
    LoopStatsTable(dwf.status.current_soul.mental_attrs, function(attribute) attribute.value = 0 end)

    local count_max = count_this(df.job_skill)
    utils.sort_vector(dwf.status.current_soul.skills, 'id')
    for i=0, count_max do
        utils.erase_sorted_key(dwf.status.current_soul.skills, i, 'id')
    end

    dfhack.units.setNickname(dwf, "")
    dwf.custom_profession = ""
    dwf.profession = df.profession['DRUNK']
    dwf.profession2 = df.profession['DRUNK']

    for id, dwf_data in pairs(DwarvesData) do
        if next(dwf_data) ~= nil and id == tostring(dwf.id) then
            print("Clearing loaded dwf data for dwf id: " .. id)
            local jobName = dwf_data.job
            local job = cloned.jobs[jobName]
            OpData[jobName].count = OpData[jobName].count - 1
            for i, prof in pairs(dwf_data.professions) do
                OpData[jobName].profs[prof].count = OpData[jobName].profs[prof].count - 1
                if args.debug and tonumber(args.debug) >= 1 then print("dwf id:", dwf.id, "count: ", OpData[jobName].profs[prof].count, jobName, prof) end
            end
            DwarvesData[id] = nil
            --table.remove(DwarvesData,id)
        elseif next(dwf_data) == nil and id == tostring(dwf.id) then
            print(":WARNING: ZeroDwarf(dwf) - dwf was zeroed, but had never been optimized before")
            --error("this dwf_data shouldn't be nil, I think.. I guess maybe if you were clearing dwarves that weren't optimized")
        end
    end
    return true
end

function Reroll(dwf)
    local id = tostring(dwf.id)
    local jobName = DwarvesData[id].job
    if cloned.jobs[jobName] then
        if args.reroll ~= 'inclusive' then
            ZeroDwarf(dwf)
        end
        ApplyJob(dwf, jobName)
        return true
    end
    return false
end

function RenameJob(dwf)
    if args.renamejob ~= nil then
        dwf.custom_profession = args.renamejob
        return true
    end
    return false
end

waves={}
local ticks_per_day = 1200;
local ticks_per_month = 28 * ticks_per_day;
local ticks_per_season = 3 * ticks_per_month;
local ticks_per_year = 12 * ticks_per_month;
local current_tick = df.global.cur_year_tick
local seasons = {
    'spring',
    'summer',
    'autumn',
    'winter',
}
function GetWave(dwf)
    arrival_time = current_tick - dwf.curse.time_on_site;
    --print(string.format("Current year %s, arrival_time = %s, ticks_per_year = %s", df.global.cur_year, arrival_time, ticks_per_year))
    arrival_year = df.global.cur_year + (arrival_time // ticks_per_year);
    arrival_season = 1 + (arrival_time % ticks_per_year) // ticks_per_season;
    wave = 10 * arrival_year + arrival_season
    if waves[wave] == nil then
        waves[wave] = {}
    end
    table.insert(waves[wave],dwf)
    --print(string.format("Arrived in the %s of the year %s. Wave %s, arrival time %s",seasons[season+1],year, wave, arrival_month))
end

function Show(dwf)
    local name_ptr = dfhack.units.getVisibleName(dwf)
    local name = dfhack.TranslateName(name_ptr)
    print(string.format("%6d [wave:%2d] - %-23s (%3d,%3d) %s", dwf.id, tonumber(FindValueKey(zwaves,dwf)), name, dwf.profession, dwf.profession2, dwf.custom_profession))
    --print('('..dwf.id..') - '..name..spaces..dwf.profession,dwf.custom_profession)
end

function LoopUnits(units, check, fn, checkoption, profmin, profmax) --cause nothing else will use arg 5 or 6
    local count = 0
    for _, unit in pairs(units) do
        if check ~= nil then
            if check(unit, checkoption, profmin, profmax) then
                if fn ~= nil then
                    if fn(unit) then
                        count = count + 1
                    end
                else
                    count = count + 1
                end
            end
        elseif fn ~= nil then
            if fn(unit) then
                count = count + 1
            end
        end
    end
    if args.debug and tonumber(args.debug) >= 1 then
        print("loop count: ", count)
    end
    return count
end

function LoopTable_Apply_ToUnits(units, apply, applytable, checktable, profmin, profmax) --cause nothing else will use arg 5 or 6
    local count = 0
    local temp = 0
    for _,tvalue in pairs(applytable) do
        if checktable[tvalue] then
            temp = LoopUnits(units, apply, nil, tvalue, profmin, profmax)
            count = count < temp and temp or count
        else
            error("\nInvalid option: " .. tvalue .. "\nLook-up table: " .. checktable)
        end
    end
    return count
end

------------
--CHECKERS--
------------

--Returns true if the DWARF has a user-given name
function isDwarfNamed(dwf)
    return dwf.status.current_soul.name.nickname ~= ""
end

--Returns true if the DWARF has a custom_profession
function isDwarfEmployed(dwf)
    return dwf.custom_profession ~= ""
end

--Returns true if the DWARF is in the DwarvesData table
function isDwarfOptimized(dwf)
    local id = tostring(dwf.id)
    local dorf = DwarvesData[id]
    return dorf ~= nil
end

--Returns true if the DWARF is not in the DwarvesData table
function isDwarfUnoptimized(dwf)
    return (not isDwarfOptimized(dwf))
end

--Returns true if the DWARF uses a protection signal in its name or profession
function isDwarfProtected(dwf)
    if dwf.custom_profession ~= "" then
        for _,signal in pairs(protected_dwarf_signals) do
            if string.find(dwf.custom_profession, signal, 1, true) then
                return true
            end
        end
    end
    if dwf.status.current_soul.name.nickname ~= "" then
        for _,signal in pairs(protected_dwarf_signals) do
            if string.find(dwf.status.current_soul.name.nickname, signal, 1, true) then
                return true
            end
        end
    end
    return false
end

--Returns true if the DWARF doesn't use a protection signal in its name or profession
function isDwarfUnprotected(dwf)
    return (not isDwarfProtected(dwf))
end

function isDwarfCitizen(dwf)
    return dfhack.units.isCitizen(dwf)
end

function CanWork(dwf)
    return dfhack.units.isCitizen(dwf) and dfhack.units.isAdult(dwf)
end

local includeProtectedDwfs = false
function CheckWorker(dwf, option)
    if CanWork(dwf) then
        local name = dfhack.TranslateName(dfhack.units.getVisibleName(dwf))
        local nickname = dwf.status.current_soul.name.nickname
        --check option data type (string/table)
            --normal
                --check if option specifies a pattern which matches the name of this dwf
                --check if we want highlighted dwf & whether that is this dwf
                --check if option starts with 'p'
                --check all the possible options
            --list:
                --check if option[1] starts with 'p'
                --check all possible options
        local list
        if type(option) == 'string' then
            list = argparse.stringList(option)
        else
            error("The select option entered is not a string.")
        end
        local N = #list

        -- parse the selection arguments
        if N == 1 then
            -- highlighted
            if option == 'highlighted' then
                if CanWork(dfhack.gui.getSelectedUnit()) then
                    return dwf == dfhack.gui.getSelectedUnit()
                else
                    error("The selected unit isn't a dwarf, or can't work. This script is not intended for such units.")
                end
            -- protected
            elseif GetChar(option,1) == 'p' then
                includeProtectedDwfs = true
                if option ~= 'protected' then
                    -- the p was simply a signal, we need to remove it
                    option = string.sub(option,2)
                end
            end
            -- primary selection criteria
            if includeProtectedDwfs or isDwarfUnprotected(dwf) then
                if option == 'all' then
                    return true
                elseif option == 'named' then
                    return isDwarfNamed(dwf)
                elseif option == 'unnamed' then
                    return (not isDwarfNamed(dwf))
                elseif option == 'employed' then
                    return isDwarfEmployed(dwf)
                elseif option == 'optimized' then
                    return isDwarfOptimized(dwf)
                elseif option == 'unoptimized' then
                    return isDwarfUnoptimized(dwf)
                elseif option == 'protected' then
                    return isDwarfProtected(dwf)
                elseif option == 'unprotected' then
                    return isDwarfUnprotected(dwf)
                elseif option == 'drunks' or option == 'drunk' then
                    return dwf.profession == df.profession['DRUNK'] and dwf.profession2 == df.profession['DRUNK']
                end
            end
        -- argument list for `--select`
        elseif N > 1 then
            local select_type = list[1]
            if GetChar(select_type,1) == 'p' or select_type == 'name' or select_type == 'names' then
                includeProtectedDwfs = true
                if GetChar(select_type,1) == 'p' then
                    select_type = string.sub(select_type,2)
                else
                    select_type = string.sub(select_type,1)
                end
            end

            local n=0
            for _,v in pairs(list) do
                n=n+1
                if n > 1 and (includeProtectedDwfs or isDwarfUnprotected(dwf)) then
                    if select_type == 'name' or select_type == 'names' then
                        if string.find(name,v) or string.find(nickname,v) then
                            return true
                        end
                    elseif select_type == 'job' or select_type == 'jobs' then
                        if dwf.custom_profession == v then
                            return true
                        end
                    elseif select_type == 'wave' or select_type == 'waves' then
                        if TableContainsValue(zwaves[tonumber(v)],dwf) then
                            return true
                        end
                    end
                end
            end
        end
    end
    return false
end

----------------
--END CHECKERS--
----------------

function Prepare()
    print("Loading persistent data..")
    --Load /current/fort.json.dat or /world/fort.json.dat
    LoadPersistentData()
    if not OpData.Dwarves then
        OpData.Dwarves = {}
    end
    DwarvesData = OpData.Dwarves

    --[[ We need to validate the persistent data/
        Perhaps I/you/we updated the dorf_tables, so we should check.]]
    --Initialize OpData
    for jobName, job in pairs(cloned.jobs) do --should be looping the distribution table instead (probably a major refactor needed)
        PrepareDistributionMax(jobName)
        if not OpData[jobName] then
            OpData[jobName] = {}
            OpData[jobName].count = 0
            OpData[jobName].profs = {}
        end
        for prof, p in pairs(job) do
            if tonumber(p) then
                if not OpData[jobName].profs[prof] then
                    OpData[jobName].profs[prof] = {}
                    OpData[jobName].profs[prof].count = 0
                end
                OpData[jobName].profs[prof].p = p
            end
        end
    end
    if args.debug and tonumber(args.debug) >= 4 then
        print("OpData, job counts")
        DisplayTable(OpData) --this is gonna print out a lot of data, including the persistent data
    end
    --Count Professions from 'DwarvesData'
    --[[for id, dwf_data in pairs(DwarvesData) do
        local jobName = dwf_data.job
        local job = cloned.jobs[jobName]
        local profs = dwf_data.professions
        OpData[jobName].count = OpData[jobName].count + 1
        for i, prof in pairs(profs) do
            OpData[jobName].profs[prof].count = OpData[jobName].profs[prof].count + 1
        end
    end--]]

    --TryClearDwarf Loop (or maybe not)
    print("Data load complete.")
    print("Calculating wave enumerations..")
    GetWaves()
    print("Done calculating.")
end

function PrepareDistributionMax(jobName)
    local jd = cloned.distributions[jobName]
    if not jd then
        error("Job distribution not found. Job: " .. jobName)
    elseif jd.max ~= nil then
        error("job distribution max is not nil - " .. jobName)
    end
    local IndexMax = 0
    for i, v in pairs(cloned.distributions.Thresholds) do
        if work_force >= v then
            IndexMax = i
        end
    end
    --print(cloned.distributions.Thresholds[IndexMax])
    local max = 0
    for i=1, IndexMax do
        max = max + jd[i]
    end
    jd.max = max
end

function SelectDwarf(dwf)
    table.insert(selection, dwf)
    return true
end

zwaves={}
function GetWaves()
    LoopUnits(df.global.world.units.active, CanWork, GetWave)
    i = 0
    for k,v in spairs(waves, utils.compare) do
        --print(string.format("zwave[%s] = wave[%s]",i,k))
        zwaves[i] = waves[k]
        i = i + 1
    end
end

function ShowHelp()
    print(dfhack.script_help())
    print("No dorfs were harmed in the building of this help screen.")
end

function ShowHint()
    print("\n============\ndwarf-op script")
    print("~~~~~~~~~~~~")
    print("To use this script, you need to select a subset of your dwarves. Then run commands on those dwarves.")
    print("Examples:")
    print("  [DFHack]# dwarf-op -select [ jobs Trader Miner Leader Rancher ] -applytype adaptable")
    print("  [DFHack]# dwarf-op -select all -clear -optimize")
    print("  [DFHack]# dwarf-op -select optimized -reroll")
    print("  [DFHack]# dwarf-op -select Urist -reroll inclusive -applyprofession RECRUIT")
end

if args.help then
    ShowHelp()
    return
end

local ActiveUnits = df.global.world.units.active
dwarf_count = LoopUnits(ActiveUnits, isDwarfCitizen)
work_force = LoopUnits(ActiveUnits, CanWork)
Prepare()
print('\nActive Units Population: ' .. ArrayLength(ActiveUnits))
print("Dwarf Population: " .. dwarf_count)
print("Work Force: " .. work_force)
print("Existing Optimized Dwarves: " .. ArrayLength(OpData.Dwarves))

function exists(thing)
    if thing then return true else return false end
end
args.b_clear = exists(args.clear) if args.debug and tonumber(args.debug) >= 0 then print(        "args.b_clear:    " .. tostring(args.b_clear)) end
args.b_optimize = exists(args.optimize) if args.debug and tonumber(args.debug) >= 0 then print(  "args.b_optimize: " .. tostring(args.b_optimize)) end
args.b_reroll = exists(args.reroll) if args.debug and tonumber(args.debug) >= 0 then print(      "args.b_reroll:   " .. tostring(args.b_reroll)) end
args.b_applyjobs = exists(args.applyjobs) if args.debug and tonumber(args.debug) >= 0 then print("args.b_applyjob: " .. tostring(args.b_applyjobs)) end
if not args.select then
    if args.reset or args.resetall then
        ClearPersistentData(exists(args.resetall))
    else
        args.select = 'highlighted'
    end
end

bRanCommands=true

if args.select and (args.debug or args.clean or args.clear or args.optimize or args.reroll or args.applyjobs or args.applyprofessions or args.applytypes or args.renamejob) then
    selection = {}
    count = 0
    print("Selected Dwarves: " .. LoopUnits(ActiveUnits, CheckWorker, SelectDwarf, args.select))

    if args.b_clear ~= args.b_reroll or not args.b_clear then
        --error("Clear is implied with Reroll. Choose one, not both.")
        if args.b_reroll and args.b_optimize then
            error("options: optimize, reroll. Choose one, and only one.")
        else
            --
            --Valid options were entered
            --
            local affected = 0
            local temp = 0
            if args.reset or args.resetall then
                ClearPersistentData(exists(args.resetall))
            end
            if args.clear then
                print("\nResetting selected dwarves..")
                temp = LoopUnits(selection, nil, ZeroDwarf)
                affected = affected < temp and temp or affected
            end

            if args.optimize then
                print("\nOptimizing selected dwarves..")
                temp = LoopUnits(selection, nil, FindJob)
                affected = affected < temp and temp or affected
            elseif args.reroll then
                print("\nRerolling selected dwarves..")
                temp = LoopUnits(selection, nil, Reroll)
                affected = affected < temp and temp or affected
            end

            if args.clean then
                print("\nCleaning skills list of selected dwarves..")
                temp = LoopUnits(selection, nil, CleanDwarf)
                affected = affected < temp and temp or affected
            end

            if args.applyjobs then
                if type(args.applyjobs) ~= 'table' then
                    args.applyjobs = argparse.stringList(args.applyjobs)
                end
                print("Applying jobs:" .. TableToString(args.applyjobs) .. ", to selected dwarves")
                temp = LoopTable_Apply_ToUnits(selection, ApplyJob, args.applyjobs, cloned.jobs)
                affected = affected < temp and temp or affected
            end
            if args.applyprofessions then
                if type(args.applyprofessions) ~= 'table' then
                    args.applyprofessions = argparse.stringList(args.applyprofessions)
                end
                print("Applying professions:" .. TableToString(args.applyprofessions) .. ", to selected dwarves")
                temp = LoopTable_Apply_ToUnits(selection, ApplyProfession, args.applyprofessions, cloned.professions,1,5)
                affected = affected < temp and temp or affected
            end
            if args.applytypes then
                if type(args.applytypes) ~= 'table' then
                    args.applytypes = argparse.stringList(args.applytypes)
                end
                print("Applying types:" .. TableToString(args.applytypes) .. ", to selected dwarves")
                temp = LoopTable_Apply_ToUnits(selection, ApplyType, args.applytypes, cloned.types)
                affected = affected < temp and temp or affected
            end
            if args.renamejob and type(args.renamejob) == 'string' then
                temp = LoopUnits(selection, nil, RenameJob)
                affected = affected < temp and temp or affected
            end
            print(affected .. " dwarves affected.")

            if args.debug and tonumber(args.debug) >= 1 then
                print("\n")
                print("cur", "max", "job", "\n  ~~~~~~~~~")
                for k,v in pairs(cloned.distributions) do
                    print(v.cur, v.max, k)
                end
            end
            --
            --Valid options code block ending
            --
        end
    else
        error("Clear is implied with Reroll. Choose one, not both.")
    end
else
    bRanCommands=false
end

if args.list then
    if args.list == "all" then
        dfhack.run_command("devel/query -1 -alignto 38 -script internal/dwarf-op/dorf_tables -nopointers")
    elseif args.list == "job_distributions" then
        dfhack.run_command("devel/query -1 -alignto 21 -script internal/dwarf-op/dorf_tables -getfield job_distributions -nopointers")
    elseif args.list == "attrib_levels" then
        dfhack.run_command("devel/query -1 -alignto 21 -script internal/dwarf-op/dorf_tables -getfield attrib_levels -nopointers")
    elseif args.list == "jobs" then
        dfhack.run_command("devel/query -1 -alignto 18 -script internal/dwarf-op/dorf_tables -getfield jobs -nopointers")
    elseif args.list == "professions" then
        dfhack.run_command("devel/query -1 -alignto 28 -script internal/dwarf-op/dorf_tables -getfield professions -nopointers")
    elseif args.list == "types" then
        dfhack.run_command("devel/query -1 -alignto 38 -script internal/dwarf-op/dorf_tables -getfield types -nopointers")
    else
        error("Invalid argument provided.")
    end
    bRanCommands = true
end

if args.show then
    selection = {}
    print("Selected Dwarves: " .. LoopUnits(ActiveUnits, CheckWorker, SelectDwarf, args.select))
    LoopUnits(selection, nil, Show)
elseif not bRanCommands then
    print("It looks like you may have entered an invalid combination of arguments. Check -help or report a bug.")
end
SavePersistentData()
print('\n')

--Query(dfhack, '','dfhack')
--Query(OpData, '', 'pd')

attrib_seq = nil
rng.DestroyEngine(engineID)
collectgarbage()
