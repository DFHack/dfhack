-- Example of a lua script.
--[[=begin

devel/lua-example
=================
An example lua script, which reports the number of times it has
been called.  Useful for testing environment persistence.

=end]]

run_count = (run_count or 0) + 1

print('Arguments: ',...)
print('Command called '..run_count..' times.')
