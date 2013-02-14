local rect = {0,0,99999,99999,0,0}
local rects = {}
local bitmapcache = {}

if paint_core == nil then paint_core = function() end end
if get_resource == nil then get_resource = function() end end
if load_bitmap_core == nil then load_bitmap_core = function() end end
if bit32 == nil then bit32 = require("bit")end

function BeginChild(left, top, right, bottom)
	local left_unclip, top_unclip = left, top
	left = math.max(rect[1], left)
	top = math.max(rect[2], top)
	right = math.min(rect[3], right)
	bottom = math.min(rect[4], bottom)
	local new_rect = {left, top, right, bottom, left_unclip, top_unclip}
	table.insert(rects, rect)
	rect = new_rect
	
	dwindow.set_clip_rect_core(left, top, right, bottom)
end

function EndChild(left, top, right, bottom)
	rect = table.remove(rects)
	dwindow.set_clip_rect_core(rects[1], rects[2], rects[3], rects[4])
end

-- base class
BaseFrame ={}

LEFT = 1
RIGHT = 2
TOP = 4
BOTTOM = 8
CENTER = 0
TOPLEFT = TOP + LEFT
TOPRIGHT = TOP + RIGHT
BOTTOMLEFT = BOTTOM + LEFT
BOTTOMRIGHT = BOTTOM + RIGHT

function debug_print(...)
	--print("--DEBUG", ...)
end

function info(...)
	--print("--INFO", ...)
end

function error(...)
	print("--ERROR:", ...)
end

function BaseFrame:Create(o)
	o = o or {}
	o.childs = {}
	setmetatable(o, self)
	self.__index = self

	return o
end

function BaseFrame:render(...)

	self:RenderThis(...)

	for i=1,#self.childs do
		local v = self.childs[i]
		if v and v.render then
			local l,t,r,b = v:GetAbsRect();
			BeginChild(l,t,r,b)
			v:render(...)
			EndChild(l,t,r,b)
		end
	end

end


function BaseFrame:RenderThis(...)
	local left, top, right, bottom = self:GetRect()
	debug_print("default rendering(draw nothing)", self, " at", left, top, right, bottom, rect[1], rect[2], rect[3], rect[4])
end

function BaseFrame:AddChild(frame, pos)
	if frame == nil then return end
	if frame.parent ~= nil then
		error("illegal AddChild(), frame already have a parent : adding " .. tostring(frame) .. " to ".. tostring(self))
		return
	end

	frame.parent = self;
	if pos ~= nil then
		table.insert(self.childs, n, frame)
	else
		table.insert(self.childs, frame)
	end
end

function BaseFrame:IsParentOf(frame, includeParentParent)
	if frame == nil then
		return false
	end
	
	if frame.parent == self then
		return true
	end
		
	if not includeParentParent then 
		return false
	else
		return self:IsParentOf(frame.parent) 
	end
end

function BaseFrame:RemoveChild(frame)
	if frame == nil then return end
	if frame.parent ~= self then		-- illegal removal
		error("illegal RemoveChild() " .. tostring(frame) .. " from ".. tostring(self))
		return
	end

	for i=1,#self.childs do
		if self.childs[i] == frame then
			table.remove(self.childs, i)
			break;
		end
	end

	frame.parent = nil
end

function BaseFrame:RemoveFromParent()
	if self.parent then
		self.parent:RemoveChild(self)
	else
		debug_print("illegal RemoveFromParent() : " .. tostring(self) .. " has no parent")
	end
end

function BaseFrame:GetParent()
	return self.parent
end

function BaseFrame:GetChild(n)
	return self.childs[n]
end

function BaseFrame:GetChildCount()
	return #self.childs
end

function BaseFrame:SetRelativeTo(frame, point, anchor)
	if self==frame or self:IsParentOf(frame) then
		error("SetRelativeTo() failed: target is same or parent of this frame")
		return
	end

	self.relative_to = frame;
	self.relative_point = point;
	self.anchor = anchor;
end

function BaseFrame:BringToTop(include_parent)
	if not self.parent then return end
	local parent = self.parent
	self:RemoveFromParent()
	parent:AddChild(self)
	
	if include_parent then parent:BringToTop(include_parent) end
end

function BaseFrame:HitTest(x, y)	-- client point
	-- default hittest: by Rect
	local l,t,r,b = self:GetRect()
	info("HitTest", x, y, self, l, t, r, b)
	if 0<=x and x<r-l and 0<=y and y<b-t then
		return true
	else
		return false
	end
end

function BaseFrame:GetFrameByPoint(x, y) -- abs point
	local result = nil
	local l,t,r,b = self:GetAbsRect()
	if l<=x and x<r and t<=y and y<b then
		local _,_,_,_,dx,dy = self:GetRect()
		dx, dy = dx or 0, dy or 0
		if self:HitTest(x-l, y-t) then
			result = self
		end
		for i=1,self:GetChildCount() do
			result = self:GetChild(i):GetFrameByPoint(x, y) or result
		end
		
	end
	
	return result
end

function BaseFrame:GetAbsAnchorPoint(point)
	local l, t, r, b = self:GetAbsRect()
	local w, h = r-l, b-t
	local px, py = l+w/2, t+h/2
	if bit32.band(point, LEFT) == LEFT then
		px = l
	elseif bit32.band(point, RIGHT) == RIGHT then
		px = l+w
	end

	if bit32.band(point, TOP) == TOP then
		py = t
	elseif bit32.band(point, BOTTOM) == BOTTOM then
		py = t+h
	end
	
	return px, py
end

function BaseFrame:GetAbsRect()
	local relative = self.relative_to or self.parent				-- use parent as default relative_to frame
	local l,t,r,b = 0,0,dwindow.width or 1,dwindow.height or 1		-- use screen as default relative_to frame if no parent & relative
	local relative_point = self.relative_point or TOPLEFT
	local anchor = self.anchor or relative_point
	
	if relative then
		local l,t,r,b = relative:GetAbsRect()						-- get their AbsRect
		local l2,t2,r2,b2,dx,dy = self:GetRect()					-- get our Rect
		local w,h = r-l, b-t										-- their width and height
		local w2,h2 = r2-l2, b2-t2									-- out width and height
		dx,dy = dx or 0, dy or 0

		local px, py = l+w/2, t+h/2
		if bit32.band(relative_point, LEFT) == LEFT then
			px = l
		elseif bit32.band(relative_point, RIGHT) == RIGHT then
			px = l+w
		end

		if bit32.band(relative_point, TOP) == TOP then
			py = t
		elseif bit32.band(relative_point, BOTTOM) == BOTTOM then
			py = t+h
		end

		local px2, py2 = w2/2, h2/2
		if bit32.band(anchor, LEFT) == LEFT then
			px2 = 0
		elseif bit32.band(anchor, RIGHT) == RIGHT then
			px2 = w2
		end

		if bit32.band(anchor, TOP) == TOP then
			py2 = 0
		elseif bit32.band(anchor, BOTTOM) == BOTTOM then
			py2 = h2
		end

		return l2+px-px2+dx,t2+py-py2+dy,r2+px-px2+dx,b2+py-py2+dy
	end
	
	return l,t,r,b
end

function BaseFrame:GetRect()
	return -99999,-99999,99999,99999
end

-- time events, usally happen on next render
function BaseFrame:OnUpdate(time, delta_time) end

-- for these mouse events or focus related events, return anything other than false and nil cause it to be sent to its parents
function BaseFrame:OnMouseDown(button, x, y) end
function BaseFrame:OnMouseUp(button, x, y) end
function BaseFrame:OnClick(button, x, y) end
function BaseFrame:OnDoubleClick(button, x, y) end
function BaseFrame:OnMouseOver() end
function BaseFrame:OnMouseLeave() end
function BaseFrame:OnMouseMove() end
function BaseFrame:OnMouseWheel() end
function BaseFrame:OnKeyDown() end
function BaseFrame:OnKeyUp() end
function BaseFrame:OnDropFile(a,b,c)
	print("BaseFrame:OnDropFile")
end


-- for these mouse events or focus related events, return anything other than false and nil cause it to be sent to its parents


-- window or dshow or etc callback
function OnDirectshowEvents() end
function OnMove() end
function OnSize() end

-- some controlling function
function StartDragging() end
function StartResizing() end

root = BaseFrame:Create()
root.name = "ROOT"
function BaseFrame:OnEvent(event, ...)
	return (self[event] and self[event](self, ...)) or (self.parent and self.parent:OnEvent(event, ...))
end

function BaseFrame:BroadCastEvent(event, ...)
	if self[event] then
		self[event](self, ...)
	end
	for _,v in ipairs(self.childs) do
		v:BroadCastEvent(event, ...)
	end
end

-- the Main Render function
local last_render_time = 0
function RenderUI(view)
	local delta_time = 0;
	if last_render_time > 0 then delta_time = dwindow.GetTickCount() - last_render_time end
	last_render_time = dwindow.GetTickCount();
	local t = dwindow.GetTickCount()
	root:BroadCastEvent("OnUpdate", last_render_time, delta_time)
	local dt = dwindow.GetTickCount() -t
	t = dwindow.GetTickCount()
	
	root:render(view)
	
	local dt2 = dwindow.GetTickCount() -t
	
	if dt > 0 or dt2 > 0 then
		info(string.format("slow RenderUI() : OnUpdate() cost %dms, render() cost %dms", dt, dt2))
	end
end


-- GPU resource management
function OnInitCPU()
	debug_print("OnInitCPU")
	-- load resources here (optional)
end

function OnInitGPU()
	debug_print("OnInitGPU")
	-- commit them to GPU (optional)
	-- handle resize changes here (must)
end

function OnReleaseGPU()
	-- decommit all GPU resources (must)
	debug_print("OnReleaseGPU")
	releaseCache(true)
end

function OnReleaseCPU()
	-- release all resources (must)
	debug_print("OnReleaseCPU")
	releaseCache(false)
end

function releaseCache(is_decommit)
	if is_decommit then
		for _,v in pairs(bitmapcache) do
			dwindow.decommit_resource_core(v)
		end
	else
		for _,v in pairs(bitmapcache) do
			dwindow.release_resource_core(v)
			bitmapcache = {}
		end
	end
end

function get_bitmap(filename)
	if string.find(filename, ":\\") ~= 2 then filename = GetCurrentLuaPath(1) .. filename end

	if bitmapcache[filename] == nil then
		local res, width, height = dwindow.load_bitmap_core(filename)		-- width is also used as error msg output.
		
		bitmapcache[filename] = {res = res, width = width, height = height, filename=filename}
		if not res then
			error(width, filename)
			bitmapcache[filename].width = nil
			bitmapcache[filename].error_msg = width
		end
		info("loaded", filename, width, height)
	end
	
	-- reset ROI
	local rtn = bitmapcache[filename]
	rtn.left = 0
	rtn.right = 0
	rtn.top = 0
	rtn.bottom = 0
	return rtn
end

function unload_bitmap(filename, is_decommit)
	local tbl = bitmapcache[filename]
	if not tbl or not istable(tbl) then return end

	if is_decommit then
		dwindow.decommit_resource_core(bitmapcache[filename].res)
	else
		dwindow.release_resource_core(bitmapcache[filename].res)
		bitmapcache[filename] = nil
	end
end

-- set region of interest
function set_bitmap_rect(bitmap, left, top, right, bottom)
	if not bitmap or "table" ~= type(bitmap) then return end
	bitmap.left = left
	bitmap.right = right
	bitmap.top = top
	bitmap.bottom = bottom
end

function paint(left, top, right, bottom, bitmap, alpha)
	--bitmap = get_bitmap("Z:\\skin\\solid.png")
	if not bitmap or not bitmap.res then return end
	local x,y  = rect[5], rect[6]
	local a = alpha or 1.0
	return dwindow.paint_core(left+x, top+y, right+x, bottom+y, bitmap.res, bitmap.left, bitmap.top, bitmap.right, bitmap.bottom, a)
end




--- default mouse event delivering

function OnMouseEvent(event,x,y,...)
	local frame = root:GetFrameByPoint(x,y)
	if frame then
		local l,_,t = frame:GetAbsRect()
		return frame:OnEvent(event, x-l, y-t, ...)
	end
end

function OnMouseDown(x, y, key)
	return OnMouseEvent("OnMouseDown",x,y,key)
end



-- helper functions
function GetCurrentLuaPath(offset)
	local info = debug.getinfo(2+(offset or 0), "Sl")
	return GetPath(string.sub(info.source, 2))
end

function GetPath(pathname)
	local t = string.reverse(pathname)
	t = string.sub(t, string.find(t, "\\") or 1)
	return string.reverse(t)
	
end


if dwindow and dwindow.execute_luafile then print(dwindow.execute_luafile(GetCurrentLuaPath() .. "bp.lua")) end
