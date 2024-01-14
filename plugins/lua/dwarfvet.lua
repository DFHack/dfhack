local _ENV = mkmodule('plugins.dwarfvet')

local argparse = require('argparse')
local utils = require('utils')

local function is_valid_animal(unit)
    return unit and
        dfhack.units.isActive(unit) and
        dfhack.units.isAnimal(unit) and
        dfhack.units.isFortControlled(unit) and
        dfhack.units.isTame(unit) and
        not dfhack.units.isDead(unit)
end

local function get_cur_patients()
    local cur_patients = {}
    for _,job in utils.listpairs(df.global.world.jobs.list) do
        if job.job_type ~= df.job_type.Rest then goto continue end
        local unit = dfhack.job.getWorker(job)
        if is_valid_animal(unit) then
            cur_patients[unit] = true
        end
        ::continue::
    end
    return cur_patients
end

local function get_new_patients(cur_patients)
    cur_patients = cur_patients or get_cur_patients()
    local new_patients = {}
    for _,unit in ipairs(df.global.world.units.active) do
        if unit.job.current_job then goto continue end
        if cur_patients[unit] or not is_valid_animal(unit) then goto continue end
        if not unit.health or not unit.health.flags.needs_healthcare then goto continue end
        table.insert(new_patients, unit)
        ::continue::
    end
    return new_patients
end

local function print_status()
    print(('dwarfvet is %srunning'):format(isEnabled() and '' or 'not '))
    print()
    print('The following animals are receiving treatment:')
    local cur_patients = get_cur_patients()
    if not next(cur_patients) then
        print('  None')
    else
        for unit in pairs(cur_patients) do
            print(('  %s (%d)'):format(dfhack.units.getReadableName(unit), unit.id))
        end
    end
    print()
    print('The following animals are injured and need treatment:')
    local new_patients = get_new_patients(cur_patients)
    if #new_patients == 0 then
        print('  None')
    else
        for _,unit in ipairs(new_patients) do
            print(('  %s (%d)'):format(dfhack.units.getReadableName(unit), unit.id))
        end
    end
end

HospitalZone = defclass(HospitalZone)
HospitalZone.ATTRS{
    zone=DEFAULT_NIL,
}

local ONE_TILE = xy2pos(1, 1)

function HospitalZone:find_spot(unit_pos)
    self.x = self.x or self.zone.x1
    self.y = self.y or self.zone.y1
    local zone = self.zone
    for y=self.y,zone.y2 do
        for x=self.x,zone.x2 do
            local pos = xyz2pos(x, y, zone.z)
            if dfhack.maps.canWalkBetween(unit_pos, pos) and
                dfhack.buildings.containsTile(zone, x, y) and
                dfhack.buildings.checkFreeTiles(pos, ONE_TILE)
            then
                return pos
            end
        end
    end
end

-- TODO: If health.requires_recovery is set, the creature can't move under its own power
--   and a Recover Wounded or Pen/Pasture job must be created by hand
function HospitalZone:assign_spot(unit, unit_pos)
    local pos = self:find_spot(unit_pos)
    if not pos then return false end
    local job = df.new(df.job)
    dfhack.job.linkIntoWorld(job, true)
    job.pos.x = pos.x
    job.pos.y = pos.y
    job.pos.z = pos.z
    job.flags.special = true
    job.job_type = df.job_type.Rest
    local gref = df.new(df.general_ref_unit_workerst)
    gref.unit_id = unit.id
    job.general_refs:insert('#', gref)
    unit.job.current_job = job
    return true
end

local function get_hospital_zones()
    local hospital_zones = {}
    local site = dfhack.world.getCurrentSite() or {buildings={}}
    for _,location in ipairs(site.buildings) do
        if not df.abstract_building_hospitalst:is_instance(location) then goto continue end
        for _,bld_id in ipairs(location.contents.building_ids) do
            local zone = df.building.find(bld_id)
            if zone then
                table.insert(hospital_zones, HospitalZone{zone=zone})
            end
        end
        ::continue::
    end
    return hospital_zones
end

local function distance(zone, pos)
    return math.abs(zone.x1 - pos.x) + math.abs(zone.y1 - pos.y) + 50*math.abs(zone.z - pos.z)
end

function checkup()
    local new_patients = get_new_patients()
    if #new_patients == 0 then return end
    local hospital_zones = get_hospital_zones()
    local assigned = 0
    for _,unit in ipairs(new_patients) do
        local unit_pos = xyz2pos(dfhack.units.getPosition(unit))
        table.sort(hospital_zones,
            function(a, b) return distance(a.zone, unit_pos) < distance(b.zone, unit_pos) end)
        for _,hospital_zone in ipairs(hospital_zones) do
            if hospital_zone:assign_spot(unit, unit_pos) then
                assigned = assigned + 1
                break
            end
        end
    end
    print(('dwarfvet scheduled treatment for %d of %d injured animals'):format(assigned, #new_patients))
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
end

function parse_commandline(args)
    local opts = {}
    local positionals = process_args(opts, args)

    if opts.help or not positionals then
        return false
    end

    local command = positionals[1]
    if not command or command == 'status' then
        print_status()
    elseif command == 'now' then
        dwarfvet_cycle()
    else
        return false
    end

    return true
end

return _ENV
