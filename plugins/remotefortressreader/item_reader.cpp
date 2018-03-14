#include "item_reader.h"
#include "Core.h"
#include "VersionInfo.h"
#include "ItemdefInstrument.pb.h"

#include "df/art_image.h"
#include "df/art_image_chunk.h"
#include "df/art_image_element.h"
#include "df/art_image_element_creaturest.h"
#include "df/art_image_element_itemst.h"
#include "df/art_image_element_plantst.h"
#include "df/art_image_element_shapest.h"
#include "df/art_image_element_treest.h"
#include "df/art_image_element_type.h"
#include "df/art_image_property.h"
#include "df/art_image_property_intransitive_verbst.h"
#include "df/art_image_property_transitive_verbst.h"
#include "df/art_image_ref.h"
#include "df/descriptor_shape.h"
#include "df/instrument_piece.h"
#include "df/instrument_register.h"
#include "df/item_type.h"
#include "df/item_constructed.h"
#include "df/item_gemst.h"
#include "df/item_smallgemst.h"
#include "df/item_statuest.h"
#include "df/item_threadst.h"
#include "df/item_toolst.h"
#include "df/itemdef_instrumentst.h"
#include "df/itemdef_toolst.h"
#include "df/itemimprovement.h"
#include "df/itemimprovement_art_imagest.h"
#include "df/itemimprovement_bandsst.h"
#include "df/itemimprovement_coveredst.h"
#include "df/itemimprovement_illustrationst.h"
#include "df/itemimprovement_itemspecificst.h"
#include "df/itemimprovement_sewn_imagest.h"
#include "df/itemimprovement_specific_type.h"
#include "df/itemimprovement_threadst.h"
#include "df/itemdef.h"
#include "df/map_block.h"
#include "df/vehicle.h"
#include "df/world.h"

#include "modules/Items.h"
#include "modules/MapCache.h"
#include "modules/Materials.h"
#include "MiscUtils.h"


using namespace DFHack;
using namespace df::enums;
using namespace RemoteFortressReader;
using namespace ItemdefInstrument;
using namespace std;
using namespace df::global;


void CopyImage(const df::art_image * image, ArtImage * netImage)
{
    auto id = netImage->mutable_id();
    id->set_mat_type(image->id);
    id->set_mat_index(image->subid);
    for (int i = 0; i < image->elements.size(); i++)
    {
        auto element = image->elements[i];
        auto netElement = netImage->add_elements();
        auto elementType = element->getType();
        netElement->set_type((ArtImageElementType)elementType);

        netElement->set_count(element->count);

        switch (elementType)
        {
        case df::enums::art_image_element_type::CREATURE:
        {
            VIRTUAL_CAST_VAR(creature, df::art_image_element_creaturest, element);
            auto cret = netElement->mutable_creature_item();
            cret->set_mat_type(creature->race);
            cret->set_mat_index(creature->caste);
            break;
        }
        case df::enums::art_image_element_type::PLANT:
        {
            VIRTUAL_CAST_VAR(plant, df::art_image_element_plantst, element);
            netElement->set_id(plant->plant_id);
            break;
        }
        case df::enums::art_image_element_type::TREE:
        {
            VIRTUAL_CAST_VAR(tree, df::art_image_element_treest, element);
            netElement->set_id(tree->plant_id);
            break;
        }
        case df::enums::art_image_element_type::SHAPE:
        {
            VIRTUAL_CAST_VAR(shape, df::art_image_element_shapest, element);
            netElement->set_id(shape->shape_id);
            break;
        }
        case df::enums::art_image_element_type::ITEM:
        {
            VIRTUAL_CAST_VAR(item, df::art_image_element_itemst, element);
            auto it = netElement->mutable_creature_item();
            it->set_mat_type(item->item_type);
            it->set_mat_index(item->item_subtype);
            netElement->set_id(item->item_id);
            switch (item->item_type)
            {
            case item_type::PLANT:
                it->set_mat_index(item->mat_index);
            default:
                break;
            }
            auto mat = netElement->mutable_material();
            mat->set_mat_type(item->mat_type);
            mat->set_mat_index(item->mat_index);
            break;
        }
        default:
            break;
        }
    }
    for (int i = 0; i < image->properties.size(); i++)
    {
        auto dfProperty = image->properties[i];
        auto netProperty = netImage->add_properties();
        auto propertyType = dfProperty->getType();

        netProperty->set_type((ArtImagePropertyType)propertyType);

        switch (propertyType)
        {
        case df::enums::art_image_property_type::transitive_verb:
        {
            VIRTUAL_CAST_VAR(transitive, df::art_image_property_transitive_verbst, dfProperty);
            netProperty->set_subject(transitive->subject);
            netProperty->set_object(transitive->object);
            netProperty->set_verb((ArtImageVerb)transitive->verb);
            break;
        }
        case df::enums::art_image_property_type::intransitive_verb:
        {
            VIRTUAL_CAST_VAR(intransitive, df::art_image_property_intransitive_verbst, dfProperty);
            netProperty->set_subject(intransitive->subject);
            netProperty->set_verb((ArtImageVerb)intransitive->verb);
            break;
        }
        default:
            break;
        }
    }
}

void CopyItem(RemoteFortressReader::Item * NetItem, df::item * DfItem)
{
    NetItem->set_id(DfItem->id);
    NetItem->set_flags1(DfItem->flags.whole);
    NetItem->set_flags2(DfItem->flags2.whole);
    auto pos = NetItem->mutable_pos();
    pos->set_x(DfItem->pos.x);
    pos->set_y(DfItem->pos.y);
    pos->set_z(DfItem->pos.z);
    auto mat = NetItem->mutable_material();
    mat->set_mat_index(DfItem->getMaterialIndex());
    mat->set_mat_type(DfItem->getMaterial());
    auto type = NetItem->mutable_type();
    type->set_mat_type(DfItem->getType());
    type->set_mat_index(DfItem->getSubtype());

    bool isProjectile = false;

    item_type::item_type itemType = DfItem->getType();

    switch (itemType)
    {
    case df::enums::item_type::NONE:
        break;
    case df::enums::item_type::BAR:
        break;
    case df::enums::item_type::SMALLGEM:
    {
        VIRTUAL_CAST_VAR(smallgem_item, df::item_smallgemst, DfItem);
        type->set_mat_index(smallgem_item->shape);
        break;
    }
    case df::enums::item_type::BLOCKS:
        break;
    case df::enums::item_type::ROUGH:
        break;
    case df::enums::item_type::BOULDER:
        break;
    case df::enums::item_type::WOOD:
        break;
    case df::enums::item_type::DOOR:
        break;
    case df::enums::item_type::FLOODGATE:
        break;
    case df::enums::item_type::BED:
        break;
    case df::enums::item_type::CHAIR:
        break;
    case df::enums::item_type::CHAIN:
        break;
    case df::enums::item_type::FLASK:
        break;
    case df::enums::item_type::GOBLET:
        break;
    case df::enums::item_type::INSTRUMENT:
        break;
    case df::enums::item_type::TOY:
        break;
    case df::enums::item_type::WINDOW:
        break;
    case df::enums::item_type::CAGE:
        break;
    case df::enums::item_type::BARREL:
        break;
    case df::enums::item_type::BUCKET:
        break;
    case df::enums::item_type::ANIMALTRAP:
        break;
    case df::enums::item_type::TABLE:
        break;
    case df::enums::item_type::COFFIN:
        break;
    case df::enums::item_type::STATUE:
    {
        VIRTUAL_CAST_VAR(statue, df::item_statuest, DfItem);

        df::art_image_chunk * chunk = NULL;
        GET_ART_IMAGE_CHUNK GetArtImageChunk = reinterpret_cast<GET_ART_IMAGE_CHUNK>(Core::getInstance().vinfo->getAddress("get_art_image_chunk"));
        if (GetArtImageChunk)
        {
            chunk = GetArtImageChunk(&(world->art_image_chunks), statue->image.id);
        }
        else
        {
            for (int i = 0; i < world->art_image_chunks.size(); i++)
            {
                if (world->art_image_chunks[i]->id == statue->image.id)
                    chunk = world->art_image_chunks[i];
            }
        }
        if (chunk)
        {
            CopyImage(chunk->images[statue->image.subid], NetItem->mutable_image());
        }


        break;
    }
    case df::enums::item_type::CORPSE:
        break;
    case df::enums::item_type::WEAPON:
        break;
    case df::enums::item_type::ARMOR:
        break;
    case df::enums::item_type::SHOES:
        break;
    case df::enums::item_type::SHIELD:
        break;
    case df::enums::item_type::HELM:
        break;
    case df::enums::item_type::GLOVES:
        break;
    case df::enums::item_type::BOX:
        type->set_mat_index(DfItem->isBag());
        break;
    case df::enums::item_type::BIN:
        break;
    case df::enums::item_type::ARMORSTAND:
        break;
    case df::enums::item_type::WEAPONRACK:
        break;
    case df::enums::item_type::CABINET:
        break;
    case df::enums::item_type::FIGURINE:
        break;
    case df::enums::item_type::AMULET:
        break;
    case df::enums::item_type::SCEPTER:
        break;
    case df::enums::item_type::AMMO:
        break;
    case df::enums::item_type::CROWN:
        break;
    case df::enums::item_type::RING:
        break;
    case df::enums::item_type::EARRING:
        break;
    case df::enums::item_type::BRACELET:
        break;
    case df::enums::item_type::GEM:
    {
        VIRTUAL_CAST_VAR(gem_item, df::item_gemst, DfItem);
        type->set_mat_index(gem_item->shape);
        break;
    }
    case df::enums::item_type::ANVIL:
        break;
    case df::enums::item_type::CORPSEPIECE:
        break;
    case df::enums::item_type::REMAINS:
        break;
    case df::enums::item_type::MEAT:
        break;
    case df::enums::item_type::FISH:
        break;
    case df::enums::item_type::FISH_RAW:
        break;
    case df::enums::item_type::VERMIN:
        break;
    case df::enums::item_type::PET:
        break;
    case df::enums::item_type::SEEDS:
        break;
    case df::enums::item_type::PLANT:
        //For convenience, we encode the plant type into item type, even if it's available in the material.
        type->set_mat_index(DfItem->getMaterialIndex());
        break;
    case df::enums::item_type::SKIN_TANNED:
        break;
    case df::enums::item_type::PLANT_GROWTH:
        break;
    case df::enums::item_type::THREAD:
    {
        VIRTUAL_CAST_VAR(thread, df::item_threadst, DfItem);
        if (thread && thread->dye_mat_type >= 0)
        {
            DFHack::MaterialInfo info;
            if (info.decode(thread->dye_mat_type, thread->dye_mat_index))
                ConvertDFColorDescriptor(info.material->powder_dye, NetItem->mutable_dye());
        }
        if (DfItem->flags.bits.spider_web)
            type->set_mat_index(1);
        else
            type->set_mat_index(0);
        break;
    }
    case df::enums::item_type::CLOTH:
        break;
    case df::enums::item_type::TOTEM:
        break;
    case df::enums::item_type::PANTS:
        break;
    case df::enums::item_type::BACKPACK:
        break;
    case df::enums::item_type::QUIVER:
        break;
    case df::enums::item_type::CATAPULTPARTS:
        break;
    case df::enums::item_type::BALLISTAPARTS:
        break;
    case df::enums::item_type::SIEGEAMMO:
        break;
    case df::enums::item_type::BALLISTAARROWHEAD:
        break;
    case df::enums::item_type::TRAPPARTS:
        break;
    case df::enums::item_type::TRAPCOMP:
        break;
    case df::enums::item_type::DRINK:
        break;
    case df::enums::item_type::POWDER_MISC:
        break;
    case df::enums::item_type::CHEESE:
        break;
    case df::enums::item_type::FOOD:
        break;
    case df::enums::item_type::LIQUID_MISC:
        break;
    case df::enums::item_type::COIN:
        break;
    case df::enums::item_type::GLOB:
        break;
    case df::enums::item_type::ROCK:
        break;
    case df::enums::item_type::PIPE_SECTION:
        break;
    case df::enums::item_type::HATCH_COVER:
        break;
    case df::enums::item_type::GRATE:
        break;
    case df::enums::item_type::QUERN:
        break;
    case df::enums::item_type::MILLSTONE:
        break;
    case df::enums::item_type::SPLINT:
        break;
    case df::enums::item_type::CRUTCH:
        break;
    case df::enums::item_type::TRACTION_BENCH:
        break;
    case df::enums::item_type::ORTHOPEDIC_CAST:
        break;
    case df::enums::item_type::TOOL:
        if (!isProjectile)
        {
            VIRTUAL_CAST_VAR(tool, df::item_toolst, DfItem);
            if (tool)
            {
                auto vehicle = binsearch_in_vector(world->vehicles.active, tool->vehicle_id);
                if (vehicle)
                {
                    NetItem->set_subpos_x(vehicle->offset_x / 100000.0);
                    NetItem->set_subpos_y(vehicle->offset_y / 100000.0);
                    NetItem->set_subpos_z(vehicle->offset_z / 140000.0);
                }
            }
        }
        break;
    case df::enums::item_type::SLAB:
        break;
    case df::enums::item_type::EGG:
        break;
    case df::enums::item_type::BOOK:
        break;
    case df::enums::item_type::SHEET:
        break;
    case df::enums::item_type::BRANCH:
        break;
    default:
        break;
    }

    VIRTUAL_CAST_VAR(actual_item, df::item_actual, DfItem);
    if (actual_item)
    {
        NetItem->set_stack_size(actual_item->stack_size);
    }

    VIRTUAL_CAST_VAR(constructed_item, df::item_constructed, DfItem);
    if (constructed_item)
    {
        for (int i = 0; i < constructed_item->improvements.size(); i++)
        {
            auto improvement = constructed_item->improvements[i];

            if (!improvement)
                continue;

            improvement_type::improvement_type impType = improvement->getType();

            auto netImp = NetItem->add_improvements();

            netImp->set_type((ImprovementType)impType);

            auto mat = netImp->mutable_material();
            mat->set_mat_type(improvement->mat_type);
            mat->set_mat_index(improvement->mat_index);

            switch (impType)
            {
            case df::enums::improvement_type::ART_IMAGE:
            {
                VIRTUAL_CAST_VAR(artImage, df::itemimprovement_art_imagest, improvement);
                CopyImage(artImage->getImage(DfItem), netImp->mutable_image());
                break;
            }
            case df::enums::improvement_type::COVERED:
            {
                VIRTUAL_CAST_VAR(covered, df::itemimprovement_coveredst, improvement);
                netImp->set_shape(covered->shape);
                break;
            }
            case df::enums::improvement_type::RINGS_HANGING:
                break;
            case df::enums::improvement_type::BANDS:
            {
                VIRTUAL_CAST_VAR(bands, df::itemimprovement_bandsst, improvement);
                netImp->set_shape(bands->shape);
                break;
            }
            case df::enums::improvement_type::ITEMSPECIFIC:
            {
                VIRTUAL_CAST_VAR(specific, df::itemimprovement_itemspecificst, improvement);
                netImp->set_specific_type(specific->type);
                break;
            }
            case df::enums::improvement_type::THREAD:
            {
                VIRTUAL_CAST_VAR(improvement_thread, df::itemimprovement_threadst, improvement);
                if (improvement_thread->dye.mat_type >= 0)
                {
                    DFHack::MaterialInfo info;
                    if (!info.decode(improvement_thread->dye.mat_type, improvement_thread->dye.mat_index))
                        continue;
                    ConvertDFColorDescriptor(info.material->powder_dye, NetItem->mutable_dye());
                }
                break;
            }
            case df::enums::improvement_type::CLOTH:
                break;
            case df::enums::improvement_type::SEWN_IMAGE:
                break;
            case df::enums::improvement_type::PAGES:
                break;
            case df::enums::improvement_type::ILLUSTRATION:
                break;
            case df::enums::improvement_type::INSTRUMENT_PIECE:
                break;
            case df::enums::improvement_type::WRITING:
                break;
            default:
                break;
            }
        }
    }

    NetItem->set_volume(DfItem->getVolume());
}

DFHack::command_result GetItemList(DFHack::color_ostream &stream, const DFHack::EmptyMessage *in, RemoteFortressReader::MaterialList *out)
{
    if (!Core::getInstance().isWorldLoaded()) {
        //out->set_available(false);
        return CR_OK;
    }
    FOR_ENUM_ITEMS(item_type, it)
    {
        MaterialDefinition *mat_def = out->add_material_list();
        mat_def->mutable_mat_pair()->set_mat_type((int)it);
        mat_def->mutable_mat_pair()->set_mat_index(-1);
        mat_def->set_id(ENUM_KEY_STR(item_type, it));
        switch (it)
        {
        case df::enums::item_type::GEM:
        case df::enums::item_type::SMALLGEM:
        {
            for (int i = 0; i < world->raws.descriptors.shapes.size(); i++)
            {
                auto shape = world->raws.descriptors.shapes[i];
                if (shape->gems_use.whole == 0)
                    continue;
                mat_def = out->add_material_list();
                mat_def->mutable_mat_pair()->set_mat_type((int)it);
                mat_def->mutable_mat_pair()->set_mat_index(i);
                mat_def->set_id(ENUM_KEY_STR(item_type, it) + "/" + shape->id);
            }
            break;
        }
        case df::enums::item_type::PLANT:
        {
            for (int i = 0; i < world->raws.plants.all.size(); i++)
            {
                auto plantRaw = world->raws.plants.all[i];
                mat_def = out->add_material_list();
                mat_def->mutable_mat_pair()->set_mat_type((int)it);
                mat_def->mutable_mat_pair()->set_mat_index(plantRaw->index);
                mat_def->set_id(ENUM_KEY_STR(item_type, it) + "/" + plantRaw->id);
            }
            break;
        }
        case df::enums::item_type::BOX:
        {
            mat_def = out->add_material_list();
            mat_def->mutable_mat_pair()->set_mat_type((int)it);
            mat_def->mutable_mat_pair()->set_mat_index(0);
            mat_def->set_id("BOX/CHEST");
            mat_def = out->add_material_list();
            mat_def->mutable_mat_pair()->set_mat_type((int)it);
            mat_def->mutable_mat_pair()->set_mat_index(1);
            mat_def->set_id("BOX/BAG");
            break;
        }
        case df::enums::item_type::THREAD:
        {
            mat_def = out->add_material_list();
            mat_def->mutable_mat_pair()->set_mat_type((int)it);
            mat_def->mutable_mat_pair()->set_mat_index(0);
            mat_def->set_id("THREAD/NORMAL");
            mat_def = out->add_material_list();
            mat_def->mutable_mat_pair()->set_mat_type((int)it);
            mat_def->mutable_mat_pair()->set_mat_index(1);
            mat_def->set_id("THREAD/WEB");
        }
        }
        int subtypes = Items::getSubtypeCount(it);
        if (subtypes >= 0)
        {
            for (int i = 0; i < subtypes; i++)
            {
                mat_def = out->add_material_list();
                mat_def->mutable_mat_pair()->set_mat_type((int)it);
                mat_def->mutable_mat_pair()->set_mat_index(i);
                df::itemdef * item = Items::getSubtypeDef(it, i);
                mat_def->set_id(ENUM_KEY_STR(item_type, it) + "/" + item->id);
                switch (it)
                {
                case df::enums::item_type::INSTRUMENT:
                {
                    VIRTUAL_CAST_VAR(instrument, df::itemdef_instrumentst, item);
                    mat_def->set_name(DF2UTF(instrument->name));
                    auto send_instrument = mat_def->mutable_instrument();
                    auto flags = send_instrument->mutable_flags();
                    flags->set_indefinite_pitch(instrument->flags.is_set(instrument_flags::INDEFINITE_PITCH));
                    flags->set_placed_as_building(instrument->flags.is_set(instrument_flags::PLACED_AS_BUILDING));
                    flags->set_metal_mat(instrument->flags.is_set(instrument_flags::METAL_MAT));
                    flags->set_stone_mat(instrument->flags.is_set(instrument_flags::STONE_MAT));
                    flags->set_wood_mat(instrument->flags.is_set(instrument_flags::WOOD_MAT));
                    flags->set_glass_mat(instrument->flags.is_set(instrument_flags::GLASS_MAT));
                    flags->set_ceramic_mat(instrument->flags.is_set(instrument_flags::CERAMIC_MAT));
                    flags->set_shell_mat(instrument->flags.is_set(instrument_flags::SHELL_MAT));
                    flags->set_bone_mat(instrument->flags.is_set(instrument_flags::BONE_MAT));
                    send_instrument->set_size(instrument->size);
                    send_instrument->set_value(instrument->value);
                    send_instrument->set_material_size(instrument->material_size);
                    for (int j = 0; j < instrument->pieces.size(); j++)
                    {
                        auto piece = send_instrument->add_pieces();
                        piece->set_type(instrument->pieces[j]->type);
                        piece->set_id(instrument->pieces[j]->id);
                        piece->set_name(DF2UTF(instrument->pieces[j]->name));
                        piece->set_name_plural(DF2UTF(instrument->pieces[j]->name_plural));
                    }
                    send_instrument->set_pitch_range_min(instrument->pitch_range_min);
                    send_instrument->set_pitch_range_max(instrument->pitch_range_max);
                    for (int j = 0; j < instrument->sound_production.size(); j++)
                    {
                        send_instrument->add_sound_production((SoundProductionType)instrument->sound_production[j]);
                    }
                    for (int j = 0; j < instrument->sound_production_parm1.size(); j++)
                    {
                        send_instrument->add_sound_production_parm1(*(instrument->sound_production_parm1[j]));
                    }
                    for (int j = 0; j < instrument->sound_production_parm2.size(); j++)
                    {
                        send_instrument->add_sound_production_parm2(*(instrument->sound_production_parm2[j]));
                    }
                    for (int j = 0; j < instrument->pitch_choice.size(); j++)
                    {
                        send_instrument->add_pitch_choice((PitchChoiceType)instrument->pitch_choice[j]);
                    }
                    for (int j = 0; j < instrument->pitch_choice_parm1.size(); j++)
                    {
                        send_instrument->add_pitch_choice_parm1(*(instrument->pitch_choice_parm1[j]));
                    }
                    for (int j = 0; j < instrument->pitch_choice_parm2.size(); j++)
                    {
                        send_instrument->add_pitch_choice_parm2(*(instrument->pitch_choice_parm2[j]));
                    }
                    for (int j = 0; j < instrument->tuning.size(); j++)
                    {
                        send_instrument->add_tuning((TuningType)instrument->tuning[j]);
                    }
                    for (int j = 0; j < instrument->tuning_parm.size(); j++)
                    {
                        send_instrument->add_tuning_parm(*(instrument->tuning_parm[j]));
                    }
                    for (int j = 0; j < instrument->registers.size(); j++)
                    {
                        auto reg = send_instrument->add_registers();
                        reg->set_pitch_range_min(instrument->registers[j]->pitch_range_min);
                        reg->set_pitch_range_max(instrument->registers[j]->pitch_range_max);
                    }
                    send_instrument->set_description(DF2UTF(instrument->description));
                    break;
                }
                case df::enums::item_type::TOOL:
                {
                    VIRTUAL_CAST_VAR(tool, df::itemdef_toolst, item);
                    mat_def->set_name(DF2UTF(tool->name));
                }
                default:
                    break;
                }
            }
        }
    }
    return CR_OK;
}
