// forceequip plugin
// Moves local items from the ground into a unit's inventory
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "DataDefs.h"
#include "df/item.h"
#include "df/itemdef.h"
#include "df/world.h"
#include "df/general_ref.h"
#include "df/unit.h"
#include "df/body_part_raw.h"
#include "MiscUtils.h"
#include "df/unit_inventory_item.h"
#include "df/body_part_raw_flags.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/body_detail_plan.h"
#include "df/body_template.h"
#include "df/body_part_template.h"
#include "df/unit_soul.h"
#include "df/unit_skill.h"
#include "df/general_ref.h"
#include "df/caste_raw.h"

#include "DFHack.h"

using namespace DFHack;
using namespace df::enums;

using MapExtras::Block;
using MapExtras::MapCache;

DFHACK_PLUGIN("forceequip");
REQUIRE_GLOBAL(world);

const int const_GloveRightHandedness = 1;
const int const_GloveLeftHandedness = 2;

command_result df_forceequip(color_ostream &out, vector <string> & parameters);

const string forceequip_help =
    "ForceEquip moves local items into a unit's inventory.  It is typically\n"
    "used to equip specific clothing/armor items onto a dwarf, but can also\n"
    "be used to put armor onto a war animal or to add unusual items (such\n"
    "as crowns) to any unit.\n"
    "This plugin can process multiple items in a single call, but will only\n"
    "work with a single unit (the first one it finds under the cursor).\n"
    "In order to minimize confusion, it is recommended that you use\n"
    "forceequip only when you have a unit standing alone atop a pile of\n"
    "gear that you would like it to wear.  Items which are stored in bins\n"
    "or other containers (e.g. chests, armor racks) may also work, but\n"
    "piling items on the floor (via a garbage dump activity zone, of the\n"
    "DFHack autodump command) is the most reliable way to do it.\n"
    "The plugin will ignore any items that are forbidden.  Hence, you\n"
    "can setup a large pile of surplus gear, walk a unit onto it (or\n"
    "pasture an animal on it), unforbid a few items and run forceequip.\n"
    "The (forbidden) majority of your gear will remain in-place, ready\n"
    "for the next passerby."
    "\n"
    "As mentioned above, this plugin can be used to equip items onto\n"
    "units (such as animals) which cannot normally equip gear.  There's\n"
    "an important caveat here - such creatures will automatically drop\n"
    "inappropriate gear almost immediately (within 10 game ticks).\n"
    "If you want them to retain their equipment, you must forbid it\n"
    "AFTER using forceequip to get it into their inventory.\n"
    "This technique can also be used to clothe dwarven infants, but\n"
    "only if you're able to separate them from their mothers.\n"
    "\n"
    "By default, the forceequip plugin will attempt to avoid\n"
    "conflicts and outright cheating.  For instance, it will skip\n"
    "any item which is flagged for use in a job, and will not\n"
    "equip more than one piece of clothing/armor onto any given\n"
    "body part.  These restrictions can be overridden via command\n"
    "switches (see examples below) but doing so puts you at greater\n"
    "risk of unexpected consequences.  For instance, a dwarf who\n"
    "is wearing three breastplates will not be able to move very\n"
    "quickly.\n"
    "\n"
    "Items equipped by this plugin DO NOT become owned by the\n"
    "recipient.  Adult dwarves are free to adjust their own\n"
    "wardrobe, and may promptly decide to doff your gear in\n"
    "favour of their owned items.  Animals, as described above,\n"
    "will tend to discard ALL clothing immediately unless it is\n"
    "manually forbidden.  Armor items seem to be an exception;\n"
    "an animal will tend to retain an equipped suit of mail\n"
    "even if you neglect to Forbid it.\n"
    "\n"
    "Please note that armored animals are quite vulnerable to ranged\n"
    "attacks.  Unlike dwarves, animals cannot block, dodge, or deflect\n"
    "arrows, and they are slowed by the weight of their armor.\n"
    "\n"
    "This plugin currently does not support weapons.\n"
    "\n"
    "Options:\n"
    "  here, h      - process the unit and item(s) under the cursor.\n"
    "               - This option is enabled by default since the plugin\n"
    "               - does not currently support remote equpping.\n"
    "  ignore, i    - bypasses the usual item eligibility checks (such as\n"
    "               - \"Never equip gear belonging to another dwarf\" and\n"
    "               - \"Nobody is allowed to equip a Hive\".)\n"
    "  multi, m     - bypasses the 1-item-per-bodypart limit, allowing\n"
    "               - the unit to receive an unlimited amount of gear.\n"
    "               - Can be used legitimately (e.g. mitten + gauntlet)\n"
    "               - or for cheating (e.g. twelve breastplates).\n"
    "  m2, m3, m4   - alters the 1-item-per-bodypart limit, allowing\n"
    "               - each part to receive 2, 3, or 4 pieces of gear.\n"
    "  selected, s  - rather than processing all items piled at a unit's\n"
    "               - feet, process only the one item currently selected.\n"
    "  bodypart, bp - must be followed by a bodypart code (e.g. LH).\n"
    "               - Instructs the plugin to equip all available items\n"
    "               - onto this body part only.  Typically used in\n"
    "               - conjunction with the f switch (to over-armor\n"
    "               - a particular bodypart) or the i switch (to equip\n"
    "               - an unusual item onto a specific slot).\n"
    "  verbose, v   - provides detailed narration and error messages.\n"
    "               - Can be helpful in resolving failures; not needed\n"
    "               - for casual use.\n"
    "\n"
    "Examples:\n"
    "  forceequip\n"
    "    attempts to equip all of the items under the cursor onto the unit\n"
    "    under the cursor.  Uses only clothing/armor items; ignores all\n"
    "    other types.  Equips a maximum of 1 item onto each bodypart,\n"
    "    and equips only \"appropriate\" items in each slot (e.g. glove\n"
    "    --> hand).  Bypasses any item which might cause a conflict,\n"
    "    such as a Boot belonging to a different dwarf.\n"
    "  forceequip bp LH\n"
    "    attempts to equip all local items onto the left hand of the local\n"
    "    unit.  If the hand is already equipped then nothing will happen,\n"
    "    and if it is not equipped then only one appropriate item (e.g. \n"
    "    a single mitten or gauntlet) will be equipped.  This command can\n"
    "    be useful if you don't want to selectively forbid individual items\n"
    "    and simply want the unit to equip, say, a Helmet while leaving\n"
    "    the rest of the pile alone.\n"
    "  forceequip m bp LH\n"
    "    as above, but will equip ALL appropriate items onto the unit's\n"
    "    left hand.  After running this command, it might end up wearing\n"
    "    a dozen left-handed mittens.  Use with caution, and remember\n"
    "    that dwarves will tend to drop supernumary items ASAP.\n"
    "  forceequip m\n"
    "    as above, but will equip ALL appropriate items onto any\n"
    "    appropriate bodypart.  Tends to put several boots onto the right\n"
    "    foot while leaving the left foot bare.\n"
    "  forceequip m2\n"
    "    as above, but will equip up to two appropriate items onto each\n"
    "    bodypart.  Helps to balance footwear, but doesn't ensure proper\n"
    "    placement (e.g. left foot gets two socks, right foot gets two\n"
    "    shoes).  For best results, use \"selected bp LH\" and\n"
    "    \"selected bp RH\" instead.\n"
    "  forceequip i\n"
    "    performs the standard \"equip appropriate items onto appropriate\n"
    "    bodyparts\" logic, but also includes items that would normally\n"
    "    be considered ineligible (such as a sock which is owned by\n"
    "    a different dwarf).\n"
    "  forceequip bp NECK\n"
    "    attempts to equip any appropriate gear onto the Neck of the\n"
    "    local unit.  Since the plugin believes that no items are actually\n"
    "    appropriate for the Neck slot, this command does nothing.\n"
    "  forceequip i bp NECK\n"
    "    attempts to equip items from the local pile onto the Neck\n"
    "    of the local unit.  Ignores appropriateness restrictions.\n"
    "    If there's a millstone or an albatross carcass sitting on\n"
    "    the same square as the targeted unit, then there's a good\n"
    "    chance that it will end up around his neck.  For precise\n"
    "    control, remember that you can selectively forbid some of\n"
    "    the items that are piled on the ground.\n"
    "  forceequip i m bp NECK\n"
    "    as above, but equips an unlimited number of items onto the\n"
    "    targeted bodypart.  Effectively, all unforbidden items\n"
    "    (including helms, millstones, boulders, etc) will be\n"
    "    moved from the local pile and placed in the dwarf's\n"
    "    inventory (specifically, on his neck).  When used with\n"
    "    a large pile of goods, this will leave the dwarf heavily\n"
    "    encumbered and very slow to move.\n"
    "  forceequip s\n"
    "    requires that a single item be selected using the k menu.\n"
    "    This item must occupy the same square as the target unit,\n"
    "    and must be unforbidden.  Attempts to equip this single\n"
    "    item onto an appropriate slot in the unit's inventory.\n"
    "    Can serve as a quicker alternative to the selective-\n"
    "    unforbidding approach described above.\n"
    "  forceequip s m i bp HD\n"
    "    equips the selected item onto the unit's head.  Ignores\n"
    "    all possible restrictions and conflicts.  If you know\n"
    "    exactly what you want to equip, and exactly where you\n"
    "    want it to go, then this is the most straightforward\n"
    "    and reliable option.\n"
    "  forceequip v bp QQQ\n"
    "    guaranteed to fail (and accomplish nothing) because\n"
    "    there are no bodyparts called QQQ.  However, since the\n"
    "    verbose switch is used, the resulting error messages\n"
    "    will list every bodypart that the unit DOES possess.\n"
    "    May be useful if you're unfamiliar with the BP codes\n"
    "    used by Dwarf Fortress, or if you're experimenting\n"
    "    with an exotic creature.\n"
    "\n"
    ;

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "forceequip", "Move items from the ground into a unit's inventory",
        df_forceequip, false,
        forceequip_help.c_str()
    ));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static bool moveToInventory(MapExtras::MapCache &mc, df::item *item, df::unit *unit, df::body_part_raw * targetBodyPart, bool ignoreRestrictions, int multiEquipLimit, bool verbose)
{
    // Step 1: Check for anti-requisite conditions
    df::unit * itemOwner = Items::getOwner(item);
    if (ignoreRestrictions)
    {
        // If the ignoreRestrictions cmdline switch was specified, then skip all of the normal preventative rules
        if (verbose) { Core::print("Skipping integrity checks...\n"); }
    }
    else if(!item->isClothing() && !item->isArmorNotClothing())
    {
        if (verbose) { Core::printerr("Item %d is not clothing or armor; it cannot be equipped.  Please choose a different item (or use the Ignore option if you really want to equip an inappropriate item).\n", item->id); }
        return false;
    }
    else if (item->getType() != df::enums::item_type::GLOVES &&
        item->getType() != df::enums::item_type::HELM &&
        item->getType() != df::enums::item_type::ARMOR &&
        item->getType() != df::enums::item_type::PANTS &&
        item->getType() != df::enums::item_type::SHOES &&
        !targetBodyPart)
    {
        if (verbose) { Core::printerr("Item %d is of an unrecognized type; it cannot be equipped (because the module wouldn't know where to put it).\n", item->id); }
        return false;
    }
    else if (itemOwner && itemOwner->id != unit->id)
    {
        if (verbose) { Core::printerr("Item %d is owned by someone else.  Equipping it on this unit is not recommended.  Please use DFHack's Confiscate plugin, choose a different item, or use the Ignore option to proceed in spite of this warning.\n", item->id); }
        return false;
    }
    else if (item->flags.bits.in_inventory)
    {
        if (verbose) { Core::printerr("Item %d is already in a unit's inventory.  Direct inventory transfers are not recommended; please move the item to the ground first (or use the Ignore option).\n", item->id); }
        return false;
    }
    else if (item->flags.bits.in_job)
    {
        if (verbose) { Core::printerr("Item %d is reserved for use in a queued job.  Equipping it is not recommended, as this might interfere with the completion of vital jobs.  Use the Ignore option to ignore this warning.\n", item->id); }
        return false;
    }

    // ASSERT: anti-requisite conditions have been satisfied (or disregarded)


    // Step 2: Try to find a bodypart which is eligible to receive equipment AND which is appropriate for the specified item
    df::body_part_raw * confirmedBodyPart = NULL;
    int bpIndex;
    for(bpIndex = 0; bpIndex < unit->body.body_plan->body_parts.size(); bpIndex++)
    {
        df::body_part_raw * currPart = unit->body.body_plan->body_parts[bpIndex];

        // Short-circuit the search process if a BP was specified in the function call
        // Note: this causes a bit of inefficient busy-looping, but the search space is tiny (<100) and we NEED to get the correct bpIndex value in order to perform inventory manipulations
        if (!targetBodyPart)
        {
            // The function call did not specify any particular body part; proceed with normal iteration and evaluation of BP eligibility
        }
        else if (currPart == targetBodyPart)
        {
            // A specific body part was included in the function call, and we've found it; proceed with the normal BP evaluation (suitability, emptiness, etc)
        }
        else if (bpIndex < unit->body.body_plan->body_parts.size())
        {
            // The current body part is not the one that was specified in the function call, but we can keep searching
            if (verbose) { Core::printerr("Found bodypart %s; not a match; continuing search.\n", currPart->token.c_str()); }
            continue;
        }
        else
        {
            // The specified body part has not been found, and we've reached the end of the list.  Report failure.
            if (verbose) { Core::printerr("The specified body part (%s) does not belong to the chosen unit.  Please double-check to ensure that your spelling is correct, and that you have not chosen a dismembered bodypart.\n"); }
            return false;
        }

        if (verbose) { Core::print("Inspecting bodypart %s.\n", currPart->token.c_str()); }

        // Inspect the current bodypart
        if (item->getType() == df::enums::item_type::GLOVES && currPart->flags.is_set(df::body_part_raw_flags::GRASP) &&
            ((item->getGloveHandedness() == const_GloveLeftHandedness && currPart->flags.is_set(df::body_part_raw_flags::LEFT)) ||
            (item->getGloveHandedness() == const_GloveRightHandedness && currPart->flags.is_set(df::body_part_raw_flags::RIGHT))))
        {
            if (verbose) { Core::print("Hand found (%s)...", currPart->token.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::HELM && currPart->flags.is_set(df::body_part_raw_flags::HEAD))
        {
            if (verbose) { Core::print("Head found (%s)...", currPart->token.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::ARMOR && currPart->flags.is_set(df::body_part_raw_flags::UPPERBODY))
        {
            if (verbose) { Core::print("Upper body found (%s)...", currPart->token.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::PANTS && currPart->flags.is_set(df::body_part_raw_flags::LOWERBODY))
        {
            if (verbose) { Core::print("Lower body found (%s)...", currPart->token.c_str()); }
        }
        else if (item->getType() == df::enums::item_type::SHOES && currPart->flags.is_set(df::body_part_raw_flags::STANCE))
        {
            if (verbose) { Core::print("Foot found (%s)...", currPart->token.c_str()); }
        }
        else if (targetBodyPart && ignoreRestrictions)
        {
            // The BP in question would normally be considered ineligible for equipment.  But since it was deliberately specified by the user, we'll proceed anyways.
            if (verbose) { Core::print("Non-standard bodypart found (%s)...", targetBodyPart->token.c_str()); }
        }
        else if (targetBodyPart)
        {
            // The BP in question is not eligible for equipment and the ignore flag was not specified.  Report failure.
            if (verbose) { Core::printerr("Non-standard bodypart found, but it is ineligible for standard equipment.  Use the Ignore flag to override this warning.\n"); }
            return false;
        }
        else
        {
            if (verbose) { Core::print("Skipping ineligible bodypart.\n"); }
            // This body part is not eligible for the equipment in question; skip it
            continue;
        }

        // ASSERT: The current body part is able to support the specified equipment (or the test has been overridden).  Check whether it is currently empty/available.

        if (multiEquipLimit == INT_MAX)
        {
            // Note: this loop/check is skipped if the MultiEquip option is specified; we'll simply add the item to the bodyPart even if it's already holding a dozen gloves, shoes, and millstones (or whatever)
            if (verbose) { Core::print(" inventory checking skipped..."); }
            confirmedBodyPart = currPart;
            break;
        }
        else
        {
            confirmedBodyPart = currPart;        // Assume that the bodypart is valid; we'll invalidate it if we detect too many collisions while looping
            int collisions = 0;
            for (int inventoryID=0; inventoryID < unit->inventory.size(); inventoryID++)
            {
                df::unit_inventory_item * currInvItem = unit->inventory[inventoryID];
                if (currInvItem->body_part_id == bpIndex)
                {
                    // Collision detected; have we reached the limit?
                    if (++collisions >= multiEquipLimit)
                    {
                        if (verbose) { Core::printerr(" but it already carries %d piece(s) of equipment.  Either remove the existing equipment or use the Multi option.\n", multiEquipLimit); }
                        confirmedBodyPart = NULL;
                        break;
                    }
                }
            }

            if (confirmedBodyPart)
            {
                // Match found; no need to examine any other BPs
                if (verbose) { Core::print(" eligibility confirmed..."); }
                break;
            }
            else if (!targetBodyPart)
            {
                // This body part is not eligible to receive the specified equipment; return to the loop and check the next BP
                continue;
            }
            else
            {
                // A specific body part was designated in the function call, but it was found to be ineligible.
                // Don't return to the BP loop; just fall-through to the failure-reporting code a few lines below.
                break;
            }
        }
    }

    if (!confirmedBodyPart) {
        // No matching body parts found; report failure
        if (verbose) { Core::printerr("\nThe item could not be equipped because the relevant body part(s) of the unit are missing or already occupied.  Try again with the Multi option if you're like to over-equip a body part, or choose a different unit-item combination (e.g. stop trying to put shoes on a trout).\n" ); }
        return false;
    }

    if (!Items::moveToInventory(mc, item, unit, df::unit_inventory_item::Worn, bpIndex))
    {
        if (verbose) { Core::printerr("\nEquipping failed - failed to retrieve item from its current location/container/inventory.  Please move it to the ground and try again.\n"); }
        return false;
    }

    if (verbose) { Core::print("  Success!\n"); }
    return true;
}


command_result df_forceequip(color_ostream &out, vector <string> & parameters)
{
    // The "here" option is hardcoded to true, because the plugin currently doesn't support
    // equip-at-a-distance (e.g. grab items within 10 squares of the targeted unit)
    bool here = true;
    // For balance (anti-cheating) reasons, the plugin applies a limit on the number of
    // item that can be equipped on any bodypart.  This limit defaults to 1 but can be
    // overridden with cmdline switches.
    int multiEquipLimit = 1;
    // The plugin applies several pre-checks in order to reduce the risk of conflict
    // and unintended side-effects.  Most of these checks can be disabled via cmdline
    bool ignore = false;
    // By default, the plugin uses all gear piled on the selected square.  Optionally,
    // it can target only a single item (selected on the k menu) instead
    bool selected = false;
    // Most of the plugin's text output is suppressed by default.  It can be enabled
    // to provide insight into errors, and/or for debugging purposes.
    bool verbose = false;
    // By default, the plugin will mate each item to an appropriate bodypart.  This
    // behaviour can be skipped if the user specifies a particular BP in the cmdline input.
    std::string targetBodyPartCode;

    // Parse the input
    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];

        if (p == "help" || p == "?" || p == "h" || p == "/?" || p == "info" || p == "man")
        {
            out << forceequip_help << endl;
            return CR_OK;
        }
        else if (p == "here" || p == "h")
        {
            here = true;
        }
        else if (p == "ignore" || p == "i")
        {
            ignore = true;
        }
        else if (p == "multi" || p == "m")
        {
            multiEquipLimit = INT_MAX;
        }
        else if (p == "m2")
        {
            multiEquipLimit = 2;
        }
        else if (p == "m3")
        {
            multiEquipLimit = 3;
        }
        else if (p == "m4")
        {
            multiEquipLimit = 4;
        }
        else if (p == "selected" || p == "s")
        {
            selected = true;
        }
        else if (p == "verbose" || p == "v")
        {
            verbose = true;
        }
        else if (p == "bodypart" || p == "bp" )
        {
            // must be followed by bodypart code (e.g. NECK)
            if(i == parameters.size()-1 || parameters[i+1].size() == 0)
            {
                out.printerr("The bp switch must be followed by a bodypart code!\n");
                return CR_FAILURE;
            }
            targetBodyPartCode = parameters[i+1];
            i++;
        }
        else
        {
            out << p << ": Unknown command!  Type \"forceequip help\" for assistance." << endl;
            return CR_FAILURE;
        }
    }

    // Ensure that the map information is available (e.g. a game is actually in-progress)
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    // Lookup the cursor position
    int cx, cy, cz;
    DFCoord pos_cursor;

    // needs a cursor
    if (!Gui::getCursorCoords(cx,cy,cz))
    {
        out.printerr("Cursor position not found. Please enable the cursor.\n");
        return CR_FAILURE;
    }
    pos_cursor = DFCoord(cx,cy,cz);

    // Iterate over all units, process the first one whose pos == pos_cursor
    df::unit * targetUnit;
    size_t numUnits = world->units.all.size();
    for(size_t i=0; i< numUnits; i++)
    {
        targetUnit = world->units.all[i];    // tentatively assume that we have a match; then verify
        DFCoord pos_unit(targetUnit->pos.x, targetUnit->pos.y, targetUnit->pos.z);

        if (pos_unit == pos_cursor)
            break;

        if (i + 1 == numUnits)
        {
            out.printerr("No unit found at cursor!\n");
            return CR_FAILURE;
        }
    }

    // Assert: unit found.

    // If a specific bodypart was included in the command arguments, then search for it now
    df::body_part_raw * targetBodyPart = NULL;
    if (targetBodyPartCode.size() > 0) {
        for (int bpIndex = 0; bpIndex < targetUnit->body.body_plan->body_parts.size(); bpIndex ++)
        {
            // Tentatively assume that the part is a match
            targetBodyPart = targetUnit->body.body_plan->body_parts.at(bpIndex);
            if (targetBodyPart->token.compare(targetBodyPartCode) == 0)
            {
                // It is indeed a match; exit the loop (while leaving the variable populated)
                if (verbose) { out.print("Matching bodypart (%s) found.\n", targetBodyPart->token.c_str()); }
                break;
            }
            else
            {
                // Not a match; nullify the variable (it will get re-populated on the next pass through the loop)
                if (verbose) { out.printerr("Bodypart \"%s\" does not match \"%s\".\n", targetBodyPart->token.c_str(), targetBodyPartCode.c_str()); }
                targetBodyPart = NULL;
            }
        }

        if (!targetBodyPart)
        {
            // Loop iteration is complete but no match was found.
            out.printerr("The unit does not possess a bodypart of type \"%s\".  Please check the spelling or choose a different unit.\n", targetBodyPartCode.c_str());
            return CR_FAILURE;
        }
    }

    // Search for item(s)
    MapCache mc;

    // iterate over all items, process those where pos == pos_cursor
    int itemsEquipped = 0;
    int itemsFound = 0;
    int numItems = world->items.all.size();        // Normally, we iterate through EVERY ITEM in the world.  This is expensive, but currently necessary.
    if (selected) { numItems = 1; }                // If the user wants to process only the selected item, then the loop is trivialized (only one pass is needed).
    for(int i=0; i< numItems; i++)
    {
        df::item * currentItem;

        // Search behaviour depends on whether the operation is driven by cursor location or UI selection
        if (selected)
        {
            // The "search" is trivial - the selection must always cover either one or zero items
            currentItem = Gui::getSelectedItem(out);
            if (!currentItem) { return CR_FAILURE; }
        }
        else
        {
            // Lookup the current item in the world-space
            currentItem = world->items.all[i];
            // Test the item's position
            DFCoord pos_item(currentItem->pos.x, currentItem->pos.y, currentItem->pos.z);
            if (pos_item != pos_cursor)
            {
                // The item is in the wrong place; skip it
                // Note: we do not emit any notification, even with the "verbose" switch, because the search space is enormous and we'd invariably flood the UI with useless text
                continue;
            }
            // Bypass any forbidden items
            else if (currentItem->flags.bits.forbid == 1)
            {
                // The item is forbidden; skip it
                if (verbose) { out.printerr("Forbidden item encountered; skipping to next item.\n"); }
            }

        }

        // Test the item; check whether we have any grounds to disqualify/reject it
        if (currentItem->flags.bits.in_inventory == 1)
        {
            // The item is in a unit's inventory; skip it
            if (verbose) { out.printerr("Inventory item encountered; skipping to next item.\n"); }
        }
        else
        {
            itemsFound ++;                        // Track the number of items found under the cursor (for feedback purposes)
            if (moveToInventory(mc, currentItem, targetUnit, targetBodyPart, ignore, multiEquipLimit, verbose))
            {
//                // TODO TEMP EXPERIMENTAL - try to alter the item size in order to conform to its wearer
//                currentItem->getRace();
//                out.print("Critter size: %d| %d | Armor size: %d", world->raws.creatures.all[targetUnit->race]->caste[targetUnit->caste]->body_size_1, world->raws.creatures.all[targetUnit->race]->caste[targetUnit->caste]->body_size_2, currentItem->getTotalDimension());

                itemsEquipped++;                // Track the number of items successfully processed (for feedback purposes)
            }
        }
    }

    if (itemsFound == 0) {
        out.printerr("No usable items found at the cursor position.  Please choose a different location and try again.\n");
        return CR_OK;
    }


    if (itemsEquipped == 0 && !verbose) { out.printerr("Some items were found but no equipment changes could be made.  Use the /verbose switch to display the reasons for failure.\n"); }
    if (itemsEquipped > 0) { out.print("%d items equipped.\n", itemsEquipped); }
    // At this point, some changes may have been made (such as detaching items from their original position), regardless of whether any equipment changes succeeded.
    // Therefore, we must update the map.
    mc.WriteAll();

    // Note: we might expect to recalculate the unit's weight at this point, in order to account for the
    // added items.  In fact, this recalculation occurs automatically during each dwarf's "turn".
    // The slight delay in recalculation is probably not worth worrying about.

    // Work complete; report success
    return CR_OK;
}
