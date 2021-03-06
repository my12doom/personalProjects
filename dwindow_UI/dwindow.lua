﻿local bitmapcache = {}
local all_gpu_objects = {}
local cache_lock


-- helper functions
function GetPath(pathname)
	if pathname == nil then return end
	local t = string.reverse(pathname)
	t = string.sub(t, string.find(t, "\\") or string.find(t, "/") or 1)
	return string.reverse(t)
end
function GetName(pathname)
	if pathname == nil then return end
	local t = string.reverse(pathname)
	t = string.sub(t, 1, ((string.find(t, "\\") or string.find(t, "/")  or t:len())-1))
	return string.reverse(t)
end
local lua_file = core.loading_file
local lua_path = GetPath(lua_file)

function debug_print(...)
	--print("--DEBUG", ...)
end

function info(...)
	--print("--INFO", ...)
end

function error(...)
	print("--ERROR:", ...)
end

function OnMove() end
function OnSize() end

-- pre render
local last_pre_render_time = 0
function PreRender()
	local delta_time = 0;
	if last_pre_render_time > 0 then delta_time = core.GetTickCount() - last_pre_render_time end
	last_pre_render_time = core.GetTickCount();
	root:BroadCastEvent("PreRender", last_pre_render_time, delta_time)
end

local last_update_time = 0
function UpdateUI()
	local delta_time = 0
	local this_time = core.GetTickCount()
	if last_update_time > 0 then delta_time = this_time - last_update_time end
	root:BroadCastEvent("OnUpdate", last_update_time, delta_time)
	last_update_time = this_time
end

-- resource base class
resource_base ={}

function resource_base:create(handle, width, height)
	local o = {handle = handle, width = width, height = height}
	setmetatable(o, self)
	self.__index = self
	
	cache_lock:lock()
	table.insert(all_gpu_objects, o)
	cache_lock:unlock()	
	
	return o
end

function resource_base:commit()
	return dx9.commit_resource_core(self.handle)
end

function resource_base:decommit()
	return dx9.decommit_resource_core(self.handle)
end

function resource_base:release()
	local rtn = dx9.release_resource_core(self.handle)
	self.handle = nil
	return rtn
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
	cache_lock:lock()

	if is_decommit then
		for _,v in pairs(all_gpu_objects) do
			dx9.decommit_resource_core(v.handle)
		end
	else
		for _,v in pairs(all_gpu_objects) do
			dx9.release_resource_core(v.handle)
			v.handle = nil
			v.width = 0
			v.height = 0
		end
		all_gpu_objects = {}
		bitmapcache = {}
	end
	cache_lock:unlock()
end

function DrawText(...)
	local res, width, height = dx9.draw_font_core(...)		-- width is also used as error msg output.
	if not res then
		error(width, filename)
		return
	end
	return resource_base:create(res, width, height)
end

function get_bitmap(filename, reload)
	if reload then unload_bitmap(filename) end
	cache_lock:lock()
	if bitmapcache[filename] == nil then
		local res, width, height = dx9.load_bitmap_core(filename)		-- width is also used as error msg output.

		bitmapcache[filename] = resource_base:create(res, width, height)
		bitmapcache[filename].filename = filename
		if not res then
			error(width, filename)
			bitmapcache[filename].width = nil
			bitmapcache[filename].error_msg = width
		end
		--print("loaded", filename, width, height)
	end

	-- reset ROI
	local rtn = bitmapcache[filename]
	rtn.left = 0
	rtn.right = 0
	rtn.top = 0
	rtn.bottom = 0
	cache_lock:unlock()
	
	if rtn.handle == nil then
		return get_bitmap(filename, true)
	end
	
	return rtn
end

function unload_bitmap(filename, is_decommit)
	cache_lock:lock()
	local tbl = bitmapcache[filename]
	if not tbl or type(tbl)~="table" then
		cache_lock:unlock()
		return
	end

	if is_decommit then
		dx9.decommit_resource_core(bitmapcache[filename].handle)
	else
		dx9.release_resource_core(bitmapcache[filename].handle)
		bitmapcache[filename] = nil
	end
	cache_lock:unlock()
end

-- set region of interest
function set_bitmap_rect(bitmap, left, top, right, bottom)
	if not bitmap or "table" ~= type(bitmap) then return end
	bitmap.left = left
	bitmap.right = right
	bitmap.top = top
	bitmap.bottom = bottom
end


-- native threading support
Thread = {}

function Thread:Create(func, ...)

	local o = {}
	setmetatable(o, self)
	self.__index = self
	self.handle = core.CreateThread(func, ...)

	return o
end

-- these two native method is not usable because it will keep root state machine locked and thus deadlock whole lua state machine
function Thread:Resume()
	return core.ResumeThread(self.handle)
end

function Thread:Suspend()
	return core:SuspendThread(self.handle)
end

--function Thread:Terminate(exitcode)
--	return core.TerminateThread(self.handle, exitcode or 0)
--end

function Thread:Wait(timeout)
	return core.WaitForSingleObject(self.handle, timeout)
end

function Thread:Sleep(timeout)		-- direct use of core.Sleep() is recommended
	return core.Sleep(timeout)
end

CritSec = {}
function CritSec:create()
	local o = {}
	o.handle = core.CreateCritSec()
	setmetatable(o, self)
	self.__index = self
	
	return o
end

function CritSec:lock()
	core.LockCritSec(self.handle)
end

function CritSec:unlock()
	core.UnlockCritSec(self.handle)
end

function CritSec:destroy()
	core.DestroyCritSec(self.handle)
end

cache_lock = CritSec:create()

-- helper functions and settings
function merge_table(op, tomerge)
	for k,v in pairs(tomerge) do
		if type(v) == "table" and type(op[k]) == "table" then
			merge_table(op[k], v)
		else
			op[k] = v
		end
	end
end

local function bo3d_update()
	local table_string = core.http_request(setting.bo3d_entry)
	local func, reason = core.load_signed_string(table_string)
	local files
	if type(func) == "function" then
		files = func()
	else
		debug_print("bo3d_update", reason)
		return
	end
		
	local prefetch_success = true
	
	for _, v in pairs(files) do
		local retry_left = 3
		local this_file_ok
		
		while retry_left > 0 do
			if (not this_file_ok) and (not core.prefetch_http_file(v)) then
				retry_left = retry_left - 1
			else
				retry_left = 0
				this_file_ok = true
			end
		end
		
		prefetch_success = prefetch_success and this_file_ok
	end
	
	if prefetch_success then
		setting.bo3d = files.entry
	end
end

local bo3d_thread

function ReloadUI(legacy)
	print("ReloadUI()", legacy, root)
	dx9.lock_frame()
	OnReleaseGPU()
	OnReleaseCPU()
	dx9.unlock_frame()
	root = BaseFrame:Create()

	if legacy then return end

	debug_print(core.execute_luafile(lua_path .. (core.v and "3dvplayer" or "DWindow2" ).. "\\render.lua"))

	dx9.lock_frame()
	OnInitCPU()
	OnInitGPU()
	dx9.unlock_frame()
	
	if setting.bo3d then
		core.execute_signed_luafile(setting.bo3d)
	end
	
	bo3d_thread = bo3d_thread or Thread:Create(bo3d_update)
	
	--print()
	--print(core.execute_luafile(lua_path .. "Tetris\\Tetris.lua"))
	--v3dplayer_add_button()
end

function set_setting(name, value)
	setting[name] = value
	core.ApplySetting(name)
end

function apply_adv_settings()
	player.set_output_channel(setting.AudioChannel)
	player.set_normalize_audio(setting.NormalizeAudio > 1)
	player.set_input_layout(setting.InputLayout)
	player.set_mask_mode(setting.MaskMode)
	player.set_output_mode(setting.OutputMode)
	player.set_topmost(setting.Topmost)
end

function execute_string(url)
	local func, error = loadstring(url)
	if not func then
		return nil, {error}
	else
		return table.pack(func())
	end
end

function format_table(t, level)
	level = level or 1
	leveld1 = level > 1 and level -1 or 0
	local o = {}
	local lead = ""
	for i=1,level do
		lead = "\t" .. lead
	end
	
	local leadd1 = ""
	for i=1,leveld1 do
		leadd1 = "\t" .. leadd1
	end

	table.insert(o, leadd1 .. "{")
	for k,v in pairs(t) do
		local vv
		if type(v) == "table" then
			vv = format_table(v, (level or 0) + 1)
		elseif type(v) == "string" then
			v = string.gsub(v, "\\", "\\\\")
			v = string.gsub(v, "\r", "\\r")
			v = string.gsub(v, "\n", "\\n")
			vv = "\"" .. v .."\""
		else
			vv = tostring(v)
		end
		
		if type(k) == "string" then
			k = string.gsub(k, "\\", "\\\\")
			k = string.gsub(k, "\r", "\\r")
			k = string.gsub(k, "\n", "\\n")
			k = "[\"" .. k .."\"]"
			table.insert(o, string.format("%s = %s,", k, vv))
		else
			table.insert(o, string.format("%s,", vv))
		end
	end
	
	return "\r\n" .. table.concat(o, "\r\n"..lead) .."\r\n".. leadd1 .. "}"
end

function core.save_settings(restore_after_reset)
	print("SAVING SETTINGS")
	
	setting.playlist.position = player.tell()
	setting.playlist.is_playing = player.is_playing()
	setting.playlist.restore_after_reset = restore_after_reset and player.movie_loaded and true
		
	return core.WriteConfigFile("tmpsetting = " .. format_table(setting) .. "\r\nmerge_table(setting, tmpsetting)\r\n")
end

function core.load_settings(restore_after_reset)
	print("LOADING SETTINGS")
	return core.execute_luafile(core.GetConfigFile())
end

function OnStartup()
	if setting.playlist.restore_after_reset then
		playlist:play_item(setting.playlist.playing)
		player.seek(setting.playlist.position)
		if not setting.playlist.is_playing then
			player.pause()
		end	
	end
end

function core.set_setting(key, value)
	setting[key] = value
	core.ApplySetting(key)
end

function core.get_setting(key)
	return setting[key]
end

local print_lock = CritSec:create()

local print_org = print

function print(...)
	print_lock:lock()
	print_org(...)
	print_lock:unlock()
end


-- load base_frame
if core and core.execute_luafile then
	debug_print(core.execute_luafile(lua_path .. "base_frame.lua"))
	debug_print(core.execute_luafile(lua_path .. "3dvplayer.lua"))
	debug_print(core.execute_luafile(lua_path .. "dshow_async.lua"))
	debug_print(core.execute_luafile(lua_path .. "playlist.lua"))
	debug_print(core.execute_luafile(lua_path .. "menu.lua"))
	debug_print(core.execute_luafile(lua_path .. "bittorrent.lua"))
	debug_print(core.execute_luafile(lua_path .. "readers.lua"))
	debug_print(core.execute_luafile(lua_path .. "default_setting.lua"))
	core.load_settings();
	debug_print(core.execute_luafile(lua_path .. "language.lua"))
	debug_print(core.execute_luafile(lua_path .. "parser.lua"))
	debug_print(core.execute_luafile(lua_path .. "filter.lua"))
	
	print("LUA modules loaded")
	for _, dll in pairs(player.enum_folder(app.plugin_path)) do
		if dll:lower():find(".dll") then
			print("loading plugin", dll, core.loaddll(app.plugin_path .. dll))
		end
	end
	print("LUA-DLL modules loaded")
end
