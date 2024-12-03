local utils = require('utils')

config.target = 'core'
config.mode = 'fortress'

-- EventManager job completion handling expects sorted order
function test.jobIDsAreSorted()
    local is_sorted = true
    local prev_id = nil

    for _, job in utils.listpairs(df.global.world.jobs.list) do
        if prev_id and job.id < prev_id then
            is_sorted = false
            break
        end
        prev_id = job.id
    end

    expect.true_(is_sorted)
end
