#include "adventure_control.h"
#include "DataDefs.h"

#include "df/adventure_movement_attack_creaturest.h"
#include "df/adventure_movement_building_interactst.h"
#include "df/adventure_movement_climbst.h"
#include "df/adventure_movement_hold_itemst.h"
#include "df/adventure_movement_hold_tilest.h"
#include "df/adventure_movement_optionst.h"
#include "df/adventurest.h"
#include "df/viewscreen.h"

#include "modules/Gui.h"

#include <queue>

using namespace AdventureControl;
using namespace df::enums;
using namespace DFHack;
using namespace Gui;

std::queue<interface_key::interface_key> keyQueue;

void KeyUpdate()
{
    if (!keyQueue.empty())
    {
        getCurViewscreen()->feed_key(keyQueue.front());
        keyQueue.pop();
    }
}

void SetCoord(df::coord in, RemoteFortressReader::Coord *out)
{
    out->set_x(in.x);
    out->set_y(in.y);
    out->set_z(in.z);
}

command_result MoveCommand(DFHack::color_ostream &stream, const MoveCommandParams *in)
{
/* Removed for v50 which has no adventure mode.
    auto viewScreen = getCurViewscreen();
    if (!in->has_direction())
        return CR_WRONG_USAGE;
    if (!df::global::adventure->menu == ui_advmode_menu::Default)
        return CR_OK;
    auto dir = in->direction();
    switch (dir.x())
    {
    case -1:
        switch (dir.y())
        {
        case -1:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_NW_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_NW);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_NW_UP);
                break;
            }
            break;
        case 0:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_W_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_W);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_W_UP);
                break;
            }
            break;
        case 1:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_SW_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_SW);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_SW_UP);
                break;
            }
            break;
        }
        break;
    case 0:
        switch (dir.y())
        {
        case -1:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_N_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_N);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_N_UP);
                break;
            }
            break;
        case 0:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_DOWN);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_UP);
                break;
            }
            break;
        case 1:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_S_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_S);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_S_UP);
                break;
            }
            break;
        }
        break;
    case 1:
        switch (dir.y())
        {
        case -1:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_NE_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_NE);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_NE_UP);
                break;
            }
            break;
        case 0:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_E_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_E);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_E_UP);
                break;
            }
            break;
        case 1:
            switch (dir.z())
            {
            case -1:
                viewScreen->feed_key(interface_key::A_MOVE_SE_DOWN);
                break;
            case 0:
                viewScreen->feed_key(interface_key::A_MOVE_SE);
                break;
            case 1:
                viewScreen->feed_key(interface_key::A_MOVE_SE_UP);
                break;
            }
            break;
        }
        break;
    }
*/    return CR_OK;
}

command_result JumpCommand(DFHack::color_ostream &stream, const MoveCommandParams *in)
{
/* Removed for v50 which has no adventure mode.
        if (!in->has_direction())
        return CR_WRONG_USAGE;
    if (!df::global::adventure->menu == ui_advmode_menu::Default)
        return CR_OK;
    auto dir = in->direction();
    keyQueue.push(interface_key::A_JUMP);
    int x = dir.x();
    int y = dir.y();
    if (x > 0)
    {
        for (int i = 0; i < x; i++)
        {
            keyQueue.push(interface_key::CURSOR_RIGHT);
        }
    }
    if (x < 0)
    {
        for (int i = 0; i > x; i--)
        {
            keyQueue.push(interface_key::CURSOR_LEFT);
        }
    }
    if (y > 0)
    {
        for (int i = 0; i < y; i++)
        {
            keyQueue.push(interface_key::CURSOR_DOWN);
        }
    }
    if (y < 0)
    {
        for (int i = 0; i > y; i--)
        {
            keyQueue.push(interface_key::CURSOR_UP);
        }
    }
    keyQueue.push(interface_key::SELECT);
*/
    return CR_OK;
}

command_result MenuQuery(DFHack::color_ostream &stream, const EmptyMessage *in, MenuContents *out)
{
/* Removed for v50 which has no adventure mode.
        auto advUi = df::global::adventure;

    if (advUi == NULL)
        return CR_FAILURE;

    out->set_current_menu((AdvmodeMenu)advUi->menu);

    //Fixme: Needs a proper way to control it, but for now, this is the only way to allow Armok Vision to keep going without the user needing to switch to DF.
    if (advUi->menu == ui_advmode_menu::FallAction)
    {
        getCurViewscreen()->feed_key(interface_key::OPTION1);
    }

    switch (advUi->menu)
    {
    case ui_advmode_menu::MoveCarefully:
        for (size_t i = 0; i < advUi->movements.size(); i++)
        {
            auto movement = advUi->movements[i];
            auto send_movement = out->add_movements();
            SetCoord(movement->source, send_movement->mutable_source());
            SetCoord(movement->dest, send_movement->mutable_dest());

            STRICT_VIRTUAL_CAST_VAR(climbMovement, df::adventure_movement_climbst, movement);
            if (climbMovement)
            {
                SetCoord(climbMovement->grab, send_movement->mutable_grab());
                send_movement->set_movement_type(CarefulMovementType::CLIMB);
            }
            STRICT_VIRTUAL_CAST_VAR(holdTileMovement, df::adventure_movement_hold_tilest, movement);
            if (holdTileMovement)
            {
                SetCoord(holdTileMovement->grab, send_movement->mutable_grab());
                send_movement->set_movement_type(CarefulMovementType::HOLD_TILE);
            }
        }
    default:
        break;
    }
*/
    return CR_OK;
}

command_result MovementSelectCommand(DFHack::color_ostream &stream, const dfproto::IntMessage *in)
{
    /* Removed for v50 which has no adventure mode.
    if (!(df::global::adventure->menu == ui_advmode_menu::MoveCarefully))
        return CR_OK;
    int choice = in->value();
    int page = choice / 5;
    int select = choice % 5;
    for (int i = 0; i < page; i++)
    {
        keyQueue.push(interface_key::SECONDSCROLL_PAGEDOWN);
    }
    keyQueue.push((interface_key::interface_key)(interface_key::OPTION1 + select));
*/
    return CR_OK;
}

command_result MiscMoveCommand(DFHack::color_ostream &stream, const MiscMoveParams *in)
{
    /* Removed for v50 which has no adventure mode.
        if (!df::global::adventure->menu == ui_advmode_menu::Default)
        return CR_OK;

    auto type = in->type();

    switch (type)
    {
    case AdventureControl::SET_CLIMB:
        getCurViewscreen()->feed_key(interface_key::A_HOLD);
        break;
    case AdventureControl::SET_STAND:
        getCurViewscreen()->feed_key(interface_key::A_STANCE);
        break;
    case AdventureControl::SET_CANCEL:
        getCurViewscreen()->feed_key(interface_key::LEAVESCREEN);
        break;
    default:
        break;
    }
*/
    return CR_OK;
}
