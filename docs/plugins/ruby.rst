.. _rb:

ruby
====
Tags:
:dfhack-keybind:`rb`
:dfhack-keybind:`rb_eval`

Allow Ruby scripts to be executed. When invoked as a command, you can Eval() a
ruby string.

Usage::

    enable ruby
    rb "ruby expression"
    rb_eval "ruby expression"
    :rb ruby expression

Example
-------

``:rb puts df.unit_find(:selected).name``
    Print the name of the selected unit.
