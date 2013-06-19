playlist = {}
local playing = 0
local get_pos

function playlist:add(path, pos)
	local pos = get_pos(path)
	if pos then return pos end
	
	table.insert(playlist, pos or (#playlist+1), path)
	return pos or #playlist
end

function playlist:play(path)
	local pos = get_pos(path)
	if (pos == playing) and playing then return end
	return playlist:play_item(playlist:add(path))
end

function playlist:remove(path)
	local pos = get_pos(path)
	if pos then
		table.remove(pos)
	end
end

function playlist:clear()
	playlist = {}
	playing = 0
end

function playlist:next()
	if playing< #playlist then
		return playlist:play_item(playing+1) or playlist:play_next()
	end
end

function playlist:previous()
	if playing > 1 then
		return playlist:play_item(playing-1)
	end
end

function playlist:play_item(n)
	if not n or n < 1 or n > #playlist then
		return false
	end
	
	playing = n
	
	return dwindow.reset_and_loadfile(playlist[n])
end

function playlist:item(n)
	return playlist[n]
end

function playlist:count()
	return #playlist
end

function playlist:current_pos()
	return playing
end

function playlist:current_item()
	return playlist:item(playing)
end

get_pos = function(path)
	for i=1,#playlist do
		if playlist[i] == path then
			return i
		end
	end
end