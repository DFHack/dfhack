function set_test_stage(stage)
    local f = io.open('test_stage.txt', 'w')
    f:write(stage)
    f:close()
end

print('running tests')

set_test_stage('done')
dfhack.run_command('die')
