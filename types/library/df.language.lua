-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@alias language_word_flags bitfield

---@enum part_of_speech
df.part_of_speech = {
    Noun = 0,
    NounPlural = 1,
    Adjective = 2,
    Prefix = 3,
    Verb = 4,
    Verb3rdPerson = 5,
    VerbPast = 6,
    VerbPassive = 7,
    VerbGerund = 8,
}

---@enum language_word_table_index
df.language_word_table_index = {
    FrontCompound = 0,
    RearCompound = 1,
    FirstName = 2,
    Adjectives = 3,
    TheX = 4,
    OfX = 5,
}

---@enum sphere_type
df.sphere_type = {
    NONE = -1,
    AGRICULTURE = 0,
    ANIMALS = 1,
    ART = 2,
    BALANCE = 3,
    BEAUTY = 4,
    BIRTH = 5,
    BLIGHT = 6,
    BOUNDARIES = 7,
    CAVERNS = 8,
    CHAOS = 9,
    CHARITY = 10,
    CHILDREN = 11,
    COASTS = 12,
    CONSOLATION = 13,
    COURAGE = 14,
    CRAFTS = 15,
    CREATION = 16,
    DANCE = 17,
    DARKNESS = 18,
    DAWN = 19,
    DAY = 20,
    DEATH = 21,
    DEFORMITY = 22,
    DEPRAVITY = 23,
    DISCIPLINE = 24,
    DISEASE = 25,
    DREAMS = 26,
    DUSK = 27,
    DUTY = 28,
    EARTH = 29,
    FAMILY = 30,
    FAME = 31,
    FATE = 32,
    FERTILITY = 33,
    FESTIVALS = 34,
    FIRE = 35,
    FISH = 36,
    FISHING = 37,
    FOOD = 38,
    FORGIVENESS = 39,
    FORTRESSES = 40,
    FREEDOM = 41,
    GAMBLING = 42,
    GAMES = 43,
    GENEROSITY = 44,
    HAPPINESS = 45,
    HEALING = 46,
    HOSPITALITY = 47,
    HUNTING = 48,
    INSPIRATION = 49,
    JEALOUSY = 50,
    JEWELS = 51,
    JUSTICE = 52,
    LABOR = 53,
    LAKES = 54,
    LAWS = 55,
    LIES = 56,
    LIGHT = 57,
    LIGHTNING = 58,
    LONGEVITY = 59,
    LOVE = 60,
    LOYALTY = 61,
    LUCK = 62,
    LUST = 63,
    MARRIAGE = 64,
    MERCY = 65,
    METALS = 66,
    MINERALS = 67,
    MISERY = 68,
    MIST = 69,
    MOON = 70,
    MOUNTAINS = 71,
    MUCK = 72,
    MURDER = 73,
    MUSIC = 74,
    NATURE = 75,
    NIGHT = 76,
    NIGHTMARES = 77,
    OATHS = 78,
    OCEANS = 79,
    ORDER = 80,
    PAINTING = 81,
    PEACE = 82,
    PERSUASION = 83,
    PLANTS = 84,
    POETRY = 85,
    PREGNANCY = 86,
    RAIN = 87,
    RAINBOWS = 88,
    REBIRTH = 89,
    REVELRY = 90,
    REVENGE = 91,
    RIVERS = 92,
    RULERSHIP = 93,
    RUMORS = 94,
    SACRIFICE = 95,
    SALT = 96,
    SCHOLARSHIP = 97,
    SEASONS = 98,
    SILENCE = 99,
    SKY = 100,
    SONG = 101,
    SPEECH = 102,
    STARS = 103,
    STORMS = 104,
    STRENGTH = 105,
    SUICIDE = 106,
    SUN = 107,
    THEFT = 108,
    THRALLDOM = 109,
    THUNDER = 110,
    TORTURE = 111,
    TRADE = 112,
    TRAVELERS = 113,
    TREACHERY = 114,
    TREES = 115,
    TRICKERY = 116,
    TRUTH = 117,
    TWILIGHT = 118,
    VALOR = 119,
    VICTORY = 120,
    VOLCANOS = 121,
    WAR = 122,
    WATER = 123,
    WEALTH = 124,
    WEATHER = 125,
    WIND = 126,
    WISDOM = 127,
    WRITING = 128,
    YOUTH = 129,
}

---@class language_word
---@field word string
---@field forms string[]
---@field adj_dist integer
---@field pad_1 integer looks like garbage
---@field flags bitfield
---@field str string[] since v0.40.01
df.language_word = nil

---@class language_translation
---@field name string
---@field unknown1 string[] looks like english words
---@field unknown2 string[] looks like translated words
---@field words string[]
---@field flags integer 1 = generated, since v0.40.01
---@field str string[] since v0.40.01
df.language_translation = nil

---@class language_symbol
---@field name string
---@field unknown any[] empty
---@field words any[]
---@field flags integer since v0.40.01
---@field str string[] since v0.40.01
df.language_symbol = nil

---@class language_name
---@field first_name string
---@field nickname string
---@field words any[]
---@field parts_of_speech any[]
---@field language integer
---@field type language_name_type
---@field has_name boolean
df.language_name = nil

---@class language_word_table word_selectorst
---@field words any[]
---@field parts any[]
df.language_word_table = nil

---@enum language_name_category
df.language_name_category = {
    Unit = 0,
    Artifact = 1,
    ArtifactEvil = 2,
    Swamp = 3,
    Desert = 4,
    Forest = 5,
    Mountains = 6,
    Lake = 7,
    Ocean = 8,
    Glacier = 9,
    Tundra = 10,
    Grassland = 11,
    Hills = 12,
    Region = 13,
    Cave = 14,
    SwampEvil = 15,
    DesertEvil = 16,
    ForestEvil = 17,
    MountainsEvil = 18,
    LakeEvil = 19,
    OceanEvil = 20,
    GlacierEvil = 21,
    TundraEvil = 22,
    GrasslandEvil = 23,
    HillsEvil = 24,
    SwampGood = 25,
    DesertGood = 26,
    ForestGood = 27,
    MountainsGood = 28,
    LakeGood = 29,
    OceanGood = 30,
    GlacierGood = 31,
    TundraGood = 32,
    GrasslandGood = 33,
    HillsGood = 34,
    ArtImage = 35,
    MountainPeak = 36,
    River = 37,
    Volcano = 38,
    SmallIsland = 39,
    Island = 40,
    Continent = 41,
    CommonReligion = 42,
    Temple = 43,
    Keep = 44,
    Unknown2 = 45,
    SymbolArtifice = 46,
    SymbolViolent = 47,
    SymbolProtect = 48,
    SymbolDomestic = 49,
    SymbolFood = 50,
    War = 51,
    Battle = 52,
    Siege = 53,
    Road = 54,
    Wall = 55,
    Bridge = 56,
    Tunnel = 57,
    Tomb = 58,
    SymbolProtect2 = 59,
    Library = 60,
    Festival = 61,
    EntityMerchantCompany = 62,
    CountingHouse = 63,
    EntityMerchantCompany2 = 64,
    Guildhall = 65,
    NecromancerTower = 66,
    Hospital = 67,
}

---@enum language_name_type
df.language_name_type = {
    NONE = -1,
    Figure = 0,
    Artifact = 1,
    Civilization = 2,
    Squad = 3,
    Site = 4,
    World = 5,
    Region = 6,
    Dungeon = 7,
    LegendaryFigure = 8,
    FigureNoFirst = 9,
    FigureFirstOnly = 10,
    ArtImage = 11,
    AdventuringGroup = 12,
    ElfTree = 13,
    SiteGovernment = 14,
    NomadicGroup = 15,
    Vessel = 16,
    MilitaryUnit = 17,
    Religion = 18,
    MountainPeak = 19,
    River = 20,
    Temple = 21,
    Keep = 22,
    MeadHall = 23,
    SymbolArtifice = 24,
    SymbolViolent = 25,
    SymbolProtect = 26,
    SymbolDomestic = 27, -- Market
    SymbolFood = 28, -- Tavern
    War = 29,
    Battle = 30,
    Siege = 31,
    Road = 32,
    Wall = 33,
    Bridge = 34,
    Tunnel = 35,
    PretentiousEntityPosition = 36,
    Monument = 37,
    Tomb = 38,
    OutcastGroup = 39,
    TrueName = 40, -- vault slabs
    SymbolProtect2 = 41,
    PerformanceTroupe = 42,
    Library = 43,
    PoeticForm = 44,
    MusicalForm = 45,
    DanceForm = 46,
    Festival = 47,
    FalseIdentity = 48,
    MerchantCompany = 49,
    CountingHouse = 50,
    CraftGuild = 51,
    Guildhall = 52,
    NecromancerTower = 53,
    Hospital = 54, -- since v0.50.01
}

---@enum language_name_component
df.language_name_component = {
    FrontCompound = 0,
    RearCompound = 1,
    FirstAdjective = 2,
    SecondAdjective = 3,
    HyphenCompound = 4,
    TheX = 5,
    OfX = 6,
}


