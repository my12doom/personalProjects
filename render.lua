 -- global functions and variables
local x = 0
local y = 0
local cx = 99999
local cy = 99999
local cxs = {}
local cys = {}
local root
local b
local c
local d
local width = 500
local height = 500
local tbl ={}

local bitmapcache = {}

if begin_paint == nil then begin_paint = function() end end
if end_paint == nil then end_paint = function() end end
if paint == nil then paint = function() end end
if get_resource == nil then get_resource = function() end end
if load_bitmap_core == nil then load_bitmap_core = function() end end


function getname(frame)
	return tbl[frame] or tostring(frame)
end

function BeginChild(left, top, right, bottom)
	x = x + left
	y = y + top
	table.insert(cxs, cx)
	cx = right-left
	table.insert(cys, cy)
	cy = bottom - top
end

function EndChild(left, top, right, bottom)
	cx = table.remove(cxs)
	cy = table.remove(cys)
	x = x - left
	y = y - top
end

-- base class
BaseFrame =
{
	LEFT = 1,
	RIGHT = 2,
	TOP = 4,
	BOTTOM = 8,
	CENTER = 16,
}

function debug(...)
	print("--DEBUG", ...)
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

	BeginChild(self:GetClientRect())

	for i=1,#self.childs do
		local v = self.childs[i]
		if v and v.render then
			v:render(arg)
		end
	end

	EndChild(self:GetClientRect())
end

function clip(v, _min, _max)
	if v > _max then
		return _max
	elseif v < _min then
		return _min
	else
		return v
	end
end

function BaseFrame:RenderThis(arg)
	local left, top, right, bottom = self:GetRect();
	left = clip(left+x, x, x+cx)
	right = clip(right+x, x, x+cx)
	top = clip(top+y, y, y+cy)
	bottom = clip(bottom+y, y, y+cy)
	debug("default rendering", getname(self), " at", left, top, right, bottom, x, y, cx, cy)

	local resource = get_resource(0)
	res = get_bitmap("Z:\\00013.bmp")

	if self == root then resource = res end
	print("resource=", resource, res)
	paint(left, top, right, bottom, resource)
end

function BaseFrame:AddChild(frame, pos)
	if frame == nil then return end
	if frame.parent ~= nil then
		debug("illegal AddChild(), frame already have a parent : adding " .. tostring(frame) .. " to ".. tostring(self))
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
		debug("illegal RemoveChild() " .. tostring(frame) .. " from ".. tostring(self))
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

function BaseFrame:GetID()
	return self.id
end

function BaseFrame:SetRelativeTo(anchor, frame, self_anchor)
	if nil == frame or self.parent ~= frame.parent then
		debug("SetRelativeTo() fail : different parent frame  " ..  tostring(fame) .. " to " ..tostring(self))
	end

	self.relative_to = frame;
	self.self_anchor = self_anchor;
	self.anchor = anchor;
end

function BaseFrame:GetAnchorPoint()
	local left, top, right, bottom = GetClientRect()
end

function BaseFrame:GetRect()
	local left,right,top,bottom;
	if self.parent == nil then
		left, top, right, bottom = 200,200,200+width,200+height;
	else
		local l, t, r, b = self.parent:GetRect()
		local w = r-l
		local h = b-t
		left, top, right, bottom = w/4,h/4,w*3/4,h*3/4
	end
	debug("default GetRect()", left, top, right, bottom)
	return left, top, right, bottom
end

function BaseFrame:GetClientRect()
	debug("default GetClientRect()", self)
	return self:GetRect()
end

function BaseFrame:GetScreenRect()
	if self.parent == nil then
		return self:GetRect()
	else
		local pl,pt,pr,pb = self.parent:GetScreenRect()
		local l,t,r,b = self:GetRect()
		return l+pl, t+pt, r+pl, b+pt
	end
end


-- frame creation and rendering implementation
root = BaseFrame:Create()
b = BaseFrame:Create()
c = BaseFrame:Create()


function root:RenderThis(arg)
	--print("--rendering Root")
	BaseFrame.RenderThis(self, arg)
end

function b:RenderThis(arg)
	--print("--rendering B")
	BaseFrame.RenderThis(self, arg)
end

function c:RenderThis(arg)
	--print("--rendering C")
	BaseFrame.RenderThis(self, arg)
end


d = BaseFrame:Create({RenderThis = function(self,arg)
	--print("--rendering D")
	BaseFrame.RenderThis(self, arg)
end});

tbl[root] = "ROOT"
tbl[b] = "B"
tbl[c] = "C"
tbl[d] = "D"

function b:GetRect()
	return 100,100,200,200
end


function list_frame(frame, n)
	n = n or 0
	local i
	prefix = "+"

	for i=1,n do
		prefix = prefix .. "- "
	end
	print(prefix .. getname(frame))

	for i=1,frame:GetChildCount() do
		list_frame(frame:GetChild(i), n+1)
	end
end

-- layout testing
root:AddChild(d)
root:AddChild(b)
root:AddChild(c)
root:AddChild(d)
--list_frame(root)
--root:render()

--[[
root:RemoveChild(c)
root:RemoveChild(c)
c:RemoveFromParent()
c:RemoveFromParent()
print("REMOVED c")
--list_frame(root)
--root:render()

d:AddChild(c)
print("ADDED c to D")
--list_frame(root)
--root:render()

--b:RemoveFromParent()
--c:AddChild(b)
--list_frame(root)
--root:render()

--c:RemoveFromParent()
--root:AddChild(c)
--list_frame(root)
--root:render()
]]--
print(b:GetRect())

function RenderUI(view)
	releaseCache(true)
	width = GetTickCount() % 1000
	if width > 500 then width = 1000 - width end
	width = 200+width * 300 / 500
	height = width
	--print("RenderUI")
	root:render()
end

function onInitCPU()
	-- load resources here (optional)
end

function onInitGPU()
	-- commit them to GPU (optional)
end

function onInvalidateGPU()
	-- decommit all GPU objects (must)
	releaseCache(true)
end

function onInvalidateCPU()
	-- release all objects (must)
	releaseCache(false)
end

function releaseCache(is_decommit)
	if is_decommit then
		for _,v in pairs(bitmapcache) do
			decommit_resource_core(v)
		end
	else
		for _,v in pairs(bitmapcache) do
			release_resource_core(v)
			bitmapcache = {}
		end
	end
end

function get_bitmap(filename)
	if bitmapcache[filename] == nil then
		bitmapcache[filename] = load_bitmap_core(filename)
	end
	return bitmapcache[filename]
end

function unload_bitmap(filename, is_decommit)
	if not bitmapcache[filename] then return end

	if is_decommit then
		decommit_resource_core(bitmapcache[filename])
	else
		release_resource_core()
		bitmapcache[filename] = nil
	end
end
