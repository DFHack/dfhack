local workflow = require('plugins.workflow')

function test.job_outputs()
    for job_type in pairs(workflow.test_data.job_outputs) do
        expect.true_(df.job_type[job_type], "Found unrecognized job type: " .. tostring(job_type))
    end
end
