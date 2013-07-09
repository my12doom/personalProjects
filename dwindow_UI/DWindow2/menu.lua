function popup_dwindow2()
	local m = 
	{
		{
			string = L("Open File..."),
			on_command = function(v)
				local url = ui.OpenURL()
				if url then
					playlist:play(url)
				end
			end
		},
		{
			string = L("Open Left And Right File..."),
			on_command = function(v)
				local left, right = ui.OpenDoubleFile()
				if left and right then
					playlist:play(left, right)
				end
			end
		},
		{
			string = L("Open BluRay3D"),
			on_command = function(v)
				playlist:clear()
			end
		},
		{
			string = L("Close"),
			grayed = not player.movie_loaded,
		},
		{
			seperator = true,
		},
		{
			string = L("Video"),
		},
		{
			string = L("Audio"),
		},
		{
			string = L("Subtitle"),
		},
		{
			string = L("Movie Layout"),
		},
		{
			string = L("Viewing Device"),
		},
		{
			seperator = true,
		},
		{
			string = L("Play\t(Space)"),
		},
		{
			string = L("Fullscreen\t(Alt+Enter)"),
		},
		{
			string = L("Swap Left/Right\t(Tab)"),
		},
		{
			seperator = true,
		},
		
		
		{
			string = L("Media Infomation..."),
		},
		{
			string = L("Language"),
		},
		{
			string = L("About..."),
		},
		{
			string = L("Logout..."),
		},
		{
			string = L("Exit"),
		},
		
	}
	
	m = menu_builder:Create(m)
	m:popup()
	m:destroy()	
end