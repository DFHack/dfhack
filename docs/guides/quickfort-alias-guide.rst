.. _quickfort-alias-guide:

Quickfort Keystroke Alias Guide
===============================

Aliases allow you to use simple words to represent complicated key sequences
when configuring buildings and stockpiles in quickfort ``#query`` blueprints.

For example, say you have the following ``#build`` and ``#place`` blueprints::

    #build masonry workshop
    ~, ~,~,`,`,`
    ~,wm,~,`,`,`
    ~, ~,~,`,`,`

    #place stockpile for mason
    ~,~,~,s,s,s
    ~,~,~,s,s,s
    ~,~,~,s,s,s

and you want to configure the stockpile to hold only non-economic ("other")
stone and to give to the adjacent mason workshop. You could write the
key sequences directly::

    #query configure stockpile with expanded key sequences
    ~,~,~,s{Down 5}deb{Right}{Down 2}p^,`,`
    ~,~,~,g{Left 2}&,                   `,`
    ~,~,~,`,                            `,`

or you could use aliases::

    #query configure stockpile with aliases
    ~,~,~,otherstone,`,`
    ~,~,~,give2left, `,`
    ~,~,~,`,         `,`

If the stockpile had only a single tile, you could also replay both aliases in
a single cell::

    #query configure mason with multiple aliases in one cell
    ~,~,~,{otherstone}{give2left},`,`
    ~,~,~,`,                      `,`
    ~,~,~,`,                      `,`

With aliases, blueprints are much easier to read and understand. They also
save you from having to copy the same long key sequences everywhere.

Alias definition files
----------------------

DFHack comes with a library of aliases for you to use that are always
available when you run a ``#query`` blueprint. Many blueprints can be built
with just those aliases. This "standard alias library" is stored in
:source:`data/quickfort/aliases-common.txt` (installed under the ``hack`` folder
in your DFHack installation). The aliases in that file are described at the
`bottom of this document <quickfort-alias-library>`.

Please do not edit the aliases in the standard library directly. The file will
get overwritten when DFHack is updated and you'll lose your changes. Instead,
add your custom aliases to :source:`dfhack-config/quickfort/aliases.txt` or
directly to your blueprints in an `#aliases <quickfort-aliases-blueprints>`
section. Your custom alias definitions take precedence over any definitions in
the standard library.

Alias syntax and usage
----------------------

The syntax for defining aliases is::

    aliasname: expansion

Where ``aliasname`` is at least two letters or digits long (dashes and
underscores are also allowed) and ``expansion`` is whatever you would type
into the DF UI.

You use an alias by typing its name into a ``#query`` blueprint cell where you
want it to be applied. You can use an alias by itself or as part of a larger
sequence, potentially with other aliases. If the alias is the only text in the
cell, the alias name is matched and its expansion is used. If the alias has
other keys before or after it, the alias name must be surrounded in curly
brackets (:kbd:`{` and :kbd:`}`). An alias can be surrounded in curly brackets
even if it is the only text in the cell, it just isn't necesary. For example,
the following blueprint uses the ``aliasname`` alias by itself in the first
two rows and uses it as part of a longer sequence in the third row::

    #query apply alias 'aliasname' in three different ways
    aliasname
    {aliasname}
    literaltext{aliasname}literaltext

For a more concrete example of an alias definition, a simple alias that
configures a stockpile to have no bins (:kbd:`C`) and no barrels (:kbd:`E`)
assigned to it would look like this::

    nocontainers: CE

The alias definition can also contain references to other aliases by including
the alias names in curly brackets. For example, ``nocontainers`` could be
equivalently defined like this::

    nobins: C
    nobarrels: E
    nocontainers: {nobins}{nobarrels}

Aliases used in alias definitions *must* be surrounded by curly brackets, even
if they are the only text in the definition::

    alias1: text1
    alias2: alias1
    alias3: {alias1}

Here, ``alias1`` and ``alias3`` expand to ``text1``, but ``alias2`` expands to
the literal text ``alias1``.

Keycodes
~~~~~~~~

Non-printable characters, like the arrow keys, are represented by their
keycode name and are also surrounded by curly brackets, like ``{Right}`` or
``{Enter}``. Keycodes are used exactly like aliases -- they just have special
expansions that you wouldn't be able to write yourself. In order to avoid
naming conflicts between aliases and keycodes, the convention is to start
aliases with a lowercase letter.

Any keycode name from the DF interface definition file
(data/init/interface.txt) is valid, but only a few keycodes are actually
useful for blueprints::

    Up
    Down
    Left
    Right
    Enter
    ESC
    Backspace
    Space
    Tab

There is also one pseudo-keycode that quickfort recognizes::

    Empty

which has an empty expansion. It is primarily useful for defining blank default values for `Sub-aliases`_.

Repetitions
~~~~~~~~~~~

Anything enclosed within curly brackets can also have a number, indicating how
many times that alias or keycode should be repeated. For example:
``{togglesequence 9}`` or ``{Down 5}`` will repeat the ``togglesequence``
alias nine times and the ``Down`` keycode five times, respectively.

Modifier keys
~~~~~~~~~~~~~

Ctrl, Alt, and Shift modifiers can be specified for the next key by adding
them into the key sequence. For example, Alt-h is written as ``{Alt}h``.

Shorthand characters
~~~~~~~~~~~~~~~~~~~~

Some frequently-used keycodes are assigned shorthand characters. Think of them
as single-character aliases that don't need to be surrounded in curly
brackets::

    &   expands to {Enter}
    @   expands to {Shift}{Enter}
    ~   expands to {Alt}
    !   expands to {Ctrl}
    ^   expands to {ESC}

If you need literal versions of the shorthand characters, surround them in
curly brackets, for example: use ``{!}`` for a literal exclamation point.

Built-in aliases
~~~~~~~~~~~~~~~~

Most aliases that come with DFHack are in ``aliases-common.txt``, but there is
one alias built into the code for the common shorthand for "make room"::

    r+  expands to r+{Enter}

This needs special code support since ``+`` can't normally be used in alias
names. You can use it just like any other alias, either by itself in a cell
(``r+``) or surrounded in curly brackets (``{r+}``).

Sub-aliases
~~~~~~~~~~~

You can specify sub-aliases that will only be defined while the current alias
is being resolved. This is useful for "injecting" custom behavior into the
middle of a larger alias. As a simple example, the ``givename`` alias is defined
like this::

    givename: !n{name}&

Note the use of the ``name`` alias inside of the ``givename`` expansion. In your
``#query`` blueprint, you could write something like this, say, while over your
main drawbridge::

    {givename name="Front Gate"}

The value that you give the sub-alias ``name`` will be used when the
``givename`` alias is expanded. Without sub-aliases, we'd have to define
``givename`` like this::

    givenameprefix: !n
    givenamesuffix: &

and use it like this::

    {givenameprefix}Front Gate{givenamesuffix}

which is more difficult to write and more difficult to understand.

A handy technique is to define an alias with some sort of default
behavior and then use sub-aliases to override that behavior as necessary. For
example, here is a simplified version of the standard ``quantum`` alias that
sets up quantum stockpiles::

    quantum_enable: {enableanimals}{enablefood}{enablefurniture}...
    quantum: {linksonly}{nocontainers}{quantum_enable}

You can use the default behavior of ``quantum_enable`` by just using the
``quantum`` alias by itself. But you can override ``quantum_enable`` to just
enable furniture for some specific stockpile like this::

    {quantum quantum_enable={enablefurniture}}

If an alias uses a sub-alias in its expansion, but the sub-alias is not defined
when the alias is used, quickfort will halt the ``#query`` blueprint with an
error. If you want your aliases to work regardless of whether sub-aliases are
defined, then you must define them with default values like ``quantum_enable``
above. If a default value should be blank, like the ``name`` sub-alias used by
the ``givename`` alias above, define it with the ``{Empty}`` pesudo-keycode::

    name: {Empty}

Sub-aliases must be in one of the following formats::

    subaliasname=keyswithnospaces
    subaliasname="keys with spaces or {aliases}"
    subaliasname={singlealias}

If you specify both a sub-alias and a number of repetitions, the number for
repetitions goes last, right before the :kbd:`}`::

    {alias subaliasname=value repetitions}

Beyond query mode
-----------------
``#query`` blueprints normally do things in DF :kbd:`q`\uery mode, but nobody
said that we have to *stay* in query mode. ``#query`` blueprints send
arbitrary key sequences to Dwarf Fortress. Anything you can do by typing keys
into DF, you can do in a ``#query`` blueprint. It is absolutely fine to
temporarily exit out of query mode, go into, say, hauling or zone or hotkey
mode, and do whatever needs to be done.

You just have to make certain to exit out of that alternate mode and get back
into :kbd:`q`\uery mode at the end of the key sequence. That way quickfort can
continue on configuring the next tile -- a tile configuration that assumes the
game is still in query mode.

For example, here is the standard library alias for giving a name to a zone::

    namezone: ^i{givename}^q

The first :kbd:`\^` exits out of query mode. Then :kbd:`i` enters zones mode.
We then reuse the standard alias for giving something a name. Finally, we exit
out of zones mode with another :kbd:`\^` and return to :kbd:`q`\uery mode.

.. _quickfort-alias-library:

The DFHack standard alias library
---------------------------------

DFHack comes with many useful aliases for you to use in your blueprints. Many
blueprints can be built with just these aliases alone, with no custom aliases
required.

This section goes through all aliases provided by the DFHack standard alias
library, discussing their intended usage and detailing sub-aliases that you
can define to customize their behavior.

If you do define your own custom aliases in
``dfhack-config/quickfort/aliases.txt``, try to build on library alias
components. For example, if you create an alias to modify particular furniture
stockpile settings, start your alias with ``{furnitureprefix}`` instead of
``s{Down 2}``. Using library prefixes will allow library sub-aliases to work
with your aliases just like they do with library aliases. In this case, using
``{furnitureprefix}`` will allow your stockpile customization alias to work
with both stockpiles and hauling routes.

Note that some aliases use the DFHack-provided search prompts. If you get errors
while running ``#query`` blueprints, ensure the DFHack `search-plugin` plugin is
enabled.

Naming aliases
~~~~~~~~~~~~~~

These aliases give descriptive names to workshops, levers, stockpiles, zones,
etc. Dwarf Fortress building, stockpile, and zone names have a maximum length
of 20 characters.

========  ===========
Alias     Sub-aliases
========  ===========
givename  name
namezone  name
========  ===========

``givename`` works anywhere you can hit Ctrl-n to customize a name, like when
the cursor is over buildings and stockpiles. Example::

    #place
    f(10x2)

    #query
    {booze}{givename name=booze}

``namezone`` is intended to be used when over an activity zone. It includes
commands to get into zones mode, set the zone name, and get back to query
mode. Example::

    #zone
    n(2x2)

    #query
    {namezone name="guard dog pen"}

Quantum stockpile aliases
~~~~~~~~~~~~~~~~~~~~~~~~~

These aliases make it easy to create :wiki:`minecart stop-based quantum stockpiles <Quantum_stockpile#The_Minecart_Stop>`.

+----------------------+------------------+
| Alias                | Sub-aliases      |
+======================+==================+
| quantum              | | name           |
|                      | | quantum_enable |
+----------------------+------------------+
| quantumstopfromnorth | | name           |
+----------------------+ | stop_name      |
| quantumstopfromsouth | | route_enable   |
+----------------------+                  |
| quantumstopfromeast  |                  |
+----------------------+                  |
| quantumstopfromwest  |                  |
+----------------------+------------------+
| sp_link              | | move           |
|                      | | move_back      |
+----------------------+------------------+
| quantumstop          | | name           |
|                      | | stop_name      |
|                      | | route_enable   |
|                      | | move           |
|                      | | move_back      |
|                      | | sp_links       |
+----------------------+------------------+

The idea is to use a minecart on a track stop to dump an infinite number of
items into a receiving "quantum" stockpile, which significantly simplifies
stockpile management. These aliases configure the quantum stockpile and
hauling route that make it all work. Here is a complete example for quantum
stockpiling weapons, armor, and ammunition. It has a 3x1 feeder stockpile on
the bottom (South), the trackstop in the center, and the quantum stockpile on
the top (North). Note that the feeder stockpile is the only stockpile that
needs to be configured to control which types of items end up in the quantum
stockpile. By default, the hauling route and quantum stockpile itself simply
accept whatever is put into them.

::

    #place
     ,c
     ,
    pdz(3x1)

    #build
     ,
     ,trackstopN

    #query message(remember to assign a minecart to the new route)
     ,quantum
     ,quantumstopfromsouth
    nocontainers

The ``quantum`` alias configures a 1x1 stockpile to be a quantum stockpile. It
bans all containers and prevents the stockpile from being manually filled. By
default, it also enables storage of all item categories (except corpses and
refuse), so it doesn't really matter what letter you use to place the
stockpile. :wiki:`Refuse` is excluded by default since otherwise clothes and
armor in the quantum stockpile would rot away. If you want corpses or bones in
your quantum stockpile, use :kbd:`y` and/or :kbd:`r` to place the stockpile
and the ``quantum`` alias will just enable the remaining types. If you *do*
enable refuse in your quantum stockpile, be sure you avoid putting useful
clothes or armor in there!

The ``quantumstopfromsouth`` alias is run over the track stop and configures
the hauling route, again, allowing all item categories into the minecart by
default so any item that can go into the feeder stockpile can then be placed
in the minecart. It also links the hauling route with the feeder stockpile to
the South.The track stop does not need to be fully constructed before the
``#query`` blueprint is run, but the feeder stockpile needs to exist so we can
link to it. This means that the three blueprints above can be run one right
after another, without any dwarven labor in between them, and the quantum
stockpile will work properly.

Finally, the ``nocontainers`` alias simply configures the feeder stockpile to
not have any containers (which would just get in the way here). If we wanted
to be more specific about what item types we want in the quantum stockpile, we
could configure the feeder stockpile further, for example with standard
`stockpile adjustment aliases <quickfort-stockpile-aliases>`.

After the blueprints are run, the last step is to manually assign a minecart
to the newly-defined hauling route.

You can define sub-aliases to customize how these aliases work, for example to
have fine-grained control over what item types are enabled for the route and
quantum stockpile. We'll go over those options below, but first, here is an
example for how to just give names to everything::

    #query message(remember to assign a minecart to the new route)
     ,{quantum name="armory quantum"}
     ,{quantumstopfromsouth name="Armory quantum" stop_name="Armory quantum stop"}{givename name="armory dumper"}
    {givename name="armory feeder"}

All ``name`` sub-aliases are completely optional, of course. Keep in mind that
hauling route names have a maximum length of 22 characters, hauling route stop
names have a maximum length of 21 characters, and all other names have a
maximum length of 20 characters.

If you want to be absolutely certain that nothing ends up in your quantum
stockpile other than what you've configured in the feeder stockpile, you can
set the ``quantum_enable`` sub-alias for the ``quantum`` alias. This can
prevent, for example, somebody's knocked-out tooth from being considered part
of your furniture quantum stockpile when it happened to land on it during a
fistfight::

    #query
    {quantum name="furniture quantum" quantum_enable={enablefurniture}}

You can have similar control over the hauling route if you need to be more
selective about what item types are allowed into the minecart. If you have
multiple specialized quantum stockpiles that use a common feeder pile, for
example, you can set the ``route_enable`` sub-alias::

    #query
    {quantumstopfromsouth name="Steel bar quantum" route_enable="{enablebars}{steelbars}"}

Any of the `stockpile configuration aliases <quickfort-stockpile-aliases>` can
be used for either the ``quantum_enable`` or ``route_enable`` sub-aliases.
Experienced Dwarf Fortress players may be wondering how the same aliases can
work in both contexts since the keys for entering the configuration screen
differ. Fear not! There is some sub-alias magic at work here. If you define
your own stockpile configuraiton aliases, you can use the magic yourself by
building your aliases on the ``*prefix`` aliases described later in this
guide.

Finally, the ``quantumstop`` alias is a more general version of the simpler
``quantumstopfrom*`` aliases. The ``quantumstopfrom*`` aliases assume that a
single feeder stockpile is orthogonally adjacent to your track stop (which is
how most people set them up). If your feeder stockpile is somewhere further
away, or you have multiple feeder stockpiles to link, you can use the
``quantumstop`` alias directly. In addition to the sub-aliases used in the
``quantumstopfrom*`` alias, you can define the ``move`` and ``move_back``
sub-aliases, which let you specify the cursor keys required to move from the
track stop to the (single) feeder stockpile and back again, respectively::

    #query
    {quantumstop move="{Right 2}{Up}" move_back="{Down}{Left 2}"}

If you have multiple stockpiles to link, define the ``sp_links`` sub-alias,
which can chain several ``sp_link`` aliases together, each with their own
movement configuration::

    #query
    {quantumstop sp_links="{sp_link move=""{Right}{Up}"" move_back=""{Down}{Left}""}{sp_link move=""{Right}{Down}"" move_back=""{Up}{Left}""}"}

Note the doubled quotes for quoted elements that are within the outer quotes.

Farm plots
~~~~~~~~~~

Sets a farm plot to grow the first or last type of seed in the list of
available seeds for all four seasons. The last seed is usually Plump helmet
spawn, suitable for post-embark. But if you only have one seed type, that'll
be grown instead.

+------------------+
| Alias            |
+==================+
| growlastcropall  |
+------------------+
| growfirstcropall |
+------------------+

Instead of these aliases, though, it might be more useful to use the DFHack
`autofarm` plugin.

Stockpile configuration utility aliases
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

================  ===========
Alias             Sub-aliases
================  ===========
linksonly
maxbins
maxbarrels
nobins
nobarrels
nocontainers
give2up
give2down
give2left
give2right
give10up
give10down
give10left
give10right
give              move
togglesequence
togglesequence2
forbidsearch      search
permitsearch      search
togglesearch      search
masterworkonly    prefix
artifactonly      prefix
togglemasterwork  prefix
toggleartifact    prefix
================  ===========

``linksonly``, ``maxbins``, ``maxbarrels``, ``nobins``, ``nobarrels``, and
``nocontainers`` set the named basic properties on stockpiles. ``nocontainers``
sets bins and barrels to 0, but does not affect wheelbarrows since the hotkeys
for changing the number of wheelbarrows depend on whether you have DFHack's
``tweak max-wheelbarrow`` enabled. It is better to set the number of
wheelbarrows via the `quickfort` ``stockpiles_max_wheelbarrows`` setting (set to
``0`` by default), or explicitly when you define the stockpile in the ``#place``
blueprint.

The ``give*`` aliases set a stockpile to give to a workshop or another
stockpile located at the indicated number of tiles in the indicated direction
from the current tile. For example, here we use the ``give2down`` alias to
connect an ``otherstone`` stockpile with a mason workshop::

    #place
    s,s,s,s,s
    s, , , ,s
    s, , , ,s
    s, , , ,s
    s,s,s,s,s

    #build
    `,`,`,`,`
    `, , , ,`
    `, ,wm,,`
    `, , , ,`
    `,`,`,`,`

    #query
     , ,give2down
    otherstone

and here is a generic stone stockpile that gives to a stockpile that only
takes flux::

    #place
    s(10x1)
    s(10x10)

    #query
    flux
    ,
    give2up

If you want to give to some other tile that is not already covered by the
``give2*`` or ``give10*`` aliases, you can use the generic ``give`` alias and
specify the movement keys yourself in the ``move`` sub-alias. Here is how to
give to a stockpile or workshop one z-level above, 9 tiles to the left, and 14
tiles down::

    #query
    {give move="<{Left 9}{Down 14}"}

``togglesequence`` and ``togglesequence2`` send ``{Down}{Enter}`` or
``{Down 2}{Enter}`` to toggle adjacent (or alternating) items in a list. This
is useful when toggling a bunch of related item types in the stockpile config.
For example, the ``dye`` alias in the standard alias library needs to select
four adjacent items::

    dye: {foodprefix}b{Right}{Down 11}{Right}{Down 28}{togglesequence 4}^

``forbidsearch``, ``permitsearch``, and ``togglesearch`` use the DFHack
`search-plugin` plugin to forbid or permit a filtered list, or toggle the first
(or only) item in the list. Specify the search string in the ``search``
sub-alias. Be sure to move the cursor over to the right column before invoking
these aliases. The search filter will be cleared before this alias completes.

Finally, the ``masterwork`` and ``artifact`` group of aliases configure the
corresponding allowable core quality for the stockpile categories that have
them. This alias is used to implement category-specific aliases below, like
``artifactweapons`` and ``forbidartifactweapons``.

.. _quickfort-stockpile-aliases:

Stockpile adjustment aliases
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For each stockpile item category, there are three standard aliases:

* ``*prefix`` aliases enter the stockpile configuration screen and position
  the cursor at a particular item category in the left-most column, ready for
  further keys that configure the elements within that category. All other
  stockpile adjustment aliases are built on these prefixes. You can use them
  yourself to create stockpile adjustment aliases that aren't already covered
  by the standard library aliases. Using the library prefix instead of
  creating your own also allows your stockpile configuration aliases to be
  used for both stockpiles and hauling routes. For example, here is the
  library definition for ``booze``::

    booze: {foodprefix}b{Right}{Down 5}p{Down}p^

* ``enable*`` aliases enter the stockpile configuration screen, enable all
  subtypes of the named category, and exit the stockpile configuration screen
* ``disable*`` aliases enter the stockpile configuration screen, disable all
  subtypes of the named category, and exit the stockpile configuration screen

====================  ====================  =====================
Prefix                Enable                Disable
====================  ====================  =====================
animalsprefix         enableanimals         disableanimals
foodprefix            enablefood            disablefood
furnitureprefix       enablefurniture       disablefurniture
corpsesprefix         enablecorpses         disablecorpses
refuseprefix          enablerefuse          disablerefuse
stoneprefix           enablestone           disablestone
ammoprefix            enableammo            disableammo
coinsprefix           enablecoins           disablecoins
barsprefix            enablebars            disablebars
gemsprefix            enablegems            disablegems
finishedgoodsprefix   enablefinishedgoods   disablefinishedgoods
leatherprefix         enableleather         disableleather
clothprefix           enablecloth           disablecloth
woodprefix            enablewood            disablewood
weaponsprefix         enableweapons         disableweapons
armorprefix           enablearmor           disablearmor
sheetprefix           enablesheet           disablesheet
====================  ====================  =====================

Then, for each item category, there are aliases that manipulate interesting
subsets of that category:

* Exclusive aliases forbid everthing within a category and then enable only
  the named item type (or named class of items)
* ``forbid*`` aliases forbid the named type and leave the rest of the
  stockpile untouched.
* ``permit*`` aliases permit the named type and leave the rest of the
  stockpile untouched.

Note that for specific item types (items in the third stockpile configuration
column), you can only toggle the item type on and off. Aliases can't know
whether sending the ``{Enter}`` key will enable or disable the type. The
``forbid*`` aliases that affect these item types assume the item type was
enabled and toggle it off. Likewise, the ``permit*`` aliases assume the item
type was disabled and toggle it on. If the item type is not in the expected
enabled/disabled state when the alias is run, the aliases will not behave
properly.

Animal stockpile adjustments
````````````````````````````

===========  ===========  ============
Exclusive    Forbid       Permit
===========  ===========  ============
cages        forbidcages  permitcages
traps        forbidtraps  permittraps
===========  ===========  ============

Food stockpile adjustments
``````````````````````````

===============  ====================  ====================
Exclusive        Forbid                Permit
===============  ====================  ====================
preparedfood     forbidpreparedfood    permitpreparedfood
unpreparedfish   forbidunpreparedfish  permitunpreparedfish
plants           forbidplants          permitplants
booze            forbidbooze           permitbooze
seeds            forbidseeds           permitseeds
dye              forbiddye             permitdye
tallow           forbidtallow          permittallow
miscliquid       forbidmiscliquid      permitmiscliquid
wax              forbidwax             permitwax
===============  ====================  ====================

Furniture stockpile adjustments
```````````````````````````````

===================  =========================  =========================
Exclusive            Forbid                     Permit
===================  =========================  =========================
pots                 forbidpots                 permitpots
bags
buckets              forbidbuckets              permitbuckets
sand                 forbidsand                 permitsand
masterworkfurniture  forbidmasterworkfurniture  permitmasterworkfurniture
artifactfurniture    forbidartifactfurniture    permitartifactfurniture
===================  =========================  =========================

Notes:

* The ``bags`` alias excludes coffers and other boxes by forbidding all
  materials other than cloth, yarn, silk, and leather. Therefore, it is
  difficult to create ``forbidbags`` and ``permitbags`` without affecting other
  types of furniture stored in the same stockpile.

* Because of the limitations of Dwarf Fortress, ``bags`` cannot distinguish
  between empty bags and bags filled with gypsum powder.

Refuse stockpile adjustments
````````````````````````````

===========  ==================  ==================
Exclusive    Forbid              Permit
===========  ==================  ==================
corpses      forbidcorpses       permitcorpses
rawhides     forbidrawhides      permitrawhides
tannedhides  forbidtannedhides   permittannedhides
skulls       forbidskulls        permitskulls
bones        forbidbones         permitbones
shells       forbidshells        permitshells
teeth        forbidteeth         permitteeth
horns        forbidhorns         permithorns
hair         forbidhair          permithair
usablehair   forbidusablehair    permitusablehair
craftrefuse  forbidcraftrefuse   permitcraftrefuse
===========  ==================  ==================

Notes:

* ``usablehair`` Only hair and wool that can make usable clothing is included,
  i.e. from sheep, llamas, alpacas, and trolls.
* ``craftrefuse`` includes everything a craftsdwarf or tailor can use: skulls,
  bones, shells, teeth, horns, and "usable" hair/wool (defined above).

Stone stockpile adjustments
```````````````````````````

=============  ====================  ====================
Exclusive      Forbid                Permit
=============  ====================  ====================
metal          forbidmetal           permitmetal
iron           forbidiron            permitiron
economic       forbideconomic        permiteconomic
flux           forbidflux            permitflux
plaster        forbidplaster         permitplaster
coalproducing  forbidcoalproducing   permitcoalproducing
otherstone     forbidotherstone      permitotherstone
bauxite        forbidbauxite         permitbauxite
clay           forbidclay            permitclay
=============  ====================  ====================

Ammo stockpile adjustments
``````````````````````````

==============  ====================  ====================
Exclusive       Forbid                Permit
==============  ====================  ====================
bolts
\               forbidmetalbolts
\               forbidwoodenbolts
\               forbidbonebolts
masterworkammo  forbidmasterworkammo  permitmasterworkammo
artifactammo    forbidartifactammo    permitartifactammo
==============  ====================  ====================

Bar stockpile adjustments
`````````````````````````

===========  ==================
Exclusive    Forbid
===========  ==================
bars         forbidbars
metalbars    forbidmetalbars
ironbars     forbidironbars
steelbars    forbidsteelbars
pigironbars  forbidpigironbars
otherbars    forbidotherbars
coal         forbidcoal
potash       forbidpotash
ash          forbidash
pearlash     forbidpearlash
soap         forbidsoap
blocks       forbidblocks
===========  ==================

Gem stockpile adjustments
`````````````````````````

===========  ================
Exclusive    Forbid
===========  ================
roughgems    forbidroughgems
roughglass   forbidroughglass
cutgems      forbidcutgems
cutglass     forbidcutglass
cutstone     forbidcutstone
===========  ================

Finished goods stockpile adjustments
````````````````````````````````````

=======================  =============================  =============================
Exclusive                Forbid                         Permit
=======================  =============================  =============================
stonetools
woodentools
crafts                   forbidcrafts                   permitcrafts
goblets                  forbidgoblets                  permitgoblets
masterworkfinishedgoods  forbidmasterworkfinishedgoods  permitmasterworkfinishedgoods
artifactfinishedgoods    forbidartifactfinishedgoods    permitartifactfinishedgoods
=======================  =============================  =============================

Cloth stockpile adjustments
```````````````````````````

================  ======================  ======================
Exclusive         Forbid                  Permit
================  ======================  ======================
thread            forbidthread            permitthread
adamantinethread  forbidadamantinethread  permitadamantinethread
cloth             forbidcloth             permitcloth
adamantinecloth   forbidadamantinecloth   permitadamantinecloth
================  ======================  ======================

Notes:

* ``thread`` and ``cloth`` refers to all materials that are not adamantine.

Weapon stockpile adjustments
````````````````````````````

=================  ========================  =======================
Exclusive          Forbid                    Permit
=================  ========================  =======================
\                  forbidweapons             permitweapons
\                  forbidtrapcomponents      permittrapcomponents
metalweapons       forbidmetalweapons        permitmetalweapons
\                  forbidstoneweapons        permitstoneweapons
\                  forbidotherweapons        permitotherweapons
ironweapons        forbidironweapons         permitironweapons
bronzeweapons      forbidbronzeweapons       permitbronzeweapons
copperweapons      forbidcopperweapons       permitcopperweapons
steelweapons       forbidsteelweapons        permitsteelweapons
masterworkweapons  forbidmasterworkweapons   permitmasterworkweapons
artifactweapons    forbidartifactweapons     permitartifactweapons
=================  ========================  =======================

Armor stockpile adjustments
```````````````````````````

===============  ======================  =====================
Exclusive        Forbid                  Permit
===============  ======================  =====================
metalarmor       forbidmetalarmor        permitmetalarmor
otherarmor       forbidotherarmor        permitotherarmor
ironarmor        forbidironarmor         permitironarmor
bronzearmor      forbidbronzearmor       permitbronzearmor
copperarmor      forbidcopperarmor       permitcopperarmor
steelarmor       forbidsteelarmor        permitsteelarmor
masterworkarmor  forbidmasterworkarmor   permitmasterworkarmor
artifactarmor    forbidartifactarmor     permitartifactarmor
===============  ======================  =====================
