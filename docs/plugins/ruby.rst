.. _rb:

ruby
====

.. dfhack-tool::
    :summary: Allow Ruby scripts to be executed as DFHack commands.
    :tags: dev
    :no-command:

.. dfhack-command:: rb
   :summary: Eval() a ruby string.

.. dfhack-command:: rb_eval
   :summary: Eval() a ruby string.

Usage
-----

::

    enable ruby
    rb "ruby expression"
    rb_eval "ruby expression"
    :rb ruby expression

Example
-------

``:rb puts df.unit_find(:selected).name``
    Print the name of the selected unit.
