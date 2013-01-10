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
local g_width = 500
local g_height = 500
local tbl ={}
local bitmapcache = {}

if paint_core == nil then paint_core = function() end end
if get_resource == nil then get_resource = function() end end
if load_bitmap_core == nil then load_bitmap_core = function() end end

print("Hello!!!")
print(dwindow.GetTickCount())

function getname(frame)
	return tbl[frame] or tostring(frame)
end


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
	--print("--DEBUG", ...)
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
			local l,r,t,b = v:GetRect();
			BeginChild(l,r,t,b)
			v:render(arg)
			EndChild(l,r,t,b)
		end
	end

end


function BaseFrame:RenderThis(arg)
	local left, top, right, bottom = self:GetRect()
	debug("default rendering", getname(self), " at", left, top, right, bottom, x, y, cx, cy)
	local resource = get_resource(0)
	--paint(left, top, right, bottom, resource)
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
	return 0,0,g_width,g_height
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
logo = BaseFrame:Create()
toolbar_bg = BaseFrame:Create()


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

function logo:GetRect()
	return 0,0,g_width,g_height
end

function logo:RenderThis(arg)
	if not movie_loaded then
		local 	res = get_bitmap("Z:\\skin\\logo2.bmp")

		paint(g_width/2 - 960, g_height/2 - 540, g_width/2 + 960, g_height/2 + 540, res)
	end
end

function toolbar_bg:GetRect()
	local toolbar_height = 65
	return 0, g_height - toolbar_height, g_width, g_height
end

function toolbar_bg:RenderThis(arg)
	local l,t,r,b = self:GetRect()
	local res = get_bitmap("Z:\\skin\\toolbar_background.png");
	paint(0,0,r-l,b-t, res)
end

d = BaseFrame:Create()


-- helper function
tbl[root] = "ROOT"
tbl[b] = "B"
tbl[c] = "C"
tbl[d] = "D"

function b:GetRect()
	return 0,0,200,200
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
root:AddChild(c)
c:AddChild(d)
d:AddChild(b)
root:AddChild(d)
root:AddChild(logo)
root:AddChild(toolbar_bg)

-- the Main Render function
function RenderUI(view)
	g_width = dwindow.width
	g_height = dwindow.height
	root:render()
end


-- GPU resource management
function OnInitCPU()
	-- load resources here (optional)
	print("OnInitCPU")
end

function OnInitGPU()
	print("OnInitGPU")
	-- commit them to GPU (optional)
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
		if msg then debug(msg, filename) end
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
