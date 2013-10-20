-- Support for scripted interaction sequences via coroutines.

local _ENV = mkmodule('gui.script')

local dlg = require('gui.dialogs')

--[[
  Example:

  start(function()
    sleep(100, 'frames')
    print(showYesNoPrompt('test', 'print true?'))
  end)
]]

-- Table of running background scripts.
if not scripts then
    scripts = {}
    setmetatable(scripts, { __mode = 'k' })
end

local function do_resume(inst, ...)
    inst.gen = inst.gen + 1
    return (dfhack.saferesume(inst.coro, ...))
end

-- Starts a new background script by calling the function.
function start(fn,...)
    local coro = coroutine.create(fn)
    local inst = {
        coro = coro,
        gen = 0,
    }
    scripts[coro] = inst
    return do_resume(inst, ...)
end

-- Checks if called from a background script
function in_script()
    return scripts[coroutine.running()] ~= nil
end

local function getinst()
    local inst = scripts[coroutine.running()]
    if not inst then
        error('Not in a gui script coroutine.')
    end
    return inst
end

local function invoke_resume(inst,gen,quiet,...)
    local state = coroutine.status(inst.coro)
    if state ~= 'suspended' then
        if state ~= 'dead' then
            dfhack.printerr(debug.traceback('resuming a non-waiting continuation'))
        end
    elseif inst.gen > gen then
        if not quiet then
            dfhack.printerr(debug.traceback('resuming an expired continuation'))
        end
    else
        do_resume(inst, ...)
    end
end

-- Returns a callback that resumes the script from wait with given return values
function mkresume(...)
    local inst = getinst()
    return curry(invoke_resume, inst, inst.gen, false, ...)
end

-- Like mkresume, but does not complain if already resumed from this wait
function qresume(...)
    local inst = getinst()
    return curry(invoke_resume, inst, inst.gen, true, ...)
end

-- Wait until a mkresume callback is called, then return its arguments.
-- Once it returns, all mkresume callbacks created before are invalidated.
function wait()
    getinst() -- check that the context is right
    return coroutine.yield()
end

-- Wraps dfhack.timeout for coroutines.
function sleep(time, quantity)
    if dfhack.timeout(time, quantity, mkresume()) then
        wait()
        return true
    else
        return false
    end
end

-- Some dialog wrappers:

function showMessage(title, text, tcolor)
    dlg.MessageBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        on_close = qresume(nil)
    }:show()

    return wait()
end

function showYesNoPrompt(title, text, tcolor)
    dlg.MessageBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        on_accept = mkresume(true),
        on_cancel = mkresume(false),
        on_close = qresume(nil)
    }:show()

    return wait()
end

function showInputPrompt(title, text, tcolor, input, min_width)
    dlg.InputBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        input = input,
        frame_width = min_width,
        on_input = mkresume(true),
        on_cancel = mkresume(false),
        on_close = qresume(nil)
    }:show()

    return wait()
end

function showListPrompt(title, text, tcolor, choices, min_width, filter)
    dlg.ListBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        choices = choices,
        frame_width = min_width,
        with_filter = filter,
        on_select = mkresume(true),
        on_cancel = mkresume(false),
        on_close = qresume(nil)
    }:show()

    return wait()
end

function showMaterialPrompt(title, prompt)
    require('gui.materials').MaterialDialog{
        frame_title = title,
        prompt = prompt,
        on_select = mkresume(true),
        on_cancel = mkresume(false),
        on_close = qresume(nil)
    }:show()

    return wait()
end

return _ENV
