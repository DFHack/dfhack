.. _material-encoding:

#####################################################
How DF encodes materials: ``mat_type``/``mat_index``
#####################################################

Almost everywhere that Dwarf Fortress needs to refer to a material -- items,
item improvements, job requirements, plants, reactions, syndromes, map
features, and so on -- it does so with a pair of integers. In the names
provided by Bay 12, the pair is usually called ``material`` and
``matgloss``, although other naming is sometimes seen, such as ``subcat1``/
``subcat2``. Before we learned otherwise, the DFHack team originally called
what we now know to be a matgloss a ``mat_index`` ("material index"), and so
this term tends to be used in DFHack code instead of "matgloss". Thus, you may
see these two terms used interchangeably in DFHack code, in documentation, and
in community discussions.

.. contents:: Contents
  :local:

The ``mat_type`` half
=====================

``mat_type`` is an index into ``world.raws.mat_table.builtin``
(a ``special_mat_table``, DF name ``material_infost.misc_def``), which is a
static array of pointers to ``df::material`` objects indexed by the
``builtin_mats`` enum (DF name ``MaterialType``). The enum is laid out as
follows:

=================  =====================  =============================================
Numeric range      Enum values            Meaning
=================  =====================  =============================================
-1                 ``NONE``               No material / invalid
0                  ``INORGANIC``          ``mat_index`` selects a ``df::inorganic_raw``
1..18              ``AMBER``..``GRIME``   True built-in materials
19..218            ``CREATURE_1..200``    Creature raw materials
219..418           ``HIST_FIG_1..200``    Historical figure creature materials
419..618           ``PLANT_1..200``       Plant raw materials
619..658           ``UNUSED01..40``       Unused; treated as built-ins, normally null
=================  =====================  =============================================

The named built-ins are ``AMBER``, ``CORAL``, ``GLASS_GREEN``,
``GLASS_CLEAR``, ``GLASS_CRYSTAL``, ``WATER``, ``COAL``, ``POTASH``,
``ASH``, ``PEARLASH``, ``LYE``, ``MUD``, ``VOMIT``, ``SALT``, ``FILTH_B``
(feces), ``FILTH_Y`` (urine), ``UNKNOWN_SUBSTANCE`` (semen), and ``GRIME``.
Several of these built-ins exist solely for legacy purposes and do not normally
arise in unmodded games; for example, ``FILTH_B`` and ``FILTH_Y`` only appear
in very specific situations, and ``UNKNOWN_SUBSTANCE`` was only ever seen in
connection with tentacle demons, which haven't been in the game since version
0.28.181.40d. These are materials constructed by the game itself (not loaded
from raws). The array still has a slot for every enum value, but entries outside
the named built-ins are not real materials -- for the ``CREATURE_*``,
``HIST_FIG_*``, and ``PLANT_*`` ranges the actual ``df::material`` lives
inside a raw object and ``mat_index`` is needed to find it. Entries in
the array may be null, so consumers should always check the resolved
pointer.  

The ``mat_index`` half
======================

The meaning of ``mat_index`` depends entirely on the ``mat_type`` range:

* ``mat_type`` < 0 (``NONE``): no material. ``mat_index`` should also be -1.

* ``mat_index`` == -1, any non-negative ``mat_type``: a "bare" reference to
  the built-in material object ``mat_table.builtin[mat_type]``. For most
  built-ins this is just the material itself, but ``(0, -1)`` is special:
  it is the *inorganic wildcard*, meaning "any inorganic material", used in
  contexts such as job item constraints (e.g. "any stone").

* ``mat_type`` == 0 (``INORGANIC``), ``mat_index`` >= 0: ``mat_index`` is
  an index into ``world.raws.inorganics.all`` (a ``df::inorganic_raw``),
  and the material is ``inorganic->material``. For example,
  ``(0, <index of IRON>)`` is iron.

* 1 <= ``mat_type`` <= 18 (built-ins), ``mat_index`` >= 0: still a
  built-in material; ``mat_index`` is normally ignored. The one exception
  is ``COAL`` (7), where ``mat_index`` distinguishes the product:
  0 means coke and 1 means charcoal.

* 19 <= ``mat_type`` <= 218 (``CREATURE_1``..``CREATURE_200``):
  ``mat_index`` is an index into ``world.raws.creatures.all``
  (a ``df::creature_raw``), and ``mat_type - CREATURE_1`` is an index into
  that creature's ``material`` vector. In other words, ``mat_type`` picks
  both the table *and* which of the creature's (up to 200) defined
  materials is meant, while ``mat_index`` picks the creature.
  For example, dwarf bone is
  ``(CREATURE_1 + <bone slot>, <index of DWARF>)``.

* 219 <= ``mat_type`` <= 418 (``HIST_FIG_1``..``HIST_FIG_200``): like the
  creature range, except that ``mat_index`` is a **historical figure id**,
  not a vector index -- the figure is looked up by its ``id`` field in
  ``world.history.figures``. The material itself is
  ``creature->material[mat_type - HIST_FIG_1]`` of the figure's race
  (``creature_raw::find(figure->race)``). These pairs identify materials
  belonging to a *specific* historical figure, e.g. products of named
  creatures such as forgotten beasts, titans, and demons.

* 419 <= ``mat_type`` <= 618 (``PLANT_1``..``PLANT_200``):
  ``mat_index`` is an index into ``world.raws.plants.all``
  (a ``df::plant_raw``), and ``mat_type - PLANT_1`` is an index into that
  plant's ``material`` vector.

* ``mat_type`` > 618 (``UNUSED*``): treated as a built-in;
  ``mat_table.builtin[mat_type]`` is used directly and is normally null.

Where the materials themselves live
===================================

All of the above resolves to a ``df::material`` object (DF name
``material_definitionst``), which holds the material id, per-state names
and colors, flags, heat properties, and so on:

* Built-ins: ``world.raws.mat_table.builtin[mat_type]``.
* Inorganics: embedded in ``world.raws.inorganics.all[mat_index]->material``.
* Creatures: ``world.raws.creatures.all[mat_index]->material[subtype]``.
* Plants: ``world.raws.plants.all[mat_index]->material[subtype]``.
  Additionally, each ``df::plant_raw`` has a ``material_defs`` block --
  parallel ``type``/``idx`` arrays (DF names ``use_mat``/``use_matg``)
  indexed by ``plant_material_def`` -- that records the canonical
  ``(mat_type, mat_index)`` pair for each defined use of the plant
  (``basic_mat``, ``tree``, ``drink``, ``seed``, ``thread``, ``mill``,
  ``extract_vial``, ``extract_barrel``, ``extract_still_vial``).

Where the pairs appear
======================

The ``mat_type``/``mat_index`` naming convention is used fairly consistently
in DF structures for individual fields, so ``MaterialInfo::decode`` can
consume any structure that exposes those members. Some representative
examples:

* Most item types (``item_barst``, ``item_woodst``, ``item_boulderst``,
  ``item_threadst``, etc.) store the pair directly in ``mat_type`` and
  ``mat_index`` fields.
* ``item_body_component`` (corpses and body parts) instead stores
  ``race``, ``caste``, ``hist_figure_id``, and ``unit_id``. The item
  virtual methods ``getActualMaterial()`` and ``getActualMaterialIndex()``
  (DF names ``get_forced_material``/``get_forced_matgloss``) compute the
  proper pair -- "an actual material, never a race/caste" -- so
  ``MaterialInfo::decode(item)`` always uses those methods rather than the
  raw fields.
* ``material_vec_ref`` is a DFHack convenience compound (not a real DF
  structure) that bundles the parallel ``mat_type``/``mat_index`` vectors
  DF uses for multi-material lists, e.g. in reaction products and
  ``world.raws.mat_table.syndromes``/``effects``. In the actual Bay 12
  headers, these structures are parallel arrays appearing as sequential
  members within a larger compound.
* ``job_item`` (work order and job requirements) uses ``mat_type``/
  ``mat_index`` for the required material, where the ``(0, -1)`` inorganic
  wildcard is common.
* Historical-figure materials appear in things like body part items of
  named creatures.

Other names for the pair in DF code include ``material``/``matgloss``
(items, improvements), ``use_mat``/``use_matg`` (plant material
definitions), ``itemneeded_material``/``itemneeded_matgloss`` (job items),
``dye_material``/``dye_matgloss`` (dyed improvements),
``hardens_with_water_mat``/``hardens_with_water_matg``, and
``syndrome_mat``/``syndrome_matg`` (syndrome tables).

Working with the encoding in DFHack
===================================

``DFHack::MaterialInfo`` (exposed to Lua as ``dfhack.matinfo``) is the
canonical decoder for a pair. Constructing one via ``decode(type, index)``,
``decode(item)``, ``decode(material_vec_ref, idx)``, or
``decode(some_struct_pointer)`` fills in:

* ``type``/``index``: the raw pair.
* ``material``: the resolved ``df::material`` pointer (``isValid()`` is
  false when it is null).
* ``mode``: one of ``None``, ``Builtin``, ``Inorganic``, ``Creature``, or
  ``Plant`` -- note that historical figure materials report ``Creature``,
  distinguished by a non-null ``figure``.
* ``subtype``: the position within the creature/plant material vector.
* ``inorganic``, ``creature``, ``plant``: the resolved raw object for the
  matching mode.
* ``figure``: the ``df::historical_figure`` for ``HIST_FIG_*`` pairs.

``MaterialInfo::find`` and ``dfhack.matinfo.find`` also parse the
string-token forms produced by ``getToken()``:
``INORGANIC:<token>`` (e.g. ``INORGANIC:IRON``),
``CREATURE:<creature>:<material>`` (e.g. ``CREATURE:DWARF:BONE``),
``PLANT:<plant>:<material>``, plain ``<token>`` for built-ins, and the
special cases ``COAL:COKE``/``COAL:CHARCOAL`` and ``NONE``.

For plain integer pairs, ``DFHack::t_matpair`` and
``DFHack::t_matglossPair`` are simple ``{mat_type, mat_index}`` /
``{type, index}`` bundles, and ``isAnyInorganic()``/``isInorganicWildcard()``
test for ``mat_type == 0`` and the ``(0, -1)`` wildcard respectively.

See the `lua-api` documentation for the Lua-side ``dfhack.matinfo``
interface.
