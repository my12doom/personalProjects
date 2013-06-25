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


-- bottom bar
local bottombar = BaseFrame:Create()
oroot:AddChild(bottombar)
bottombar:SetRelativeTo(BOTTOMLEFT)
bottombar:SetRelativeTo(BOTTOMRIGHT)
bottombar:SetHeight(44)

function bottombar:RenderThis()
	local res = get_bitmap(lua_path .. "bar.png")
	paint(0,0,dwindow.width,44, res)
end


-- top bar
local topbar = BaseFrame:Create()
oroot:AddChild(topbar)
topbar:SetRelativeTo(TOPLEFT)
topbar:SetRelativeTo(TOPRIGHT)
topbar:SetHeight(30)
topbar.item = " "

function topbar:PreRender()
	local left, right = playlist:current_item();
	local dis = GetName(left) or " "
	if right then dis = dis .. " + " .. GetName(right) end
	if not dwindow.movie_loaded then dis = " " end
	if self.item ~= dis then
		self.item = dis
		
		if self.res then
			dwindow.release_resource_core(self.res.res)
		end
		self.res = DrawText(self.item)
	end
end

function topbar:RenderThis()
	local res = get_bitmap(lua_path .. "bar.png")
	paint(0,0,dwindow.width,34, res)
	
	if self.res then 
		local h = (34-self.res.height)/2
		paint(15, h, 15+self.res.width, h+self.res.height, self.res)
	end
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
bottombar:AddChild(b3d)
b3d:SetRelativeTo(BOTTOMLEFT, bottombar, BOTTOMLEFT, 14, -7)
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
bottombar:AddChild(full)
full:SetRelativeTo(BOTTOMRIGHT, bottombar, nil, -14, -7)
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
bottombar:AddChild(play)
play:SetRelativeTo(BOTTOM)
play:SetSize(109,38)

function play:RenderThis()
	if dwindow.is_playing() then
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "pause_mousedown.png" or "pause_mouseover.png")))	
	else
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "play_mousedown.png" or "play_mouseover.png")))	
	end
end

function play:OnMouseDown()
	if not dwindow.movie_loaded then
		local file = dwindow.OpenFile()
		if file then
			playlist:play(file)
		end
	else
		dwindow.pause()
	end
	
	return true
end


-- progress bar
local progress = BaseFrame:Create()
oroot:AddChild(progress)
progress:SetRelativeTo(TOPLEFT, bottombar, nil, 0, -8)
progress:SetRelativeTo(TOPRIGHT, bottombar, nil, 0, -8)
progress:SetHeight(16+6)

function progress:RenderThis()
	local v = (dwindow.tell() or 0) / (dwindow.total() or 1)
	v = math.max(math.min(v, 1), 0)
	local l,_,r = self:GetAbsRect()
	
	paint(0,8,r-l, 8+(self:IsMouseOver() and 6 or 2), get_bitmap(lua_path .. "progress_bg.png"))
	paint(0,8,(r-l)*v,8+6, get_bitmap(lua_path .. (self:IsMouseOver() and "progress_thick.png" or "progress.png")))
end

function progress:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local l,_,r=self:GetAbsRect()
	local w = r-l
	local v = x/w
	v = math.max(0,math.min(v,1))
	
	dwindow.seek(dwindow.total()*v)
end

function progress:OnMouseUp(x, y, button)
	self.dragging = false;
end

function progress:OnMouseDown(...)
	self.dragging = true
	self:OnMouseMove(...)
end


-- progress slider
local progress_slider = BaseFrame:Create()
oroot:AddChild(progress_slider)
progress_slider:SetSize(17, 17)
progress_slider:SetRelativeTo(CENTER, progress, LEFT)

function progress_slider:RenderThis()
	if progress:IsMouseOver() then
		paint(0,0,17, 17, get_bitmap(lua_path .. "slider.png"))
	end
end

function progress_slider:PreRender()
	local v = (dwindow.tell() or 0) / (dwindow.total() or 1)
	v = math.max(math.min(v, 1), 0)
	local l,_,r = progress:GetAbsRect()
	self:SetRelativeTo(CENTER, progress, LEFT, (r-l)*v, 0.5)
end

function progress_slider:HitTest()
	return false
end


-- volume bar
local volume = BaseFrame:Create()
bottombar:AddChild(volume)
volume:SetRelativeTo(RIGHT, full, LEFT, -20, 0)
volume:SetSize(68, 16+4)

function volume:RenderThis()
	local v = math.max(math.min(dwindow.get_volume(), 1), 0)
	local l,_,r = self:GetAbsRect()
	
	paint(0,8,r-l, 8+4, get_bitmap(lua_path .. "volume_bg.png"))
	paint(0,8,(r-l)*v,8+4, get_bitmap(lua_path .. "volume.png"))
end

function volume:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local l,_,r=self:GetAbsRect()
	local w = r-l
	local v = x/w
	v = math.max(0,math.min(v,1))
	
	dwindow.set_volume(v)
end

function volume:OnMouseUp(x, y, button)
	self.dragging = false;
end

function volume:OnMouseDown(...)
	self.dragging = true
	self:OnMouseMove(...)
end


-- volume slider
local volume_slider = BaseFrame:Create()
local volume_slider = BaseFrame:Create()
oroot:AddChild(volume_slider)
volume_slider:SetSize(17, 17)
volume_slider:SetRelativeTo(CENTER, volume, LEFT)

function volume_slider:RenderThis()
	paint(0,0,17, 17, get_bitmap(lua_path .. "slider.png"))
end

function volume_slider:HitTest()
	return false;
end

function volume_slider:PreRender()
	local l,_,r = volume:GetAbsRect()
	self:SetRelativeTo(CENTER, volume, LEFT, (r-l)*dwindow.get_volume(), 0.5)
end


-- volume button
local volume_button = BaseFrame:Create()
bottombar:AddChild(volume_button)
volume_button:SetRelativeTo(RIGHT, volume, LEFT, -1, 0)
volume_button:SetSize(25, 24)

function volume_button:RenderThis()
	paint(0,0,25, 24, get_bitmap(lua_path .. (dwindow.get_volume() > 0 and "volume_button_normal.png" or "volume_button_mute.png")))
end

function volume_button:OnMouseDown()
	if dwindow.get_volume() > 0 then
		self.saved_volume = dwindow.get_volume()
		dwindow.set_volume(0)
	else
		dwindow.set_volume(self.saved_volume or 1)
		self.saved_volume = nil
	end
end


-- open file button
local open = BaseFrame:Create()
oroot:AddChild(open)
open:SetRelativeTo(CENTER)
open:SetSize(165,36)

function open:RenderThis()
	if not dwindow.movie_loaded then
		paint(0,0,165,36, get_bitmap(lua_path .. "open.png"))
		local x = (165-self.caption.width)/2
		local y = (36-self.caption.height)/2
		paint(x,y,x+self.caption.width, y+self.caption.height, self.caption)
	end
end

function open:OnInitGPU()
	local font = dwindow.CreateFont({name="楷体"})
	self.caption = DrawText("打开文件...", font, 0x00ffff)
	dwindow.ReleaseFont(font)
end

function open:OnReleaseGPU()
	if (self.caption and self.caption.res) then
		dwindow.release_resource_core(self.caption.res)
		self.caption = nil
	end	
end

function open:HitTest()
	return not dwindow.movie_loaded
end

function open:OnMouseDown()
	local file = dwindow.OpenFile()
	if file then
		playlist:play(file)
	end

	return true
end


-- open file button drop down
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
			on_command = function()
				local url = dwindow.OpenURL()
				if url then
					playlist:play(url)
				end
			end
		},
		{
			string = "打开左右分离文件...",
			on_command = function()
				local left, right = dwindow.OpenDoubleFile()
				if left and right then
					playlist:play(left, right)
				end
			end
		},
		{
			string = "打开文件夹...",
			on_command = function()
				local folder = dwindow.OpenFolder()
				if folder then
					playlist:play(folder)
				end
			end
		},
	}
	
	m = menu_builder:Create(m)
	m:popup(open:GetAbsAnchorPoint(BOTTOMLEFT))
	m:destroy()
	return true
end


-- event handler
local event = BaseFrame:Create()
oroot:AddChild(event)
function event:PreRender(t, dt)
	if dwindow.is_fullscreen() then
		dwindow.set_movie_rect(0, 0, dwindow.width, dwindow.height)
	else
		local l, t = topbar:GetAbsAnchorPoint(BOTTOMLEFT)
		local r, b = bottombar:GetAbsAnchorPoint(TOPRIGHT)
		dwindow.set_movie_rect(l, t, r, b)
	end
end