config.target = 'core'
config.mode = 'title' -- alters world state, not safe when a world is loaded

function test.removeJob()
    -- removeJob() calls DF code, so ensure that that DF code is actually running

    -- for an explanation of why this is necessary to check,
    -- see https://github.com/DFHack/dfhack/pull/3713 and Job.cpp:removeJob()

    expect.nil_(df.global.world.jobs.list.next, 'job list is not empty')

    local job = df.job:new()  -- will be deleted by removeJob() if the test passes
    dfhack.job.linkIntoWorld(job)
    expect.true_(df.global.world.jobs.list.next, 'job list is empty')
    expect.eq(df.global.world.jobs.list.next.item, job, 'expected job not found in list')

    expect.true_(dfhack.job.removeJob(job))
    expect.nil_(df.global.world.jobs.list.next, 'job list is not empty after removeJob()')
end
