-- Kills the specified historical figure

--[====[

devel/kill-hf
=============

Kills the specified historical figure, even if off-site, or terminates a
pregnancy. Useful for working around :bug:`11549`.

Usage::

    devel/kill-hf [-p|--pregnancy] [-n|--dry-run] HISTFIG_ID

Arguments:

``histfig_id``:
    the ID of the historical figure to target

``-p``, ``--pregnancy``:
    if specified, and if the historical figure is pregnant, terminate the
    pregnancy instead of killing the historical figure

``-n``, ``--dry-run``:
    if specified, only print the name of the historical figure

]====]

local target_hf = -1
local target_pregnancy = false
local dry_run = false

for _, arg in ipairs({...}) do
    if arg == '-p' or arg == '--pregnancy' then
        target_pregnancy = true
    elseif arg == '-n' or arg == '--dry-run' then
        dry_run = true
    elseif tonumber(arg) and target_hf == -1 then
        target_hf = tonumber(arg)
    else
        qerror('unrecognized argument: ' .. arg)
    end
end

local hf = df.historical_figure.find(target_hf)
    or qerror('histfig not found: ' .. target_hf)
local hf_name = dfhack.df2console(dfhack.TranslateName(hf.name))
local hf_desc = ('%i: %s (%s)'):format(target_hf, hf_name, dfhack.units.getRaceNameById(hf.race))

if dry_run then
    print('Would target histfig ' .. hf_desc)
    return
end

if target_pregnancy then
    hf.info.wounds.childbirth_year = -1
    hf.info.wounds.childbirth_tick = -1
    print('Terminated pregnancy of histfig ' .. hf_desc)
else
    hf.old_year = df.global.cur_year
    hf.old_seconds = df.global.cur_year_tick + 1
    hf.died_year = df.global.cur_year
    hf.died_seconds = df.global.cur_year_tick + 1
    print('Killed histfig ' .. hf_desc)
end
