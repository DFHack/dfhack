-- Holds custom descriptions for view-item-info
-- By PeridexisErrant
--[[=begin

item-descriptions
=================
Exports a table with custom description text for every item in the game.
Used by `view-item-info`; see instructions there for how to override
for mods.

=end]]

-- Each line near the bottom has 53 characters of room until
-- it starts clipping over the UI in an ugly fashion.
-- For proper spacing, 50 characters is the maximum.
-- Descriptions which aren't pushed down the page by
-- barrel contents or such line up with the UI on the
-- 11th line down. There is room for a 10th long line
-- without clipping, but stopping at 9 leaves enough space
-- for ideal legibility.

-- The following people contributed descriptions:
-- Raideau, PeridexisErrant, /u/Puffin4Tom, /u/KroyMortlach
-- /u/genieus, /u/TeamsOnlyMedic, /u/johny5w, /u/DerTanni
-- /u/schmee101, /u/coaldiamond, /u/stolencatkarma, /u/sylth01
-- /u/MperorM, /u/SockHoarder, /u/_enclave_, WesQ3
-- /u/Xen0nex, /u/Jurph

if not moduleMode then
    print("scripts/item-descriptions.lua is a content library; calling it does nothing.")
end

local help --[[
This script has a single function: to return a custom description for every
vanilla item in the game.

If "raw/scripts/item-descriptions.lua" exists, it will entirely replace this one.
Instead, mods should use "raw/scripts/more-item-descriptions.lua" to add content or replace
descriptions on a case-by-case basis.  If an item description cannot be found in
the latter script, view-item-info will fall back to the former.
]]

-- see http://dwarffortresswiki.org/index.php/cv:Item_token
descriptions = {
    AMULET = {  "An item of jewellery worn around the neck for it's aesthetic value.",
                "An amulet does not interfere with wearing other equipment."},
    ANIMALTRAP = {
                "This tiny trap is used by your trappers to catch vermin. Some dwarves",
                "like vermin as pets - if your cats don't get them first. May be built",
                "from wood or metal. Catching vermin requires trap to be set with bait."},
    ANVIL = {   "An essential component of the forge."},
    ARMORSTAND = {
                "A rack for the storage of military equipment, specifically armor.",
                "Barracks may be designated and assigned from them. Military squads",
                "may use their assigned barracks for training, storage, or sleeping,",
                "depending on the settings. Some nobles demand an armor stand of",
                "their own."},
    BACKPACK = {"A backpack can be used by militia to carry rations in the field.",
                "In Adventure mode, backpacks can be used to carry more equipment."},
    BALLISTAARROWHEAD = {
                "The arrowtip used to create metal ballista arrows in a siege workshop."},
    BALLISTAPARTS = {
                "Three of these can be used to construct a Ballista."},
    BAR = {     "A small ingot made of metal, fuel, ash, or soap, made to facilitate",
                "stacking and storage. Aside from the uses unique to each material, bars",
                "of any type can also be used as building materials in place of blocks.",
                "",
                "Metal bars are used with fuel at a Metalsmith's Forge to make metal",
                "goods and decorations. Fuel is used at furnaces and workshops requiring",
                "intense heat, with the exception of magma furnaces or a Wood Furnace.",
                "Soap is used by hospitals to greatly reduce the chance of infected",
                "wounds, and rarely used by individual dwarves to clean themselves or",
                "surrounding tiles. Ash is processed at an Ashery to make potash or lye,",
                "potash is used as farm plot fertilizer or made into pearlash, and",
                "pearlash is used to make clear or crystal glass products."},
    BARREL = {  "A hollow cylinder with a removable lid. It is used to hold liquids,",
                "food, and seeds. It can be made from metal or wood, and is replaceable",
                "with a rock pot. A barrel (or rock pot) is needed to brew drinks."},
    BED = {     "A pallet for dwarves to sleep on, which must be made from wood.",
                "It prevents the stress of sleeping on the ground, and can be used",
                "to designate a bedroom (used by one dwarf or couple), a dormitory",
                "(used by multiple dwarves), or a barracks (used by a military",
                "squad for training or sleep)."},
    BIN = {     "A container for the storage of ammunition, armor and weapons, bars,",
                "blocks, cloth and leather, coins, finished goods and gems. It can",
                "be used to carry multiple items to the Trade Depot at once.",
                "A bin can be made from wood or forged from metal."},
    BLOCKS = {  "Blocks can be used for constructions in place of raw materials such",
                "as logs or bars.  Cutting boulders into blocks gives four times as",
                "many items, all of which are lighter for faster hauling and yield",
                "smooth constructions."},
    BOX = {     "A container for storing dwarves' items. They are required by nobles,",
                "and will increase the value of rooms they are placed in. Also",
                "required to store hospital supplies. They can be made from stone or",
                "metal (coffers), wood (chests),or textiles or leather (bags)."},
    BUCKET = {  "A small cylindrical or conical container for holding and carrying",
                "small amounts of liquid such as water or lye. They are used by",
                "dwarves to give water to other dwarves, to store lye, and are",
                "required to build wells and certain workshops. They can be made",
                "from wood or metal."},
    BOOK = {    "It's a book. Some books contain the secrets of life and death."},
    BOULDER = { "Mining may yield loose stones for industry.  There are four categories:",
                "non-economic stones for building materials,  ores for metal industry,",
                "gems, and special-purpose economic stones like flux, coal and lignite."},
    BRACELET = {"A bracelet is an item of jewellery worn on the hands."},
    CABINET = { "A place for dwarves to store old clothing, used by any dwarf whose room",
                "overlaps the cabinet. May be built from wood, stone, metal, or glass."},
    CAGE = {    "A cage can be made of glass, metal or wood. All materials are equally",
                "strong as cages can not be broken, however the weight of the material",
                "affects the time taken to move cages. Cages can be combined with a",
                "mechanism to create a cage trap. Cages can also be built as furniture,",
                "after which they can store an infinite number of animals or prisoners."},
    CATAPULTPARTS = {
                "Three of these can be used to construct a Catapult."},
    CHAIN = {   "A chain made of metal. A chain or rope is required to build a well.",
                "Due to the marvels of dwarven engineering, a single chain can be used",
                "for a well of any depth. Chains are also used to create restraints",
                "for prisoners or animals."},
    CHAIR = {   "Furniture used for sitting. Named a chair if made from wood,",
                "or a throne if made from stone, glass, or metal. Offices may be",
                "designated and assigned from them.  Dwarves will complain if there",
                "aren't enough chairs in the dining room."},
    CLOTH = {   "A piece of fabric made from threads of plant fiber, yarn, silk or",
                "adamantine. Cloth may be dyed.  It is used at a Clothier's Workshop to",
                "make clothing, bags, rope, and decorative sewn images. At a",
                "Craftsdwarf's Workshop, it can be made into trade goods. Hospitals",
                "use cloth for wound dressing - though expensive cloth confers no",
                "benefit. Specific types of cloth can be required by a strange mood."},
    COFFIN = {  "A final resting place for dwarves. Must be built and assigned before",
                "burial can occur. Named a coffin when made from stone or glass,",
                "casket when made from wood, and sarcophagus when made from metal.",
                "Tombs may be designated and assigned from them. Corpses contained in",
                "coffins may still reanimate."},
    COIN = {    "A metal coin, which represents value.  Surprisingly useless in trade."},
    CROWN = {   "A crown may be worn as headgear, or on top of a helmet.  Although",
                "usually just decorative or symbolic, crowns sometimes deflect attacks."},
    CRUTCH = {  "Item used in health-care. May be made from wood or metal. Given to",
                "dwarves who receive injuries that impair or prevent normal movement.",
                "Requires one hand."},
    DOOR = {    "A barrier that covers a hole in a wall and controls horizontal passage.",
                "Intelligent creatures can open and close doors as needed. Doors can",
                "also block the flow of liquids as long as they remain closed. May be",
                "set as 'locked' to prevent all passage or 'tightly closed' to prevent",
                "animal passage. Creatures cannot manually open or close doors that are",
                "mechanically controlled. May be linked via mechanisms to devices such",
                "as pressure plates and levers. Will become stuck open if an item",
                "occupies its tile."},
    EARRING = { "Earrings are decorative jewellery.  Eleven can be worn on each ear."},
    FIGURINE = {"A small piece of art carved in the likeness of a creature."},
    FLASK = {   "A drink container that is worn on the body, keeping the hands free.",
                "Soldiers and adventurers can carry any drink of their choice in this",
                "container.  Called a flask when made from metal, or a vial when",
                "made from glass."},
    FLOODGATE = {
                "A mechanical gate used to control the flow of water. Floodgates are",
                "initially closed when installed, and must be linked to a lever or",
                "pressure plate in order to be either opened or closed. Furniture for",
                "blocking horizontal passage. It is built as a solid tile and may be",
                "linked via mechanism to devices such as pressure plates and levers.",
                "When activated, the floodgate will disappear. Will become stuck open",
                "if an item occupies its tile."},
    GOBLET = {  "A small drink container that is held in one hand."},
    GRATE = {   "A barrier with small openings, grates block solid objects and",
                "creatures - but not line of sight, liquids, or projectiles. Grates can",
                "be installed vertically on a floor, or horizontally over open space.",
                "Grates can be retracted if they are mechanically linked to a lever or",
                "pressure plate. Grates are not stable enough to support constructions."},
    HATCH_COVER = {
                "A barrier that covers a hole in the floor. A hatch cover acts like a",
                "door, but placed over a vertical opening. Hatches can also cover carved",
                "stairways and ramps. Hatches can be linked to mechanical controls.",
                "Creatures cannot manually open or close hatches that are mechanically",
                "controlled.  They may also be set as 'locked' to prevent all passage or",
                "'tightly closed' to prevent animal passage."},
    ITEM_AMMO_ARROWS = {
                "Ammunition for bows, which are primarily used by elves."},
    ITEM_AMMO_BOLTS = {
                "Ammunition for crossbows, which are a dwarf's preferred ranged weapon.",
                "It is not recommended to store bolts in bins, due to pickup bugs."},
    ITEM_ARMOR_BREASTPLATE = {
                "A breastplate is a piece of plate armor that covers the upper body and",
                "the lower body. It is usually worn in the armor layer."},
    ITEM_ARMOR_CAPE = {
                "A (cool-looking) cape. Protects the chest."},
    ITEM_ARMOR_CLOAK = {
                "A cloth cloak. Protects the face, neck, chest, arms and upper legs."},
    ITEM_ARMOR_COAT = {
                "A heavy cloth coat. Protects the face, neck, chest, arms and upper legs."},
    ITEM_ARMOR_DRESS = {
                "A cloth dress. Protects the face, neck, chest, arms and legs."},
    ITEM_ARMOR_LEATHER = {
                "Leather armor is light and covers both arms and legs",
                "in addition to body"},
    ITEM_ARMOR_MAIL_SHIRT = {
                "A chainmail shirt. Protects the face, neck, chest,",
                "upper arms and upper legs."},
    ITEM_ARMOR_ROBE = {
                "A cloth robe. Protects the face, neck, chest, arms and legs."},
    ITEM_ARMOR_SHIRT = {
                "A cloth shirt. Protects the neck, chest and arms."},
    ITEM_ARMOR_TOGA = {
                "A cloth toga. Protects the face, neck, chest, upper arms and upper legs."},
    ITEM_ARMOR_TUNIC = {
                "A cloth tunic. Protects the neck, chest and upper arms."},
    ITEM_ARMOR_VEST = {
                "A cloth vest. Protects the chest."},
    ITEM_FOOD_BISCUITS = {
                "Biscuits are the lowest tier of meals that can be prepared by your",
                "dwarves. They are made in a kitchen with the 'Prepare Easy Meal' order",
                "and use two ingredients. Preparing easy meals is the easiest way to,",
                "get experience for you cooks, but the larger volume produced means more",
                "hauling to take them to storage."},
    ITEM_FOOD_ROAST = {
                "Roasts are the highest tier of meals that can be prepared by your ",
                "dwarves. They are made in a kitchen with the 'Prepare Lavish Meal'",
                "order, and use four ingredients. As there are more ingredients, there",
                "is a better chance that a dwarf will like at least one ingredient."},
    ITEM_FOOD_STEW = {
                "Stews are the middle tier of meals that can be prepared by your ",
                "dwarves. They are made in a kitchen with the 'Prepare Fine Meal' order,",
                "and use three ingredients. They provide more food than Biscuits,",
                "but are less valuable than Roasts."},
    ITEM_GLOVES_GAUNTLETS = {
                "Gauntlets are armor worn on any body part that can grasp, which for",
                "dwarves are the hands. They are similar to mittens and gloves, but",
                "act as an armor layer and provide much more protection. Like other",
                "armor, gauntlets can be made of metal, shell, or bone."},
    ITEM_GLOVES_GLOVES = {
                "Gloves cover the hands, wrapping each finger and thumb individually",
                "to preserve the wearer's dexterity at the cost of some warmth"},
    ITEM_GLOVES_MITTENS = {
                "Mittens cover the fingers together and thumb separately, preserving",
                "the ability to grasp but keeping fingers together for more warmth in",
                "cold climates"},
    ITEM_HELM_CAP = {
                "A cap covers only the crown of the head. It prevents heat loss through",
                "a bald pate and protects the skull from falling objects and",
                "downward strikes."},
    ITEM_HELM_HELM = {
                "A helm covers the entire face and head. It protects the wearer from",
                "falling objects and a variety of weapon strikes from all directions.",
                "Every other type of head covering, save a hood, is worn under a helm",
                "for padding."},
    ITEM_HELM_HOOD = {
                "A hood is a soft loose covering for the head and sides of the face.",
                "It shields the wearer from cold breezes and can cushion blows from",
                "any direction. It is pulled over the wearer's other headgear, providing",
                "a final outer layer of protection from the elements."},
    ITEM_HELM_MASK = {
                "A mask hides the wearer's face from view and protects it from all but",
                "the most accurate piercing attacks. Some can be carved to present the",
                "enemy with a more fearsome visage, or to show no face at all.",
                "Masks are worn underneath other layers of headgear."},
    ITEM_HELM_SCARF_HEAD = {
                "A head scarf is a loose wrap of cloth or leather that is typically",
                "worn in hot climates to protect the head from the rays of the sun.",
                "It provides light cushioning against some blows."},
    ITEM_HELM_TURBAN = {
                "A turban is a length of cloth or leather that is wrapped many times",
                "around the head to shield the wearer from the sun's rays and provide",
                "several layers of insulation. A turban can be pinned or clasped in",
                "place, or simply folded and tucked into a stable configuration."},
    ITEM_HELM_VEIL_FACE = {
                "A face veil is a soft covering that protects the lower half of the",
                "wearer's face, leaving only the eyes to gaze out. It can prevent",
                "noxious fluids from splashing into the wearer's mouth.",
                "It is worn under every other layer of headgear."},
    ITEM_HELM_VEIL_HEAD = {
                "A veil for the whole head is a wall of sheer cloth or finely-punched",
                "leather extending from above the brow to below the chin, and often hung",
                "from a more solid cloth headpiece that covers the crown and cheeks.",
                "It admits some light but almost entirely obscures the wearer's face."},
    ITEM_INSTRUMENT_DRUM = {
                "Short, wide, and round, this cylindrical percussion instrument can",
                "play music when banged by one's hands.  It is only useful to trade."},
    ITEM_INSTRUMENT_FLUTE = {
                "This long cylindrical woodwind instrument can make a wide array of",
                "tones and music when blown into.  It is only useful to trade."},
    ITEM_INSTRUMENT_HARP = {
                "Vaguely triangular in shape, this stringed instrument can play a",
                "variety of notes by plucking the strings with one's fingers.",
                "It is only useful to trade."},
    ITEM_INSTRUMENT_PICCOLO = {
                "Similar to a flute, but smaller and with a higher tone, a piccolo is",
                "a cylindrical woodwind instrument.  It is only useful to trade."},
    ITEM_INSTRUMENT_TRUMPET = {
                "A dwarven brass instrument - which need not be made of brass.",
                "It is only useful to trade."},
    ITEM_PANTS_BRAIES = {
                "Braies are undergarments that cover from the waist to the knees.",
                "Dwarves cannot craft braies, so they must be obtained through",
                "other means."},
    ITEM_PANTS_GREAVES = {
                "Greaves are plated armor meant to protect the lower legs, though",
                "they are equipped as pants."},
    ITEM_PANTS_LEGGINGS = {
                "Leggings are garments that cover everything from the waist to the",
                "ankles, though with a tighter fit than other trousers."},
    ITEM_PANTS_LOINCLOTH = {
                "Loincloths are draped undergarments meant to cover little more than",
                "the 'geldables'. Dwarves cannot craft loincloths, so they must be",
                "obtained through other means."},
    ITEM_PANTS_PANTS = {
                "Trousers are a garment that covers everything from the waist to the",
                "ankles. They keep the legs and lower body warm."},
    ITEM_PANTS_SKIRT = {
                "A skirt is a cone-shaped garment that hangs from the waist, covering",
                "part of the legs. Its use is more for modesty than protection.",
                "Dwarves cannot craft skirts, so they must be obtained through other means."},
    ITEM_PANTS_SKIRT_LONG = {
                "A skirt is a cone-shaped garment that hangs from the waist, covering",
                "part of the legs. Its use is more for modesty than protection. Long",
                "skirts fulfil this purpose well. Dwarves cannot craft long skirts,",
                "so they must be obtained through other means."},
    ITEM_PANTS_SKIRT_SHORT = {
                "A skirt is a cone-shaped garment that hangs from the waist, covering",
                "part of the legs. Its use is more for modesty than protection, though",
                "short skirts offer less in the way of modesty. Dwarves cannot craft",
                "short skirts, so they must be obtained through other means."},
    ITEM_PANTS_THONG = {
                "Thongs are strapped undergarments meant to cover little more than",
                "the 'geldables'. Dwarves cannot craft thongs, so they must be obtained",
                "through other means."},
    ITEM_SHIELD_BUCKLER = {
                "A smaller and less protective type of shield.  A buckler can be used",
                "to block attacks, and with skill anything from a goblin axe",
                "to Dragonfire can be deflected."},
    ITEM_SHIELD_SHIELD = {
                "Larger and more defensive than a buckler, a full-sized shield can be",
                "used to block attacks. With skill anything from a goblin axe to",
                "Dragonfire can be deflected."},
    ITEM_SHOES_BOOTS = {
                "Boots are more protective kind of shoe, covering from the foot up to",
                "the knee."},
    ITEM_SHOES_BOOTS_LOW = {
                "Low boots are more protective kind of shoe, covering from the foot up",
                "to just past the ankle."},
    ITEM_SHOES_CHAUSSE = {
                "Chausses are chainmail armor meant to protect the legs, though these",
                "are equipped as footwear. Dwarves cannot craft chausses, so they",
                "must be obtained through other means."},
    ITEM_SHOES_SANDAL = {
                "Sandals are open footwear consisting of soles and some number of",
                "straps. Dwarves cannot craft sandals, so they must be obtained",
                "through other means."},
    ITEM_SHOES_SHOES = {
                "Shoes are closed footwear meant to protect the feet from rough terrain",
                "and the elements."},
    ITEM_SHOES_SOCKS = {
                "Socks are tubular articles of clothing, worn on each foot along with",
                "shoes or other footwear."},
    ITEM_SIEGEAMMO_BALLISTA = {
                "Ballista ammunition, for an enormous siege weapon."},
    ITEM_TOOL_BOWL = {
                "Bowls are used to contain individual servings of meals.",
                "At the moment, dwarves have no use for these."},
    ITEM_TOOL_CAULDRON = {
                "Cauldrons are large metal pots used to cook meals like soups or stews",
                "over an open fire. At the moment, dwarves have no use for these."},
    ITEM_TOOL_FORK_CARVING = {
                "A carving fork typically has only two prongs and is exceptionally long.",
                "It is used to hold down a piece of cooked meat while using a knife."},
    ITEM_TOOL_HIVE = {
                " Hives are structures that house colonies of honey bees. To be",
                "productive, they need to be constructed on an above-ground tile with",
                "an accessible honey bee colony somewhere on the map. Some time after",
                "bees are 'installed' by a beekeeper, the hive will be ready to harvest",
                "or split into new colonies."},
    ITEM_TOOL_HONEYCOMB = {
                "Honeycomb is an intermediate product of beekeeping, produced along",
                "with royal jelly when a beekeeper harvests a suitable hive. It must",
                "be processed by a Presser at a Screw Press to produce honey, which may",
                "be used in cooking or made into mead and a wax cake, which can be used",
                "to make low-value crafts."},
    ITEM_TOOL_JUG = {
                "Jugs are small food storage containers that hold royal jelly, honey,",
                "or oil. They are used by beekeepers when harvesting suitable hives and",
                "by pressers when processing honeycomb or seed pastes at a screw press."},
    ITEM_TOOL_KNIFE_BONING = {
                "A boning knife has a sharp point and narrow blade. It is an excellent",
                "all-around kitchen knife and decent weapon in a pinch."},
    ITEM_TOOL_KNIFE_CARVING = {
                "A carving knife is for cutting thin slices of cooked meat for serving.",
                "It may be useful as an improvised weapon."},
    ITEM_TOOL_KNIFE_MEAT_CLEAVER = {
                "A meat cleaver is a heavy square-bladed knife for cutting bone and",
                "meat alike."},
    ITEM_TOOL_KNIFE_SLICING = {
                "A slicing knife is for cutting thin slices of cooked meat."},
    ITEM_TOOL_LADLE = {
                "A ladle is a large spoon with a long handle and a deep bowl,",
                "intended for serving out portions of soups and stews."},
    ITEM_TOOL_LARGE_POT = {
                "Large pots are storage containers made of any hard material. They",
                "function identically to barrels when brewing or storing food, while",
                "being much lighter than barrels made of the same material.",
                "Unfortunately, they cannot be used when making dwarven syrup or when",
                "building asheries and dyer's workshops."},
    ITEM_TOOL_MINECART = {
                "A minecart is a tool for hauling, and can be made from wood or metal.",
                "",
                "Minecart systems are the most efficient and most complicated way to",
                "move items, and can do anything from improving industrial efficiency,",
                "to transporting magma or launching hundreds of weapons at enemies.",
                "Misuse may result in horrific injury to drivers and pedestrians."},
    ITEM_TOOL_MORTAR = {
                "Half of a mortar and pestle, the mortar is a bowl in which to grind",
                "up plants or other reagents."},
    ITEM_TOOL_NEST_BOX = {
                "A place for birds to lay eggs. Must be built before use.",
                "Forbid eggs to hatch into chicks before a dwarf picks them up."},
    ITEM_TOOL_PESTLE = {
                "Half of a mortar and pestle, the pestle is a stick used to grind up",
                "plants or other reagents."},
    ITEM_TOOL_POUCH = {
                "A small bag used to carry a variety of tools."},
    ITEM_TOOL_STEPLADDER = {
                "A small stepladder. If you have one of these, you can use zones to",
                "assign your dwarves to pick fruit from certain trees."},
    ITEM_TOOL_WHEELBARROW = {
                "A small hand-cart with long handles and a single wheel, this",
                "wheelbarrow makes heavy hauling jobs much more manageable."},
    ITEM_TOY_AXE = {
                "A small toy axe without an edge. Useless except as a trade good."},
    ITEM_TOY_BOAT = {
                "A tiny model of a boat. Only good for trade."},
    ITEM_TOY_HAMMER = {
                "A toy hammer. Its only use is to sell."},
    ITEM_TOY_MINIFORGE = {
                "A model of a blacksmith's forge that dwarf children love.",
                "Only useful as a trade good."},
    ITEM_TOY_PUZZLEBOX = {
                "A perplexing toy that dwarves of all ages enjoy.",
                "Its only use is as a trade good."},
    ITEM_TRAPCOMP_ENORMOUSCORKSCREW = {
                "A massive screw-like object. Can be used to make a pump,",
                "or as a component in a trap."},
    ITEM_TRAPCOMP_GIANTAXEBLADE = {
                "This massive blade is typically made of metal and can be used in weapon",
                "traps, swinging once to slice anyone unfortunate enough to activate it."},
    ITEM_TRAPCOMP_LARGESERRATEDDISC = {
                "Serrated discs are typically made of metal and can be used in weapon",
                "traps, in which they eviscerate its victims with three powerful slicing",
                "attacks. Such traps have a tendency to sever multiple body parts and",
                "make a gigantic mess."},
    ITEM_TRAPCOMP_MENACINGSPIKE = {
                "Menacing spikes are made of wood or metal and can be used in weapon",
                "traps or upright spike traps, in which they impale the victim. They",
                "are especially effective against unarmored foes or, in an upright",
                "spike trap, anyone falling from great heights."},
    ITEM_TRAPCOMP_SPIKEDBALL = {
                "This trap component menaces with spikes of wood or metal. It hits three",
                "times with its spikes, but does not penetrate as deeply as a menacing",
                "spike. Compared to other trap components, spiked balls are slightly",
                "more effective against heavily armored foes. They also make for a",
                "surprisingly valuable trade good, on par with serrated discs."},
    ITEM_WEAPON_AXE_BATTLE = {
                "A battle axe is an edged weapon: essentially a sharp blade",
                "mounted along the end of a short and heavy handle.",
                "",
                "Dwarves can forge battle axes out of any weapon-grade metal,",
                "though those with superior edge properties are more effective.",
                "",
                "A battle axe may also be used as a tool for chopping down trees."},
    ITEM_WEAPON_AXE_GREAT = {
                "This is an axe nearly twice as large as a battle axe. Its size",
                "makes it unsuitable for a dwarf, but those who can wield it find",
                "its increased size and weight contribute to its effectiveness."},
    ITEM_WEAPON_AXE_TRAINING = {
                "As a battleaxe made from wood, this practise weapon is useful for",
                "training recruits.  Thanks to good craftsdwarfship, it can also",
                "be used to cut down trees."},
    ITEM_WEAPON_BLOWGUN = {
                "A very simple ranged weapon: blow into one end of the long narrow",
                "tube, and project a pellet or dart into the body of one's prey.",
                "If the prey approaches, this blowgun makes a useless melee weapon."},
    ITEM_WEAPON_BOW = {
                "Bows are the preferred ranged weapon for elves and goblins, and",
                "shoot arrows as projectiles. As they are a foreign weapon, they",
                "cannot be made in your fort. In melee, bowmen will use their bow as",
                "a weapon, training the swordsman skill."},
    ITEM_WEAPON_CROSSBOW = {
                "The favoured ranged weapon of choice for any dwarf, crossbows can be",
                "made of wood, bones or metal, and shoot bolts as projectiles. Hunters",
                "or marks-dwarves that run out of ammunition will use their crossbow",
                "as a melee weapon, training the hammerdwarf skill."},
    ITEM_WEAPON_DAGGER_LARGE = {
                "A large dagger is a edge weapon that is essentially just a bit smaller",
                "than a short sword. It's used for stabbing rather than slashing. Large",
                "daggers use and train the knife user skill, and are common weapons for",
                "kobold and goblin thieves. As foreign weapons dwarves cannot forge",
                "large daggers."},
    ITEM_WEAPON_FLAIL = {
                "A flail is a blunt weapon that consists of a rounded weight attached to",
                "a handle by a length of chain. Flails are the same size as a morningstar,",
                "but have a contact area twice as large as a much larger maul. As foreign",
                "weapons dwarves cannot forge flails. Flails use and train the",
                "macedwarf skill."},
    ITEM_WEAPON_HALBERD = {
                "A halberd is a foreign weapon, and cannot be made by your dwarves.",
                "Even the largest and strongest dwarves cannot use a halberd, making",
                "them useless in military terms. They can however be placed in a weapon",
                "trap or melted down to provide metal bars, redeeming them."},
    ITEM_WEAPON_HAMMER_WAR = {
                "A war hammer is a blunt weapon that is essentially a hammer with a long",
                "handle. War hammers use and train the hammerdwarf skill. Dwarves can",
                "forge war hammers out of any weapons-grade metal, though those with",
                "higher densities tend to cause more damage."},
    ITEM_WEAPON_MACE = {
                "A mace is a blunt weapon that consists of a rounded or flanged weight",
                "mounted on the end of a handle. Despite similarities to a morningstar",
                "in appearance, a mace is 60% larger and has twice the contact area.",
                "Maces use and train the macedwarf skill. Dwarves can forge maces out of",
                "any weapons-grade metal, though those with higher densities",
                "(like silver) cause more damage."},
    ITEM_WEAPON_MAUL = {
                "A maul is a blunt weapon that is essentially a very large war hammer,",
                "similar to a sledgehammer. Mauls are more than three times larger than",
                "standard war hammers, with a similar 'bash' attack. Mauls also have",
                "ten times the contact area and greatly reduced penetration, which",
                "reduces their effectiveness. Mauls use and train the hammerdwarf",
                "skill. Being foreign weapons, dwarves cannot forge mauls."},
    ITEM_WEAPON_MORNINGSTAR = {
                "A morningstar is an edged weapon that consists of a spiked ball mounted",
                "on the end of a handle. Despite similarities to a mace in appearance,",
                "a morningstar's size and contact area are closer to those of a war",
                "hammer. Specifically, a morningstar is 25% larger than a war hammer",
                "with the same contact area, and uses piercing damage to inflict internal",
                "injuries. Morningstars use and train the macedwarf skill."},
    ITEM_WEAPON_PICK = {
                "The most important item for a beginning fortress, a pick can",
                "get a party underground.  Also crucial mining for stone or",
                "metals, expansion of living space, and so on.",
                "",
                "A pick is also useful as a weapon, though putting miners in the",
                "military causes equipment clashes."},
    ITEM_WEAPON_PIKE = {
                "A pike is a weapon that is essentially a very long spear.",
                "Pikes use and train the Pikedwarf skill. As foreign weapons,",
                "dwarves cannot forge pikes."},
    ITEM_WEAPON_SCIMITAR = {
                "A scimitar is an edged weapon with a curved blade that is very similar",
                "to a short sword. Scimitars use and train the swordsdwarf skill.",
                "As foreign weapons dwarves cannot forge scimitars."},
    ITEM_WEAPON_SCOURGE = {
                "A scourge is an edge weapon that consists of a spike or bladed weight",
                "on the end of a flexible length of material that can be swung at",
                "high speed. Scourges are similar to whips, though the whip is a blunt",
                "weapon with an even smaller contact area.Scourges use and train the",
                "lasher skill. As foreign weapons dwarves cannot forge scourges."},
    ITEM_WEAPON_SPEAR = {
                "A pole weapon consisting of a shaft, usually of wood, with a pointed",
                "head made of metal or just the sharpened end of the shaft itself.",
                "With the ability to pin opponents, spears are most effective with axe",
                "or macedwarves for combo attacks. "},
    ITEM_WEAPON_SPEAR_TRAINING = {
                "A wooden training spear, this has no sharp edges and thus presents",
                "little risk of injury. Military dwarves can become",
                "attached to them, and refuse to swap them for weapons that cause",
                "actual injury to your enemies."},
    ITEM_WEAPON_SWORD_2H = {
                "An enormous sword taller than many humans. Victims may be split in",
                "two by a single blow, though no dwarf is large enough to wield a",
                "greatsword and do so.  As foreign weapons, dwarves cannot forge them."},
    ITEM_WEAPON_SWORD_LONG = {
                "A longsword is a classic weapon, consisting of a short handle and a",
                "long sharp blade. Most dwarves are large enough to use a longsword,",
                "but as foreign weapons cannot forge them."},
    ITEM_WEAPON_SWORD_SHORT = {
                "A sword just the right size for dwarves, though small dwarves may",
                "need both hands. Shortswords can be made from metal at a forge."},
    ITEM_WEAPON_SWORD_SHORT_TRAINING = {
                "A wooden training sword, this has no sharp edges and thus presents",
                "little risk of injury. Military dwarves can become",
                "attached to them, and refuse to swap them for weapons that cause",
                "actual injury to your enemies."},
    ITEM_WEAPON_WHIP = {
                "A highly effective weapon known to cause large amounts of pain.",
                "It cannot be forged by dwarves."},
    MEAT = {    "Butchering an animal gives meat, the amount depending on the size",
                "of the butchered animal. Along with plants, meat is the",
                "backbone of every food industry."},
    MILLSTONE = {
                "A large grinding stone, used in a mill to produce flour, sugar, and",
                "dyes much faster than a quern. It is too large to be operated by hand,",
                "and must be powered for operation.  Millstones are made of stone."},
    ORTHOPEDIC_CAST = {
                "Casts are made from plaster, and are used to keep broken bones in",
                "place until they are healed. Applying a cast requires a bucket,",
                "cloth and a water source."},
    PIPE_SECTION = {
                "An enormous piece of pipe, it is a part of a screw pump."},
    QUERN = {   "A hand-operated mill for plants, grains, and seeds. It mills plants",
                "much slower than a millstone. Must be built from stone."},
    QUIVER = {  "Item used to hold ammunition, made out of leather. Hunting dwarves",
                "and crossbow dwarves will automatically grab one to store their ammo."},
    RING = {    "A ring is an item of jewellery, which does not interfere with",
                "wearing other equipment.  Eleven rings can be worn on each finger",
                "or toe, for a maximum of 220 rings."},
    ROCK = {    "A small rock, sharpened as a weapon in Adventure mode."},
    ROUGH = {   "Rough gemstones and raw glass are cut by a Gem Cutter at a Jeweler's",
                "Workshop into small decorative gems. Sometimes, the gem-cutting job",
                "results in a craft or large gem that is useless except as a very",
                "valuable trade good."},
    SCEPTER = { "A scepter is a short, ornamental rod or wand typically associated",
                "with royalty. It's only use is as a trade good."},
    SKIN_TANNED = {
                "The tanned hide of animals is flexible enough to be made into an",
                "assortment of goods for military and civilian use. Leather can also",
                "be used to decorate items with sewn images at a Leather Works. Armor",
                "and shields made from leather are not terribly effective, but are",
                "still better than nothing at all."},
    SLAB = {    "A memorial stone, used to quiet restless ghost when engraved with",
                "the name of the deceased and built."},
    SMALLGEM = {"Cut gemstones and the odd gizzard stone (a product of butchering",
                "certain species of animals) are used by a Gem Setter to decorate items",
                "at a Jeweler's Workshop."},
    SPLINT = {  "Splints are used to immobilise fractured limbs. They are made out of",
                "wood or metal, and allow dwarves to leave the hospital and continue",
                "their normal jobs. Splints are applied with the bonedoctor skill."},
    STATUE = {  "A large piece of art carved in the likeness of a creature. Statues",
                "can be installed on any open floor space, but cannot share the space",
                "with creatures. Statues can be used as the focal point of a",
                "recreational statue garden.  Dwarves will admire or revile as they",
                "pass, depending on the statue and the individual's preferences."},
    TABLE = {   "A flat-topped piece of furniture useful as a work-surface for a",
                "scribe or a dining-surface for a hungry dwarf. Typically found in",
                "shops, dinning rooms, and offices. Dining rooms may be designated and",
                "assigned from them, though dwarves will complain if there are too few."},
    THREAD = {  "A small bundle of processed material, ready to be woven into cloth.",
                "Thread made from animal hair will not be used to make cloth. Thread",
                "can also be used by doctors to sew wounds shut.  It is sourced from",
                "shearing, plant processing, trade, or web gathering. It can be dyed",
                "for additional value before being woven."},
    TOTEM = {   "A carved and polished skull."},
    TRACTION_BENCH = {
                "A special hospital bed made to secure dwarves with complex or",
                "overlapping fractures until healed. Patients may need several months",
                "or more in a traction bench to heal. Constructed from a table,",
                "a mechanism, and a rope or chain."},
    TRAPPARTS = {
                "Used to build traps, levers and other machines."},
    WEAPONRACK = {
                "Furniture used for training. Barracks may be designated and assigned",
                "from them. Military squads may use their assigned barracks for",
                "training, storage, or sleeping, depending on the settings."},
    WINDOW = {  "Furniture used for ambiance. Either made in a glass furnace from glass",
                "or built on site using three cut gems. While it is treated as a wall,",
                "it does not support constructions. Passing dwarves will admire them."},
    WOOD = {    "A porous and fibrous structural tissue found in the stems and roots",
                "of trees and underground fungus. Wood is renewable and essential for",
                "numerous industries. It can be made into charcoal, ash for further",
                "processing, furniture, crafts, tools, some trap components, training",
                "gear, and (ineffective) weapons and armor. Elves take serious offence",
                "when wood or wooden items are offered in trade."}
}
