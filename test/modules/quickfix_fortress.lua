config.mode = 'fortress'
config.target = 'core'

local quickfix = require('quickfix')

function test.repair_site_id_restores_fortress_site()
    local plotinfo = df.global.plotinfo
    local orig = plotinfo.site_id
    local site = plotinfo.main.fortress_site
    expect.ne(nil, site, 'no fortress site on this map')
    if not site then return end

    return dfhack.with_finalize(function()
        plotinfo.site_id = orig
    end, function()
        plotinfo.site_id = -1
        quickfix.repair_site_id()
        expect.eq(site.id, plotinfo.site_id)
    end)
end

function test.repair_site_id_noop_when_assigned()
    local orig = df.global.plotinfo.site_id
    quickfix.repair_site_id()
    expect.eq(orig, df.global.plotinfo.site_id)
end
