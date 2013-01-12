local x = 0
local y = 0
local cx = 99999
local cy = 99999
local cxs = {}
local cys = {}
local bitmapcache = {}


if paint_core == nil then paint_core = function() end end
if get_resource == nil then get_resource = function() end end
if load_bitmap_core == nil then load_bitmap_core = function() end end
if bit32 == nil then bit32 = require("bit")end

if dwindow and dwindow.execute_luafile then print(dwindow.execute_luafile("D:\\private\\bp.lua")) end

function paint(left, top, right, bottom, res)
	left = left+x
	right = right+x
	top = top+y
	bottom = bottom+y
	
	dwindow.set_clip_rect_core(x,y,x+cx,y+cy)

	return dwindow.paint_core(left, top, right, bottom, res)
end

function BeginChild(left, top, right, bottom)
	x = x + left
	y = y + top
	table.insert(cxs, cx)
	cx = right-left
	table.insert(cys, cy)
	cy = bottom - top
end

function BeginChild2(left, top, right, bottom)
	x = left
	y = top
	cx = right - left
	cy = bottom - top
end

function EndChild(left, top, right, bottom)
	cx = table.remove(cxs)
	cy = table.remove(cys)
	x = x - left
	y = y - top
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

function debug(...)
	--print("--DEBUG", ...)
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

function BaseFrame:render(arg)

	self:RenderThis(self, arg)


	for i=1,#self.childs do
		local v = self.childs[i]
		if v and v.render then
			local l,r,t,b = v:GetAbsRect();
			BeginChild2(l,r,t,b)
			v:render(arg)
			--EndChild(l,r,t,b)
		end
	end

end


function BaseFrame:RenderThis(arg)
	local left, top, right, bottom = self:GetRect()
	debug("default rendering", self, " at", left, top, right, bottom, x, y, cx, cy)
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
		debug("illegal RemoveFromParent() : " .. tostring(self) .. " has no parent")
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
	self.relative_to = frame;
	self.relative_point = point;
	self.anchor = anchor;
end

function BaseFrame:GetAbsAnchorPoint(point)
	local l, t, r, b = self:GetAbsRect()
	local w, h = r-l, b-t
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
	
	return px, py
end

function BaseFrame:GetAbsRect(debug)
	local relative = self.relative_to or self.parent				-- use parent as default relative_to frame
	local l,t,r,b = 0,0,dwindow.width,dwindow.height				-- use screen as default relative_to frame if no parent & relative
	local relative_point = self.relative_point or TOPLEFT
	local anchor = self.anchor or relative_point
	
	if debug then print(anchor) end

	if relative then
		local l,t,r,b = relative:GetAbsRect()						-- get their AbsRect
		local l2,t2,r2,b2 = self:GetRect()							-- get our Rect
		local w,h = r-l, b-t										-- their width and height
		local w2,h2 = r2-l2, b2-t2									-- out width and height

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
		if debug then 
			print("px, py=", px, py)
			print(l,t,r,b)
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
		if debug then print("px2, py2=", px2, py2) end

		return l2+px-px2,t2+py-py2,r2+px-px2,b2+py-py2
	end
	
	return l,t,r,b
end

function BaseFrame:GetRect()
	return 0,0,dwindow.width,dwindow.height
end

root = BaseFrame:Create()

-- the Main Render function
function RenderUI(view)
	root:render(view)
end


function OnReleaseGPU()
	-- decommit all GPU resources (must)
	print("OnReleaseGPU")
	releaseCache(true)
end

function OnReleaseCPU()
	-- release all resources (must)
	print("OnReleaseCPU")
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
	if bitmapcache[filename] == nil then
		local msg
		bitmapcache[filename], msg = dwindow.load_bitmap_core(filename)
		if msg then error(msg, filename) end
	end
	return bitmapcache[filename]
end

function unload_bitmap(filename, is_decommit)
	if not bitmapcache[filename] then return end

	if is_decommit then
		dwindow.decommit_resource_core(bitmapcache[filename])
	else
		dwindow.release_resource_core()
		bitmapcache[filename] = nil
	end
end

bp()
