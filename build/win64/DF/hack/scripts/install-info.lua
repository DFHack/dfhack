-- Log installation info to install-info.txt

--[====[

install-info
============

Saves information about the current DFHack installation to ``install-info.txt``
in the current DF folder. Useful for bug reports.

]====]

local utils = require 'utils'

local f = io.open(dfhack.getDFPath() .. '/install-info.txt', 'w')
function log(...)
    local text = table.concat({...}, '')
    if not f then
        print(text)
    else
        f:write(text .. '\n')
    end
end

log('DFHack ', dfhack.getDFHackVersion(), ' on ', dfhack.getOSType(),
    '/', dfhack.getArchitectureName())

log('\nVersion information:')
function log_version(text, func_name)
    log('    ', text, ': ', tostring(dfhack[func_name]()))
end
log_version('DF version', 'getDFVersion')
log_version('DFHack version', 'getDFHackVersion')
log_version('DFHack release', 'getDFHackRelease')
log_version('DFHack build ID', 'getDFHackBuildID')
log_version('Compiled DF version', 'getCompiledDFVersion')
log_version('Git description', 'getGitDescription')
log_version('Git commit', 'getGitCommit')
log_version('Git XML commit', 'getGitXmlCommit')
log_version('Git XML expected commit', 'getGitXmlExpectedCommit')
log_version('Git XML match', 'gitXmlMatch')
log_version('Is release', 'isRelease')
log_version('Is prerelease', 'isPrerelease')
log_version('Is world loaded', 'isWorldLoaded')
log_version('Is map loaded', 'isMapLoaded')

function log_command(cmd)
    log('\nOutput of "' .. cmd .. '":')
    local output = select(1, dfhack.run_command_silent(cmd))
    for _, line in pairs(output:split('\n')) do
        log('    ' .. line)
    end
end

log_command('devel/save-version')
log_command('plug')

log('Tweak log entries:')
if not pcall(function()
    for line in io.lines('stderr.log') do
        if line:match('tweak') then
            log('    ' .. line)
        end
    end
end) then
    log('Failed to read stderr.log')
end

if not f then
    qerror('Could not write to install-info.txt.\nCopy the above text instead.')
else
    f:close()
    print('Saved to:\n' .. dfhack.getDFPath() .. '/install-info.txt')
end
