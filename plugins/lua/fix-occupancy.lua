local _ENV = mkmodule('plugins.fix-occupancy')

local argparse = require('argparse')

function parse_commandline(args)
    local opts = {
        dry_run=false,
    }

    local positionals = argparse.processArgsGetopt(args, {
        { 'h', 'help', handler = function() opts.help = true end },
        { 'n', 'dry-run', handler = function() opts.dry_run = true end },
    })

    if positionals[1] == 'help' or opts.help then
        return false
    end

    if not positionals[1] then
        fix_map(opts.dry_run)
    else
        fix_tile(argparse.coords(positionals[1], 'pos'), opts.dry_run)
    end
end

return _ENV
