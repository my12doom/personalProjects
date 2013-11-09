local dshow_queue = {}
local queue_lock = CritSec:create()
local core_funcs =
{
	tell = player.tell,
	play = player.play,
	pause = player.pause,
	stop = player.stop,
	seek = player.seek,
	reset = player.reset,
	reset_and_loadfile = player.reset_and_loadfile,
	list_audio_track = player.list_audio_track,
	list_subtitle_track = player.list_subtitle_track,
}

local async_funcs = {}


local function removeall()			-- a dummy function for removing all works in queue
end

function async_funcs.play(...)
	queue_lock:lock()
	async_funcs_remove(core_funcs.pause)
	async_funcs_remove(core_funcs.stop)
	table.insert(dshow_queue, {core_funcs.play,...})
	queue_lock:unlock()
end

function async_funcs.pause(...)
	queue_lock:lock()
	table.insert(dshow_queue, {core_funcs.pause,...})
	queue_lock:unlock()
end

function async_funcs.stop(...)
	queue_lock:lock()
	async_funcs_remove(core_funcs.play)
	async_funcs_remove(core_funcs.pause)
	async_funcs_remove(core_funcs.stop)
	table.insert(dshow_queue, {core_funcs.stop,...})
	queue_lock:unlock()
end

function seek_dummy(t)
	async_funcs.seeking_to = t
	core_funcs.seek(t)
	async_funcs.seeking_to = nil	
end

function async_funcs.seek(...)
	queue_lock:lock()
	async_funcs_remove(seek_dummy)
	table.insert(dshow_queue, {seek_dummy,...})
	queue_lock:unlock()
end

function async_funcs.reset(...)
	core.stop_all_handles()
	queue_lock:lock()
	async_funcs_remove(removeall)
	table.insert(dshow_queue, {core_funcs.reset,...})
	queue_lock:unlock()
end

function async_funcs.reset_and_loadfile(...)
	core.stop_all_handles()
	queue_lock:lock()
	async_funcs_remove(removeall)
	table.insert(dshow_queue, {core_funcs.reset_and_loadfile,...})
	queue_lock:unlock()
end

function async_funcs.tell()
	local last_seek = nil
	queue_lock:lock()
	for _, t in ipairs(dshow_queue) do
		if t[1] == seek_dummy then
			last_seek = t[2]
		end
	end
	queue_lock:unlock()
	return last_seek or async_funcs.seeking_to or core_funcs.tell()
end

local last_list_track = 0
local audio_tracks = {}
local subtitle_tracks = {}
local tracks_lock = CritSec:create()
function async_funcs.list_audio_track()
	tracks_lock:lock()
	local rtn = {}
	for k,v in ipairs(audio_tracks) do
		rtn[k] = v
	end
	tracks_lock:unlock()
	
	return rtn
end

function async_funcs.list_subtitle_track()
	tracks_lock:lock()
	local rtn = {}
	for k,v in ipairs(subtitle_tracks) do
		rtn[k] = v
	end
	tracks_lock:unlock()
	
	return rtn
end

function async_funcs_remove(...)
	for _, v in ipairs(table.pack(...)) do
		for k, v2 in ipairs(dshow_queue) do
			if v == removeall or v2[1] == v then
				dshow_queue[k] = nil
			end
		end
	end
end

local worker_thread = Thread:Create(function()
	while true do
		core.Sleep(1)
		
		local work = nil
		
		queue_lock:lock()
		if #dshow_queue > 0 then
			player.movie_loading = true

			work = table.remove(dshow_queue, 1)
		else
			player.movie_loading = false
		end
		queue_lock:unlock()
					
		if core.GetTickCount() - last_list_track > 100 and not player.movie_loading then
			last_list_track = core.GetTickCount()
			
			tracks_lock:lock()
			if player.movie_loaded then
				audio_tracks = table.pack(core_funcs.list_audio_track())
				subtitle_tracks = table.pack(core_funcs.list_subtitle_track())
			else
				audio_tracks = {}
				subtitle_tracks = {}
			end
			tracks_lock:unlock()
		end
		
		if work then
			work[1](select(2, unpack(work)))
		end
	end
end)

for k,v in pairs(core_funcs) do
	player[k] = async_funcs[k]
end
