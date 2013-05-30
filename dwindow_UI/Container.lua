local playlist_top = BaseFrame:Create()
function playlist_top:Create()
	local o = BaseFrame:Create()
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,3)
	
	return o
end

function playlist_top:RenderThis()
	local l,t,r,b = 0, 0, self:GetSize()
	paint(l,t,r,b, get_bitmap("menu_top.png"))
end

local playlist_bottom = BaseFrame:Create()
function playlist_bottom:Create()
	local o = BaseFrame:Create()
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,3)
	
	return o
end

function playlist_bottom:RenderThis()
	local l,t,r,b = 0, 0, self:GetSize()
	paint(l,t,r,b, get_bitmap("menu_bottom.png"))
end

local playlist_item = BaseFrame:Create()
function playlist_item:Create(text)
	local o = BaseFrame:Create()
	o.text = text
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,25)
	
	return o
end

function playlist_item:OnReleaseCPU()
	if self.res and self.res.res then
		dwindow.release_resource_core(self.res.res)
	end
	self.res = nil
end

function playlist_item:RenderThis()

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

function playlist_item:OnMouseDown()
	if load_another then load_another(self.id) end
end



local playlist = BaseFrame:Create()
function playlist:Create()
	local o = BaseFrame:Create()
	setmetatable(o, self)
	self.__index = self
	o:SetSize(198,25*10+16)
	o.top = playlist_top:Create()
	o.bottom = playlist_bottom:Create()
	o:AddChild(o.top)
	o:AddChild(o.bottom)
	o.items = {}
	
	return o
end

function playlist:PreRender()
	self.y = (self.y or 200) - 1
	self.top:SetRelativeTo(TOP, self, nil, 0, self.y)
end

function playlist:HitTest()
	return false
end

function playlist:AddItem(text)
	local item = playlist_item:Create(text)
	self:AddChild(item)
	
	if #self.items <= 0 then
		item:SetRelativeTo(TOP, self.top, BOTTOM)
	else
		local relative = self.items[#self.items]
		item:SetRelativeTo(TOP, relative, BOTTOM)
		print(item, relative)
	end
	table.insert(self.items, item)
	
	print(self.bottom:SetRelativeTo(TOP, item, BOTTOM), "ADD")
	
	return item
end

local sample = playlist:Create()
root:AddChild(sample)
sample:SetRelativeTo(TOP)

for i=1, 50 do
	sample:AddItem("").id = i
end