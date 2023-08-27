-- Example of a lua script.
--[====[

devel/lua-example
=================
An example Lua script which reports the number of times it has been called.
Useful for testing environment persistence.

]====]

run_count = run_count or 0
run_count = run_count + 1

print('Arguments: ',...)
print('Command called '..run_count..' times.')
