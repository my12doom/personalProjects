local lua_file = core.loading_file
local lua_path = GetPath(lua_file)
core.execute_luafile(lua_path .. "menu.lua")

local alpha = 0.5
local UI_fading_time = 300
local UI_show_time = 2000
local playlist_list
local playlist_button = BaseFrame:Create()
local playlist_close = BaseFrame:Create()
local font = dx9.CreateFont({name="宋体", height = 12, weight=0})
local font_black = dx9.CreateFont({name="宋体", height = 16, weight=0})

setting.MAX_WIDTH = 417
setting.MAX_HEIGHT = 277
core.ApplySetting("MAX_WIDTH")
core.ApplySetting("MAX_HEIGHT")
player.set_window_text(L("DWindow"), L("DWindow"))



-- black background and right mouse reciever
local oroot = root
--root:AddChild(oroot)
function oroot:OnInitGPU(t, dt)
	self:SetSize(ui.width, ui.height)
end

function oroot:OnMouseDown(x, y, button)
	if button ~= VK_RBUTTON and not player.is_fullscreen() then
		ui.StartDragging()
	end	
	playlist_list:hide()
	return true
end

function oroot:OnClick(x,y,button)
	if button == VK_RBUTTON then
		popup_dwindow2()
	end
	return true
end


-- background
logo = BaseFrame:Create()
oroot:AddChild(logo)
logo:SetPoint(CENTER)
logo:SetSize(1024,600)
function logo:PreRender()
	if not player.movie_loaded then
		self:set_texture(get_bitmap(lua_path .. "bg.png"))
	else
		self:set_texture(nil)
	end
end

-- loading progress icon
local loading_icon = BaseFrame:Create()
logo:AddChild(loading_icon)
loading_icon:SetPoint(CENTER)
loading_icon:SetSize(64,64)

function loading_icon:PreRender()
	if player.movie_loading then
		self:clear()
		local p = math.floor((core.GetTickCount() % 1000)/100)
		local res = get_bitmap(lua_path .. "loading.png")
		set_bitmap_rect(res, p*64,0,(p+1)*64,64)
		self:paint(0,0,64,64, res)
	elseif self.last then
		self:clear()
	end
	self.last = player.movie_loading
end

-- bottom bar
local bottombar = BaseFrame:Create()
oroot:AddChild(bottombar)
bottombar:SetPoint(BOTTOMLEFT)
bottombar:SetPoint(BOTTOMRIGHT)
bottombar:SetHeight(44)
bottombar.name = "bottom"
bottombar:set_texture(get_bitmap(lua_path .. "bar.png"))


-- top bar
local topbar = BaseFrame:Create()
oroot:AddChild(topbar)
topbar:SetPoint(TOPLEFT)
topbar:SetPoint(TOPRIGHT)
topbar:SetHeight(30)
topbar.name = "top"
topbar:set_texture(get_bitmap(lua_path .. "bar.png"))

-- caption
local caption = BaseFrame:Create()
topbar:AddChild(caption)
caption:SetPoint(LEFT, topbar, LEFT, 15)
caption.text = " "

function caption:PreRender()
	local left, right = playlist:current_item();
	local dis = GetName(left) or " "
	if right then dis = dis .. " + " .. GetName(right) end
	if not player.movie_loaded then dis = " " end
	
	if self.text ~= dis then
		self.text = dis
		self.res = DrawText(self.text, font_black)
		self:set_texture(self.res)
		self:SetSize(self.res.width, self.res.height)
	end
end


-- playlist button
bottombar:AddChild(playlist_button)
playlist_button:SetPoint(BOTTOMLEFT, bottombar, BOTTOMLEFT, 14, -7)
playlist_button:SetSize(36,30)

function playlist_button:PreRender()
	self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "playlist_mouseover.png" or "playlist_normal.png")))	
end

function playlist_button:OnClick()
	if playlist_list.showing then
		playlist_list:hide()
	else
		playlist_list:show()
	end
	
	return true
end

function playlist_button:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end


-- setting_buttons button
local setting_button = BaseFrame:Create()
bottombar:AddChild(setting_button)
setting_button:SetPoint(LEFT, playlist_button, RIGHT, -18, 0)
setting_button:SetSize(36,30)
setting_button:set_texture(get_bitmap(lua_path .. "setting_normal.png"))

function setting_button:PreRender()
	self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "setting_mouseover.png" or "setting_normal.png")))	
end

function setting_button:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end

function setting_button:OnClick()
	player.show_setting()
	return true
end

-- 3d/2d button
local b3d = BaseFrame:Create()
bottombar:AddChild(b3d)
b3d:SetPoint(LEFT, setting_button, RIGHT, -18, 0)
b3d:SetSize(36,30)

function b3d:PreRender()
	if not player.movie_loaded then
		self:set_texture(nil)
	elseif dx9.is2DMovie() then
		self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "2d_mouseover.png" or "2d_normal.png")))	
	else
		self:set_texture(get_bitmap(lua_path .. (dx9.is2DRendering() and "3d_normal.png" or "3d_mouseover.png")))	
	end
end

function b3d:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end

function b3d:OnClick()
	if not player.movie_loaded then return end
	player.toggle_3d()
	return true
end

-- LR button
local LR = BaseFrame:Create()
bottombar:AddChild(LR)
LR:SetPoint(LEFT, b3d, RIGHT, -18, 0)
LR:SetSize(36,30)

function LR:RenderThis()
	if not player.movie_loaded or dx9.is2DMovie() then
		self:set_texture(nil)	
	elseif setting.SwapEyes then
		self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "RL_mouseover.png" or "RL_normal.png")))
	else
		self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "LR_mouseover.png" or "LR_normal.png")))			
	end
end

function LR:HitTest(x, y)
	return x >= 10 and y>= 8 and x<= 25 and y <= 21
end

function LR:OnClick()
	if not player.movie_loaded or dx9.is2DMovie() then return end
	player.set_swapeyes(not setting.SwapEyes)
	return true
end


-- fullscreen button
local full = BaseFrame:Create()
bottombar:AddChild(full)
full:SetPoint(BOTTOMRIGHT, bottombar, nil, -14, -7)
full:SetSize(33,31)
full:set_texture(get_bitmap(lua_path .. "fullscreen.png"))

function full:PreRender()
	self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "fullscreen_mouseover.png" or "fullscreen.png")))
end

function full:OnClick()
	player.toggle_fullscreen()
	return true
end


-- play/pause button
local play = BaseFrame:Create()
bottombar:AddChild(play)
play:SetPoint(BOTTOM)
play:SetSize(109,38)
play:set_texture(get_bitmap(lua_path .. "pause_mouseover.png"))

function play:PreRender()
	if player.is_playing() then
		self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "pause_mousedown.png" or "pause_mouseover.png")))	
	else
		self:set_texture(get_bitmap(lua_path .. (self:IsMouseOver() and "play_mousedown.png" or "play_mouseover.png")))	
	end
end

function play:OnClick()
	if not player.movie_loaded then
		if playlist:count() > 0 then
			playlist:play_item(playlist:current_pos())
		else
			local file = ui.OpenFile()
			if file then
				playlist:play(file)
			end
		end
	else
		player.pause()
	end
	
	return true
end


-- progress bar
local progress = BaseFrame:Create()
oroot:AddChild(progress)
progress:SetPoint(TOPLEFT, bottombar, nil, 0, -8)
progress:SetPoint(TOPRIGHT, bottombar, nil, 0, -8)
progress:SetHeight(16+6)

function progress:PreRender()
	local v = (player.tell() or 0) / (player.total() or 1)
	v = math.max(math.min(v, 1), 0)
	local l,_,r = self:GetRect()
	
	local r2 = math.floor((r-l)*v+0.5)
	if self.r2 ~= r2 then
		self.r2 = r2
		self:clear()
		self:paint(0,8,r-l, 8+(self:IsMouseOver() and 6 or 2), get_bitmap(lua_path .. "progress_bg.png"))
		self:paint(0,8,r2,8+6, get_bitmap(lua_path .. (self:IsMouseOver() and "progress_thick.png" or "progress.png")))
	end
end

function progress:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local l,_,r=self:GetRect()
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
	
	return true
end


-- progress slider
local progress_slider = BaseFrame:Create()
oroot:AddChild(progress_slider)
progress_slider:SetSize(17, 17)
progress_slider:SetPoint(CENTER, progress, LEFT)

function progress_slider:PreRender()
	local v = (player.tell() or 0) / (player.total() or 1)
	v = math.max(math.min(v, 1), 0)
	local l,_,r = progress:GetRect()
	self:SetPoint(CENTER, progress, LEFT, math.floor((r-l)*v), 0.5)
	if progress:IsMouseOver() then
		self:set_texture(get_bitmap(lua_path .. "slider.png"))
	else
		self:set_texture(nil)
	end
end

function progress_slider:HitTest()
	return false
end

local function create_time_texture(time, color)
	local second = (time)%60
	local minute = time/60
	return DrawText(string.format("%02d:%02d", minute, second), font, color or 0x00ffff)
end

-- current time: to top of progress slider
local current_time = BaseFrame:Create()
oroot:AddChild(current_time)
current_time:SetSize(37, 14)
current_time:SetPoint(BOTTOM, progress_slider, TOP)

function current_time:PreRender()
	if progress:IsMouseOver() then		
		local time = player.tell()/1000
		if self.time ~= time then
			self.texture = create_time_texture(time)
			self:SetSize(self.texture.width, self.texture.height)
			self:set_texture(self.texture)
			self.time = time
		end
	else
		self:set_texture(nil)
	end
end

function current_time:HitTest()
	return false
end

--total time : to top right of progress bar
local total_time = BaseFrame:Create()
oroot:AddChild(total_time)
total_time:SetSize(37, 14)
total_time:SetPoint(BOTTOMRIGHT, progress, TOPRIGHT, nil, 3)

function total_time:PreRender()
	local time = player.total()/1000
	if self.time ~= time then 
		self.texture = create_time_texture(time, 0x00333333)
		self:SetSize(self.texture.width, self.texture.height)
		self:set_texture(self.texture)
		self.dirty = alpha ~= self.alpha
		self.alpha = alpha
		self.time = time
	end
end

-- volume bar
-- enlarged hittest width
volume = BaseFrame:Create()
bottombar:AddChild(volume)
volume:SetPoint(RIGHT, full, LEFT, -20+9, 0)
volume:SetSize(68+18, 16+4)
volume.name = "volume"
print(volume.rt, volume.rt2, "volume", volume:paint(0,0,60,20, get_bitmap(lua_path .. "volume_bg.png")))

function volume:PreRender()
	local v = math.max(math.min(player.get_volume(), 1), 0)
	if self.v ~= v then
		print("volume:PreRender():paint()")
		self:OnResize()
		self.v = self.v and v or -1
	end
end

function volume:OnResize()
	local v = math.max(math.min(player.get_volume(), 1), 0)
	local l,_,r = self:GetRect()	
	print("volume:paint()", self.rt, self.rt2)
	core.track_back()
	self:clear()
	if not self:paint(9,8,9+r-l-18, 8+4, get_bitmap(lua_path .. "volume_bg.png")) then return end
	if not self:paint(9,8,9+(r-l-18)*v,8+4, get_bitmap(lua_path .. "volume.png")) then return end
	self.v = v
end

function volume:OnMouseMove(x,y,button)
	if not self.dragging then return end
	local l,_,r=self:GetRect()
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
	
	return true
end


-- volume slider
local volume_slider = BaseFrame:Create()
oroot:AddChild(volume_slider)
volume_slider:SetSize(17, 17)
volume_slider:SetPoint(CENTER, volume, LEFT, 9)
volume_slider:set_texture(get_bitmap(lua_path .. "slider.png"))


function volume_slider:HitTest()
	return false;
end

function volume_slider:PreRender()
	local l,_,r = volume:GetRect()
	self:SetPoint(CENTER, volume, LEFT, 9+(r-l-18)*player.get_volume(), 0.5)
end


-- volume button
local volume_button = BaseFrame:Create()
bottombar:AddChild(volume_button)
volume_button:SetPoint(RIGHT, volume, LEFT, 5, 0)
volume_button:SetSize(25, 24)

function volume_button:PreRender()
	self:set_texture(get_bitmap(lua_path .. (player.get_volume() > 0 and "volume_button_normal.png" or "volume_button_mute.png")))
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
open:SetPoint(CENTER)
open:SetSize(165,36)
open:set_texture(get_bitmap(lua_path .. "open.png"))

function open:PreRender()
	local alpha 
	if not player.movie_loaded and not player.movie_loading then
		alpha = 1
	else
		alpha = 0
	end
	
	self.dirty = self.alpha ~= alpha
	self.alpha = alpha
end

function open:HitTest()
	return not player.movie_loaded and not player.movie_loading
end

function open:OnClick()
	local file = ui.OpenFile()
	if file then
		playlist:play(file)
	end

	return true
end

local open_text = BaseFrame:Create()
open:AddChild(open_text)
open_text:SetPoint(CENTER)
function open_text:OnLanguageChange()
	local font = dx9.CreateFont({name="宋体", weight=0, height=13})
	self.caption = DrawText(L("Open File..."), font, 0x00ffff)
	dx9.ReleaseFont(font)
	self:SetSize(self.caption.width, self.caption.height)
	self:set_texture(self.caption)
end

function open_text:HitTest()
	return false
end

open_text:OnLanguageChange()


-- playlist items
local playlist_item = BaseFrame:Create()
function playlist_item:Create(text)
	local o = BaseFrame:Create()
	o.res = DrawText(text, font, 0x1a6eb0)
	setmetatable(o, self)
	self.__index = self
	o:SetHeight(28)
	
	return o
end

function playlist_item:OnResize()

	local l,t,r,b = self:GetRect()
	l, t, r, b = 0, 0, r-l, b-t
	
	self:clear()
	
	if self.id == playlist:current_pos() then
		self:paint(l,t,r,b, get_bitmap(lua_path .. "menu_item.png"))
	end
	
	local dy = (28-self.res.height)/2
	local dx = 10
	self:paint(dx,dy, dx+self.res.width, dy+self.res.height, self.res, 1, bilinear_mipmap_minus_one)
end

function playlist_item:OnPlaylistChange()
	self:OnResize()
end

function playlist_item:OnDoubleClick()
	playlist:play_item(self.id)
	
	return true
end

function playlist_item:OnClick(x, y, button)
	if button ~= VK_RBUTTON then return end
	local m = 
	{
		{
			string = L("Delete this item"),
			id = self.id,
			on_command = function(v)
				playlist:remove_item(v.id)
			end
		},
		{
			string = L("Play this item"),
			id = self.id,
			on_command = function(v)
				playlist:play_item(v.id)
			end
		},
		{
			string = L("Clear all"),
			id = self.id,
			on_command = function(v)
				playlist:clear()
			end
		},
	}
	
	m = menu_builder:Create(m)
	m:popup()
	m:destroy()
	return true
end
-- the "playlist"
playlist_list = BaseFrame:Create()
oroot:AddChild(playlist_list)
playlist_list.dragging = false
setting.playlist_width = 180
playlist_list:SetWidth(setting.playlist_width)
playlist_list:SetPoint(TOPLEFT, topbar, BOTTOMLEFT, -setting.playlist_width)
playlist_list:SetPoint(BOTTOMLEFT, bottombar, TOPLEFT, -setting.playlist_width)

function playlist_list:OnResize()
	local l,t,r,b = self:GetRect()
	local res = get_bitmap(lua_path .. "blue.bmp")
	self:clear()
	--paint(0,0,r-l,b-t, )
	self:paint(0, 0, r-l, b-t, get_bitmap(lua_path .. "playlist_bg.png"))
	self:paint(0, 0, r-l, 2, res)
	--paint(0, 0, 2, b-t, res)
	self:paint(0, b-t-2, r-l, b-t, res)
	self:paint(r-l-2, 0, r-l, b-t, res)
end

function playlist_list:OnUpdate(t, dt)
	self.pos = self.pos or 0
	local pre_pos = self.pos
	local da = dt/UI_fading_time
	if self.showing then
		self.pos = self.pos + da
	else
		self.pos = self.pos - da
	end
	
	self.pos = math.max(0, self.pos)
	self.pos = math.min(1, self.pos)
	
	if self.pos ~= pre_pos then
		local l,_,r = self:GetRect()
		playlist_list:SetPoint(TOPLEFT, topbar, BOTTOMLEFT, -setting.playlist_width *(1 - self.pos) )
		playlist_list:SetPoint(BOTTOMLEFT, bottombar, TOPLEFT, -setting.playlist_width *(1 - self.pos) )
	end
end

function playlist_list:OnMouseDown(x, y)
	if x > setting.playlist_width - 5 then
		self.dragging = true
	end
	
	return true
end

function playlist_list:OnPlaylistChange()
	print("playlist_list:OnPlaylistChange()")
	self:RemoveAllChilds()
	
	for i=1, playlist:count() do
		local item = playlist:item(i)
		local text = GetName(item.L) or " "
		if item.R then text = text .. " + " .. GetName(item.R) end
		if text == "" then text = item.L end
		
		local obj = playlist_item:Create(text)
		obj.id = i
		self:AddChild(obj)
		if i == 1 then
			obj:SetPoint(TOPLEFT)
			obj:SetPoint(TOPRIGHT)
		else
			local last = self:GetChild(i-1)
			obj:SetPoint(TOPLEFT, last, BOTTOMLEFT)
			obj:SetPoint(TOPRIGHT, last, BOTTOMRIGHT)
		end
	end
	
	playlist_list:AddChild(playlist_close)
end

function playlist_list:OnMouseMove(x, y)
	if self.dragging then
		local l = self:GetRect()
		local x = ui.get_mouse_pos()
		setting.playlist_width = (x - l)
		setting.playlist_width = math.max(10, setting.playlist_width)
		
		self:SetWidth(setting.playlist_width)
	end
end

function playlist_list:OnMouseUp()
	self.dragging = false
	return true
end

function playlist_list:hide()
	if self.dragging then return end
	self.showing = false
end

function playlist_list:show()
	if self.dragging then return end
	self.showing = true
end

-- load the playlist now
playlist_list:OnPlaylistChange()

-- the close button
playlist_close:SetSize(30,30)
playlist_close:SetPoint(RIGHT)
playlist_close:set_texture(get_bitmap(lua_path .. "playlist_close.png"))

function playlist_close:OnClick()
	playlist_list:hide()	
	return true
end


-- open file button drop down
local open2 = BaseFrame:Create()
oroot:AddChild(open2)
open2:SetPoint(LEFT, open, RIGHT)
open2:SetSize(31,36)

function open2:PreRender()
	if not player.movie_loaded and not player.movie_loading then
		self:set_texture(get_bitmap(lua_path .. "open_more.png"))
	else
		self:set_texture(nil)
	end
end

function open2:HitTest()
	return not player.movie_loaded and not player.movie_loading
end


function istable(t)
	return type(t) == "table"
end

function json_url2table(url)
	local bytes, response_code, headers = core.http_request(url)
	
	--print(bytes, response_code, headers)

	bytes = string.gsub( bytes, "\r\n", "\\n")
	
	local json = core.cjson()

	local suc,result = pcall(json.decode, bytes)

	--print(suc, result)
	if not istable(result) then return nil, result end

	return result
end

function printtable(table, level)
	level = level or 0
	local prefix = ""
	for i=1,level do
		prefix = prefix .. "  "
	end

	for k,v in pairs(table) do
		if type(v) == "table" then
			print(prefix..k..":")
			printtable(v, level+1)
		else
			print(prefix..k .. ":\t" .. tostring(v))
		end
	end
end

function open2:OnClick()
	local m = 
	{
		{
			string = L("Open URL..."),
			on_command = function()
				local url = ui.OpenURL()

				if url then
					playlist:play(url)
				end
			end
		},
		{
			string = L("Open Left And Right File..."),
			on_command = function()
				local left, right = ui.OpenDoubleFile()
				if left and right then
					playlist:play(left, right)
				end
			end
		},
		{
			string = L("Open Folder..."),
			on_command = function()
				local folder = ui.OpenFolder()
				if folder then
					playlist:play(folder)
				end
			end
		},
	}
	
	m = menu_builder:Create(m)
	m:popup(open:GetPoint(BOTTOMLEFT))
	m:destroy()
	return true
end


-- event handler
local event = BaseFrame:Create()
oroot:AddChild(event)
function event:PreRender(t, dt)
	if player.is_fullscreen() then
		dx9.set_movie_rect(nil)
	else
		local l, t = topbar:GetPoint(BOTTOMLEFT)
		local r, b = bottombar:GetPoint(TOPRIGHT)
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
		last_mousemove = core.GetTickCount()
		mousex, mousey = ui.get_mouse_pos()
	end
		
	local da = dt/UI_fading_time
	local old_alpha = alpha
	local mouse_in_pannel = (px>=0 and px<ui.width and py>=ui.height-44 and py<ui.height) -- bottombar
	mouse_in_pannel = mouse_in_pannel or (px>=0 and px<ui.width and py>0 and py<30)	-- topbar
	mouse_in_pannel = mouse_in_pannel or (playlist_list.showing and px >= 0 and px <= setting.playlist_width)	-- playlist
	local hide_mouse = (not mouse_in_pannel) and (t > last_mousemove + UI_show_time) and (player.is_fullscreen())
	player.show_mouse(not hide_mouse)
	
	if hide_mouse then
		alpha = alpha - da
	else
		alpha = alpha + da
	end
	alpha = math.min(1, math.max(alpha, 0))
	
	topbar:SetPoint(TOPLEFT, nil, nil, 0, -30+30*alpha)
	topbar:SetPoint(TOPRIGHT, nil, nil, 0, -30+30*alpha)
	bottombar:SetPoint(BOTTOMLEFT, nil, nil, 0, 44-44*alpha)
	bottombar:SetPoint(BOTTOMRIGHT, nil, nil, 0, 44-44*alpha)
	
	if (alpha < 0.1) then
		playlist_list:hide()
	end
	
	if (px < 5 and px >= 0 and py >= 0 and py < ui.height) then
		playlist_list:show()
	end
end