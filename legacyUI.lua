local path = "Z:\\skin2\\"
local alpha = 0.5
local UI_fading_time = 500
local UI_show_time = 2000

-- toolbar background
local bg = BaseFrame:Create()
root:AddChild(bg)
bg:SetRelativeTo(nil, BOTTOMLEFT)
function bg:GetRect()
	return 0, 0, dwindow.width, 64
end

function bg:RenderThis()
	local res = get_bitmap(path.."bg.png")
	paint(0,0,dwindow.width,64,res,alpha)
end

function bg:HitTest()
end

-- LOGO
local logo = BaseFrame:Create()
root:AddChild(logo)
logo:SetRelativeTo(nil, CENTER)
function logo:GetRect()
	self.w = math.min(math.min(dwindow.width, dwindow.height-40)*3/5, 512)
	return 0,0,self.w+0.025*self.w,self.w,0.0125*self.w,-20
end

function logo:RenderThis(view)
	local d = self.d
	if not movie_loaded then
		print("view", view)
		local delta = (1-view) * 0.025*self.w
		paint(0+delta,0,self.w+delta,self.w,get_bitmap(path.."logo.png"))
	end
end

function logo:HitTest(x, y)
	if movie_loaded then return end
	
	local r = self.w/2
	local dx = x-self.w/2
	local dy = y-self.w/2
	
	return dx*dx+dy*dy < r*r
end

function logo:OnMouseDown()
	dwindow.popup_menu()
end


local last_mousemove =  0
function root:OnMouseMove(...)
	last_mousemove = dwindow.GetTickCount()
	print("OnMouseMove", ...)
	print(root:GetAbsRect())
end

function root:OnUpdate(t, dt)
	local da = dt/UI_fading_time
	if t > last_mousemove + UI_show_time then
		alpha = alpha - da
	else
		alpha = alpha + da
	end
	alpha = math.min(1, math.max(alpha, 0))
	dwindow.show_mouse(alpha>0)
	print("OnUpdate", alpha, da, last_mousemove, t, t - (last_mousemove + UI_show_time))
end

-- Play/Pause button
local play = BaseFrame:Create()
root:AddChild(play)
play:SetRelativeTo(nil, BOTTOMLEFT)
function play:GetRect()
	return 0, 0, 14, 14, 14, -8
end

function play:RenderThis()
	local res = get_bitmap(path.."ui.png")
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
full:SetRelativeTo(nil, BOTTOMRIGHT)
function full:GetRect()
	return 0, 0, 14, 14, -14, -8
end

function full:RenderThis()
	local res = get_bitmap(path.."ui.png")
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
volume:SetRelativeTo(full, LEFT, RIGHT)
function volume:GetRect()
	return 0,0,34,14,-14,0
end

function volume:RenderThis()
	local volume =  dwindow.get_volume()
	volume = math.min(volume, 1)
	volume = math.max(volume, 0)
	local res = get_bitmap(path.."ui.png")
	set_bitmap_rect(res, 124+34,0,124+34+34,14)
	paint(0,0,34,14,res,alpha)
	set_bitmap_rect(res, 124,0,124+34*volume,14)
	paint(0,0,34*volume,14,res,alpha)
end

-- progress bar
local progress = BaseFrame:Create()
root:AddChild(progress)
progress:SetRelativeTo(nil, BOTTOMLEFT)
function progress:GetRect()
	local w = dwindow.width - 113 - 161 + 6
	return 0,0,w,14,113-3,-8
end

function progress:RenderThis()
	local w = dwindow.width - 113 - 161
	local fv = dwindow.tell() / dwindow.total()
	fv = math.max(0,math.min(fv,1))
	
	local res = get_bitmap(path.."ui.png")
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
number_current:SetRelativeTo(play, RIGHT, LEFT)
function number_current:GetRect()
	return 0,0,9*5+6*2,14,14
end

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

	local res = get_bitmap(path.."ui.png")
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
number_total:SetRelativeTo(volume, LEFT, RIGHT)
number_total.RenderThis = number_current.RenderThis
function number_total:GetTime()
	return dwindow.total()
end

function number_total:GetRect()
	return 0,0,9*5+6*2,14,-14
end

function number_total:GetTime()
	return dwindow.total()
end
