-- an experimental lighting engine for df. param: "static" to not recalc when in game. press "~" to recalculate. "`" to exit
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local render = require 'plugins.rendermax'

local levelDim=0.05
local tile_attrs = df.tiletype.attrs

local args={...}

function setCell(x,y,cell)
	cell=cell or {}
	cell.fm=cell.fm or {r=1,g=1,b=1}
	cell.bm=cell.bm or {r=1,g=1,b=1}
	cell.fo=cell.fo or {r=0,g=0,b=0}
	cell.bo=cell.bo or {r=0,g=0,b=0}
	render.setCell(x,y,cell)
end
function getCursorPos()
	local g_cursor=df.global.cursor
    if g_cursor.x ~= -30000 then
        return copyall(g_cursor)
    end
end
function falloff(color,sqDist,maxdist)
	local v1=1/(sqDist/maxdist+1)
	local v2=v1-1/(1+maxdist*maxdist)
	local v=v2/(1-1/(1+maxdist*maxdist))
	return {r=v*color.r,g=v*color.g,b=v*color.b}
end
function blend(c1,c2)
return {r=math.max(c1.r,c2.r),
		g=math.max(c1.g,c2.g),
		b=math.max(c1.b,c2.b)}
end
LightOverlay=defclass(LightOverlay,guidm.DwarfOverlay)
LightOverlay.ATTRS {
    lightMap={},
	dynamic=true,
	dirty=false,
}
function LightOverlay:init(args)
	
	self.tick=df.global.cur_year_tick_advmode
end

function lightPassable(shape)
	if shape==df.tiletype_shape.WALL or
	   shape==df.tiletype_shape.BROOK_BED or
	   shape==df.tiletype_shape.TREE then
	   return false
	else
		return true
	end
end
function circle(xm, ym,r,plot)
   local x = -r
   local y = 0
   local err = 2-2*r -- /* II. Quadrant */ 
   repeat 
      plot(xm-x, ym+y);--/*   I. Quadrant */
      plot(xm-y, ym-x);--/*  II. Quadrant */
      plot(xm+x, ym-y);--/* III. Quadrant */
      plot(xm+y, ym+x);--/*  IV. Quadrant */
      r = err;
      if (r <= y) then
	    y=y+1
		err =err+y*2+1;           --/* e_xy+e_y < 0 */
	  end
      if (r > x or err > y) then
	    x=x+1
		err =err+x*2+1;  --/* e_xy+e_x > 0 or no 2nd y-step */
	  end
   until (x >= 0);
end
function line(x0, y0, x1, y1,plot)
	local dx = math.abs(x1-x0)
	local dy = math.abs(y1-y0) 
	local sx,sy
	if x0 < x1 then sx = 1 else sx = -1 end
	if y0 < y1 then sy = 1 else sy = -1 end
	local err = dx-dy

	while true do
		if not plot(x0,y0) then
			return
		end
		if x0 == x1 and y0 == y1 then
			break
		end
		local e2 = 2*err
		if e2 > -dy then 
			err = err - dy
			x0 = x0 + sx
		end
		if x0 == x1 and y0 == y1 then 
			if not plot(x0,y0) then
				return
			end
			break
		end 
		if e2 <  dx then 
			err = err + dx
			y0 = y0 + sy 
		end
	end
end
function LightOverlay:calculateFovs()
	self.fovs=self.fovs or {}
	self.precalc=self.precalc or {}
	for k,v in ipairs(self.fovs) do
		self:calculateFov(v.pos,v.radius,v.color)
	end
end
function LightOverlay:calculateFov(pos,radius,color)
	local vp=self:getViewport()
	local map = self.df_layout.map
	local ray=function(tx,ty)
		local power=copyall(color)
		local lx=pos.x
		local ly=pos.y
		local setTile=function(x,y)
			if x>0 and y>0 and x<=map.width and y<=map.height then
				local dtsq=(lx-x)*(lx-x)+(ly-y)*(ly-y)
				local dt=math.sqrt(dtsq)
				local tile=x+y*map.width
				if self.precalc[tile] then
					local tcol=blend(self.precalc[tile],power)
					if tcol.r==self.precalc[tile].r and tcol.g==self.precalc[tile].g and self.precalc[tile].b==self.precalc[tile].b 
						and dtsq>0 then
						return false
					end
				end
				local ocol=self.lightMap[tile] or {r=0,g=0,b=0}
				local ncol=blend(power,ocol)
				
				self.lightMap[tile]=ncol
				local v=self.ocupancy[tile]
				if dtsq>0 then
					power.r=power.r*(v.r^dt)
					power.g=power.g*(v.g^dt)
					power.b=power.b*(v.b^dt)
				end
				lx=x
				ly=y
				local pwsq=power.r*power.r+power.g*power.g+power.b*power.b
				return pwsq>levelDim*levelDim
			end
			return false
		end
		line(pos.x,pos.y,tx,ty,setTile)
	end
	circle(pos.x,pos.y,radius,ray)
end
function LightOverlay:placeLightFov(pos,radius,color)
	local map = self.df_layout.map
	local tile=pos.x+pos.y*map.width
	local ocol=self.precalc[tile] or {r=0,g=0,b=0}
	local ncol=blend(color,ocol)
	self.precalc[tile]=ncol
	local ocol=self.lightMap[tile] or {r=0,g=0,b=0}
	local ncol=blend(color,ocol)
	self.lightMap[tile]=ncol
	table.insert(self.fovs,{pos=pos,radius=radius,color=color})
end
function LightOverlay:placeLightFov2(pos,radius,color,f,rays)
	f=f or falloff
	local raycount=rays or 25
	local vp=self:getViewport()
	local map = self.df_layout.map
	local off=math.random(0,math.pi)
	local done={}
	for d=0,math.pi*2,math.pi*2/raycount do
		local dx,dy
		dx=math.cos(d+off)
		dy=math.sin(d+off)
		local cx=0
		local cy=0
		
		for dt=0,radius,0.01 do
			if math.abs(math.floor(dt*dx)-cx)>0 or math.abs(math.floor(dt*dy)-cy)> 0then
				local x=cx+pos.x
				local y=cy+pos.y
				
				if x>0 and y>0 and x<=map.width and y<=map.height and not done[tile] then
					local tile=x+y*map.width
					done[tile]=true
					local ncol=f(color,dt*dt,radius)
					local ocol=self.lightMap[tile] or {r=0,g=0,b=0}
					ncol=blend(ncol,ocol)
					self.lightMap[tile]=ncol
					
					
					if --(ncol.r==ocol.r and ncol.g==ocol.g and ncol.b==ocol.b) or
						 not self.ocupancy[tile] then
						break
					end
				end
				cx=math.floor(dt*dx)
				cy=math.floor(dt*dy)
			end
		end
	end
end
function LightOverlay:placeLight(pos,radius,color,f)
	f=f or falloff
	local vp=self:getViewport()
	local map = self.df_layout.map

	for i=-radius,radius do
	for j=-radius,radius do
		local x=pos.x+i+1
		local y=pos.y+j+1
		if x>0 and y>0 and x<=map.width and y<=map.height then
			local tile=x+y*map.width
			local ncol=f(color,(i*i+j*j),radius)
			local ocol=self.lightMap[tile] or {r=0,g=0,b=0}
			self.lightMap[tile]=blend(ncol,ocol)
		end
	end
	end
end
function LightOverlay:calculateLightLava()
	local vp=self:getViewport()
	local map = self.df_layout.map
	for i=map.x1,map.x2 do
	for j=map.y1,map.y2 do
		local pos={x=i+vp.x1-1,y=j+vp.y1-1,z=vp.z}
		local pos2={x=i+vp.x1-1,y=j+vp.y1-1,z=vp.z-1}
		local t1=dfhack.maps.getTileFlags(pos)
		local tt=dfhack.maps.getTileType(pos)
		if tt then
			local shape=tile_attrs[tt].shape
			local t2=dfhack.maps.getTileFlags(pos2)
			if  (t1 and t1.liquid_type and t1.flow_size>0) or 
				(shape==df.tiletype_shape.EMPTY and t2 and t2.liquid_type and t2.flow_size>0) then
				--self:placeLight({x=i,y=j},5,{r=0.8,g=0.2,b=0.2})
				self:placeLightFov({x=i,y=j},5,{r=0.8,g=0.2,b=0.2},nil)
			end
		end
	end
	end
end
function LightOverlay:calculateLightSun()
	local vp=self:getViewport()
	local map = self.df_layout.map
	for i=map.x1,map.x2+1 do
	for j=map.y1,map.y2+1 do
		local pos={x=i+vp.x1-1,y=j+vp.y1-1,z=vp.z}
		
		local t1=dfhack.maps.getTileFlags(pos)
		
		if  (t1 and t1.outside ) then
			
			self:placeLightFov({x=i,y=j},15,{r=1,g=1,b=1},nil)
		end
	end
	end
end
function LightOverlay:calculateLightCursor()
	local c=getCursorPos()
	
	if c then
		
		local vp=self:getViewport()
		local pos=vp:tileToScreen(c)
		--self:placeLight(pos,11,{r=0.96,g=0.84,b=0.03})
		self:placeLightFov({x=pos.x+1,y=pos.y+1},11,{r=0.96,g=0.84,b=0.03})
		
	end
end
function LightOverlay:buildOcupancy()
	self.ocupancy={}
	local vp=self:getViewport()
	local map = self.df_layout.map
	for i=map.x1,map.x2+1 do
	for j=map.y1,map.y2+1 do
		local pos={x=i+vp.x1-1,y=j+vp.y1-1,z=vp.z}
		local tile=i+j*map.width
		local tt=dfhack.maps.getTileType(pos)
		local t1=dfhack.maps.getTileFlags(pos)
		if tt then
			local shape=tile_attrs[tt].shape
			if not lightPassable(shape) then
				self.ocupancy[tile]={r=0,g=0,b=0}
			else
				if t1 and not t1.liquid_type and t1.flow_size>2 then
					self.ocupancy[tile]={r=0.5,g=0.5,b=0.7}
				else
					self.ocupancy[tile]={r=0.8,g=0.8,b=0.8}
				end
			end
		end
	end
	end
end
function LightOverlay:changed()
	if self.dirty or self.tick~=df.global.cur_year_tick_advmode then
		self.dirty=false
		self.tick=df.global.cur_year_tick_advmode
		return true
	end
	return false
end
function LightOverlay:makeLightMap()
	if not self:changed() then
		return
	end
	self.fovs={}
	self.precalc={}
	self.lightMap={}
	
	self:buildOcupancy()
	self:calculateLightCursor()
	self:calculateLightLava()
	self:calculateLightSun()
	
	self:calculateFovs()
end
function LightOverlay:onIdle()
	self._native.parent:logic()
end
function LightOverlay:render(dc)
	if self.dynamic then
		self:makeLightMap()
	end
	self:renderParent()
	local vp=self:getViewport()
	local map = self.df_layout.map
	
	self.lightMap=self.lightMap or {}
	render.lockGrids()
	render.invalidate({x=map.x1,y=map.y1,w=map.width,h=map.height})
	render.resetGrids()
	for i=map.x1,map.x2 do
	for j=map.y1,map.y2 do
		local v=self.lightMap[i+j*map.width]
		if v then
			setCell(i,j,{fm=v,bm=v})
		else
			local dimRgb={r=levelDim,g=levelDim,b=levelDim}
			setCell(i,j,{fm=dimRgb,bm=dimRgb})
		end
	end
	end
	render.unlockGrids()
	
end
function LightOverlay:onDismiss()
	render.lockGrids()
	render.resetGrids()
	render.invalidate()
	render.unlockGrids()
	
end
function LightOverlay:onInput(keys)
	if keys.STRING_A096 then
		self:dismiss()
	else
		self:sendInputToParent(keys)
		
		if keys.CHANGETAB then
			self:updateLayout()
		end
		if keys.STRING_A126 and not self.dynamic then
			self:makeLightMap()
		end
		self.dirty=true
	end
end
if not render.isEnabled() then
	qerror("Lua rendermode not enabled!")
end
local dyn=true
if #args>0 and args[1]=="static" then dyn=false end
local lview = LightOverlay{ dynamic=dyn}
lview:show()