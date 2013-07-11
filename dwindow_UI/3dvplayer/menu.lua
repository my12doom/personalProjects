function popup3dvstar()
	local m = 
	{
		{
			string = L("Open File..."),
			on_command = function(v)
				local url = ui.OpenFile()
				if url then
					playlist:play(url)
				end
			end
		},
		{
			string = L("Open Folder..."),
			on_command = function(v)
				local folder = ui.OpenFolder()
				if folder then
					local tbl = player.enum_folder(folder)
					local to_play
					for _,v in pairs(tbl) do
						if v:sub(v:len()) ~= "\\" then
							to_play = folder .. v
							playlist:add(folder .. v)
						else
						end
					end						
							
					if to_play then
						playlist:play(to_play)
					end
				end
			end
		},
		{
			string = L("Close File"),
			grayed = not player.movie_loaded,
			on_command = function(v)
				player.reset()
			end
		},
		{
			seperator = true,
		},
		{
			string = L("Play\t(Space)"),
			grayed = player.is_playing(),
			on_command = function(v) player.pause() end
		},
		{
			string = L("Pause\t(Space)"),
			grayed = not player.is_playing(),
			on_command = function(v) player.pause() end
		},
		{
			string = L("Stop"),
			on_command = function(v) player.stop() end
		},
		{
			string = L("Previous\t(PageUp)"),
			grayed = playlist:current_pos() <= 1,
			on_command = function(v) playlist:previous() end
		},
		{
			string = L("Next\t(PageDown)"),
			grayed = playlist:current_pos() >= playlist:count(),
			on_command = function(v) playlist:next() end
		},
		{
			string = L("Playlist"),
			sub_items =
			{
				{
					string = "Item1",
					checked = true
				},
				{
					string = "Item2",
				},
				{
					seperator = true,
				},
				{
					string = "Clear",
				},
			},
		},		
		{
			seperator = true,
		},
		{
			string = L("Switch To 2D Mode"),
			on_command = function() set_setting("Force2D", true) end
		},
		{
			string = L("Switch To 3D Mode"),
			on_command = function() set_setting("Force2D", false) end
		},
		{
			string = L("Swap Eyes"),
			on_command = function() set_setting("SwapEyes", not setting.SwapEyes) end
		},
		{
			seperator = true,
		},
		{
			string = L("Settings..."),
		},
		{
			string = L("About..."),
			on_command = function() player.show_about() end
		},
		{
			string = L("Exit"),
			on_command = function() player.exit() end
		},
	}
	
	-- playlist
	local t = {}
	for i=1,playlist:count() do
		local item = playlist:item(i)
		local text = GetName(item.L) or " "
		if item.R then text = text .. " + " .. GetName(item.R) end
				
		table.insert(t, {string=text, checked = (playlist:current_pos() == i), id = i, on_command = function(v) playlist:play_item(v.id) end})
	end
	
	table.insert(t, {seperator = true})
	table.insert(t, {string = "清空", on_command = function(v) playlist:clear() end})	
	m[10].sub_items = t
	
	m = menu_builder:Create(m)
	m:popup()
	m:destroy()	
end