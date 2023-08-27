-- Fixes instruments that never got played during a performance

local argparse = require('argparse')

function fixInstruments(opts)
    local fixed = 0

    for _, item in ipairs(df.global.world.items.other.INSTRUMENT) do
        for i, ref in pairs(item.general_refs) do
            if ref:getType() == df.general_ref_type.ACTIVITY_EVENT then
                local activity = df.activity_entry.find(ref.activity_id)
                if not activity then
                    print(('Found stuck instrument: %s'):format(dfhack.items.getDescription(item, 0, true)))
                    if not opts.dry_run then
                        --remove dead activity reference
                        item.general_refs[i]:delete()
                        item.general_refs:erase(i)
                        item.flags.in_job = false
                    end
                    fixed = fixed + 1
                    break
                end
            end
        end
    end

    if fixed > 0 or opts.dry_run then
        print(("%s %d stuck instrument(s)."):format(
                opts.dry_run and "Found" or "Fixed",
                fixed
        ))
    end
end

local opts = {}

local positionals = argparse.processArgsGetopt({...}, {
    { 'h', 'help', handler = function() opts.help = true end },
    { 'n', 'dry-run', handler = function() opts.dry_run = true end },
})

if positionals[1] == 'help' or opts.help then
    print(dfhack.script_help())
    return
end

fixInstruments(opts)
