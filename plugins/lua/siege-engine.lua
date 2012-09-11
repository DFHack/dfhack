local _ENV = mkmodule('plugins.siege-engine')

--[[

 Native functions:

 * getTargetArea(building) -> point1, point2
 * clearTargetArea(building)
 * setTargetArea(building, point1, point2) -> true/false

--]]

Z_STEP_COUNT = 15
Z_STEP = 1/31

function findShotHeight(engine, target)
    local path = { target = target, delta = 0.0 }

    if projPathMetrics(engine, path).goal_step then
        return path
    end

    for i = 1,Z_STEP_COUNT do
        path.delta = i*Z_STEP
        if projPathMetrics(engine, path).goal_step then
            return path
        end

        path.delta = -i*Z_STEP
        if projPathMetrics(engine, path).goal_step then
            return path
        end
    end
end

function doAimProjectile(engine, target_min, target_max)
    local targets = proposeUnitHits(engine)
    if #targets > 0 then
        local rnd = math.random(#targets)
        return findShotHeight(engine, targets[rnd].pos)
    end
end

return _ENV