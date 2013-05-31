local playlist = {}
local playing = 0
local get_pos

function playlist_add(path, pos)
	local pos = get_pos(path)
	if pos then return pos end
	
	table.insert(playlist, pos or (#playlist+1), path)
	return pos or #playlist
end

function playlist_play(path)
	local pos = get_pos(path)
	if (pos == playing) and playing then return end
	return playlist_play_item(playlist_add(path))
end

function playlist_remove(path)
	local pos = get_pos(path)
	if pos then
		table.remove(pos)
	end
end

function playlist_clear()
	playlist = {}
end

function playlist_play_next()
	if playing< #playlist then
		return playlist_play_item(playing+1) or playlist_play_next()
	end
end

function playlist_play_previous()
	if playing > 1 then
		return playlist_play_item(playing-1)
	end
end

function playlist_play_item(n)
	if not n or n < 1 or n > #playlist then
		return false
	end
	
	playing = n
	
	return dwindow.reset_and_loadfile(playlist[n])
end

function playlist_get_item(n)
	return playlist[n]
end

function playlist_get_count()
	return #playlist
end

function playlist_get_current_item()
	return playing
end


get_pos = function(path)
	for i=1,#playlist do
		if playlist[i] == path then
			return i
		end
	end
end