# run many dfhack commands separated by ;
# ex: multicmd locate-ore IRON ; digv ; digcircle 16

=begin
BEGIN_DOCS

.. _scripts/multicmd:

multicmd
========
Run multiple dfhack commands. The argument is split around the
character ; and all parts are run sequentially as independent
dfhack commands. Useful for hotkeys.

Example::

    multicmd locate-ore iron ; digv

END_DOCS
=end

$script_args.join(' ').split(/\s*;\s*/).each { |cmd| df.dfhack_run cmd }
