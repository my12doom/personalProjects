-- 3dvplayer UI renderer

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

logo = BaseFrame:Create()
logo.name = "LOGO"
root:AddChild(logo)
logo:SetRelativeTo(CENTER)
logo:SetSize(1920,1080)

function logo:RenderThis()
	if not dwindow.movie_loaded then
		local res = get_bitmap("logo_bg.png")
		paint(0,0,1920,1080, res)
	end
end

function logo:OnMouseDown(x, y, key)
	if key == VK_RBUTTON then
		dwindow.popup_menu()
	end
	
	return false
end

logo_hot = BaseFrame:Create()
logo_hot.name = "LOGO HOT AREA"
logo:AddChild(logo_hot)
logo_hot:SetRelativeTo(CENTER)
logo_hot:SetSize(400,171)

function logo_hot:RenderThis()
	if not dwindow.movie_loaded then
		local res = get_bitmap("logo_hot.png")
		paint(0,0,400,171, res)
	end
end

function logo_hot:OnMouseDown(...)
	if dwindow.movie_loaded then
		return false
	end
	dwindow.popup_menu()
	return true
end

toolbar_bg = BaseFrame:Create()
toolbar_bg.name = "toolbar_bg"
root:AddChild(toolbar_bg)
toolbar_bg:SetRelativeTo(BOTTOMLEFT)
toolbar_bg:SetRelativeTo(BOTTOMRIGHT)
toolbar_bg:SetSize(nil, toolbar_height)
print("toolbar_height=", toolbar_height)

function toolbar_bg:RenderThis()
	local l,t,r,b = self:GetAbsRect()
	local res = get_bitmap("toolbar_background.png");
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

local button_functions = 
{
	dwindow.toggle_fullscreen,
	dwindow.set_volume,
	playlist.next,
	dwindow.pause,
	playlist.previous,
	dwindow.reset,
	dwindow.toggle_3d,
}


local function button_GetRect(self)
	return 0, 0, space_of_each_button, button_size, self.dx, self.dy
end

local function button_RenderThis(self)
	paint(11,0,button_size+11,button_size, get_bitmap(self.pic[1]))
end

local function button_OnMouseDown(self)
	return button_functions[self.id]()
end


local x = - margin_button_right
local y = - margin_button_bottom
local buttons = {}

for i=1,#button_pictures/2 do
	local button = buttons[i] or BaseFrame:Create()
	buttons[i] = button
	toolbar_bg:AddChild(button)
	button.pic = {button_pictures[i*2-1], button_pictures[i*2]}
	button.RenderThis = button_RenderThis
	button.OnMouseDown = button_OnMouseDown
	button.name = button.pic[1]
	button:SetRelativeTo(BOTTOMRIGHT, nil, nil, x, y)
	button:SetSize(space_of_each_button, button_size)
	button.id = i

	x = x - space_of_each_button
	
	print("loaded button", i)
end

progressbar = BaseFrame:Create()
progressbar.name = "progressbar"
toolbar_bg:AddChild(progressbar)
progressbar:SetRelativeTo(BOTTOMLEFT, nil, nil, margin_progress_left, -progress_margin_bottom+progress_height)
progressbar:SetRelativeTo(BOTTOMRIGHT, nil, nil, -margin_progress_right, -progress_margin_bottom+progress_height)
progressbar:SetSize(nil, progress_height)

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
	return progress_pic[n]
end

function progressbar:OnMouseDown(x)
	local l,_,r = self:GetAbsRect()
	local fv = x/(r-l)
	dwindow.seek(dwindow.total()*fv)	
end

function progressbar:RenderThis()
	local l,t,r,b = self:GetAbsRect()
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
number_current:SetRelativeTo(BOTTOMLEFT, nil, nil, numbers_left_margin, - numbers_bottom_margin)
number_current:SetSize(numbers_width * 8, numbers_height)

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
		paint(x, 0, x+numbers_width, numbers_height, get_bitmap(math.floor(numbers[i]) .. ".png"))
		x = x + numbers_width
	end

end

number_total = BaseFrame:Create()
number_total.name = "number_total"
toolbar_bg:AddChild(number_total)
number_total:SetRelativeTo(BOTTOMRIGHT, nil, nil, -numbers_right_margin, - numbers_bottom_margin)
number_total.t = 23456000
number_total:SetSize(numbers_width * 8, numbers_height)
number_total.RenderThis = number_current.RenderThis


grow = BaseFrame:Create()
grow.x = 0
grow.y  = 0
local last_in_time = 0
local last_out_time = 0
local last_in = false
local alpha_tick = 0
toolbar_bg:AddChild(grow)
grow.name = "GROW"
grow:SetSize(250, 250)
grow:SetRelativeTo(BOTTOMLEFT, nil, nil, -125, 125)

function grow:Stick(dt)
	local frame = root:GetFrameByPoint(self.px or 0, self.py or 0)
	local isbutton = false
	for _,v in ipairs(buttons) do
		if v == frame then
			isbutton = true
		end
	end
	if not isbutton then return end
	
	local l,t,r,b = frame:GetAbsRect()
	local dx = (l+r)/2 - self.x	
	print(self.x, dx)
	self.x = self.x + 0.65 * dx
	self:SetRelativeTo(BOTTOMLEFT, nil, nil, self.x-125, 125)
end

function grow:PreRender()
	self.px, self.py = dwindow.get_mouse_pos()
	local px,py = self.px,self.py
	
	local r,b = toolbar_bg:GetAbsAnchorPoint(BOTTOMRIGHT)
	r,b = r - margin_button_right, b - margin_button_bottom
	local l,t = toolbar_bg:GetAbsAnchorPoint(BOTTOMRIGHT)
	l,t = l - margin_progress_right, t - margin_button_bottom - button_size
	local dt = 0;
	if l<=px and px<r and t<=py and py<b then
		if not last_in then 
			self:OnEnter()
		else
			alpha_tick = alpha_tick + (dwindow.GetTickCount() - last_in_time)+2
			dt = dwindow.GetTickCount() - last_in_time
		end
		last_in = true
		last_in_time = dwindow.GetTickCount()
	else
		if last_in then
			self:OnLeave()
		else
			alpha_tick = alpha_tick - (dwindow.GetTickCount() - last_out_time)*0.8
			dt = dwindow.GetTickCount() - last_out_time
		end
		last_in = false
		last_out_time = dwindow.GetTickCount()
	end
	
	alpha_tick = math.max(alpha_tick, 0)
	alpha_tick = math.min(alpha_tick, 300)
	
	if dt > 0 then self:Stick(dt) end
end

function grow:OnEnter()
	print("OnEnter")
end

function grow:OnLeave()
	print("OnLeave")
end

function grow:RenderThis()
	local res = get_bitmap("grow.png")
	local alpha = alpha_tick / 200
	paint(0, 0, 250, 250, res, alpha)
end

function grow:HitTest()
	return false
end
