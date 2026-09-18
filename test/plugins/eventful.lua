config.mode = 'fortress'
config.target = 'eventful'

local eventful = require('plugins.eventful')

local function cleanup()
    eventful._registeredStuff = {}
    eventful.onReactionCompleting._library = nil
    eventful.onReactionComplete._library = nil
    eventful.onWorkshopFillSidebarMenu._library = nil
    eventful.postWorkshopFillSidebarMenu._library = nil
    dfhack.onStateChange.eventful = nil
end

config.wrapper = function(test_fn)
    cleanup()
    return dfhack.with_finalize(cleanup, test_fn)
end

function test.eventType_table()
    expect.eq(0, eventful.eventType.TICK)
    expect.eq(3, eventful.eventType.JOB_COMPLETED)
    expect.eq(16, eventful.eventType.EVENT_MAX)
end

function test.registerReaction()
    local cb = function() end
    eventful.registerReaction('TEST_REACTION', cb)
    expect.eq(cb, eventful._registeredStuff.reactionCallbacks.TEST_REACTION)
    expect.ne(nil, eventful.onReactionCompleting._library)
    expect.ne(nil, dfhack.onStateChange.eventful)
end

function test.reaction_callback_dispatches()
    local got
    eventful.registerReaction('TEST_REACTION',
        function(reaction) got = reaction.code end)
    eventful.onReactionCompleting._library({code='TEST_REACTION'})
    expect.eq('TEST_REACTION', got)
end

function test.reaction_callback_ignores_unregistered()
    local called = false
    eventful.registerReaction('TEST_REACTION', function() called = true end)
    eventful.onReactionCompleting._library({code='OTHER_REACTION'})
    expect.false_(called)
end

function test.registerSidebar()
    local cb = function() end
    eventful.registerSidebar('CARPENTERS', cb)
    expect.eq(cb, eventful._registeredStuff.customSidebar.CARPENTERS)
    expect.ne(nil, eventful.onWorkshopFillSidebarMenu._library)
end

function test.addReactionToShop()
    eventful.addReactionToShop('TEST_REACTION', 'MASONS')
    expect.table_eq({'TEST_REACTION'},
        eventful._registeredStuff.reactionToShop.MASONS)
    expect.ne(nil, eventful.postWorkshopFillSidebarMenu._library)
end

function test.removeNative()
    eventful.removeNative('STILL', 'COOK_FOOD')
    expect.table_eq({'COOK_FOOD'},
        eventful._registeredStuff.shopNonNative.STILL)
    eventful.removeNative('STILL')
    expect.true_(eventful._registeredStuff.shopNonNative.STILL.all)
end

function test.world_unload_clears_registration()
    eventful.registerReaction('TEST_REACTION', function() end)
    eventful.registerSidebar('CARPENTERS', function() end)
    eventful.addReactionToShop('TEST_REACTION', 'MASONS')
    eventful.removeNative('STILL')
    dfhack.onStateChange.eventful(SC_WORLD_UNLOADED)
    expect.table_eq({}, eventful._registeredStuff)
    expect.nil_(eventful.onReactionCompleting._library)
    expect.nil_(eventful.onWorkshopFillSidebarMenu._library)
    expect.nil_(eventful.postWorkshopFillSidebarMenu._library)
    expect.nil_(dfhack.onStateChange.eventful)
end
