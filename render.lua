 -- global functions and variables

local buttons = {}
local toolbar_bg
local progress_bar
local tbl ={}

print("Hello!!!")
if require then require("base_frame") end

logo = BaseFrame:Create()
root:AddChild(logo)
function logo:GetRect()
	return 0,0,dwindow.width,dwindow.height
end

function logo:RenderThis(arg)
	if not movie_loaded then
		local res = get_bitmap("Z:\\skin\\logo2.jpg")

		paint(dwindow.width/2 - 960, dwindow.height/2 - 540, dwindow.width/2 + 960, dwindow.height/2 + 540, res)
	end
end

toolbar_bg = toolbar_bg or BaseFrame:Create()
root:AddChild(toolbar_bg)
function toolbar_bg:GetRect()
	local toolbar_height = 65
	return 0, dwindow.height - toolbar_height, dwindow.width, dwindow.height
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
	return self.x, self.y, self.x+button_size, self.y + button_size
end

local function button_RenderThis(self)
	local filename = ("Z:\\skin\\" .. self.pic[1])
	paint(0,0,button_size,button_size, get_bitmap(filename))
end


-- GPU resource management
function OnInitCPU()
	-- load resources here (optional)
	print("OnInitCPU")

end

function OnInitGPU()
	print("OnInitGPU")
	-- commit them to GPU (optional)
	-- handle resize changes here (must)

	local x = dwindow.width - button_size - margin_button_right
	local y = dwindow.height - button_size - margin_button_bottom

	for i=1,#button_pictures/2 do
		local button = buttons[i] or BaseFrame:Create()
		buttons[i] = button
		root:AddChild(button)
		button.x = x
		button.y = y
		button.pic = {button_pictures[i*2-1], button_pictures[i*2]}
		button.GetRect = button_GetRect
		button.RenderThis = button_RenderThis

		x = x - space_of_each_button
	end
end

progressbar = BaseFrame:Create()
root:AddChild(progressbar)
function progressbar:GetRect()
	local x_max = dwindow.width - margin_progress_right
	local x_min = margin_progress_left
	local y_min = dwindow.height - progress_margin_bottom
	local y_max = y_min + progress_height

	return x_min, y_min, x_max, y_max
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

function progressbar:RenderThis()
	local l,t,r,b = self:GetRect()
	l,r,t,b = 0,r-l,0,b-t
	local fv = (dwindow.GetTickCount()%1000) / 1000.0
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
number_current.t = 12345000
root:AddChild(number_current)
function number_current:GetRect()
	return numbers_left_margin, dwindow.height - numbers_bottom_margin - numbers_height,
			numbers_left_margin + numbers_width * 8, dwindow.height - numbers_bottom_margin
end

function number_current:RenderThis()
	local ms = self.t % 1000
	local s = self.t / 1000
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
number_total.t = 23456000
number_total.x = numbers_left_margin + 500
function number_total:GetRect()
	return dwindow.width - numbers_right_margin - numbers_width * 8,
			dwindow.height - numbers_bottom_margin - numbers_height,
			dwindow.width - numbers_right_margin,
			dwindow.height - numbers_bottom_margin
end

root:AddChild(number_total)

print(number_current, number_total, number_current.parent, number_total.parent)

