-- Common startup file for all dfhack plugins with lua support

-- The global dfhack table is already created by C++ init
-- code. Feed it back to the require() mechanism.
return dfhack
