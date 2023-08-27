-- common logic for the quickfort modules
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

verbose = false

function log(...)
    if verbose then print(string.format(...)) end
end

function logfn(target_fn, ...)
    if verbose then target_fn{...} end
end
