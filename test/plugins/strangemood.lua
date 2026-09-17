config.mode = 'fortress'
config.target = 'strangemood'

-- note: the --force/--type/--skill happy path is deliberately not tested
-- here: inducing a real strange mood mutates the fort (dwarf claims a
-- workshop and can go berserk if unmet), which is too disruptive mid-suite.

function test.help_is_wrong_usage()
    local _, status = dfhack.run_command_silent('strangemood', 'help')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.unknown_option_is_wrong_usage()
    local output, status = dfhack.run_command_silent('strangemood', '--bogus')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('Unrecognized parameter', output)
end

function test.id_missing_value()
    local output, status = dfhack.run_command_silent('strangemood', '--id')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('No unit id specified', output)
end

function test.id_not_a_number()
    -- regression: the raw std::stoi call threw an uncaught exception on
    -- non-numeric input, crashing the game
    local output, status = dfhack.run_command_silent('strangemood', '--id', 'abc')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('Invalid unit id', output)
end

function test.id_nonexistent_unit()
    local _, status = dfhack.run_command_silent('strangemood', '--id', '99999999')
    expect.eq(CR_FAILURE, status)
end

function test.type_missing_value()
    local _, status = dfhack.run_command_silent('strangemood', '--type')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.type_bad_value()
    local output, status = dfhack.run_command_silent('strangemood', '--type', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('not recognized', output)
end

-- the following tests trigger real strange moods and validate the generated
-- job_items against the vanilla request tables. they restore all unit and
-- plotinfo state they touch. the mooded dwarf cannot act on the job because
-- the whole test runs atomically on the main thread.

-- skill name -> expected item_type of the mood's base item
local BASE_ITEMS = {
    miner='BOULDER', engraver='BOULDER', mason='BOULDER',
    stonecutter='BOULDER', stonecarver='BOULDER', mechanic='BOULDER',
    carpenter='WOOD', woodcrafter='WOOD', bowyer='WOOD',
    tanner='SKIN_TANNED', leatherworker='SKIN_TANNED',
    weaver='CLOTH', clothier='CLOTH',
    weaponsmith='BAR', armorsmith='BAR', metalsmith='BAR',
    metalcrafter='BAR',
    gemcutter='ROUGH', gemsetter='ROUGH', glassmaker='ROUGH',
    bonecarver='NONE',
}

-- every item_type a fey/secretive/possessed mood may ever request
-- (bars/wafers and cloth are the only dimensional demands; thread is
-- never requested)
local ALLOWED_ITEM_TYPES = {
    BOULDER=true, WOOD=true, SKIN_TANNED=true, CLOTH=true, BAR=true,
    ROUGH=true, SMALLGEM=true, BLOCKS=true, NONE=true,
}

-- mirrors isUnitMoodable in strangemood.cpp, plus "not already mid-job"
local function pick_moodable_unit()
    for _, u in ipairs(dfhack.units.getCitizens()) do
        if not u.flags1.had_mood and not u.flags1.has_mood and
                u.mood == df.mood_type.None and not u.job.current_job and
                u.status2.limbs_grasp_count > 0 and
                df.profession.attrs[u.profession].moodable then
            return u
        end
    end
end

local function clear_mood(unit)
    local job = unit.job.current_job
    unit.job.current_job = nil
    if job then
        dfhack.job.removeJob(job)
    end
    unit.mood = df.mood_type.None
    unit.mood_copy = df.mood_type.None
    unit.flags1.has_mood = false
    unit.flags1.had_mood = false
    unit.moodstage = df.mood_stage_type.INITIAL
    unit.status.artifact_name.type = -1
end

local function trigger_mood(unit, ...)
    local _, status = dfhack.run_command_silent('strangemood',
        '--force', '--id', tostring(unit.id), ...)
    return status == CR_OK and unit.job.current_job or nil
end

local function check_dimension(item, expected_type)
    -- strangemood pre-multiplies quantity by the item's dimension and sets
    -- min_dimension, working around a vanilla bug
    if item.item_type == df.item_type.BAR then
        expect.eq(150, item.min_dimension)
        expect.eq(0, item.quantity % 150)
    elseif item.item_type == df.item_type.CLOTH then
        expect.eq(10000, item.min_dimension)
        expect.eq(0, item.quantity % 10000)
    else
        expect.eq(-1, item.min_dimension)
    end
end

function test.base_items_match_skill()
    local unit = pick_moodable_unit()
    expect.true_(unit, 'need a moodable citizen without a job')
    local saved_cooldown = df.global.plotinfo.mood_cooldown
    return dfhack.with_finalize(function()
        clear_mood(unit)
        df.global.plotinfo.mood_cooldown = saved_cooldown
    end, function()
        for skill, expected in pairs(BASE_ITEMS) do
            local job = trigger_mood(unit, '--skill', skill)
            expect.true_(job, 'mood job for ' .. skill)
            local base = job.job_items.elements[0]
            expect.eq(expected, df.item_type[base.item_type],
                'base item for ' .. skill)
            check_dimension(base)
            for _, item in ipairs(job.job_items.elements) do
                expect.ne(df.item_type.THREAD, item.item_type)
                expect.true_(ALLOWED_ITEM_TYPES[df.item_type[item.item_type]],
                    ('unexpected %s item for %s'):format(
                        df.item_type[item.item_type], skill))
                if item.item_type == df.item_type.NONE then
                    expect.true_(item.flags2.body_part)
                end
                check_dimension(item)
            end
            clear_mood(unit)
        end
    end)
end

function test.fell_and_macabre_base_items()
    local unit = pick_moodable_unit()
    expect.true_(unit, 'need a moodable citizen without a job')
    local saved_cooldown = df.global.plotinfo.mood_cooldown
    return dfhack.with_finalize(function()
        clear_mood(unit)
        df.global.plotinfo.mood_cooldown = saved_cooldown
    end, function()
        local job = trigger_mood(unit, '--type', 'fell')
        expect.true_(job, 'fell mood job')
        expect.eq(df.job_type.StrangeMoodFell, job.job_type)
        expect.eq(df.item_type.CORPSE, job.job_items.elements[0].item_type)
        expect.true_(job.job_items.elements[0].flags1.murdered)
        clear_mood(unit)

        job = trigger_mood(unit, '--type', 'macabre')
        expect.true_(job, 'macabre mood job')
        expect.eq(df.job_type.StrangeMoodBrooding, job.job_type)
        local base = job.job_items.elements[0]
        if base.item_type == df.item_type.NONE then
            expect.true_(base.flags2.body_part)
            expect.true_(base.flags2.bone or base.flags2.totemable)
        else
            expect.eq(df.item_type.REMAINS, base.item_type)
        end
    end)
end
