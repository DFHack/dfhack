-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum improvement_type
df.improvement_type = {
    ART_IMAGE = 0,
    COVERED = 1,
    RINGS_HANGING = 2,
    BANDS = 3,
    SPIKES = 4,
    ITEMSPECIFIC = 5,
    THREAD = 6,
    CLOTH = 7,
    SEWN_IMAGE = 8,
    PAGES = 9,
    ILLUSTRATION = 10,
    INSTRUMENT_PIECE = 11,
    WRITING = 12,
    IMAGE_SET = 13,
}

---@class dye_info
---@field mat_type integer
---@field mat_index integer
---@field dyer integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
df.dye_info = nil

---@param arg_0 item
---@return art_image
function df.itemimprovement.getImage(arg_0) end

---@param colors integer
---@param shapes integer
---@param arg_2 integer
function df.itemimprovement.getColorAndShape(colors, shapes, arg_2) end

---@return itemimprovement
function df.itemimprovement.clone() end

---@param file file_compressorst
function df.itemimprovement.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.itemimprovement.read_file(file, loadversion) end

---@return improvement_type
function df.itemimprovement.getType() end

---@return boolean
function df.itemimprovement.isDecoration() end

---@param caravan caravan_state
---@return integer
function df.itemimprovement.getDyeValue(caravan) end

---@param shape integer
function df.itemimprovement.setShape(shape) end

---@class itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
df.itemimprovement = nil

---@class itemimprovement_art_imagest
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field image art_image_ref
df.itemimprovement_art_imagest = nil

---@class itemimprovement_coveredst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field cover_flags bitfield
---@field shape integer
df.itemimprovement_coveredst = nil

---@class itemimprovement_rings_hangingst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
df.itemimprovement_rings_hangingst = nil

---@class itemimprovement_bandsst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field shape integer
df.itemimprovement_bandsst = nil

---@class itemimprovement_spikesst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
df.itemimprovement_spikesst = nil

---@enum itemimprovement_specific_type
df.itemimprovement_specific_type = {
    HANDLE = 0,
    ROLLERS = 1,
}

---@class itemimprovement_itemspecificst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field type itemimprovement_specific_type
df.itemimprovement_itemspecificst = nil

---@class itemimprovement_threadst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field dye dye_info
df.itemimprovement_threadst = nil

---@class itemimprovement_clothst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
df.itemimprovement_clothst = nil

---@class itemimprovement_sewn_imagest_cloth
---@field unit_id integer
---@field quality integer
---@field unk_1 integer

---@class itemimprovement_sewn_imagest
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field image art_image_ref
---@field cloth itemimprovement_sewn_imagest_cloth
---@field dye dye_info
df.itemimprovement_sewn_imagest = nil

---@class itemimprovement_pagesst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field count integer
---@field contents integer[]
df.itemimprovement_pagesst = nil

---@class itemimprovement_illustrationst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field image art_image_ref
---@field unk_2 integer since v0.34.01
df.itemimprovement_illustrationst = nil

---@class itemimprovement_instrument_piecest
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field type string instrument_piece.type
df.itemimprovement_instrument_piecest = nil

---@class itemimprovement_writingst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field contents integer[]
df.itemimprovement_writingst = nil

---@class itemimprovement_image_setst
-- inherited from itemimprovement
---@field mat_type integer
---@field mat_index integer
---@field maker integer
---@field masterpiece_event integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field unk_1 integer
-- end itemimprovement
---@field image_set_id integer
df.itemimprovement_image_setst = nil

---@enum written_content_type
df.written_content_type = {
    Manual = 0,
    Guide = 1,
    Chronicle = 2,
    ShortStory = 3,
    Novel = 4,
    Biography = 5,
    Autobiography = 6,
    Poem = 7,
    Play = 8,
    Letter = 9,
    Essay = 10,
    Dialog = 11,
    MusicalComposition = 12,
    Choreography = 13,
    ComparativeBiography = 14,
    BiographicalDictionary = 15,
    Genealogy = 16,
    Encyclopedia = 17,
    CulturalHistory = 18,
    CulturalComparison = 19,
    AlternateHistory = 20,
    TreatiseOnTechnologicalEvolution = 21,
    Dictionary = 22,
    StarChart = 23,
    StarCatalogue = 24,
    Atlas = 25,
}

---@enum written_content_style
df.written_content_style = {
    Meandering = 0,
    Cheerful = 1,
    Depressing = 2,
    Rigid = 3,
    Serious = 4,
    Disjointed = 5,
    Ornate = 6,
    Forceful = 7,
    Humorous = 8,
    Immature = 9,
    SelfIndulgent = 10,
    Touching = 11,
    Compassionate = 12,
    Vicious = 13,
    Concise = 14,
    Scornful = 15,
    Witty = 16,
    Ranting = 17,
}

---@class written_content
---@field id integer
---@field title string
---@field page_start integer
---@field page_end integer
---@field refs general_ref[] interactions learned
---@field ref_aux integer[] if nonzero, corresponding ref is ignored
---@field unk1 integer
---@field unk2 integer
---@field type written_content_type
---@field poetic_form integer since v0.42.01
---@field styles written_content_style[]
---@field style_strength integer[] 0 = maximum, 1 = significant, 2 = partial
---@field author integer
---@field author_roll integer
df.written_content = nil

---@alias engraving_flags bitfield

---@class engraving
---@field artist integer
---@field masterpiece_event integer
---@field skill_rating skill_rating at the moment of creation
---@field pos coord
---@field flags bitfield
---@field tile integer
---@field art_id integer
---@field art_subid integer
---@field quality item_quality
---@field unk1 integer since v0.34.06
---@field unk2 integer since v0.34.06
df.engraving = nil


