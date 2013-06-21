local lua_file = dwindow.loading_file
local lua_path = GetPath(lua_file)
local function GetCurrentLuaPath(offset)
	return lua_path
end

playlist = {}
local list = {}
local playing = 0
local get_pos

function playlist:add(path, pos)
	local pos = get_pos(path)
	if pos then return pos end
	
	table.insert(list, pos or (#list+1), path)
	return pos or #list
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
	list = {}
	playing = 0
end

function playlist:next()
	if playing< #list then
		return playlist:play_item(playing+1) or playlist:next()
	end
end

function playlist:previous()
	if playing > 1 then
		return playlist:play_item(playing-1)
	end
end

function playlist:play_item(n)
	if not n or n < 1 or n > #list then
		return false
	end
	
	playing = n
	
	return dwindow.reset_and_loadfile(list[n])
end

function playlist:item(n)
	return list[n]
end

function playlist:count()
	return #list
end

function playlist:current_pos()
	return playing
end

function playlist:current_item()
	return playlist:item(playing)
end

get_pos = function(path)
	for i=1,#list do
		if list[i] == path then
			return i
		end
	end
end