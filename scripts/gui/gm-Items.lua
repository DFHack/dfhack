-- Interface powered item editor.
-- TODO use this:  MechanismList = defclass(MechanismList, guidm.MenuOverlay)
local gui = require 'gui'

local my_trg
if dfhack.gui.getCurFocus() == 'item' then
	my_trg=dfhack.gui.getCurViewscreen().item
elseif dfhack.gui.getCurFocus() == 'joblist' then
	local t_screen=dfhack.gui.getCurViewscreen()
	my_trg=t_screen.jobs[t_screen.cursor_pos]
elseif dfhack.gui.getCurFocus() == 'createquota' then
	local t_screen=dfhack.gui.getCurViewscreen()
	my_trg=t_screen.orders[t_screen.sel_idx]
elseif dfhack.gui.getCurFocus() == 'dwarfmode/LookAround/Flow' then
	local t_look=df.global.ui_look_list.items[df.global.ui_look_cursor]
	my_trg=t_look.flow

elseif dfhack.gui.getSelectedUnit(true) then
	my_trg=dfhack.gui.getSelectedUnit(true)
elseif dfhack.gui.getSelectedItem(true) then
	my_trg=dfhack.gui.getSelectedItem(true)
elseif dfhack.gui.getSelectedJob(true) then
	my_trg=dfhack.gui.getSelectedJob(true)
else
	qerror("No valid target found")
end

TextInputDialog = defclass(TextInputDialog, gui.FramedScreen)

function TextInputDialog:init(prompt)
    self.frame_style=GREY_LINE_FRAME
    self.frame_title=prompt
	self.input=""
    return self
end
function TextInputDialog:onRenderBody(dc)
    dc:seek(1,1):string(self.input, COLOR_WHITE):newline()
end

local MODE_BROWSE=0
local MODE_EDIT=1
local item_screen={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "GameMaster's editor",
	stack={},
	item_count=0,
	mode=MODE_BROWSE,
	
	keys={},
	insertNew=function(self)
		--[=[local trg=self:currentTarget() -- not sure if possible...
		if trg.target and trg.target._kind and trg.target._kind=="container" then
			local thing=df.new('general_ref_contained_itemst')
			trg.target:insert('#',trg.keys[trg.selected])
		end]=]
	end,
	deleteSelected=function(self)
		local trg=self:currentTarget()
		if trg.target and trg.target._kind and trg.target._kind=="container" then
			trg.target:erase(trg.keys[trg.selected])
		end
	end,
	currentTarget=function(self)
		return self.stack[#self.stack]
	end,
	changeSelected = function (self,delta)
		local trg=self:currentTarget()
		if trg.item_count <= 1 then return end
		trg.selected = 1 + (trg.selected + delta - 1) % trg.item_count
	end,
	editSelected = function(self)
		local trg=self:currentTarget()
		if trg.target and trg.target._kind and trg.target._kind=="bitfield" then
			trg.target[trg.keys[trg.selected]]= not trg.target[trg.keys[trg.selected]]
		else
			--print(type(trg.target[trg.keys[trg.selected]]),trg.target[trg.keys[trg.selected]]._kind or "")
			local trg_type=type(trg.target[trg.keys[trg.selected]])
			if trg_type=='number' or trg_type=='string' then --ugly TODO: add metatable get selected
				self.mode=MODE_EDIT
				self.input=tostring(trg.target[trg.keys[trg.selected]])
			elseif trg_type=='userdata' then
				self:pushTarget(trg.target[trg.keys[trg.selected]])
				--local screen = mkinstance(gui.FramedScreen,item_screen):init(trg.target[trg.keys[trg.selected]]) -- does not work
				--screen:show()
			else
				print("Unknow type:"..trg_type)
				print("Subtype:"..tostring(trg.target[trg.keys[trg.selected]]._kind))
			end
		end
	end,
	cancelEdit = function(self)
		self.mode=MODE_BROWSE
		self.input=""
	end,
	commitEdit = function(self)
		local trg=self:currentTarget()
		self.mode=MODE_BROWSE
		if type(trg.target[trg.keys[trg.selected]])=='number' then
			trg.target[trg.keys[trg.selected]]=tonumber(self.input)
		elseif type(trg.target[trg.keys[trg.selected]])=='string' then
			trg.target[trg.keys[trg.selected]]=self.input
		end
	end,
    onRenderBody = function(self, dc)
		local trg=self:currentTarget()
        dc:seek(2,1):string(tostring(trg.target), COLOR_RED)
		local offset=2
		local page_offset=0
		local current_item=1
		local t_col
		if math.floor(trg.selected / (self.frame_height-offset-2)) >0 then
			page_offset=math.floor(trg.selected / (self.frame_height-offset-2))*(self.frame_height-offset-2)-1
		end
		for k,v in pairs(trg.target) do
			
			if current_item==trg.selected then
				t_col=COLOR_LIGHTGREEN
			else
				t_col=COLOR_GRAY
			end
			
			if current_item-page_offset > 0 then
				local y_pos=current_item-page_offset+offset
				dc:seek(2,y_pos):string(tostring(k),t_col)
				
				if self.mode==MODE_EDIT and current_item==trg.selected then
					dc:seek(20,y_pos):string(self.input..'_',COLOR_GREEN)
				else
					dc:seek(20,y_pos):string(tostring(v),t_col)
				end
			end
			current_item=current_item+1
		end
    end,
	
    onInput = function(self,keys)
		if self.mode==MODE_BROWSE then
			if keys.LEAVESCREEN  then
				self:popTarget()
			elseif keys.CURSOR_UP then
				self:changeSelected(-1)
			elseif keys.CURSOR_DOWN then
				self:changeSelected(1)
			elseif keys.CURSOR_UP_FAST then
				self:changeSelected(-10)
			elseif keys.CURSOR_DOWN_FAST then
				self:changeSelected(10)
			elseif keys.SELECT then
				self:editSelected()
			elseif keys.CUSTOM_ALT_E then
				--self:specialEditor()
				local screen = mkinstance(TextInputDialog):init("Input new coordinates")
				screen:show()
			elseif keys.CUSTOM_ALT_I then --insert
				self:insertNew()
			elseif keys.CUSTOM_ALT_D then --delete
				self:deleteSelected()
			end
		elseif self.mode==MODE_EDIT then
			if keys.LEAVESCREEN  then
				self:cancelEdit()
			elseif keys.SELECT then
				self:commitEdit()
			elseif keys._STRING then
				if keys._STRING==0 then
					self.input=string.sub(self.input,1,-2)
				else
					self.input=self.input.. string.char(keys._STRING)
				end
			end
		end
    end,
	pushTarget=function(self,target_to_push)
		local new_tbl={}
		new_tbl.target=target_to_push
		new_tbl.keys={}
		new_tbl.selected=1
		for k,v in pairs(target_to_push) do
			table.insert(new_tbl.keys,k)
		end
		new_tbl.item_count=#new_tbl.keys
		table.insert(self.stack,new_tbl)
	end,
	popTarget=function(self)
		table.remove(self.stack) --removes last element
		if #self.stack==0 then
			self:dismiss()
		end
	end,
	init = function(self,item_to_edit)
		self:pushTarget(item_to_edit)
		self.frame_width,self.frame_height=dfhack.screen.getWindowSize()
		return self
	end
	}



local screen = mkinstance(gui.FramedScreen,item_screen):init(my_trg)
screen:show()