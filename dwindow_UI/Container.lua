local playlist = BaseFrame:Create()
local playlist_item = BaseFrame:Create()

root:AddChild(playlist)
playlist:SetRelativeTo(TOP)

function playlist_item:Create(text)
	local o = {}
	o.text = text
	setmetatable(o, self)
	self.__index = self
	
	return o
end

function playlist_item:GetRect()
	return 0,0,500,54
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
	local l,t,r,b = self:GetRect()
	paint(l,t,r,b, get_bitmap("playlist_item_bg.png"))
	
	if self.res and self.res.width and self.res.height then
		paint(0,0,self.res.width,self.res.height, self.res, 1, bilinear_mipmap_minus_one)
	end

end


local t1234 = 0
function playlist:GetRect()
	--if self:GetChildCount() <= 0 then return 0,0,500,0 end
	--local _,t,_,b = self:GetChild(1):GetRect()
	
	return 0,t1234,500,54*10
end

function playlist:PreRender()
	t1234 = t1234 - 10
	
	--print(t1234)	
end

function playlist:AddItem(text)
	local item = playlist_item:Create(text)
		
	if self:GetChildCount() <= 0 then
		item:SetRelativeTo(TOP, self)
	else
		local relative = self:GetChild(self:GetChildCount())
		item:SetRelativeTo(BOTTOM, relative, TOP)
	end
	self:AddChild(item)
	
	return item
end


for i=1, 100 do
	playlist:AddItem("").id = i
end