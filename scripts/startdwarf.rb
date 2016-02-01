# patch start dwarf count
=begin

startdwarf
==========
Use at the embark screen to embark with the specified number of dwarves.  Eg.
``startdwarf 500`` would lead to a severe food shortage and FPS issues, while
``startdwarf 10`` would just allow a few more warm bodies to dig in.
The number must be 7 or greater.

=end

nr = $script_args[0].to_i

raise 'too low' if nr < 7

addr = df.get_global_address('start_dwarf_count')
raise 'patch address not available' if addr == 0
df.memory_patch(addr, [nr].pack('L'))

