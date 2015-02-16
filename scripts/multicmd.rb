# run many dfhack commands separated by ;
# ex: multicmd locate-ore IRON ; digv ; digcircle 16

$script_args.join(' ').split(/\s*;\s*/).each { |cmd| df.dfhack_run cmd }
