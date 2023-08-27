-- basic check for release readiness
--[====[
devel/check-release
===================
Basic checks for release readiness
]====]

local ok = true
function err(s)
    dfhack.printerr(s)
    ok = false
end
function warn(s)
    dfhack.color(COLOR_YELLOW)
    dfhack.print(s .. '\n')
    dfhack.color(nil)
end

local dfhack_ver = dfhack.getDFHackVersion()
local git_desc = dfhack.getGitDescription()
local git_commit = dfhack.getGitCommit()
if not dfhack.isRelease() then
    err('This build is not tagged as a release')
    print[[
This is probably due to missing git tags.
Try running `git fetch origin --tags` in the DFHack source tree.
]]
end

local expected_git_desc = ('%s-0-g%s'):format(dfhack_ver, git_commit:sub(1, 8))
if git_desc:sub(1, #expected_git_desc) ~= expected_git_desc then
    err(([[Bad git description:
Expected %s, got %s]]):format(expected_git_desc, git_desc))
    print[[
Ensure that the DFHack source tree is up-to-date (`git pull`) and CMake is
installing DFHack to this DF folder.
]]
end

if not dfhack.gitXmlMatch() then
    err('library/xml submodule commit does not match tracked commit\n' ..
        ('Expected %s, got %s'):format(
            dfhack.getGitXmlExpectedCommit():sub(1, 7),
            dfhack.getGitXmlCommit():sub(1, 7)
        ))
    print('Try running `git submodule update` in the DFHack source tree.')
end

if dfhack.isPrerelease() then
    warn('This build is marked as a prerelease.')
    print('If this is not intentional, be sure your DFHack tree is up-to-date\n' ..
        '(`git pull`) and try again.')
end

if not ok then
    err('This build is not release-ready!')
else
    print('Release checks succeeded')
end
