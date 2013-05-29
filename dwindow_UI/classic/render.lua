﻿local alpha = 0.5
local UI_fading_time = 500
local UI_show_time = 2000

-- toolbar background
local bg = BaseFrame:Create()
root:AddChild(bg)
bg:SetRelativeTo(BOTTOMLEFT)
bg:SetSize(dwindow.width, 64)

function bg:PreRender()
	bg:SetSize(dwindow.width, 64)
end

function bg:RenderThis()
	local res = get_bitmap("bg.png")
	paint(0,0,dwindow.width,64,res,alpha)
end

function bg:HitTest()
end

-- right mouse button reciever
local rbutton = BaseFrame:Create()
root:AddChild(rbutton)
rbutton:SetRelativeTo(CENTER)
rbutton:SetSize(dwindow.width,dwindow.height)

function rbutton:PreRender()
	rbutton:SetSize(dwindow.width,dwindow.height)
end

function rbutton:OnMouseDown(x,y,button)
	if button == VK_RBUTTON then
		alpha = 1
		dwindow.show_mouse(true)
		dwindow.popup_menu()
		return true
	end
end

-- LOGO
local logo = BaseFrame:Create()
rbutton:AddChild(logo)


function logo:PreRender()
	self.w = math.min(math.min(dwindow.width, dwindow.height-40)*3/5, 512)
	logo:SetRelativeTo(CENTER, rbutton, CENTER, 0.0125*self.w,-20)
	logo:SetSize(self.w+0.025*self.w, self.w)
	
	print("logo:PreRender", self.w)
end

function logo:RenderThis(view)
	local d = self.d
	if not dwindow.movie_loaded then
		local delta = (1-view) * 0.025*self.w
		paint(0+delta,0,self.w+delta,self.w,get_bitmap("logo.png"))
	end
end

function logo:HitTest(x, y)
	if dwindow.movie_loaded then return end
	
	local r = self.w/2
	local dx = x-self.w/2
	local dy = y-self.w/2
	
	return dx*dx+dy*dy < r*r
end

function logo:OnMouseDown()
	alpha = 1
	dwindow.show_mouse(true)
	dwindow.popup_menu()
	return true
end

local last_mousemove =  0
local mousex = -999
local mousey = -999
function root:OnMouseMove(...)
	local px, py = dwindow.get_mouse_pos()
	if (mousex-px)*(mousex-px)+(mousey-py)*(mousey-py) > 100 then	
		last_mousemove = dwindow.GetTickCount()
		mousex, mousey = dwindow.get_mouse_pos()
	end
end

function root:OnUpdate(t, dt)
	local da = dt/UI_fading_time
	local old_alpha = alpha
	local hide_mouse = t > last_mousemove + UI_show_time
	dwindow.show_mouse(not hide_mouse)
	
	if hide_mouse then
		alpha = alpha - da
	else
		alpha = alpha + da
	end
	alpha = math.min(1, math.max(alpha, 0))	
end

-- Play/Pause button
local play = BaseFrame:Create()
root:AddChild(play)
play:SetRelativeTo(BOTTOMLEFT, nil, nil, 14, -8)
play:SetSize(14, 14)

function play:RenderThis()
	local res = get_bitmap("ui.png")
	if dwindow.is_playing() then
		set_bitmap_rect(res, 110,0,110+14,14)
	else
		set_bitmap_rect(res, 96,0,96+14,14)
	end
	paint(0,0,14,14,res,alpha)
end

function play:OnMouseDown()
	dwindow.pause()
end

-- Fullscreen button
local full = BaseFrame:Create()
root:AddChild(full)
full:SetRelativeTo(BOTTOMRIGHT, nil, nil, -14, -8)
full:SetSize(14, 14)

function full:RenderThis()
	local res = get_bitmap("ui.png")
	set_bitmap_rect(res, 192,0,192+14,14)
	paint(0,0,14,14,res,alpha)
end

function full:OnMouseDown()
	print("FULL")
	dwindow.toggle_fullscreen()
end

-- Volume
local volume = BaseFrame:Create()
root:AddChild(volume)
volume:SetRelativeTo(LEFT, full, RIGHT, -14, 0)
volume:SetSize(34,14)

function volume:RenderThis()
	local volume =  dwindow.get_volume()
	volume = math.min(volume, 1)
	volume = math.max(volume, 0)
	local res = get_bitmap("ui.png")
	set_bitmap_rect(res, 124+34,0,124+34+34,14)
	paint(0,0,34,14,res,alpha)
	set_bitmap_rect(res, 124,0,124+34*volume,14)
	paint(0,0,34*volume,14,res,alpha)
end

-- progress bar
local progress = BaseFrame:Create()
root:AddChild(progress)
progress:SetRelativeTo(BOTTOMLEFT, nil, nil, 113-3, -8)

function progress:PreRender()
	progress:SetSize(dwindow.width - 113 - 161 + 6 ,14)
end

function progress:RenderThis()
	local w = dwindow.width - 113 - 161
	local fv = dwindow.tell() / dwindow.total()
	fv = math.max(0,math.min(fv,1))
	
	local res = get_bitmap("ui.png")
	set_bitmap_rect(res, 216,0,220,14)
	paint(3,0,w+3,14,res,alpha)
	set_bitmap_rect(res, 208,0,212,14)
	paint(3,0,fv*w+3,14,res,alpha)
end

function progress:OnMouseDown(x,y,button)
	local w = dwindow.width - 113 - 161
	local v = (x-3) / w
	v = math.max(0,math.min(v,1))
	
	dwindow.seek(dwindow.total()*v)
end

-- numbers
local number_current = BaseFrame:Create()
root:AddChild(number_current)
number_current:SetRelativeTo(RIGHT, play, LEFT, 14)
number_current:SetSize(9*5+6*2,14)

function number_current:GetTime()
	return dwindow.tell()
end

function number_current:RenderThis()
	local t = self:GetTime()
	local ms = t % 1000
	local s = t / 1000
	local h = (s / 3600) % 10
	local m1 = (s/60 /10) % 6
	local m2 = (s/60) % 10
	local s1 = (s/10) % 6
	local s2 = s%10

	local numbers =
	{
		h, -1, m1, m2, -1, s1, s2,   -- -1 = : symbol in time
	}

	local res = get_bitmap("ui.png")
	local x = 0
	for i=1,#numbers do
		local tex = 6 + math.floor(numbers[i])*9
		local ctex = 9
		if tex<6 then
			tex =  0
			ctex = 6
		end
		
		set_bitmap_rect(res, tex,0,tex+ctex,14)
		paint(x, 0, x+ctex, 14, res,alpha)
		x = x + ctex
	end
end

local number_total = BaseFrame:Create()
root:AddChild(number_total)
number_total:SetRelativeTo(LEFT, volume, RIGHT, -14)
number_total.RenderThis = number_current.RenderThis
function number_total:GetTime()
	return dwindow.total()
end

number_total:SetSize(9*5+6*2,14)

function number_total:GetTime()
	return dwindow.total()
end
