-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum art_image_element_type
df.art_image_element_type = {
    CREATURE = 0,
    PLANT = 1,
    TREE = 2,
    SHAPE = 3,
    ITEM = 4,
}

---@param file file_compressorst
function df.art_image_element.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.art_image_element.read_file(file, loadversion) end

---@return art_image_element_type
function df.art_image_element.getType() end

---@param ID integer
function df.art_image_element.setID(ID) end

---@return art_image_element
function df.art_image_element.clone() end

---@param sym integer
---@param arg_1 integer
function df.art_image_element.getSymbol(sym, arg_1) end

---@param name string
---@param useThe boolean
---@param useName boolean
function df.art_image_element.getName1(name, useThe, useName) end

---@param name string
---@param arg_1 boolean
function df.art_image_element.getName2(name, arg_1) end

function df.art_image_element.markDiscovered() end

---@param colors integer
---@param shapes integer
function df.art_image_element.getColorAndShape(colors, shapes) end

---@class art_image_element
---@field count integer
df.art_image_element = nil

---@class art_image_element_creaturest
-- inherited from art_image_element
---@field count integer
-- end art_image_element
---@field race integer
---@field caste integer
---@field histfig integer
df.art_image_element_creaturest = nil

---@class art_image_element_plantst
-- inherited from art_image_element
---@field count integer
-- end art_image_element
---@field plant_id integer
df.art_image_element_plantst = nil

---@class art_image_element_treest
-- inherited from art_image_element
---@field count integer
-- end art_image_element
---@field plant_id integer
df.art_image_element_treest = nil

---@class art_image_element_shapest
-- inherited from art_image_element
---@field count integer
-- end art_image_element
---@field shape_id integer
---@field shape_adj integer
df.art_image_element_shapest = nil

---@class art_image_element_itemst
-- inherited from art_image_element
---@field count integer
-- end art_image_element
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field flags bitfield
---@field item_id integer
df.art_image_element_itemst = nil

---@enum art_image_property_type
df.art_image_property_type = {
    transitive_verb = 0,
    intransitive_verb = 1,
}

---@param file file_compressorst
function df.art_image_property.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.art_image_property.read_file(file, loadversion) end

---@return art_image_property_type
function df.art_image_property.getType() end

---@return art_image_element
function df.art_image_property.clone() end

---@param arg_0 string
---@param arg_1 art_image
---@param useName boolean
function df.art_image_property.getName(arg_0, arg_1, useName) end

---@class art_image_property
---@field flags any[]
df.art_image_property = nil

---@enum art_image_property_verb
df.art_image_property_verb = {
    Withering = 0,
    SurroundedBy = 1,
    Massacring = 2,
    Fighting = 3,
    Laboring = 4,
    Greeting = 5,
    Refusing = 6,
    Speaking = 7,
    Embracing = 8,
    StrikingDown = 9,
    MenacingPose = 10,
    Traveling = 11,
    Raising = 12,
    Hiding = 13,
    LookingConfused = 14,
    LookingTerrified = 15,
    Devouring = 16,
    Admiring = 17,
    Burning = 18,
    Weeping = 19,
    LookingDejected = 20,
    Cringing = 21,
    Screaming = 22,
    SubmissiveGesture = 23,
    FetalPosition = 24,
    SmearedIntoSpiral = 25,
    Falling = 26,
    Dead = 27,
    Laughing = 28,
    LookingOffended = 29,
    BeingShot = 30,
    PlaintiveGesture = 31,
    Melting = 32,
    Shooting = 33,
    Torturing = 34,
    CommittingDepravedAct = 35,
    Praying = 36,
    Contemplating = 37,
    Cooking = 38,
    Engraving = 39,
    Prostrating = 40,
    Suffering = 41,
    BeingImpaled = 42,
    BeingContorted = 43,
    BeingFlayed = 44,
    HangingFrom = 45,
    BeingMutilated = 46,
    TriumphantPose = 47,
}

---@class art_image_property_transitive_verbst
-- inherited from art_image_property
---@field flags any[]
-- end art_image_property
---@field subject integer
---@field object integer
---@field verb art_image_property_verb
df.art_image_property_transitive_verbst = nil

---@class art_image_property_intransitive_verbst
-- inherited from art_image_property
---@field flags any[]
-- end art_image_property
---@field subject integer
---@field verb art_image_property_verb
df.art_image_property_intransitive_verbst = nil

---@enum art_facet_type
df.art_facet_type = {
    OWN_RACE = 0,
    FANCIFUL = 1,
    GOOD = 2,
    EVIL = 3,
}

---@class art_image
---@field elements art_image_element[]
---@field properties art_image_property[]
---@field event integer
---@field name language_name
---@field spec_ref_type specific_ref_type
---@field mat_type integer
---@field mat_index integer
---@field quality item_quality
---@field artist integer
---@field site integer
---@field ref general_ref
---@field year integer
---@field unk_1 integer
---@field id integer
---@field subid integer
df.art_image = nil

---@class art_image_chunk
---@field id integer art_image_*.dat
---@field images any[]
df.art_image_chunk = nil

---@class art_image_ref
---@field id integer
---@field subid integer
---@field civ_id integer since v0.34.01
---@field site_id integer since v0.34.01
df.art_image_ref = nil

---@enum poetic_form_action
df.poetic_form_action = {
    None = -1,
    Describe = 0,
    Satirize = 1,
    AmuseAudience = 2,
    Complain = 3,
    Renounce = 4,
    MakeApology = 5,
    ExpressPleasure = 6,
    ExpressGrief = 7,
    Praise = 8,
    TeachMoralLesson = 9,
    MakeAssertion = 10,
    MakeCounterAssertion = 11,
    MakeConsession = 12,
    SynthesizePreviousIdeas = 13,
    DevelopPreviousIdea = 14,
    InvertTheAssertion = 15,
    UndercutAssertion = 16,
    MoveAwayFromPreviousIdeas = 17,
    ReflectPreviousIdeas = 18,
    ConsoleAudience = 19,
    RefuseConsolation = 20,
    OfferDifferentPerspective = 21,
    Beseech = 22, -- since v0.47.01
}

---@enum poetic_form_pattern
df.poetic_form_pattern = {
    None = -1,
    AA = 0,
    AB = 1,
    BA = 2,
    BB = 3,
    AAA = 4,
    BAA = 5,
    ABA = 6,
    AAB = 7,
    ABB = 8,
    BBA = 9,
    BAB = 10,
    BBB = 11,
}

---@enum poetic_form_caesura_position
df.poetic_form_caesura_position = {
    None = -1,
    Initial = 0,
    Medial = 1,
    Terminal = 2,
}

---@enum poetic_form_mood
df.poetic_form_mood = {
    None = -1,
    Narrative = 0,
    Dramatic = 1,
    Reflective = 2,
    Riddle = 3,
    Ribald = 4,
    Light = 5,
    Solemn = 6,
}

---@enum poetic_form_subject
df.poetic_form_subject = {
    None = -1,
    Past = 0,
    CurrentEvents = 1,
    Future = 2,
    SomeoneRecentlyDeceased = 3,
    SomeoneRecentlyRetired = 4,
    Religion = 5,
    SpecificPlace = 6,
    SpecificWildernessRegion = 7,
    Nature = 8,
    Lover = 9,
    Family = 10,
    AlcoholicBeverages = 11,
    Journey = 12,
    War = 13,
    Hunt = 14,
    Mining = 15,
    Death = 16,
    Immortality = 17,
    SomeonesCharacter = 18,
    Histfig = 19,
    Concept = 20,
}

---@class poetic_form_subject_target_Concept
---@field subject_topic sphere_type

---@class poetic_form_subject_target_Histfig
---@field subject_histfig integer

---@class poetic_form_subject_target
---@field Histfig poetic_form_subject_target_Histfig
---@field Concept poetic_form_subject_target_Concept
df.poetic_form_subject_target = nil

---@alias poetic_form_feature bitfield

---@enum poetic_form_additional_feature
df.poetic_form_additional_feature = {
    SharesUnderlyingMeaning = 0,
    ContrastsUnderlyingMeaning = 1,
    RequiredToMaintainPhrasing = 2,
    SameGrammaticalStructure = 3,
    SamePlacementOfAllusions = 4,
    ReverseWordOrder = 5,
    ReverseGrammaticalStructure = 6,
    PresentsDifferentView = 7,
    MustExpandIdea = 8,
}

---@class poetic_form
---@field id integer
---@field name language_name
---@field originating_entity integer
---@field original_author integer
---@field subject_hf integer
---@field flags bitfield
---@field parts poetic_form_part[]
---@field each_line_feet integer
---@field each_line_pattern poetic_form_pattern
---@field every_line_caesura_position poetic_form_caesura_position
---@field common_features any[]
---@field mood poetic_form_mood
---@field subject poetic_form_subject
---@field subject_target poetic_form_subject_target
---@field action poetic_form_action
---@field preferred_perspective integer if not -1, ALWAYS written from that perspective
---@field features bitfield
---@field perspectives poetic_form_perspective[]
df.poetic_form = nil

---@class poetic_form_part
---@field flags bitfield
---@field count_min integer
---@field count_max integer
---@field size integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field line_endings integer[]
---@field line_feet integer[]
---@field line_patterns any[]
---@field line_caesura_positions any[]
---@field line_features any[]
---@field additional_features any[]
---@field additional_targets integer[]
---@field additional_lines integer[]
---@field line_mood any[]
---@field line_subject any[]
---@field line_subject_target poetic_form_subject_target[]
---@field line_action any[]
---@field unk_5 integer[]
---@field some_lines_syllables integer
---@field some_lines_pattern integer
---@field each_line_caesura_position poetic_form_caesura_position
---@field certain_lines_additional_features any[]
---@field mood poetic_form_mood
---@field unk_6 integer
---@field unk_7 integer
---@field action poetic_form_action
---@field unk_8 integer
---@field unk_9 integer
df.poetic_form_part = nil

---@enum poetic_form_perspective_type
df.poetic_form_perspective_type = {
    Author = 0,
    Soldier = 1,
    Traveller = 2,
    RelativeOfAuthor = 3,
    PartyOfDebate = 4,
    FictionalPoet = 5,
    Histfig = 6,
    Animal = 7,
}
---@class poetic_form_perspective
---@field poetic_form_perspective_type poetic_form_perspective_type
---@field histfig integer
---@field unk_1 integer
df.poetic_form_perspective = nil

---@enum musical_form_purpose
df.musical_form_purpose = {
    Entertainment = 0,
    Commemoration = 1,
    Devotion = 2,
    Military = 3,
}

---@enum musical_form_style
df.musical_form_style = {
    None = -1,
    FreeTempo = 0,
    VerySlow = 1,
    Slow = 2,
    WalkingPace = 3,
    ModeratelyPaced = 4,
    ModeratelyFast = 5,
    Fast = 6,
    VeryFast = 7,
    ExtremelyFast = 8,
    DoubleTempo = 9, -- twice the tempo of the last passage
    HalfTempo = 10, -- half the tempo of the previous passage
    Faster = 11, -- more quickly than the last passage
    Slower = 12, -- slower than the last passage
    ResumeTempo = 13, -- resumes the previous tempo
    OriginalTempo = 14, -- resumes the original tempo
    Accelerates = 15,
    SlowsAndBroadens = 16,
    ConsistentlySlowing = 17,
    HurriedPace = 18,
    GraduallySlowsAtEnd = 19,
    WhisperedUndertones = 20,
    VerySoft = 21,
    Soft = 22,
    ModeratelySoft = 23,
    ModeratelyLoud = 24,
    Loud = 25,
    VeryLoud = 26,
    BecomeLouderAndLouder = 27,
    BecomeSofterAndSofter = 28,
    FadeIntoSilence = 29,
    StartLoudThenImmediatelySoft = 30,
    SlowsAndDiesAwayAtEnd = 31,
    BecomesCalmerAtEnd = 32,
    BecomesFrenzied = 33,
    StressRhythm = 34,
    BeStately = 35,
    BeBright = 36,
    BeLively = 37,
    BeSkilled = 38,
    BeVigorous = 39,
    BeSpirited = 40,
    BeDelicate = 41,
    BeFiery = 42,
    BringSenseOfMotion = 43,
    BeFiery2 = 44,
    WithFeeling = 45, -- since v0.47.01
    FeelAgitated = 46,
    BePassionate = 47,
    Sparkle = 48,
    BeBroad = 49,
    BeMadeSweetly = 50,
    BeStrong = 51,
    BeEnergetic = 52,
    BeForceful = 53,
    FeelHeroic = 54,
    BeMadeExpressively = 55,
    FeelFurious = 56,
    BeJoyful = 57,
    BeGrand = 58,
    BeMerry = 59,
    BeGraceful = 60,
    BuildAsItProceeds = 61,
    EvokeTears = 62,
    BeMelancholic = 63,
    FeelMournful = 64,
    BeMadeWithLightTouch = 65,
    FeelHeavy = 66,
    FeelMysterious = 67,
    BeJumpy = 68,
    FeelPlayful = 69,
    FeelTender = 70,
    FeelCalm = 71,
    BeTriumphant = 72,
}

---@enum musical_form_pitch_style
df.musical_form_pitch_style = {
    None = -1,
    SinglePitchesOnly = 0,
    IntervalsOnly = 1,
    SparseChords = 2,
    PitchClusters = 3,
    ChordLayers = 4,
}

---@alias musical_form_feature bitfield

---@enum musical_form_passage_component_type
df.musical_form_passage_component_type = {
    Melody = 0,
    Counterpoint = 1,
    Harmony = 2,
    Rhythm = 3,
    Unspecified = 4,
}

---@enum musical_form_passage_type
df.musical_form_passage_type = {
    Unrelated = 0,
    Introduction = 1,
    Exposition = 2,
    Recapitulation = 3,
    Synthesis = 4,
    Verse = 5,
    Chorus = 6,
    Finale = 7,
    Coda = 8,
    BridgePassage = 9,
    Theme = 10,
    Variation = 11,
}

---@enum musical_form_passage_length_type
df.musical_form_passage_length_type = {
    NONE = -1,
    Short = 0,
    MidLength = 1,
    Long = 2,
    Varied = 3,
}

---@enum musical_form_melody_style
df.musical_form_melody_style = {
    Rising = 0,
    Falling = 1,
    RisingFalling = 2,
    FallingRising = 3,
}

---@enum musical_form_melody_frequency
df.musical_form_melody_frequency = {
    Always = 0,
    Often = 1,
    Sometimes = 2,
}

---@class musical_form_interval
---@field degree integer
---@field flags bitfield
df.musical_form_interval = nil

---@class musical_form_melodies
---@field style musical_form_melody_style
---@field frequency musical_form_melody_frequency
---@field intervals musical_form_interval[]
---@field features bitfield
df.musical_form_melodies = nil

---@class musical_form_passage
---@field type musical_form_passage_type
---@field passage_reference integer used when doing Exposition, Recapitualation, Synthesis, and Variation
---@field passage_range_end integer when doing Synthesis of a range of passages
---@field unk_4 integer 'min_times' for a 3-5 range, but doesn't match up with 1 for both repeat 2 times and no repeat mentioned
---@field unk_5 integer 'max_times' for a 3-5 range, but doesn't match up with 1 for both repeat 2 times and no repeat mentioned
---@field poetic_form_id integer
---@field written_content_id integer suspect bug in exported legends (and possibly DF itself) as no mentioning of the poems (or any alternative) referenced here were mentioned in the two entries examined
---@field scale_id integer
---@field scale_sub_id integer references the scales element of the scale
---@field rhythm_id integer
---@field sub_rhythm integer Guess, based on the pattern above
---@field rhythm_pattern integer references the patterns element of rhythm
---@field instruments integer[] indices into the instruments vector
---@field components any[]
---@field passage_lengths any[]
---@field lowest_register_range integer[] 0-3 seen. Probably indices into the registers of the instruments referenced. Found no field for timbre description, though
---@field highest_register_range integer[] 0-3 seen. Probably indices into the registers of the instruments referenced. Found no field for timbre description, though
---@field tempo_style musical_form_style
---@field dynamic_style musical_form_style
---@field overall_style musical_form_style
---@field features bitfield
---@field pitch_style musical_form_pitch_style
---@field melodies musical_form_melodies[]
---@field unk_22 integer 0-40 seen
---@field unk_23 integer 0-78 seen
df.musical_form_passage = nil

---@class musical_form_instruments
---@field instrument_subtype integer -1 = vocal
---@field substitutions bitfield
---@field features bitfield
---@field minimum_required integer tentative
---@field maximum_permitted integer tentative
---@field dynamic_style musical_form_style
---@field overall_style musical_form_style
df.musical_form_instruments = nil

---@class musical_form_sub4
---@field passage integer the passage index this structure refers to
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
df.musical_form_sub4 = nil

---@class musical_form
---@field id integer
---@field name language_name
---@field originating_entity integer ID of the entity from which the musical form originated.
---@field original_author integer historical figure ID of the composer
---@field passages musical_form_passage[]
---@field instruments musical_form_instruments[]
---@field melodies musical_form_melodies[]
---@field unk_1 musical_form_sub4[]
---@field tempo_style musical_form_style
---@field dynamic_style musical_form_style
---@field overall_style musical_form_style
---@field poetic_form_id integer
---@field written_content_id integer
---@field scale_id integer
---@field scale_subid integer reference to scale_sub2
---@field rhythm_id integer
---@field sub_rhythm integer reference to sub_rhythms
---@field rhythm_pattern integer reference to patterns
---@field features bitfield
---@field pitch_style musical_form_pitch_style
---@field purpose musical_form_purpose
---@field devotion_target integer
---@field flags bitfield
df.musical_form = nil

---@enum dance_form_context
df.dance_form_context = {
    Sacred = 0,
    Celebration = 1,
    Participation = 2,
    Social = 3,
    Performance = 4,
    War = 5,
}

---@enum dance_form_group_size
df.dance_form_group_size = {
    Solo = 0,
    Partner = 1,
    Group = 2,
}

---@enum dance_form_configuration
df.dance_form_configuration = {
    NONE = -1, -- hard to have a configuration with a solo performer
    SingleLine = 0,
    SeveralLines = 1,
    Circle = 2,
    DoubleCircle = 3,
    LooselyMingled = 4,
}

---@enum dance_form_movement_path
df.dance_form_movement_path = {
    NONE = -1,
    TurnClockwise = 0,
    TurnCounterClockwise = 1,
    ImprovisedPath = 2,
    IntricatePath = 3,
}

---@enum dance_form_partner_distance
df.dance_form_partner_distance = {
    NONE = -1,
    Closely = 0,
    OpenContact = 1,
    RareContact = 2,
}

---@enum dance_form_partner_intent
df.dance_form_partner_intent = {
    NONE = -1,
    PushingTogether = 0,
    PullingAway = 1,
    Touch = 2,
    LightTouch = 3,
    VisualCues = 4,
    SpokenCues = 5,
}

---@enum dance_form_partner_cue_frequency
df.dance_form_partner_cue_frequency = {
    NONE = -1,
    Constantly = 0,
    Briefly = 1,
}

---@enum dance_form_partner_change_type
df.dance_form_partner_change_type = {
    NONE = -1,
    LeadAdvanceAlongMainLineOfMotion = 0,
    LeadAdvanceAgainstMainLineOfMotion = 1,
    LeadTurningOutClockwise = 2,
    LeadTurningOutCounterClockwise = 3,
}

---@enum dance_form_move_type
df.dance_form_move_type = {
    SquareStep = 0,
    CircularStep = 1,
    TriangleStep = 2,
    FigureEightStep = 3,
    IntricateStep = 4,
    Dance = 5,
    Turn = 6,
    FacialExpression = 7,
    HandGesture = 8,
    StraightWalk = 9,
    CurvedWalk = 10,
    Run = 11,
    Leap = 12,
    Kick = 13,
    LeftKick = 14,
    RightKick = 15,
    LegLift = 16,
    LeftLegLift = 17,
    RightLegLift = 18,
    BodyLevel = 19,
    BodyLevelChange = 20,
    ArmCarriage = 21,
    RaisedLeftArm = 22,
    RaisedRightArm = 23,
    RaisedArms = 24,
    Spin = 25,
    IndependentBodyMovement = 26,
    Sway = 27,
    ForwardBend = 28,
    BackwardBend = 29,
    LeftwardBend = 30,
    RightwardBend = 31,
    Footwork = 32,
    MovementAlongLineOfDance = 33,
}

---@enum dance_form_move_modifier
df.dance_form_move_modifier = {
    NONE = -1,
    Graceful = 0,
    Serene = 1,
    SharpEdged = 2,
    Grotesque = 3,
    Crude = 4,
    Refined = 5,
    Understated = 6,
    Delicate = 7,
    Elaborate = 8,
    Expressive = 9,
    Strong = 10,
    Large = 11,
    Weightless = 12,
    Fluid = 13,
    Undulating = 14,
    Soft = 15,
    Jerking = 16,
    Calm = 17,
    StraightLined = 18,
    High = 19,
    Low = 20,
    LoudlyPercussive = 21,
    SoftlyPercussive = 22,
    Aborted = 23,
    PartiallyRealized = 24,
    Energetic = 25,
    Passionate = 26,
    Vivacious = 27,
    Joyous = 28,
    Proud = 29,
    Flamboyant = 30,
    Lively = 31,
    Spirited = 32,
    Vigorous = 33,
    Intense = 34,
    Aggressive = 35,
    Powerful = 36,
    Sluggish = 37,
    Relaxed = 38,
    Passive = 39,
    Subtle = 40,
    Sensual = 41,
    Debauched = 42,
    Twisting = 43,
    Sprightly = 44,
    Sinuous = 45,
}

---@alias dance_form_move_location bitfield

---@class dance_form_section
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field movement_path dance_form_movement_path
---@field move_id integer index in the moves vector
---@field partner_distance dance_form_partner_distance
---@field partner_intent dance_form_partner_intent
---@field partner_cue_frequency dance_form_partner_cue_frequency
---@field partner_changes any[]
---@field unk_11 integer
---@field unk_12 integer
---@field unk_13 integer
---@field unk_14 integer
---@field type any[]
---@field modifier any[]
---@field parameter integer[] Depends on type (turns are in signed angles, steps are in number of steps, etc.)
---@field location any[]
---@field id integer
df.dance_form_section = nil

---@enum dance_form_move_group_type
df.dance_form_move_group_type = {
    unk_0 = 0, -- Might be a null value. Hacked in it did not result in any entry in exported Legends info
    BasicMovement = 1,
    DancePosition = 2,
    unk_3 = 3, -- Might not be a group value. Hacked into a group dance it did result in the name but not any description in exported Legends info
    DanceMove = 4,
}

---@class dance_form_move
---@field name string
---@field type any[]
---@field modifier any[]
---@field parameter integer[] Depends on type (turns are in signed angles, steps are in number of steps, etc.)
---@field location any[]
---@field group_type dance_form_move_group_type
df.dance_form_move = nil

---@class dance_form
---@field id integer
---@field name language_name
---@field musical_form_id integer
---@field music_written_content_id integer at most one of this and musical_form_id is non null
---@field context dance_form_context
---@field originating_entity integer ID of the entity from which the dance form originated.
---@field original_author integer ID of the historical figure who developed the dance form.
---@field produce_individual_dances integer 0:improvise, 1:apply by choreographers. May be bitfield if analogous to corresponding music, but no other values seen
---@field group_size dance_form_group_size
---@field unk_4 integer 1 seen, and it's always paired with the next field
---@field unk_5 integer 1 seen, and it's always paired with the previous field
---@field configuration dance_form_configuration
---@field movement_path dance_form_movement_path
---@field unk_8 integer 0 seen
---@field partner_distance dance_form_partner_distance NONE when not pair dance
---@field partner_intent dance_form_partner_intent NONE when not pair dance
---@field partner_cue_frequency dance_form_partner_cue_frequency NONE when not pair dance and when 'normal'
---@field partner_changes any[]
---@field poetry_referenced boolean Weird, but all instances where it was set examined have the dance act out any composition of a named poetic form, without any presence of the form number found
---@field unk_14 integer
---@field hfid integer Character whose story the dance acts out
---@field race integer Creature whose movements are imitated
---@field move_type any[]
---@field move_modifier any[]
---@field move_parameter integer[] Depends on type (turns are in signed angles, steps are in number of steps, etc.)
---@field move_location any[]
---@field sections dance_form_section[]
---@field moves dance_form_move[]
df.dance_form = nil

---@enum scale_type
df.scale_type = {
    Octave = 0, -- The octave is divided into X steps of even length
    Variable = 1, -- The octave is divided into notes at varying intervals, approximated by quartertones
    PerfectFourth = 2, -- The perfect fourth interval is divided into steps of even length
}

---@class chord
---@field name string
---@field notes integer[] chord_size entries used. Refers to the notes indices
---@field chord_size integer
---@field unk_3 integer 0 and 1 seen
df.chord = nil

---@class named_scale Seems odd with a 'scale' consisting of two chords, but that's what the exported XML calls it.
---@field unk_1 integer 0-4 seen. 0: nothing, for when degrees are used, 1: joined chords, 2/3: disjoined chords (varying kinds of chords seen for both), 4: as always, disjoined chords
---@field name string
---@field degrees integer[] indices into the (not necessarily named) notes of the scale
---@field degrees_used integer elements used in array above
---@field first_chord integer this pair seems to be used when degrees_used = 0. Refers to indices in the chords vector
---@field second_chord integer
df.named_scale = nil

---@class scale_notes Curiously, the named notes do not have to match the number of defined notes
---@field unk_1 integer Frequently looks like garbage for all values of type. Suspect it's actually a filler
---@field name string[]
---@field abreviation string[]
---@field number integer[]
---@field length integer number of elements of the arrays above used

---@class scale
---@field id integer
---@field flags bitfield
---@field type scale_type
---@field quartertones_used integer[] Quartertone corresponding note matches. Scale_length elements are used when type = Variable. Unused elements uninitialized
---@field scale_length integer Number of notes in the scale. When type = Variable this is the number of used indices pointing out their placement.
---@field chords chord[]
---@field scales named_scale[] Note that the top level scale doesn't have a name. These seem to be named scales using the unnamed scale's notes as their foundation
---@field notes scale_notes Curiously, the named notes do not have to match the number of defined notes
df.scale = nil

---@class rhythm
---@field id integer
---@field patterns rhythm_pattern[]
---@field sub_rhythms sub_rhythm[]
---@field unk_2 integer
df.rhythm = nil

---@enum beat_type
df.beat_type = {
    Silent = 0, -- -
    AccentedBeat = 1, -- X
    Beat = 2, -- x
    PrimaryAccent = 3, -- !
    SilentEarly = 4, -- -`
    AccentedBeatEarly = 5, -- X`
    BeatEarly = 6, -- x`
    AccentedEarly = 7, -- !`
    SilentSyncopated = 8, -- -'
    AccentedBeatSyncopated = 9, -- X'
    BeatSyncopated = 10, -- x'
    AccentedSyncopated = 11, -- !'
}

---@class rhythm_pattern
---@field name string
---@field bars any[]
---@field beat_name any[] length as per length field
---@field beat_abbreviation any[] length as per length field
---@field length integer
df.rhythm_pattern = nil

---@class sub_rhythm
---@field name string
---@field patterns integer[] indices into patterns
---@field unk_2 integer[] Same length as patterns, but with unknown purpose
---@field unk_3 integer
df.sub_rhythm = nil

---@enum occupation_type
df.occupation_type = {
    TAVERN_KEEPER = 0,
    PERFORMER = 1,
    SCHOLAR = 2,
    MERCENARY = 3,
    MONSTER_SLAYER = 4,
    SCRIBE = 5,
    MESSENGER = 6,
    DOCTOR = 7,
    DIAGNOSTICIAN = 8,
    SURGEON = 9,
    BONE_DOCTOR = 10,
}

---@class occupation
---@field id integer
---@field type occupation_type
---@field histfig_id integer
---@field unit_id integer
---@field location_id integer
---@field site_id integer
---@field group_id integer
---@field unk_1 occupation_sub1[]
---@field unk_2 integer
---@field army_controller_id integer
---@field unk_4 world_site When these haven't crashed the data has been nonsensical
---@field unk_5 abstract_building When these haven't crashed the data has been nonsensical. Has seen duplicate of unk_4 pointer value
df.occupation = nil

---@class occupation_sub1
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
---@field unk_10 integer
---@field unk_11 integer
---@field unk_12 integer
---@field unk_13 integer
---@field unk_14 integer
---@field unk_15 integer
---@field unk_16 integer
---@field unk_17 integer
---@field unk_18 integer
---@field unk_19 integer
df.occupation_sub1 = nil


