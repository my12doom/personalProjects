local lua_file = dwindow.loading_file
local lua_path = GetPath(lua_file)

-- black background and right mouse reciever
local oroot = BaseFrame:Create()
root:AddChild(oroot)
function oroot:PreRender(t, dt)
	self:SetSize(dwindow.width, dwindow.height)
end

function oroot:OnMouseDown(x,y,button)
	if button == VK_RBUTTON then
		dwindow.show_mouse(true)
		dwindow.popup_menu()
	end
	return true
end

-- background
local logo = BaseFrame:Create()
oroot:AddChild(logo)
logo:SetRelativeTo(CENTER)
logo:SetSize(1024,600)

function logo:RenderThis()
	if not dwindow.movie_loaded then
		local res = get_bitmap(lua_path .. "bg.png")
		paint(0,0,1024,600, res)
	end
end

-- top bar
local topbar = BaseFrame:Create()
oroot:AddChild(topbar)
topbar:SetRelativeTo(TOPLEFT)
topbar:SetRelativeTo(TOPRIGHT)
topbar:SetHeight(34)
topbar.name = DrawText("飘花电影 piaohua.com 乔布斯如何改变世界 BD中英双字1024高清.mkv")

function topbar:RenderThis()
	local res = get_bitmap(lua_path .. "bar.png")
	paint(0,0,dwindow.width,34, res)
	
	local h = (34-self.name.height)/2
	paint(15, h, 15+self.name.width, h+self.name.height, self.name)
end

-- left right
local leftright = BaseFrame:Create()
topbar:AddChild(leftright)
leftright:SetRelativeTo(RIGHT)
leftright.bmp = DrawText("LEFT / RIGHT")
leftright:SetSize(leftright.bmp.width, leftright.bmp.height)

function leftright:RenderThis()
	local w,h = self:GetSize()
	paint(0, 0, w, h, self.bmp)
end

function leftright:OnMouseDown()
	dwindow.set_swapeyes(not dwindow.get_swapeyes())
end

-- 3d/2d button
local b3d = BaseFrame:Create()
oroot:AddChild(b3d)
b3d:SetRelativeTo(BOTTOMLEFT, oroot, BOTTOMLEFT, 14, -7)
b3d:SetSize(36,30)

function b3d:RenderThis()
	-- todo: get 3d state
	if enabled3D then
		paint(0,0,36,30, get_bitmap(lua_path .. (self:IsMouseOver() and "3d_mouseover.png" or "3d_normal.png")))	
	else
		paint(0,0,36,30, get_bitmap(lua_path .. (self:IsMouseOver() and "2d_mouseover.png" or "2d_normal.png")))	
	end
end

function b3d:OnMouseDown()
	dwindow.toggle_3d()
	return true
end

-- fullscreen button
local full = BaseFrame:Create()
oroot:AddChild(full)
full:SetRelativeTo(BOTTOMRIGHT, oroot, nil, -14, -7)
full:SetSize(33,31)

function full:RenderThis()
	paint(0,0,33,31, get_bitmap(lua_path .. (self:IsMouseOver() and "fullscreen_mouseover.png" or "fullscreen.png")))
end

function full:OnMouseDown()
	dwindow.toggle_fullscreen()
	return true
end

-- play/pause button
local play = BaseFrame:Create()
oroot:AddChild(play)
play:SetRelativeTo(BOTTOM)
play:SetSize(109,38)

function play:RenderThis()
	if dwindow.is_playing() then
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "pause_mouseover.png" or "pause_normal.png")))	
	else
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "play_mouseover.png" or "play_normal.png")))	
	end
end

function play:OnMouseDown()
	dwindow.pause()
	return true
end

-- open file button
local open = BaseFrame:Create()
oroot:AddChild(open)
open:SetRelativeTo(CENTER)
open:SetSize(165,36)
open.caption = DrawText("打开文件...")

function open:RenderThis()
	if not dwindow.movie_loaded then
		paint(0,0,165,36, get_bitmap(lua_path .. "open.png"))
		local x = (165-self.caption.width)/2
		local y = (36-self.caption.height)/2
		paint(x,y,x+self.caption.width, y+self.caption.height, self.caption)
	end
end

function open:HitTest()
	return not dwindow.movie_loaded
end

function open:OnMouseDown()
	dwindow.pause()
	return true
end


-- open file button
local open2 = BaseFrame:Create()
oroot:AddChild(open2)
open2:SetRelativeTo(LEFT, open, RIGHT)
open2:SetSize(31,36)

function open2:RenderThis()
	if not dwindow.movie_loaded then
		paint(0,0,31,36, get_bitmap(lua_path .. "open_more.png"))
	end
end

function open2:HitTest()
	return not dwindow.movie_loaded
end

function open2:OnMouseDown()
	local m = 
	{
		{
			string = "打开URL...",
		},
		{
			string = "打开左右分离文件...",
		},
		{
			string = "打开文件夹...",
			on_command = function() dwindow.shutdown() end
		},
	}
	
	m = menu_builder:Create(m)
	m:popup(open:GetAbsAnchorPoint(BOTTOMLEFT))
	m:destroy()
	return true
end
