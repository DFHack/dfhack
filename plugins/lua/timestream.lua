local _ENV = mkmodule('plugins.timestream')

function migrate_old_config()
    local GLOBAL_KEY = 'timestream'
    local old_config = dfhack.persistent.getSiteData(GLOBAL_KEY)
    if not old_config then return end
    if old_config.enabled then dfhack.run_command('enable', GLOBAL_KEY) end
    if old_config.settings and type(old_config.settings) == 'table' and tonumber(old_config.settings.fps) then
        timestream_setFps(tonumber(old_config.settings.fps))
    end
end

--[[

------------------------------------
-- business logic

local function increment_counter(obj, counter_name, timeskip)
    if obj[counter_name] <= 0 then return end
    obj[counter_name] = obj[counter_name] + timeskip
end

local function decrement_counter(obj, counter_name, timeskip)
    if obj[counter_name] <= 0 then return end
    obj[counter_name] = math.max(1, obj[counter_name] - timeskip)
end

local function adjust_unit_counters(unit, timeskip)
    local c1 = unit.counters
    decrement_counter(c1, 'think_counter', timeskip)
    decrement_counter(c1, 'job_counter', timeskip)
    decrement_counter(c1, 'swap_counter', timeskip)
    decrement_counter(c1, 'winded', timeskip)
    decrement_counter(c1, 'stunned', timeskip)
    decrement_counter(c1, 'unconscious', timeskip)
    decrement_counter(c1, 'suffocation', timeskip)
    decrement_counter(c1, 'webbed', timeskip)
    decrement_counter(c1, 'soldier_mood_countdown', timeskip)
    decrement_counter(c1, 'pain', timeskip)
    decrement_counter(c1, 'nausea', timeskip)
    decrement_counter(c1, 'dizziness', timeskip)
    local c2 = unit.counters2
    decrement_counter(c2, 'paralysis', timeskip)
    decrement_counter(c2, 'numbness', timeskip)
    decrement_counter(c2, 'fever', timeskip)
    decrement_counter(c2, 'exhaustion', timeskip * 3)
    increment_counter(c2, 'hunger_timer', timeskip)
    increment_counter(c2, 'thirst_timer', timeskip)
    local job = unit.job.current_job
    if job and job.job_type == df.job_type.Rest then
        decrement_counter(c2, 'sleepiness_timer', timeskip * 200)
    elseif job and job.job_type == df.job_type.Sleep then
        decrement_counter(c2, 'sleepiness_timer', timeskip * 19)
    else
        increment_counter(c2, 'sleepiness_timer', timeskip)
    end
    decrement_counter(c2, 'stomach_content', timeskip * 5)
    decrement_counter(c2, 'stomach_food', timeskip * 5)
    decrement_counter(c2, 'vomit_timeout', timeskip)
    -- stored_fat wanders about based on other state; we can likely leave it alone and
    -- not materially affect gameplay
end

-- need to manually adjust job completion_timer values for jobs that are controlled by unit actions
-- with a timer of 1, which are destroyed immediately after they are created. longer-lived unit
-- actions are already sufficiently handled by dfhack.units.subtractGroupActionTimers().
-- this will also decrement timers for jobs with actions that have just expired, but on average, this
-- should balance out to be correct, since we're losing time when we subtract from the action timers
-- and cap the value so it never drops below 1.
local function adjust_job_counter(unit, timeskip)
    local job = unit.job.current_job
    if not job then return end
    for _,action in ipairs(unit.actions) do
        if action.type == df.unit_action_type.Job or action.type == df.unit_action_type.JobRecover then
            return
        end
    end
    decrement_counter(job, 'completion_timer', timeskip)
end

-- unit needs appear to be incremented on season ticks, so we don't need to worry about those
-- since the TICK_TRIGGERS check makes sure that we never skip season ticks
local function adjust_units(timeskip)
    for _, unit in ipairs(df.global.world.units.active) do
        if not dfhack.units.isActive(unit) then goto continue end
        decrement_counter(unit, 'pregnancy_timer', timeskip)
        dfhack.units.subtractGroupActionTimers(unit, timeskip, df.unit_action_type_group.All)
        if not dfhack.units.isOwnGroup(unit) then goto continue end
        adjust_unit_counters(unit, timeskip)
        adjust_job_counter(unit, timeskip)
        ::continue::
    end
end

-- behavior ascertained from in-game observation
local function adjust_activities(timeskip)
    for i, act in ipairs(df.global.world.activities.all) do
        for _, ev in ipairs(act.events) do
            if df.activity_event_training_sessionst:is_instance(ev) then
                -- no counters
            elseif df.activity_event_combat_trainingst:is_instance(ev) then
                -- has organize_counter at a non-zero value, but it doesn't seem to move
            elseif df.activity_event_skill_demonstrationst:is_instance(ev) then
                -- can be negative or positive, but always counts towards 0
                if ev.organize_counter < 0 then
                    ev.organize_counter = math.min(-1, ev.organize_counter + timeskip)
                else
                    decrement_counter(ev, 'organize_counter', timeskip)
                end
                decrement_counter(ev, 'train_countdown', timeskip)
            elseif df.activity_event_fill_service_orderst:is_instance(ev) then
                -- no counters
            elseif df.activity_event_individual_skill_drillst:is_instance(ev) then
                -- only counts down on season ticks, nothing to do here
            elseif df.activity_event_sparringst:is_instance(ev) then
                decrement_counter(ev, 'countdown', timeskip * 2)
            elseif df.activity_event_ranged_practicest:is_instance(ev) then
                -- countdown appears to never move from 0
                decrement_counter(ev, 'countdown', timeskip)
            elseif df.activity_event_harassmentst:is_instance(ev) then
                if DEBUG >= 1 then
                    print('activity_event_harassmentst ready for analysis at index', i)
                end
            elseif df.activity_event_encounterst:is_instance(ev) then
                if DEBUG >= 1 then
                    print('activity_event_encounterst ready for analysis at index', i)
                end
            elseif df.activity_event_reunionst:is_instance(ev) then
                if DEBUG >= 1 then
                    print('activity_event_reunionst ready for analysis at index', i)
                end
            elseif df.activity_event_conversationst:is_instance(ev) then
                increment_counter(ev, 'pause', timeskip)
            elseif df.activity_event_guardst:is_instance(ev) then
                -- no counters
            elseif df.activity_event_conflictst:is_instance(ev) then
                increment_counter(ev, 'inactivity_timer', timeskip)
                increment_counter(ev, 'attack_inactivity_timer', timeskip)
                increment_counter(ev, 'stop_fort_fights_timer', timeskip)
            elseif df.activity_event_prayerst:is_instance(ev) then
                decrement_counter(ev, 'timer', timeskip)
            elseif df.activity_event_researchst:is_instance(ev) then
                -- no counters
            elseif df.activity_event_playst:is_instance(ev) then
                increment_counter(ev, 'down_time_counter', timeskip)
            elseif df.activity_event_worshipst:is_instance(ev) then
                increment_counter(ev, 'down_time_counter', timeskip)
            elseif df.activity_event_socializest:is_instance(ev) then
                increment_counter(ev, 'down_time_counter', timeskip)
            elseif df.activity_event_ponder_topicst:is_instance(ev) then
                decrement_counter(ev, 'timer', timeskip)
            elseif df.activity_event_discuss_topicst:is_instance(ev) then
                decrement_counter(ev, 'timer', timeskip)
            elseif df.activity_event_teach_topicst:is_instance(ev) then
                decrement_counter(ev, 'time_left', timeskip)
            elseif df.activity_event_readst:is_instance(ev) then
                decrement_counter(ev, 'timer', timeskip)
            elseif df.activity_event_writest:is_instance(ev) then
                decrement_counter(ev, 'timer', timeskip)
            elseif df.activity_event_copy_written_contentst:is_instance(ev) then
                decrement_counter(ev, 'timer', timeskip)
            elseif df.activity_event_make_believest:is_instance(ev) then
                decrement_counter(ev, 'time_left', timeskip)
            elseif df.activity_event_play_with_toyst:is_instance(ev) then
                decrement_counter(ev, 'time_left', timeskip)
            elseif df.activity_event_performancest:is_instance(ev) then
                increment_counter(ev, 'current_position', timeskip)
            elseif df.activity_event_store_objectst:is_instance(ev) then
                if DEBUG >= 1 then
                    print('activity_event_store_objectst ready for analysis at index', i)
                end
            end
        end
    end
end

]]

local function do_set(setting_name, arg)
    local numarg = tonumber(arg)
    if setting_name ~= 'fps' or not numarg then
        qerror('must specify setting and value')
    end
    timestream_setFps(arg)
    print(('set %s to %s'):format(setting_name, timestream_getFps()))
end

local function do_reset()
    timestream_resetSettings()
end

local function print_status()
    print(GLOBAL_KEY .. ' is ' .. (state.enabled and 'enabled' or 'not enabled'))
    print()
    print('settings:')
    for _,v in ipairs(SETTINGS) do
        print(('  %15s: %s'):format(v.name, state.settings[v.internal_name or v.name]))
    end
    if DEBUG < 2 then return end
    print()
    print(('cur_year_tick:    %d'):format(df.global.cur_year_tick))
    print(('timeskip_deficit: %.2f'):format(timeskip_deficit))
    if DEBUG < 3 then return end
    print()
    print('tick coverage:')
    for coverage_slot=0,49 do
        print(('  slot %2d: %scovered'):format(coverage_slot, tick_coverage[coverage_slot] and '' or 'NOT '))
    end
    print()
    local bdays, bdays_list = {}, {}
    for _, next_bday in pairs(birthday_triggers) do
        if not bdays[next_bday] then
            bdays[next_bday] = true
            table.insert(bdays_list, next_bday)
        end
    end
    print(('%d birthdays:'):format(#bdays_list))
    table.sort(bdays_list)
    for _,bday in ipairs(bdays_list) do
        print(('  year tick: %d'):format(bday))
    end
end

function parse_commandline(args)
    local command = table.remove(args, 1)

    if command == 'help' then
        return false
    elseif command == 'set' then
        do_set(args[1], args[2])
    elseif command == 'reset' then
        do_reset()
    elseif not command or command == 'status' then
        print_status()
    else
        return false
    end

    return true
end

return _ENV
