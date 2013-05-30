local menu_top = BaseFrame:Create()
function menu_top:Create()
	local o = BaseFrame:Create()
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,3)
	
	return o
end

function menu_top:RenderThis()
	local l,t,r,b = 0, 0, self:GetSize()
	paint(l,t,r,b, get_bitmap("menu_top.png"))
end

local menu_bottom = BaseFrame:Create()
function menu_bottom:Create()
	local o = BaseFrame:Create()
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,3)
	
	return o
end

function menu_bottom:RenderThis()
	local l,t,r,b = 0, 0, self:GetSize()
	paint(l,t,r,b, get_bitmap("menu_bottom.png"))
end

local menu_item = BaseFrame:Create()
function menu_item:Create(text)
	local o = BaseFrame:Create()
	o.text = text
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,25)
	
	return o
end

function menu_item:OnReleaseCPU()
	if self.res and self.res.res then
		dwindow.release_resource_core(self.res.res)
	end
	self.res = nil
end

function menu_item:RenderThis()

	local text = (v3dplayer_getitem(self.id) or {}).name or "@@@@"
	self.res = self.res or test_get_text_bitmap(text)
	local l,t,r,b = 0, 0, self:GetSize()
	paint(l,t,r,b, get_bitmap("menu_item.png"))
	
	if self.res and self.res.width and self.res.height then
		local dy = (25-self.res.height)/2
		local dx = 10
		paint(dx,dy, dx+self.res.width, dy+self.res.height, self.res, 1, bilinear_mipmap_minus_one)
	end

end

function menu_item:OnMouseDown()
	if load_another then load_another(self.id) end
end



local menu = BaseFrame:Create()
function menu:Create()
	local o = BaseFrame:Create()
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,25*10+16)
	o.top = menu_top:Create()
	o.bottom = menu_bottom:Create()
	o:AddChild(o.top)
	o:AddChild(o.bottom)
	o.items = {}
	
	return o
end

function menu:PreRender()
	self.y = (self.y or 200) - 1
	self.top:SetRelativeTo(TOP, self, nil, 0, self.y)
end

function menu:HitTest()
	return false
end

function menu:AddItem(text)
	local item = menu_item:Create(text)
	self:AddChild(item)
	
	if #self.items <= 0 then
		item:SetRelativeTo(TOP, self.top, BOTTOM)
	else
		local relative = self.items[#self.items]
		item:SetRelativeTo(TOP, relative, BOTTOM)
	end
	
	table.insert(self.items, item)	
	self.bottom:SetRelativeTo(TOP, item, BOTTOM)
	
	return item
end

local sample = menu:Create()
root:AddChild(sample)
sample:SetRelativeTo(TOP)

for i=1, 50 do
	sample:AddItem("").id = i
end