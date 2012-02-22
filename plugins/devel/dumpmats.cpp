// Dump all hardcoded materials
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/material.h"
#include "df/builtin_mats.h"
#include "df/matter_state.h"
#include "df/descriptor_color.h"
#include "df/item_type.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

using df::global::world;

command_result df_dumpmats (Core *c, vector<string> &parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    c->con.print("hardcoded_materials\n\n");
    c->con.print("[OBJECT:MATERIAL]\n");

    FOR_ENUM_ITEMS(builtin_mats, mat_num)
    {
        df::material *mat = world->raws.mat_table.builtin[mat_num];
        if (!mat)
            continue;
        c->con.print("\n[MATERIAL:%s] - reconstructed from data extracted from memory\n", mat->id.c_str());

        int32_t def_color[6] = {-1,-1,-1,-1,-1,-1};
        bool name_all = false;
        bool name_all_solid = false;
        string def_name[6];
        bool adj_all = false;
        bool adj_all_solid = false;
        string def_adj[6];

        int32_t solid_color = mat->state_color[matter_state::Solid];
        if (solid_color == mat->state_color[matter_state::Powder] ||
            solid_color == mat->state_color[matter_state::Paste] ||
            solid_color == mat->state_color[matter_state::Pressed])
        {
            def_color[matter_state::Solid] = solid_color;
            def_color[matter_state::Powder] = solid_color;
            def_color[matter_state::Paste] = solid_color;
            def_color[matter_state::Pressed] = solid_color;
            if (solid_color == mat->state_color[matter_state::Liquid] ||
                solid_color == mat->state_color[matter_state::Gas])
            {
                def_color[matter_state::Liquid] = solid_color;
                def_color[matter_state::Gas] = solid_color;
                c->con.print("\t[STATE_COLOR:ALL:%s]\n", world->raws.language.colors[solid_color]->id.c_str());
            }
            else
                c->con.print("\t[STATE_COLOR:ALL_SOLID:%s]\n", world->raws.language.colors[solid_color]->id.c_str());
        }

        string solid_name = mat->state_name[matter_state::Solid];
        string solid_adj = mat->state_adj[matter_state::Solid];
        if (solid_name == solid_adj)
        {
            if (solid_name == mat->state_name[matter_state::Powder] ||
                solid_name == mat->state_name[matter_state::Paste] ||
                solid_name == mat->state_name[matter_state::Pressed])
            {
                def_name[matter_state::Solid] = solid_name;
                def_name[matter_state::Powder] = solid_name;
                def_name[matter_state::Paste] = solid_name;
                def_name[matter_state::Pressed] = solid_name;
                def_adj[matter_state::Solid] = solid_name;
                def_adj[matter_state::Powder] = solid_name;
                def_adj[matter_state::Paste] = solid_name;
                def_adj[matter_state::Pressed] = solid_name;

                if (solid_name == mat->state_name[matter_state::Liquid] ||
                    solid_name == mat->state_name[matter_state::Gas])
                {
                    def_name[matter_state::Liquid] = solid_name;
                    def_name[matter_state::Gas] = solid_name;
                    def_adj[matter_state::Liquid] = solid_name;
                    def_adj[matter_state::Gas] = solid_name;
                    c->con.print("\t[STATE_NAME_ADJ:ALL:%s]\n", solid_name.c_str());
                }
                else
                    c->con.print("\t[STATE_NAME_ADJ:ALL_SOLID:%s]\n", solid_name.c_str());
            }
        }
        else
        {
            if (solid_name == mat->state_name[matter_state::Powder] ||
                solid_name == mat->state_name[matter_state::Paste] ||
                solid_name == mat->state_name[matter_state::Pressed])
            {
                def_name[matter_state::Solid] = solid_name;
                def_name[matter_state::Powder] = solid_name;
                def_name[matter_state::Paste] = solid_name;
                def_name[matter_state::Pressed] = solid_name;

                if (solid_name == mat->state_name[matter_state::Liquid] ||
                    solid_name == mat->state_name[matter_state::Gas])
                {
                    def_name[matter_state::Liquid] = solid_name;
                    def_name[matter_state::Gas] = solid_name;
                    c->con.print("\t[STATE_NAME:ALL:%s]\n", solid_name.c_str());
                }
                else
                    c->con.print("\t[STATE_NAME:ALL_SOLID:%s]\n", solid_name.c_str());
            }
            if (solid_adj == mat->state_adj[matter_state::Powder] ||
                solid_adj == mat->state_adj[matter_state::Paste] ||
                solid_adj == mat->state_adj[matter_state::Pressed])
            {
                def_adj[matter_state::Solid] = solid_adj;
                def_adj[matter_state::Powder] = solid_adj;
                def_adj[matter_state::Paste] = solid_adj;
                def_adj[matter_state::Pressed] = solid_adj;

                if (solid_adj == mat->state_adj[matter_state::Liquid] ||
                    solid_adj == mat->state_adj[matter_state::Gas])
                {
                    def_adj[matter_state::Liquid] = solid_adj;
                    def_adj[matter_state::Gas] = solid_adj;
                    c->con.print("\t[STATE_ADJ:ALL:%s]\n", solid_adj.c_str());
                }
                else
                    c->con.print("\t[STATE_ADJ:ALL_SOLID:%s]\n", solid_adj.c_str());
            }
        }
        char *state_names[6] = {"SOLID", "LIQUID", "GAS", "SOLID_POWDER", "SOLID_PASTE", "SOLID_PRESSED"};

        FOR_ENUM_ITEMS(matter_state, state)
        {
            if (mat->state_color[state] != -1 && mat->state_color[state] != def_color[state])
                c->con.print("\t[STATE_COLOR:%s:%s]\n", state_names[state], world->raws.language.colors[mat->state_color[state]]->id.c_str());
            if (mat->state_name[state] == mat->state_adj[state])
            {
                if (mat->state_name[state].size() && mat->state_name[state] != def_name[state] || mat->state_adj[state].size() && mat->state_adj[state] != def_adj[state])
                    c->con.print("\t[STATE_NAME_ADJ:%s:%s]\n", state_names[state], mat->state_name[state].c_str());
            }
            else
            {
                if (mat->state_name[state].size() && mat->state_name[state] != def_name[state])
                    c->con.print("\t[STATE_NAME:%s:%s]\n", state_names[state], mat->state_name[state].c_str());
                if (mat->state_adj[state].size() && mat->state_adj[state] != def_adj[state])
                    c->con.print("\t[STATE_ADJ:%s:%s]\n", state_names[state], mat->state_adj[state].c_str());
            }
        }
        if (mat->basic_color[0] != 7 || mat->basic_color[1] != 0)
            c->con.print("\t[BASIC_COLOR:%i:%i]\n", mat->basic_color[0], mat->basic_color[1]);
        if (mat->build_color[0] != 7 || mat->build_color[1] != 7 || mat->build_color[2] != 0)
            c->con.print("\t[BUILD_COLOR:%i:%i:%i]\n", mat->build_color[0], mat->build_color[1], mat->build_color[2]);
        if (mat->tile_color[0] != 7 || mat->tile_color[1] != 7 || mat->tile_color[2] != 0)
            c->con.print("\t[TILE_COLOR:%i:%i:%i]\n", mat->tile_color[0], mat->tile_color[1], mat->tile_color[2]);

        if (mat->tile != 0xdb)
            c->con.print("\t[TILE:%i]\n", mat->tile);
        if (mat->item_symbol != 0x07)
            c->con.print("\t[ITEM_SYMBOL:%i]\n", mat->item_symbol);
        if (mat->material_value != 1)
            c->con.print("\t[MATERIAL_VALUE:%i]\n", mat->material_value);

        if (mat->gem_name1.size())
            c->con.print("\t[IS_GEM:%s:%s]\n", mat->gem_name1.c_str(), mat->gem_name2.c_str());
        if (mat->stone_name.size())
            c->con.print("\t[STONE_NAME:%s]\n", mat->stone_name.c_str());

        if (mat->heat.spec_heat != 60001)
            c->con.print("\t[SPEC_HEAT:%i]\n", mat->heat.spec_heat);
        if (mat->heat.heatdam_point != 60001)
            c->con.print("\t[HEATDAM_POINT:%i]\n", mat->heat.heatdam_point);
        if (mat->heat.colddam_point != 60001)
            c->con.print("\t[COLDDAM_POINT:%i]\n", mat->heat.colddam_point);
        if (mat->heat.ignite_point != 60001)
            c->con.print("\t[IGNITE_POINT:%i]\n", mat->heat.ignite_point);
        if (mat->heat.melting_point != 60001)
            c->con.print("\t[MELTING_POINT:%i]\n", mat->heat.melting_point);
        if (mat->heat.boiling_point != 60001)
            c->con.print("\t[BOILING_POINT:%i]\n", mat->heat.boiling_point);
        if (mat->heat.mat_fixed_temp != 60001)
            c->con.print("\t[MAT_FIXED_TEMP:%i]\n", mat->heat.mat_fixed_temp);

        if (mat->solid_density != 0xFBBC7818)
            c->con.print("\t[SOLID_DENSITY:%i]\n", mat->solid_density);
        if (mat->liquid_density != 0xFBBC7818)
            c->con.print("\t[LIQUID_DENSITY:%i]\n", mat->liquid_density);
        if (mat->molar_mass != 0xFBBC7818)
            c->con.print("\t[MOLAR_MASS:%i]\n", mat->molar_mass);

        if (mat->strength.impact_yield != 10000)
            c->con.print("\t[IMPACT_YIELD:%i]\n", mat->strength.impact_yield);
        if (mat->strength.impact_fracture != 10000)
            c->con.print("\t[IMPACT_FRACTURE:%i]\n", mat->strength.impact_fracture);
        if (mat->strength.impact_strain_at_yield != 0)
            c->con.print("\t[IMPACT_STRAIN_AT_YIELD:%i]\n", mat->strength.impact_strain_at_yield);

        if (mat->strength.compressive_yield != 10000)
            c->con.print("\t[COMPRESSIVE_YIELD:%i]\n", mat->strength.compressive_yield);
        if (mat->strength.compressive_fracture != 10000)
            c->con.print("\t[COMPRESSIVE_FRACTURE:%i]\n", mat->strength.compressive_fracture);
        if (mat->strength.compressive_strain_at_yield != 0)
            c->con.print("\t[COMPRESSIVE_STRAIN_AT_YIELD:%i]\n", mat->strength.compressive_strain_at_yield);

        if (mat->strength.tensile_yield != 10000)
            c->con.print("\t[TENSILE_YIELD:%i]\n", mat->strength.tensile_yield);
        if (mat->strength.tensile_fracture != 10000)
            c->con.print("\t[TENSILE_FRACTURE:%i]\n", mat->strength.tensile_fracture);
        if (mat->strength.tensile_strain_at_yield != 0)
            c->con.print("\t[TENSILE_STRAIN_AT_YIELD:%i]\n", mat->strength.tensile_strain_at_yield);

        if (mat->strength.torsion_yield != 10000)
            c->con.print("\t[TORSION_YIELD:%i]\n", mat->strength.torsion_yield);
        if (mat->strength.torsion_fracture != 10000)
            c->con.print("\t[TORSION_FRACTURE:%i]\n", mat->strength.torsion_fracture);
        if (mat->strength.torsion_strain_at_yield != 0)
            c->con.print("\t[TORSION_STRAIN_AT_YIELD:%i]\n", mat->strength.torsion_strain_at_yield);

        if (mat->strength.shear_yield != 10000)
            c->con.print("\t[SHEAR_YIELD:%i]\n", mat->strength.shear_yield);
        if (mat->strength.shear_fracture != 10000)
            c->con.print("\t[SHEAR_FRACTURE:%i]\n", mat->strength.shear_fracture);
        if (mat->strength.shear_strain_at_yield != 0)
            c->con.print("\t[SHEAR_STRAIN_AT_YIELD:%i]\n", mat->strength.shear_strain_at_yield);

        if (mat->strength.bending_yield != 10000)
            c->con.print("\t[BENDING_YIELD:%i]\n", mat->strength.bending_yield);
        if (mat->strength.bending_fracture != 10000)
            c->con.print("\t[BENDING_FRACTURE:%i]\n", mat->strength.bending_fracture);
        if (mat->strength.bending_strain_at_yield != 0)
            c->con.print("\t[BENDING_STRAIN_AT_YIELD:%i]\n", mat->strength.bending_strain_at_yield);

        if (mat->strength.max_edge != 0)
            c->con.print("\t[MAX_EDGE:%i]\n", mat->strength.max_edge);
        if (mat->strength.absorption != 0)
            c->con.print("\t[ABSORPTION:%i]\n", mat->strength.absorption);

        FOR_ENUM_ITEMS(material_flags, i)
        {
            if (mat->flags.is_set(i))
                c->con.print("\t[%s]\n", ENUM_KEY_STR(material_flags, i));
        }

        if (mat->extract_storage != item_type::BARREL)
            c->con.print("\t[EXTRACT_STORAGE:%s]\n", ENUM_KEY_STR(item_type, mat->extract_storage));
        if (mat->butcher_special_type != item_type::NONE || mat->butcher_special_subtype != -1)
            c->con.print("\t[BUTCHER_SPECIAL:%s:%s]\n", ENUM_KEY_STR(item_type, mat->butcher_special_type), (mat->butcher_special_subtype == -1) ? "NONE" : "?");
        if (mat->meat_name[0].size() || mat->meat_name[1].size() || mat->meat_name[2].size())
            c->con.print("\t[MEAT_NAME:%s:%s:%s]\n", mat->meat_name[0].c_str(), mat->meat_name[1].c_str(), mat->meat_name[2].c_str());
        if (mat->block_name[0].size() || mat->block_name[1].size())
            c->con.print("\t[BLOCK_NAME:%s:%s]\n", mat->block_name[0].c_str(), mat->block_name[1].c_str());

        for (int i = 0; i < mat->reaction_class.size(); i++)
            c->con.print("\t[REACTION_CLASS:%s]\n", mat->reaction_class[i]->c_str());
        for (int i = 0; i < mat->reaction_product.id.size(); i++)
            c->con.print("\t[MATERIAL_REACTION_PRODUCT:%s:%s:%s%s%s]\n", mat->reaction_product.id[i]->c_str(), mat->reaction_product.str[0][i]->c_str(), mat->reaction_product.str[1][i]->c_str(), mat->reaction_product.str[2][i]->size() ? ":" : "", mat->reaction_product.str[2][i]->c_str());

        if (mat->hardens_with_water.mat_type != -1)
            c->con.print("\t[HARDENS_WITH_WATER:%s:%s%s%s]\n", mat->hardens_with_water.str[0].c_str(), mat->hardens_with_water.str[1].c_str(), mat->hardens_with_water.str[2].size() ? ":" : "", mat->hardens_with_water.str[2].c_str());
        if (mat->powder_dye != -1)
             c->con.print("\t[POWDER_DYE:%s]\n", world->raws.language.colors[mat->powder_dye]->id.c_str());
        if (mat->soap_level != -0)
             c->con.print("\t[SOAP_LEVEL:%o]\n", mat->soap_level);

        for (int i = 0; i < mat->syndrome.size(); i++)
             c->con.print("\t[SYNDROME] ...\n");
    }
    return CR_OK;
}

DFHACK_PLUGIN("dumpmats");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("dumpmats", "Dump raws for all hardcoded materials", df_dumpmats, false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}