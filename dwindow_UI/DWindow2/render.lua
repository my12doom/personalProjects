local lua_file = dwindow.loading_file
local lua_path = GetPath(lua_file)

local alpha = 0.5
local UI_fading_time = 300
local UI_show_time = 2000


-- black background and right mouse reciever
local oroot = BaseFrame:Create()
root:AddChild(oroot)
function oroot:PreRender(t, dt)
	self:SetSize(ui.width, ui.height)
end

function oroot:OnClick(x,y,button)
	if button == VK_RBUTTON then
		player.show_mouse(true)
		player.popup_menu()
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
	paint(0,0,ui.width,44, res)
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
			dx9.release_resource_core(self.res.res)
		end
		self.res = DrawText(self.item)
	end
end

function topbar:RenderThis()
	local res = get_bitmap(lua_path .. "bar.png")
	paint(0,0,ui.width,34, res)
	
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

function leftright:OnClick()
	player.set_swapeyes(not player.get_swapeyes())
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

function b3d:OnClick()
	player.toggle_3d()
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

function full:OnClick()
	player.toggle_fullscreen()
	return true
end


-- play/pause button
local play = BaseFrame:Create()
bottombar:AddChild(play)
play:SetRelativeTo(BOTTOM)
play:SetSize(109,38)

function play:RenderThis()
	if player.is_playing() then
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "pause_mousedown.png" or "pause_mouseover.png")))	
	else
		paint(0,0,109,38, get_bitmap(lua_path .. (self:IsMouseOver() and "play_mousedown.png" or "play_mouseover.png")))	
	end
end

function play:OnClick()
	if not dwindow.movie_loaded then
		local file = dwindow.OpenFile()
		if file then
			playlist:play(file)
		end
	else
		player.pause()
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
	local v = (player.tell() or 0) / (player.total() or 1)
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
	
	player.seek(player.total()*v)
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
	local v = (player.tell() or 0) / (player.total() or 1)
	v = math.max(math.min(v, 1), 0)
	local l,_,r = progress:GetAbsRect()
	self:SetRelativeTo(CENTER, progress, LEFT, (r-l)*v, 0.5)
end

function progress_slider:HitTest()
	return false
end


-- volume bar
-- enlarged hittest width
local volume = BaseFrame:Create()
bottombar:AddChild(volume)
volume:SetRelativeTo(RIGHT, full, LEFT, -20+9, 0)
volume:SetSize(68+18, 16+4)

function volume:RenderThis()
	local v = math.max(math.min(player.get_volume(), 1), 0)
	local l,_,r = self:GetAbsRect()
	
	paint(9,8,9+r-l-18, 8+4, get_bitmap(lua_path .. "volume_bg.png"))
	paint(9,8,9+(r-l-18)*v,8+4, get_bitmap(lua_path .. "volume.png"))
end

function volume:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local l,_,r=self:GetAbsRect()
	local w = r-l-18
	local v = (x-9)/w
	v = math.max(0,math.min(v,1))
	
	player.set_volume(v)
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
volume_slider:SetRelativeTo(CENTER, volume, LEFT, 9)

function volume_slider:RenderThis()
	paint(0,0,17, 17, get_bitmap(lua_path .. "slider.png"))
end

function volume_slider:HitTest()
	return false;
end

function volume_slider:PreRender()
	local l,_,r = volume:GetAbsRect()
	self:SetRelativeTo(CENTER, volume, LEFT, 9+(r-l-18)*player.get_volume(), 0.5)
end


-- volume button
local volume_button = BaseFrame:Create()
bottombar:AddChild(volume_button)
volume_button:SetRelativeTo(RIGHT, volume, LEFT, 5, 0)
volume_button:SetSize(25, 24)

function volume_button:RenderThis()
	paint(0,0,25, 24, get_bitmap(lua_path .. (player.get_volume() > 0 and "volume_button_normal.png" or "volume_button_mute.png")))
end

function volume_button:OnClick()
	if player.get_volume() > 0 then
		self.saved_volume = player.get_volume()
		player.set_volume(0)
	else
		player.set_volume(self.saved_volume or 1)
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
	local font = dx9.CreateFont({name="宋体", weight=0, height=13})
	self.caption = DrawText("打开文件...", font, 0x00ffff)
	dx9.ReleaseFont(font)
end

function open:OnReleaseGPU()
	if (self.caption and self.caption.res) then
		dx9.release_resource_core(self.caption.res)
		self.caption = nil
	end	
end

function open:HitTest()
	return not dwindow.movie_loaded
end

function open:OnClick()
	local file = ui.OpenFile()
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

function open2:OnClick()
	local m = 
	{
		{
			string = "打开URL...",
			on_command = function()
				local url = ui.OpenURL()
				if url then
					playlist:play(url)
				end
			end
		},
		{
			string = "打开左右分离文件...",
			on_command = function()
				local left, right = ui.OpenDoubleFile()
				if left and right then
					playlist:play(left, right)
				end
			end
		},
		{
			string = "打开文件夹...",
			on_command = function()
				local folder = ui.OpenFolder()
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
	if player.is_fullscreen() then
		dx9.set_movie_rect(0, 0, ui.width, ui.height)
	else
		local l, t = topbar:GetAbsAnchorPoint(BOTTOMLEFT)
		local r, b = bottombar:GetAbsAnchorPoint(TOPRIGHT)
		dx9.set_movie_rect(l, t, r, b)
	end
end


local mouse_hider = BaseFrame:Create()
root:AddChild(mouse_hider)
local last_mousemove =  0
local mousex = -999
local mousey = -999

function mouse_hider:PreRender(t, dt)

	local px, py = ui.get_mouse_pos()
	if (mousex-px)*(mousex-px)+(mousey-py)*(mousey-py) > 100 or ((mousex-px)*(mousex-px)+(mousey-py)*(mousey-py) > 0 and alpha > 0.5) then	
		last_mousemove = dwindow.GetTickCount()
		mousex, mousey = ui.get_mouse_pos()
	end
		
	local da = dt/UI_fading_time
	local old_alpha = alpha
	local mouse_in_pannel = (px>0 and px<ui.width and py>ui.height-44 and py<ui.height) -- bottombar
	mouse_in_pannel = mouse_in_pannel or (px>0 and px<ui.width and py>0 and py<30)	-- topbar
	local hide_mouse = (not mouse_in_pannel) and (t > last_mousemove + UI_show_time) and (player.is_fullscreen())
	player.show_mouse(not hide_mouse or dwindow.menu_open > 0)
	
	if hide_mouse then
		alpha = alpha - da
	else
		alpha = alpha + da
	end
	alpha = math.min(1, math.max(alpha, 0))
	
	topbar:SetRelativeTo(TOPLEFT, nil, nil, 0, -30+30*alpha)
	topbar:SetRelativeTo(TOPRIGHT, nil, nil, 0, -30+30*alpha)
	bottombar:SetRelativeTo(BOTTOMLEFT, nil, nil, 0, 44-44*alpha)
	bottombar:SetRelativeTo(BOTTOMRIGHT, nil, nil, 0, 44-44*alpha)
end