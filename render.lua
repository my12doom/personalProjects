 -- global functions and variables

buttons = {}

if require and not BaseFrame then require("base_frame") end

logo = BaseFrame:Create()
logo.name = "LOGO"
root:AddChild(logo)
logo:SetRelativeTo(root, CENTER)
function logo:GetRect()
	return 0,0,1920,1080
end

function logo:RenderThis(arg)
	if not movie_loaded then
		local res = get_bitmap("Z:\\skin\\logo2.png")
		paint(0,0,1920,1080, res)
	end
end

function logo:HitTest()
	return false
end

toolbar_bg = BaseFrame:Create()
toolbar_bg.name = "toolbar_bg"
root:AddChild(toolbar_bg)
toolbar_bg:SetRelativeTo(root, BOTTOM)
function toolbar_bg:GetRect()
	local toolbar_height = 65
	return 0, 0, dwindow.width, toolbar_height
end

function toolbar_bg:RenderThis(arg)
	local l,t,r,b = self:GetRect()
	local res = get_bitmap("Z:\\skin\\toolbar_background.png");
	paint(0,0,r-l,b-t, res)
end

-- buttons
local button_pictures =
{
	"fullscreen.png", "",
	"volume.png", "",
	"next.png", "",
	"play.png", "pause.png",
	"previous.png", "",
	"stop.png", "",
	"3d.png", "2d.png",
}

local button_size = 40;
local margin_button_right = 32;
local margin_button_bottom = 8;
local space_of_each_button = 62;
local toolbar_height = 65;
local width_progress_left = 5;
local width_progress_right = 6;
local margin_progress_right = 460;
local margin_progress_left = 37;
local progress_height = 21;
local progress_margin_bottom = 27;
local volume_base_width = 84;
local volume_base_height = 317;
local volume_margin_right = (156 - 84);
local volume_margin_bottom = (376 - 317);
local volume_button_zero_point = 32;
local volume_bar_height = 265;
local numbers_left_margin = 21;
local numbers_right_margin = 455;
local numbers_width = 12;
local numbers_height = 20;
local numbers_bottom_margin = 26;
local hidden_progress_width = 72;

local function button_GetRect(self)
	return 0, 0, space_of_each_button, button_size, self.dx, self.dy
end

local function button_RenderThis(self)
	local filename = ("Z:\\skin\\" .. self.pic[1])
	paint(11,0,button_size+11,button_size, get_bitmap(filename))
end

local function button_OnMouseDown(self)
	dwindow.pause()
end



function OnInitGPU()
	print("OnInitGPU")
	-- commit them to GPU (optional)
	-- handle resize changes here (must)

	local x = - margin_button_right
	local y = - margin_button_bottom

	for i=1,#button_pictures/2 do
		local button = buttons[i] or BaseFrame:Create()
		buttons[i] = button
		toolbar_bg:AddChild(button)
		button.dx = x
		button.dy = y
		button.pic = {button_pictures[i*2-1], button_pictures[i*2]}
		button.GetRect = button_GetRect
		button.RenderThis = button_RenderThis
		button.OnMouseDown = button_OnMouseDown
		button.name = button.pic[1]
		button:SetRelativeTo(nil, BOTTOMRIGHT)

		x = x - space_of_each_button
	end
end

progressbar = BaseFrame:Create()
progressbar.name = "progressbar"
toolbar_bg:AddChild(progressbar)
progressbar:SetRelativeTo(nil, BOTTOMLEFT)
function progressbar:GetRect()
	local r = self.parent:GetAbsAnchorPoint(RIGHT) - margin_progress_right	
	local l = self.parent:GetAbsAnchorPoint(LEFT) + margin_progress_left

	return 0, 0, r-l, progress_height, margin_progress_left, -progress_margin_bottom+progress_height
end
local progress_pic =
{
	"progress_left_base.png",
	"progress_center_base.png",
	"progress_right_base.png",
	"progress_left_top.png",
	"progress_center_top.png",
	"progress_right_top.png",
}

function file(n)
	return "Z:\\skin\\" .. progress_pic[n]
end

function progressbar:OnMouseDown(x)
	local l,_,r = self:GetRect()
	local fv = x/(r-l)
	dwindow.seek(dwindow.total()*fv)
	
	print("-----------------", x)
end

function progressbar:RenderThis()
	local l,t,r,b = self:GetRect()
	l,r,t,b = 0,r-l,0,b-t
	local fv = dwindow.tell() / dwindow.total()
	if fv > 1 then fv = 1 end
	if fv < 0 then fv = 0 end
	local v = fv * r
	
	-- draw bottom
	paint(0,0, width_progress_left, b, get_bitmap(file(1)))
	paint(width_progress_left,0, r-width_progress_right, b, get_bitmap(file(2)))
	paint(r-width_progress_right,0, r, b, get_bitmap(file(3)))

	-- draw top
	if (v > 1.5) then
		local bmp = get_bitmap(file(4))
		paint(0,0,math.min(width_progress_left, v),b, bmp)
	end

	if (v > width_progress_left) then
		local r = math.min(r-width_progress_right, v)
		local bmp = get_bitmap(file(5))
		paint(width_progress_left, 0, r, b, bmp)
	end

	if (v > r - width_progress_right) then
		local bmp = get_bitmap(file(6))
		paint(r-width_progress_right, 0, v, b, bmp)
	end
end

number_current = BaseFrame:Create()
number_current.name = "number_current"
toolbar_bg:AddChild(number_current)
number_current:SetRelativeTo(nil, BOTTOMLEFT)
function number_current:GetRect()
	return 0, 0, numbers_width * 8, numbers_height, numbers_left_margin, - numbers_bottom_margin
end

function number_current:RenderThis()
	local t = dwindow.total()
	if self.name == "number_current" then t= dwindow.tell() end
	local ms = t % 1000
	local s = t / 1000
	local h = (s / 3600) % 100
	local h1 = h/10
	local h2 = h%10
	local m1 = (s/60 /10) % 6
	local m2 = (s/60) % 10
	local s1 = (s/10) % 6
	local s2 = s%10

	local numbers =
	{
		h1, h2, 10, m1, m2, 10, s1, s2,   -- 10 = : symbol in time
	}

	local x = 0
	for i=1,#numbers do
		paint(x, 0, x+numbers_width, numbers_height, get_bitmap("Z:\\skin\\" .. math.floor(numbers[i]) .. ".png"))
		x = x + numbers_width
	end

end

number_total = BaseFrame:Create({RenderThis = number_current.RenderThis})
number_total.name = "number_total"
root:AddChild(number_total)
toolbar_bg:AddChild(number_total)
number_total:SetRelativeTo(nil, BOTTOMRIGHT)
number_total.t = 23456000
function number_total:GetRect()
	return 0, 0, numbers_width * 8, numbers_height, -numbers_right_margin, - numbers_bottom_margin
end


test = BaseFrame:Create()
test.name = "test"
test:SetRelativeTo(root, TOPLEFT)
root:AddChild(test)
function test:GetRect()
	return 0,0,40,40
end

function test:RenderThis()
	return paint(0,0,40,40, get_bitmap("Z:\\skin\\fullscreen.png"))
end

test2 = BaseFrame:Create()
test2.name = "test2"
test2:SetRelativeTo(test, RIGHT, LEFT)
root:AddChild(test2)
function test2:GetRect()
	return 0,0,40,40
end

function test2:RenderThis()
	return paint(0,0,40,40, get_bitmap("Z:\\skin\\play.png"))
end

test3 = BaseFrame:Create()
test3:SetRelativeTo(test, BOTTOM, TOP)
test3.name = "test3"
root:AddChild(test3)
function test3:GetRect()
	local dy = dwindow.GetTickCount()%10000
	if dy > 5000 then dy = 10000- dy end
	dy = dy * 500 / 5000
	return 0,0,40,40,0,dy
end

function test3:RenderThis()
	return paint(0,0,40,40, get_bitmap("Z:\\skin\\stop.png"))
end

test4 = BaseFrame:Create()
test4.name = "test4"
root:AddChild(test4)
test4:SetRelativeTo(test3, BOTTOMRIGHT, BOTTOMLEFT)
function test4:GetRect()
	local d = (dwindow.GetTickCount())
	dx = math.sin(d/360)*50+40
	dy = math.cos(d/360)*50
	return 0,0,40,40,dx,dy
end

function test4:RenderThis()
	return paint(0,0,40,40, get_bitmap("Z:\\skin\\É´²¼.png"))
end

if dwindow and dwindow.execute_luafile then dwindow.execute_luafile("D:\\private\\tetris.lua") end
