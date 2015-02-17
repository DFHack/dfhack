# patch start dwarf count

nr = $script_args[0].to_i

raise 'too low' if nr < 7

addr = df.get_global_address('start_dwarf_count')
df.memory_patch(addr, [nr].pack('L'))

