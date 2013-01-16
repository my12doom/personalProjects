local ww = 10
local ww = 10
local hh = 16
local typecount = 8
local max_shape_len = 5
local max_dis = -100


local CHECK_LEFT = 1
local CHECK_RIGHT = 2
local CHECK_BOTTOM = 4
local CHECK_HIT = 8

local blocks =
{
}

local posx = ww/2
local posy = 1
local shape = {}

if bit32 == nil and require then bit32 = require("bit") end

local function check()

	local out = 0
	local x,y
	for k=1,max_shape_len do
		if (shape[k].x > max_dis and shape[k].y > max_dis) then

			x=posx + shape[k].x;
			y=posy + shape[k].y;

			if (x<1 or x>ww or y<1 or y>hh) then
				out = bit32.bor(out, CHECK_HIT)
			else
				if (x<=1) then
					out = bit32.bor(out, CHECK_LEFT);
				elseif (blocks[(x-1)][y] > 0) then
					out = bit32.bor(out, CHECK_LEFT);
				end

				if (x>=ww) then
					out = bit32.bor(out, CHECK_RIGHT);
				elseif (blocks[x+1][y] > 0) then
					out = bit32.bor(out, CHECK_RIGHT);
				end

				if (y>=hh) then
					out = bit32.bor(out, CHECK_BOTTOM);
				elseif (blocks[x][y+1] > 0) then
					out = bit32.bor(out, CHECK_BOTTOM);
				end

				if (x>0 and x<=ww and y>0 and y<=hh) then
					if (blocks[x][y] > 0) then
						out = bit32.bor(out, CHECK_HIT);
					end
				end
			end
		end
	end

	return out;
end


-- init blocks
local function init()
	for x=1,ww do
		blocks[x] = {}
		for y=1,hh do
			blocks[x][y] = 0
		end
	end

	type2shape()
end


local function clone(t)
	if t == nil then return end
	local o = {}
	for k,v in ipairs(t) do
		o[k] = v
	end
	return o
end


function type2shape(type)
	if not type then type = math.random(typecount) end
	local p1 =
	{
		{{1,0}, {1,1},             {max_dis,max_dis}},		--2格竖条
		{{0,1}, {1,1},{1,0},{1,-1},{max_dis,max_dis}},		--左拐L拐条
		{{1,1}, {0,1},{0,0},{0,-1},{max_dis,max_dis}},		--右拐L拐条
		{{0,0}, {0,1},{1,1},{1,0}, {max_dis,max_dis}},		--方块
		{{0,0}, {0,1},{1,0},{0,-1},{max_dis,max_dis}},		--K条
		{{0,-1},{0,0},{0,1},{0,2}, {max_dis,max_dis}},		--4格竖长条
		{{0,-1},{0,0},{1,0},{1,1}, {max_dis,max_dis}},		--右拐Z拐条
		{{1,-1},{1,0},{0,0},{0,1}, {max_dis,max_dis}},		--左拐Z拐条
	}

	for i=1,max_shape_len do
		shape[i] = {x= (p1[type][i] or {max_dis,max_dis})[1], y = (p1[type][i] or {max_dis,max_dis})[2]}
	end
	return 0
end


local function Step()
	local r = check();
	if( bit32.band(r, CHECK_BOTTOM) > 0) then
		local x,y
		for i=1,max_shape_len do
			if (shape[i].x>max_dis or shape[i].y>max_dis) then

				x=posx + shape[i].x;
				y=posy + shape[i].y;

				blocks[x][y] = 1;

			end
		end

		-- check bottom
		local f = true;
		for j=hh,1,-1 do
			f = true;
			while f do
				for i=1,ww do
					f = f and (blocks[i][j] >0)
				end

				if f then
					for k=j,2,-1 do
						for i=1,ww do
							blocks[i][k] = blocks[i][k-1]
						end
					end
				end
			end
		end

		posy = 1;
		posx = ww/2;
		type2shape();
		r = check();
		print("check=", r)
		if bit32.band(r,CHECK_BOTTOM) > 0 then
			-- game over...
			--DrawBlocks();
			--MessageBox(0, "Game Over", "....", MB_OK);
			init();
		end
	else
		posy = posy+1;
	end

	return 0;
end

local function rotateBlock()
	-- 以(0.5,0.5)为中心旋转
	local x;

	for i=1,max_shape_len do
		if (shape[i].x>max_dis or shape[i].y>max_dis) then
			x = shape[i].x *2 -1;
			shape[i].x = (-(shape[i].y*2-1) +1 )/2;
			shape[i].y = (x + 1 )/2;
		end
	end

	--如果有碰撞，旋转回去
	local r = check();
	if bit32.band(r,CHECK_HIT) > 0 then
		for i=1,max_shape_len do
			if (shape[i].x>max_dis or shape[i].y>max_dis) then
			x = shape[i].x *2 -1;
			shape[i].x = ((shape[i].y*2-1) +1 )/2;
			shape[i].y = (-x + 1 )/2;
			end
		end
		return -1;
	end
	return 0;
end

local VK_LEFT = 0x25
local VK_UP = 0x26
local VK_RIGHT = 0x27
local VK_DOWN = 0x28

function OnKeyDown(key, id)
	print(key, id)
	local r = check()
	if key == VK_LEFT and bit32.band(r, CHECK_LEFT) == 0 then
		posx = posx - 1
	elseif key == VK_RIGHT and bit32.band(r, CHECK_RIGHT) == 0 then
		posx = posx + 1
	elseif key == VK_DOWN and bit32.band(r, CHECK_BOTTOM) == 0 then
		posy = posy + 1
	elseif key == VK_UP then
		rotateBlock()
	else
	end
end

function OnMouseDown(x, y, key)
	local frame = root:GetFrameByPoint(x,y)
	local name = tostring(frame)
	if frame then 
		name = frame.name or name 
		frame:BringToTop(true)
	end
	print("GetFrameByPoint", x, y, name)
end

local tick = 0
local px, py = 0, 0
local function OnUpdate()	
	if dwindow.GetTickCount() - tick > 200 then
		tick = dwindow.GetTickCount()
		Step()
	end
	
	if grow and grow.OnUpdate then grow:OnUpdate() end
	
	px,py = dwindow.get_mouse_pos()
end

local old_RenderUI = RenderUI
function RenderUI(...)
	OnUpdate()
	return old_RenderUI(...)
end

init()

local tetris = BaseFrame:Create()
tetris.name = "TETRIS"
tetris:SetRelativeTo(nil, RIGHT)
root:AddChild(tetris)
function tetris:GetRect()
	return 0,0,40*ww+40,40*hh+40,200,-50
end

function tetris:RenderThis()
	local res = get_bitmap("Z:\\skin\\fullscreen.png")
	local res2 = get_bitmap("Z:\\skin\\stop.png")
	set_bitmap_rect(res2, 0,0,100,100)

	-- paint border
	for i=1,hh+1 do
		paint(ww*40, (i-1)*40, (ww+1)*40, i*40, res2)
	end

	for i=1,ww do
		paint((i-1)*40, hh*40, i*40, (hh+1)*40, res2)
	end

	-- paint static blocks
	for y=1,hh do
		for x=1,ww do
			if blocks[x][y] > 0 then
				local xx = (x-1)*40
				local yy = (y-1)*40
				paint(xx,yy,xx+40,yy+40,res)
			end
		end
	end

	-- paint that shape
	for k=1,max_shape_len do
		if (shape[k].x>max_dis or shape[k].y>max_dis) then
			local i=posx + shape[k].x;
			local j=posy + shape[k].y;

			paint((i-1)*40,(j-1)*40,(i)*40,(j)*40,res)
		end
	end
end

local digittest = BaseFrame:Create()
root:AddChild(digittest)
digittest:SetRelativeTo(root, TOP)
function digittest:GetRect()
	-- 9 pixel widht, 13 pixel height
	
	return 0,0,9,14
end

function digittest:RenderThis()
	local n = math.floor((dwindow.GetTickCount() / 300) % 10)
	local res = get_bitmap("Z:\\skin\\digit.bmp")
	set_bitmap_rect(res, 6+n*9, 0, 15+n*9, 14)
	paint(0, 0, 9, 14,res)
end


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



grow = BaseFrame:Create()
grow.x = 0
grow.y  = 0
local last_in_time = 0
local last_out_time = 0
local last_in = false
local alpha_tick = 0
grow:SetRelativeTo(nil, BOTTOMLEFT)
toolbar_bg:AddChild(grow)
grow.name = "GROW"

function grow:GetRect()
	return 0,0,250,250,self.x-125,125
end

function grow:Stick(dt)
	local frame = root:GetFrameByPoint(px,py)
	local isbutton = false
	for _,v in ipairs(buttons) do
		if v == frame then
			isbutton = true
		end
	end
	if not isbutton then return end
	
	local l,t,r,b = frame:GetAbsRect()
	local dx = (l+r)/2 - self.x
	self.x = self.x + 0.82 * dx
end

function grow:OnUpdate()
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
	local res = get_bitmap("Z:\\skin\\grow.png")
	local alpha = alpha_tick / 200
	paint(0, 0, 250, 250, res, alpha)
end

function grow:HitTest()
	return false
end