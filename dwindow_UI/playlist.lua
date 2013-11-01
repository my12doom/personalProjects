playlist = {}
setting.playlist = {}
setting.playlist.playing = 0
setting.playlist.list = {}

local get_pos

function playlist:add(L, R, pos)
	local pos = get_pos(L, R)
	if pos then return pos end
	
	table.insert(setting.playlist.list, pos or (#setting.playlist.list+1), {L=L,R=R})
	root:BroadCastEvent("OnPlaylistChange")
	return pos or #setting.playlist.list
end

function playlist:play(L, R)
	local pos = get_pos(L, R)
	if (pos == setting.playlist.playing) and setting.playlist.playing then
		if not player.movie_loaded then
			return playlist:play_item(setting.playlist.playing)
		end
	end
	return playlist:play_item(playlist:add(L, R))
end

function playlist:remove(L, R)
	local pos = get_pos(L, R)
	return self:remove_item(pos)
end

function playlist:remove_item(pos)
	if pos then
		table.remove(setting.playlist.list, pos)
		root:BroadCastEvent("OnPlaylistChange")
	end
end

function playlist:clear()
	setting.playlist.list = {}
	setting.playlist.playing = 0
	root:BroadCastEvent("OnPlaylistChange")
end

function playlist:next()
	if setting.playlist.playing< #setting.playlist.list then
		return playlist:play_item(setting.playlist.playing+1) or playlist:next()
	end
end

function playlist:previous()
	if setting.playlist.playing > 1 then
		return playlist:play_item(setting.playlist.playing-1)
	end
end

function playlist:play_item(n)
	if not n or n < 1 or n > #setting.playlist.list then
		return false
	end
	
	setting.playlist.playing = n
	
	local L = setting.playlist.list[n].L
	local R = setting.playlist.list[n].R
	
	L = L and parseURL(L)
	R = R and parseURL(R)
	
	if type(L) == "table" then
		-- currently for youku only,
		playlist.L = L
		playlist.R = R
		playlist.current = 1
		
		return player.reset_and_loadfile(L[1].url, R and R[1].url)
	else
		playlist.L = nil
		playlist.R = nil
		playlist.current = nil
		
		return player.reset_and_loadfile(L, R)
	end
end

function playlist:item(n)
	return setting.playlist.list[n]
end

function playlist:count()
	return #setting.playlist.list
end

function playlist:current_pos()
	return setting.playlist.playing
end

function playlist:current_item()
	return playlist:item(setting.playlist.playing) and playlist:item(setting.playlist.playing).L, playlist:item(setting.playlist.playing) and playlist:item(setting.playlist.playing).R
end

get_pos = function(L, R)
	for i=1,#setting.playlist.list do
		if setting.playlist.list[i].L == L and setting.playlist.list[i].R == R then
			return i
		end
	end
end

local ototal = player.total
local otell = player.tell
local oseek = player.seek

function player.total()
	if not playlist.L then
		return ototal()
	end
	
	local total = 0
	for _,v in ipairs(playlist.L) do
		total = total + v.duration
	end
	
	return total * 1000
end

function player.tell()
	if not playlist.L then
		return otell()
	end
	
	local t = otell()
	for i=1, playlist.current-1 do
		t = t + playlist.L[i].duration * 1000
	end
	
	return t
end

function player.seek(target)
	if not playlist.L then
		return oseek(target)
	end
	
	if target < 0 or target >= player.total() then
		return false
	end
	
	-- calculate segment number and time
	local segment = 1
	while segment < #playlist.L and (target > tonumber(playlist.L[segment].duration) * 1000) do
		target = target - playlist.L[segment].duration * 1000
		segment = segment + 1
	end
	
	if segment ~= playlist.current then
		player.reset_and_loadfile(playlist.L[segment].url, playlist.R and R[segment].url)
		playlist.current = segment
	end
	
	oseek(target)
	return true	
end

-- global window or dshow or etc callback
function OnDirectshowEvents(event_code, param1, param2)
	print("OnDirectshowEvents", event_code, param1, param2)

	local EC_COMPLETE = 1
	if event_code == EC_COMPLETE then
	
		-- for segmented files
		if playlist.L and playlist.current < #playlist.L then
			playlist.current = playlist.current + 1
			player.reset_and_loadfile(playlist.L[playlist.current].url, playlist.R and R[playlist.current].url)
			
			return		
		end
		
	
		-- normal files or last segment
		if playlist:current_pos() >= playlist:count() then
			player.stop()
			player.seek(0)
		else
			playlist:next()
		end
	end
end

