-- interactively creates quantum stockpiles
--@ module = true

local dialogs = require('gui.dialogs')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local widgets = require('gui.widgets')

local assign_minecarts = reqscript('assign-minecarts')
local quickfort = reqscript('quickfort')
local quickfort_command = reqscript('internal/quickfort/command')
local quickfort_orders = reqscript('internal/quickfort/orders')

QuantumUI = defclass(QuantumUI, guidm.MenuOverlay)
QuantumUI.ATTRS {
    frame_inset=1,
    focus_path='quantum',
    sidebar_mode=df.ui_sidebar_mode.LookAround,
}

function QuantumUI:init()
    local cart_count = #assign_minecarts.get_free_vehicles()

    local main_panel = widgets.Panel{autoarrange_subviews=true,
                                     autoarrange_gap=1}
    main_panel:addviews{
        widgets.Label{text='Quantum'},
        widgets.WrappedLabel{
            text_to_wrap=self:callback('get_help_text'),
            text_pen=COLOR_GREY},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.EditField{
                view_id='name',
                key='CUSTOM_N',
                on_char=self:callback('on_name_char'),
                text=''},
            widgets.TooltipLabel{
                text_to_wrap='Give the quantum stockpile a custom name.',
                show_tooltip=true}}},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.CycleHotkeyLabel{
                view_id='dir',
                key='CUSTOM_D',
                options={{label='North', value={y=-1}},
                         {label='South', value={y=1}},
                         {label='East', value={x=1}},
                         {label='West', value={x=-1}}}},
            widgets.TooltipLabel{
                text_to_wrap='Set the dump direction of the quantum stop.',
                show_tooltip=true}}},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.ToggleHotkeyLabel{
                view_id='refuse',
                key='CUSTOM_R',
                label='Allow refuse/corpses',
                initial_option=false},
            widgets.TooltipLabel{
                text_to_wrap='Note that enabling refuse will cause clothes' ..
                    ' and armor in this stockpile to quickly rot away.',
                show_tooltip=true}}},
        widgets.WrappedLabel{
            text_to_wrap=('%d minecart%s available: %s will be %s'):format(
                cart_count, cart_count == 1 and '' or 's',
                cart_count == 1 and 'it' or 'one',
                cart_count > 0 and 'automatically assigned to the quantum route'
                    or 'ordered via the manager for you to assign later')},
        widgets.HotkeyLabel{
            key='LEAVESCREEN',
            label=self:callback('get_back_text'),
            on_activate=self:callback('on_back')}
    }

    self:addviews{main_panel}
end

function QuantumUI:get_help_text()
    if not self.feeder then
        return 'Please select the feeder stockpile with the cursor or mouse.'
    end
    return 'Please select the location of the new quantum stockpile with the' ..
            ' cursor or mouse.'
end

function QuantumUI:get_back_text()
    if self.feeder then
        return 'Cancel selection'
    end
    return 'Back'
end

function QuantumUI:on_back()
    if self.feeder then
        self.feeder = nil
        self:updateLayout()
    else
        self:dismiss()
    end
end

function QuantumUI:on_name_char(char, text)
    return #text < 12
end

local function is_in_extents(bld, x, y)
    local extents = bld.room.extents
    if not extents then return true end -- building is solid
    local yoff = (y - bld.y1) * (bld.x2 - bld.x1 + 1)
    local xoff = x - bld.x1
    return extents[yoff+xoff] == 1
end

function QuantumUI:select_stockpile(pos)
    local flags, occupancy = dfhack.maps.getTileFlags(pos)
    if not flags or occupancy.building == 0 then return end
    local bld = dfhack.buildings.findAtTile(pos)
    if not bld or bld:getType() ~= df.building_type.Stockpile then return end

    local tiles = {}

    for x=bld.x1,bld.x2 do
        for y=bld.y1,bld.y2 do
            if is_in_extents(bld, x, y) then
                ensure_key(ensure_key(tiles, bld.z), y)[x] = true
            end
        end
    end

    self.feeder = bld
    self.feeder_tiles = tiles

    self:updateLayout()
end

function QuantumUI:render_feeder_overlay()
    if not gui.blink_visible(1000) then return end

    local zlevel = self.feeder_tiles[df.global.window_z]
    if not zlevel then return end

    local function get_feeder_overlay_char(pos)
        return safe_index(zlevel, pos.y, pos.x) and 'X'
    end

    self:renderMapOverlay(get_feeder_overlay_char, self.feeder)
end

function QuantumUI:get_qsp_pos(cursor)
    local offsets = self.subviews.dir:getOptionValue()
    return {
        x = cursor.x + (offsets.x or 0),
        y = cursor.y + (offsets.y or 0),
        z = cursor.z
    }
end

local function is_valid_pos(cursor, qsp_pos)
    local stats = quickfort.apply_blueprint{mode='place', data='c', pos=qsp_pos,
                                            dry_run=true}
    local ok = stats.place_designated.value > 0

    if ok then
        stats = quickfort.apply_blueprint{mode='build', data='trackstop',
                                          pos=cursor, dry_run=true}
        ok = stats.build_designated.value > 0
    end

    return ok
end

function QuantumUI:render_destination_overlay()
    local cursor = guidm.getCursorPos()
    local qsp_pos = self:get_qsp_pos(cursor)
    local bounds = {x1=qsp_pos.x, x2=qsp_pos.x, y1=qsp_pos.y, y2=qsp_pos.y}

    local ok = is_valid_pos(cursor, qsp_pos)

    local function get_dest_overlay_char()
        return 'X', ok and COLOR_GREEN or COLOR_RED
    end

    self:renderMapOverlay(get_dest_overlay_char, bounds)
end

function QuantumUI:onRenderBody()
    if not self.feeder then return end

    self:render_feeder_overlay()
    self:render_destination_overlay()
end

function QuantumUI:onInput(keys)
    if self:inputToSubviews(keys) then return true end

    self:propagateMoveKeys(keys)

    local pos = nil
    if keys._MOUSE_L_DOWN then
        pos = dfhack.gui.getMousePos()
        if pos then
            guidm.setCursorPos(pos)
        end
    elseif keys.SELECT then
        pos = guidm.getCursorPos()
    end

    if pos then
        if not self.feeder then
            self:select_stockpile(pos)
        else
            local qsp_pos = self:get_qsp_pos(pos)
            if not is_valid_pos(pos, qsp_pos) then
                return
            end

            self:dismiss()
            self:commit(pos, qsp_pos)
        end
    end
end

local function get_feeder_pos(feeder_tiles)
    for z,rows in pairs(feeder_tiles) do
        for y,row in pairs(rows) do
            for x in pairs(row) do
                return xyz2pos(x, y, z)
            end
        end
    end
end

local function get_moves(move, move_back, start_pos, end_pos,
                         move_to_greater_token, move_to_less_token)
    if start_pos == end_pos then
        return move, move_back
    end
    local diff = math.abs(start_pos - end_pos)
    local move_to_greater_pattern = ('{%s %%d}'):format(move_to_greater_token)
    local move_to_greater = move_to_greater_pattern:format(diff)
    local move_to_less_pattern = ('{%s %%d}'):format(move_to_less_token)
    local move_to_less = move_to_less_pattern:format(diff)
    if start_pos < end_pos then
        return move..move_to_greater, move_back..move_to_less
    end
    return move..move_to_less, move_back..move_to_greater
end

local function get_quantumstop_data(dump_pos, feeder_pos, name)
    local move, move_back = get_moves('', '', dump_pos.z, feeder_pos.z, '<','>')
    move, move_back = get_moves(move, move_back, dump_pos.y, feeder_pos.y,
                                'Down', 'Up')
    move, move_back = get_moves(move, move_back, dump_pos.x, feeder_pos.x,
                                'Right', 'Left')

    local quantumstop_name_part, quantum_name_part = '', ''
    if name ~= '' then
        quantumstop_name_part = (' name="%s quantum"'):format(name)
        quantum_name_part = ('{givename name="%s dumper"}'):format(name)
    end

    return ('{quantumstop%s move="%s" move_back="%s"}%s')
           :format(quantumstop_name_part, move, move_back, quantum_name_part)
end

local function get_quantum_data(name)
    local name_part = ''
    if name ~= '' then
        name_part = (' name="%s"'):format(name)
    end
    return ('{quantum%s}'):format(name_part)
end

local function order_minecart(pos)
    local quickfort_ctx = quickfort_command.init_ctx{
            command='orders', blueprint_name='gui/quantum', cursor=pos}
    quickfort_orders.enqueue_additional_order(quickfort_ctx, 'wooden minecart')
    quickfort_orders.create_orders(quickfort_ctx)
end

local function create_quantum(pos, qsp_pos, feeder_tiles, name, trackstop_dir,
                              allow_refuse)
    local dsg = allow_refuse and 'yr' or 'c'
    local stats = quickfort.apply_blueprint{mode='place', data=dsg, pos=qsp_pos}
    if stats.place_designated.value == 0 then
        error(('failed to place quantum stockpile at (%d, %d, %d)')
              :format(qsp_pos.x, qsp_pos.y, qsp_pos.z))
    end

    stats = quickfort.apply_blueprint{mode='build',
                                      data='trackstop'..trackstop_dir, pos=pos}
    if stats.build_designated.value == 0 then
        error(('failed to build trackstop at (%d, %d, %d)')
              :format(pos.x, pos.y, pos.z))
    end

    local feeder_pos = get_feeder_pos(feeder_tiles)
    local quantumstop_data = get_quantumstop_data(pos, feeder_pos, name)
    stats = quickfort.apply_blueprint{mode='query', data=quantumstop_data,
                                      pos=pos}
    if stats.query_skipped_tiles.value > 0 then
        error(('failed to query trackstop at (%d, %d, %d)')
              :format(pos.x, pos.y, pos.z))
    end

    local quantum_data = get_quantum_data(name)
    stats = quickfort.apply_blueprint{mode='query', data=quantum_data,
                                      pos=qsp_pos}
    if stats.query_skipped_tiles.value > 0 then
        error(('failed to query quantum stockpile at (%d, %d, %d)')
              :format(qsp_pos.x, qsp_pos.y, qsp_pos.z))
    end
end

-- this function assumes that is_valid_pos() has already validated the positions
function QuantumUI:commit(pos, qsp_pos)
    local name = self.subviews.name.text
    local trackstop_dir = self.subviews.dir:getOptionLabel():sub(1,1)
    local allow_refuse = self.subviews.refuse:getOptionValue()
    create_quantum(pos, qsp_pos, self.feeder_tiles, name, trackstop_dir, allow_refuse)

    local message = nil
    if assign_minecarts.assign_minecart_to_last_route(true) then
        message = 'An available minecart was assigned to your new' ..
                ' quantum stockpile. You\'re all done!'
    else
        order_minecart(pos)
        message = 'There are no minecarts available to assign to the' ..
                ' quantum stockpile, but a manager order to produce' ..
                ' one was created for you. Once the minecart is' ..
                ' built, please add it to the quantum stockpile route' ..
                ' with the "assign-minecarts all" command or manually in' ..
                ' the (h)auling menu.'
    end
    -- display a message box telling the user what we just did
    dialogs.MessageBox{text=message:wrap(70)}:show()
end

if dfhack.internal.IN_TEST then
    unit_test_hooks = {
        is_in_extents=is_in_extents,
        is_valid_pos=is_valid_pos,
        get_feeder_pos=get_feeder_pos,
        get_moves=get_moves,
        get_quantumstop_data=get_quantumstop_data,
        get_quantum_data=get_quantum_data,
        create_quantum=create_quantum,
    }
end

if dfhack_flags.module then
    return
end

view = QuantumUI{}
view:show()
