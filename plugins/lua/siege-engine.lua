local _ENV = mkmodule('plugins.siege-engine')

--[[

 Native functions:

 * getTargetArea(building) -> point1, point2
 * clearTargetArea(building)
 * setTargetArea(building, point1, point2) -> true/false

 * isLinkedToPile(building,pile) -> true/false
 * getStockpileLinks(building) -> {pile}
 * addStockpileLink(building,pile) -> true/false
 * removeStockpileLink(building,pile) -> true/false

 * saveWorkshopProfile(building) -> profile

 * getAmmoItem(building) -> item_type
 * setAmmoItem(building,item_type) -> true/false

 * isPassableTile(pos) -> true/false
 * isTreeTile(pos) -> true/false
 * isTargetableTile(pos) -> true/false

 * getTileStatus(building,pos) -> 'invalid/ok/out_of_range/blocked/semiblocked'
 * paintAimScreen(building,view_pos_xyz,left_top_xy,size_xy)

 * canTargetUnit(unit) -> true/false

 proj_info = { target = pos, [delta = float/pos], [factor = int] }

 * projPosAtStep(building,proj_info,step) -> pos
 * projPathMetrics(building,proj_info) -> {
       hit_type = 'wall/floor/ceiling/map_edge/tree',
       collision_step = int,
       collision_z_step = int,
       goal_distance = int,
       goal_step = int/nil,
       goal_z_step = int/nil,
       status = 'ok/out_of_range/blocked'
   }

 * adjustToTarget(building,pos) -> pos,ok=true/false

 * traceUnitPath(unit) -> { {x=int,y=int,z=int[,from=time][,to=time]} }
 * unitPosAtTime(unit, time) -> pos

 * proposeUnitHits(building) -> { {
       pos=pos, unit=unit, time=float, dist=int,
       [lmargin=float,] [rmargin=float,]
   } }

 * computeNearbyWeight(building,hits,{[id/unit]=score}[,fname])

]]

local utils = require 'utils'

Z_STEP_COUNT = 15
Z_STEP = 1/31

ANNOUNCEMENT_FLAGS = {
    UNIT_COMBAT_REPORT = true
}

function getMetrics(engine, path)
    path.metrics = path.metrics or projPathMetrics(engine, path)
    return path.metrics
end

function findShotHeight(engine, target)
    local path = { target = target, delta = 0.0 }

    if getMetrics(engine, path).goal_step then
        return path
    end

    local tpath = { target = target, delta = Z_STEP_COUNT*Z_STEP }

    if getMetrics(engine, tpath).goal_step then
        for i = 1,Z_STEP_COUNT-1 do
            path = { target = target, delta = i*Z_STEP }
            if getMetrics(engine, path).goal_step then
                return path
            end
        end

        return tpath
    end

    tpath = { target = target, delta = -Z_STEP_COUNT*Z_STEP }

    if getMetrics(engine, tpath).goal_step then
        for i = 1,Z_STEP_COUNT-1 do
            path = { target = target, delta = -i*Z_STEP }
            if getMetrics(engine, path).goal_step then
                return path
            end
        end

        return tpath
    end
end

function findReachableTargets(engine, targets)
    local reachable = {}
    for _,tgt in ipairs(targets) do
        tgt.path = findShotHeight(engine, tgt.pos)
        if tgt.path then
            table.insert(reachable, tgt)
        end
    end
    return reachable
end

recent_targets = recent_targets or {}

if dfhack.is_core_context then
    dfhack.onStateChange[_ENV] = function(code)
        if code == SC_MAP_LOADED then
            recent_targets = {}
        end
    end
end

function saveRecent(unit)
    local id = unit.id
    local tgt = recent_targets
    tgt[id] = (tgt[id] or 0) + 1
    dfhack.timeout(3, 'days', function()
        tgt[id] = math.max(0, tgt[id]-1)
    end)
end

function getBaseUnitWeight(unit)
    local flags1 = unit.flags1
    local rv = 1

    if unit.mood == df.mood_type.Berserk
    or dfhack.units.isOpposedToLife(unit)
    or dfhack.units.isCrazed(unit)
    then
        rv = rv + 1
    else
        if dfhack.units.isCitizen(unit) then
            return -30
        elseif flags1.diplomat or flags1.merchant or flags1.forest then
            return -5
        elseif flags1.tame and unit.civ_id == df.global.ui.civ_id then
            return -1
        end
    end

    if flags1.marauder then rv = rv + 0.5 end
    if flags1.active_invader then rv = rv + 1 end
    if flags1.invader_origin then rv = rv + 1 end
    if flags1.invades then rv = rv + 1 end
    if flags1.hidden_ambusher then rv = rv + 1 end

    if unit.counters.unconscious > 0 then
        rv = rv * 0.3
    elseif unit.job.hunt_target then
        rv = rv * 3
    elseif unit.job.destroy_target then
        rv = rv * 2
    elseif unit.relations.group_leader_id < 0 and not flags1.rider then
        rv = rv * 1.5
    end

    return rv
end

function getUnitWeight(unit)
    local base = getBaseUnitWeight(unit)
    return base * math.pow(0.7, recent_targets[unit.id] or 0)
end

function unitWeightCache()
    local cache = {}
    return cache, function(unit)
        local id = unit.id
        cache[id] = cache[id] or getUnitWeight(unit)
        return cache[id]
    end
end

function scoreTargets(engine, reachable)
    local ucache, get_weight = unitWeightCache()

    for _,tgt in ipairs(reachable) do
        tgt.score = get_weight(tgt.unit)
        if tgt.lmargin and tgt.lmargin < 3 then
            tgt.score = tgt.score * tgt.lmargin / 3
        end
        if tgt.rmargin and tgt.rmargin < 3 then
            tgt.score = tgt.score * tgt.rmargin / 3
        end
    end

    computeNearbyWeight(engine, reachable, ucache)

    for _,tgt in ipairs(reachable) do
        tgt.score = (tgt.score + tgt.nearby_weight*0.7) * math.pow(0.995, tgt.time/3)
    end

    table.sort(reachable, function(a,b)
        return a.score > b.score or (a.score == b.score and a.time < b.time)
    end)
end

function pickUniqueTargets(reachable)
    local unique = {}

    if #reachable > 0 then
        local pos_table = {}
        local first_score = reachable[1].score

        for i,tgt in ipairs(reachable) do
            if tgt.score < 0 or tgt.score < 0.1*first_score then
                break
            end
            local x,y,z = pos2xyz(tgt.pos)
            local key = x..':'..y..':'..z
            if pos_table[key] then
                table.insert(pos_table[key].units, tgt.unit)
            else
                table.insert(unique, tgt)
                pos_table[key] = tgt
                tgt.units = { tgt.unit }
            end
        end
    end

    return unique
end

function describeUnit(unit)
    local desc = dfhack.units.getProfessionName(unit)
    local name = dfhack.units.getVisibleName(unit)
    if name.has_name then
        return desc .. ' ' .. dfhack.TranslateName(name)
    end
    return desc
end

function produceCombatReport(operator, item, target)
    local msg = describeUnit(operator) .. ' launches ' ..
                utils.getItemDescriptionPrefix(item) ..
                utils.getItemDescription(item) ..
                ' at '

    local pos = operator.pos

    if target then
        local units = target.units

        for i,v in ipairs(units) do
            if i > 1 then
                if i < #units then
                    msg = msg .. ', '
                elseif i > 2 then
                    msg = msg .. ', and '
                else
                    msg = msg .. ' and '
                end
            end
            msg = msg .. describeUnit(v)
        end

        msg = msg .. '!'
        pos = target.pos
    else
        msg = msg .. 'the target area.'
    end

    local id = dfhack.gui.makeAnnouncement(
        df.announcement_type.COMBAT_STRIKE_DETAILS,
        ANNOUNCEMENT_FLAGS, pos, msg, COLOR_CYAN, true
    )

    dfhack.gui.addCombatReport(operator, df.unit_report_type.Hunting, id)

    if target then
        for i,v in ipairs(target.units) do
            dfhack.gui.addCombatReportAuto(v, ANNOUNCEMENT_FLAGS, id)
        end
    end
end

function doAimProjectile(engine, item, target_min, target_max, operator, skill)
    --print(item, df.skill_rating[skill])

    local targets = proposeUnitHits(engine)
    local reachable = findReachableTargets(engine, targets)
    scoreTargets(engine, reachable)
    local unique = pickUniqueTargets(reachable)

    if #unique > 0 then
        local cnt = math.max(math.min(#unique,5), math.min(10, math.floor(#unique/2)))
        local rnd = math.random(cnt)
        local target = unique[rnd]
        for _,u in ipairs(target.units) do
            saveRecent(u)
        end
        produceCombatReport(operator, item, target)
        return target.path
    end

    produceCombatReport(operator, item, nil)
end

return _ENV