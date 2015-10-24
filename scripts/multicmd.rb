# run many dfhack commands separated by ;
=begin

multicmd
========
Run multiple dfhack commands. The argument is split around the
character ; and all parts are run sequentially as independent
dfhack commands. Useful for hotkeys.

Example::

    multicmd locate-ore IRON ; digv ; digcircle 16

=end

$script_args.join(' ').split(/\s*;\s*/).each { |cmd| df.dfhack_run cmd }
