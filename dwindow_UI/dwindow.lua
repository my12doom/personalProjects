local rect = {0,0,99999,99999,0,0}
local rects = {}
local bitmapcache = {}

if paint_core == nil then paint_core = function() end end
if get_resource == nil then get_resource = function() end end
if load_bitmap_core == nil then load_bitmap_core = function() end end
if bit32 == nil then bit32 = require("bit")end


-- helper functions
function GetPath(pathname)
	local t = string.reverse(pathname)
	t = string.sub(t, string.find(t, "\\") or 1)
	return string.reverse(t)
end
local lua_file = dwindow.loading_file
local lua_path = GetPath(lua_file)
local function GetCurrentLuaPath(offset)
	return lua_path
end


function debug_print(...)
	--print("--DEBUG", ...)
end

function info(...)
	--print("--INFO", ...)
end

function error(...)
	print("--ERROR:", ...)
end


function BeginChild(left, top, right, bottom)
	local left_unclip, top_unclip = left, top
	left = math.max(rect[1], left)
	top = math.max(rect[2], top)
	right = math.min(rect[3], right)
	bottom = math.min(rect[4], bottom)
	local new_rect = {left, top, right, bottom, left_unclip, top_unclip}
	table.insert(rects, rect)
	rect = new_rect

	dwindow.set_clip_rect_core(left, top, right, bottom)
end

function EndChild(left, top, right, bottom)
	rect = table.remove(rects)
	dwindow.set_clip_rect_core(rects[1], rects[2], rects[3], rects[4])
end

function IsCurrentDrawingVisible()
	return rect[3]>rect[1] and rect[4]>rect[2]
end


-- some controlling function
function StartDragging() end
function StartResizing() end


-- global window or dshow or etc callback
function OnDirectshowEvents(event_code, param1, param2)
	print("OnDirectshowEvents", event_code, param1, param2)

	local EC_COMPLETE = 1
	if event_code == EC_COMPLETE then
		if playlist:current_pos() >= playlist:count() then
			dwindow.stop()
			dwindow.seek(0)
		else
			playlist:next()
		end
	end
end
function OnMove() end
function OnSize() end


-- the Main Render function
local last_render_time = 0
function RenderUI(view)
	local delta_time = 0;
	if last_render_time > 0 then delta_time = dwindow.GetTickCount() - last_render_time end
	last_render_time = dwindow.GetTickCount();
	local t = dwindow.GetTickCount()
	if view == 0 then root:BroadCastEvent("PreRender", last_render_time, delta_time) end
	local dt = dwindow.GetTickCount() -t
	t = dwindow.GetTickCount()

	root:render(view)

	local dt2 = dwindow.GetTickCount() -t

	if dt > 0 or dt2 > 0 then
		info(string.format("slow RenderUI() : PreRender() cost %dms, render() cost %dms", dt, dt2))
	end
end

-- GPU resource management
function OnInitCPU()
	debug_print("OnInitCPU")
	root:BroadCastEvent("OnInitCPU", last_render_time, delta_time)
	-- load resources here (optional)
end

function OnInitGPU()
	debug_print("OnInitGPU")
	root:BroadCastEvent("OnInitGPU", last_render_time, delta_time)
	root:BroadCastEvent("OnLayoutChange")

	-- commit them to GPU (optional)
	-- handle resize changes here (must)
end

function OnReleaseGPU()
	-- decommit all GPU resources (must)
	debug_print("OnReleaseGPU")
	root:BroadCastEvent("OnReleaseGPU", last_render_time, delta_time)
	releaseCache(true)
end

function OnReleaseCPU()
	-- release all resources (must)
	debug_print("OnReleaseCPU")
	root:BroadCastEvent("OnReleaseCPU", last_render_time, delta_time)
	releaseCache(false)
end

function releaseCache(is_decommit)
	if is_decommit then
		for _,v in pairs(bitmapcache) do
			dwindow.decommit_resource_core(v.res)
		end
	else
		for _,v in pairs(bitmapcache) do
			dwindow.release_resource_core(v.res)
			bitmapcache = {}
		end
	end
end

function test_get_text_bitmap(text)
	local res, width, height = dwindow.draw_font_core(text)		-- width is also used as error msg output.
	if not res then
		error(width, filename)
		return
	end
	local rtn = {res = res, width = width, height = height, filename=filename}
	rtn.left = 0
	rtn.right = 0
	rtn.top = 0
	rtn.bottom = 0
	return rtn
end

function get_bitmap(filename, reload)
	if reload then unload_bitmap(filename) end
	if bitmapcache[filename] == nil then
		local res, width, height = dwindow.load_bitmap_core(filename)		-- width is also used as error msg output.

		bitmapcache[filename] = {res = res, width = width, height = height, filename=filename}
		if not res then
			error(width, filename)
			bitmapcache[filename].width = nil
			bitmapcache[filename].error_msg = width
		end
		print("loaded", filename, width, height)
	end

	-- reset ROI
	local rtn = bitmapcache[filename]
	rtn.left = 0
	rtn.right = 0
	rtn.top = 0
	rtn.bottom = 0
	return rtn
end

function unload_bitmap(filename, is_decommit)
	local tbl = bitmapcache[filename]
	if not tbl or type(tbl)~="table" then return end

	if is_decommit then
		dwindow.decommit_resource_core(bitmapcache[filename].res)
	else
		dwindow.release_resource_core(bitmapcache[filename].res)
		bitmapcache[filename] = nil
	end
end

-- set region of interest
function set_bitmap_rect(bitmap, left, top, right, bottom)
	if not bitmap or "table" ~= type(bitmap) then return end
	bitmap.left = left
	bitmap.right = right
	bitmap.top = top
	bitmap.bottom = bottom
end

-- paint
bilinear_mipmap_minus_one = 0
lanczos = 1
bilinear_no_mipmap = 2
lanczos_onepass = 3
bilinear_mipmap = 4

function paint(left, top, right, bottom, bitmap, alpha, resampling_method)
	if not bitmap or not bitmap.res then return end
	local x,y  = rect[5], rect[6]
	local a = alpha or 1.0
	return dwindow.paint_core(left+x, top+y, right+x, bottom+y, bitmap.res, bitmap.left, bitmap.top, bitmap.right, bitmap.bottom, a, resampling_method or bilinear_no_mipmap)
end

-- native threading support
Thread = {}

function Thread:Create(func, ...)

	local o = {}
	setmetatable(o, self)
	self.__index = self
	self.handle = dwindow.CreateThread(func, ...)

	return o
end

-- these two native method is not usable because it will keep root state machine locked and thus deadlock whole lua state machine
function Thread:Resume()
	return dwindow.ResumeThread(self.handle)
end

function Thread:Suspend()
	return dwindow:SuspendThread(self.handle)
end

--function Thread:Terminate(exitcode)
--	return dwindow.TerminateThread(self.handle, exitcode or 0)
--end

function Thread:Wait(timeout)
	return dwindow.WaitForSingleObject(self.handle, timeout)
end

function Thread:Sleep(timeout)		-- direct use of dwindow.Sleep() is recommended
	return dwindow.Sleep(timeout)
end


local hello = Thread:Create(function (...)
	print("Hello Thread!",...)
	dwindow.Sleep(1500)
	print("Bye Thread!",...)
end, 0,1,2,3)

hello:Suspend()
--Thread.Sleep(500)
hello:Resume()
--Thread.Sleep(500)



-- load base_frame
if dwindow and dwindow.execute_luafile then
	print(dwindow.execute_luafile(GetCurrentLuaPath() .. "base_frame.lua"))
	print(dwindow.execute_luafile(GetCurrentLuaPath() .. "3dvplayer.lua"))
	print(dwindow.execute_luafile(GetCurrentLuaPath() .. "Container.lua"))
	print(dwindow.execute_luafile(GetCurrentLuaPath() .. "playlist.lua"))
	print(dwindow.execute_luafile(GetCurrentLuaPath() .. "menu.lua"))
end

function ReloadUI(legacy)
	print("ReloadUI()", legacy, root)
	OnReleaseGPU()
	OnReleaseCPU()
	root = BaseFrame:Create()

	if legacy then return end

	print(dwindow.execute_luafile(GetCurrentLuaPath() .. "classic\\render.lua"))
	print(dwindow.execute_luafile(GetCurrentLuaPath() .. "Tetris\\Tetris.lua"))
	v3dplayer_add_button()

	-- the menu sample
	local sample = menu:Create()
	--root:AddChild(sample)
	sample:SetRelativeTo(TOP)

	function sample:PreRender()
		self.y = (self.y or 200) - 1
		self:SetRelativeTo(TOP, nil, nil, 0, self.y)
	end

	for i=1, 50 do
		--sample:AddItem("").id = i
	end
end
