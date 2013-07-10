function popup_dwindow2()
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
			on_command = function() player.reset() end
		},
		{
			seperator = true,
		},
		{
			string = L("Video"),
			sub_items =
			{
				{
					string = L("Adjust Color..."),
					on_command = function() player.show_color_adjust_dialog() end
				},
				{
					seperator = true,
				},
				{
					string = L("Use CUDA Accelaration"),
				},
				{
					string = L("Use DXVA2 Accelaration (Experimental)"),
				},
				{
					seperator = true,
				},
				{
					string = L("Aspect Ratio"),
					grayed = true
				},
				{
					string = L("Source Aspect"),
				},
				{
					string = L("16:9"),
				},
				{
					string = L("4:3"),
				},
				{
					string = L("2.35:1"),
				},
				{
					seperator = true,
				},
				{
					string = L("Fill Mode"),
					grayed = true
				},
				{
					string = L("Default(Letterbox)"),
				},
				{
					string = L("Horizontal Fill"),
				},
				{
					string = L("Vertical Fill"),
				},
				{
					string = L("Stretch"),
				},
				{
					seperator = true,
				},
				{
					string = L("Quality"),
					grayed = true
				},
				{
					string = L("Best Quality (Low Speed)"),
				},
				{
					string = L("Better Quality"),
				},
				{
					string = L("Normal Quality (Fast)"),
				},
				{
					seperator = true,
				},
				{
					string = L("Move Up\t(W)"),
				},
				{
					string = L("Move Down\t(S)"),
				},
				{
					string = L("Move Left\t(A)"),
				},
				{
					string = L("Move Right\t(D)"),
				},
				{
					string = L("Zoom In\t(NumPad 7)"),
				},
				{
					string = L("Zoom Out\t(NumPad 1)"),
				},
				{
					string = L("Increase Parallax\t(NumPad *)"),
				},
				{
					string = L("Decrease Parallax\t(NumPad /)"),
				},
				{
					string = L("Reset\t(NumPad 5)"),
				},
				
			}
		},
		{
			string = L("Audio"),
			sub_items =
			{
				{
					string = L("Load Audio Track..."),
					on_command = function()
						local file = ui.OpenFile(L("Audio Files").."(*.mp3;*.mxf;*.dts;*.ac3;*.aac;*.m4a;*.mka)", "*.mp3;*.mxf;*.dts;*.ac3;*.aac;*.m4a;*.mka", L("All Files"), "*.*")
						player.load_audio_track(file)
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Use ffdshow Audio Decoder"),
				},
				{
					string = L("Normalize Audio"),
				},
				{
					string = L("Latency && Stretch..."),
					on_command = function()
						local latency = player.show_latency_ratio_dialog(true, setting.AudioLatency, 1)
						set_setting("AudioLatency", latency)
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Channel Setting"),
					grayed = true,
				},
				{
					string = L("Source"),
				},
				{
					string = L("Stereo"),
				},
				{
					string = L("5.1 Channel"),
				},
				{
					string = L("7.1 Channel"),
				},
				{
					string = L("Dolby Headphone"),
				},
				{
					string = L("Bitstreaming"),
				},
			}
		},
		{
			string = L("Subtitle"),
			sub_items =
			{
				{
					string = L("Load Subtitle File..."),
					on_command = function()
						local file = ui.OpenFile(L("Subtitle Files").."(*.srt;*.sup;*.ssa;*.ass;*.sub;*.idx)", "*.srt;*.sup;*.ssa;*.ass", L("All Files"), "*.*")
						player.load_subtitle(file)
					end
				},
				{
					seperator = true,
				},
				{
					string = L("Latency && Stretch..."),
					on_command = function()
						local latency, ratio = player.show_latency_ratio_dialog(false, setting.SubtitleLatency, setting.SubtitleRatio)
						set_setting("SubtitleLatency", latency)
						set_setting("SubtitleRatio", ratio)
					end
				},
				{
					string = L("Display Subtitle"),
				},
				{
					seperator = true,
				},
				{
					string = L("Increase Parallax\t(NumPad 9)"),
				},
				{
					string = L("Decrease Parallax\t(NumPad 3)"),
				},
				{
					string = L("Move Up\t(NumPad 8)"),
				},
				{
					string = L("Move Down\t(NumPad 2)"),
				},
				{
					string = L("Move Left\t(NumPad 4)"),
				},
				{
					string = L("Move Right\t(NumPad 6)"),
				},
				{
					string = L("Reset\t(NumPad 5)"),
				},
			},
		},
		{
			string = L("Movie Layout"),
			sub_items = 
			{
				{
					string = L("Auto"),
					checked = setting.InputLayout == 0x10000,
					on_command = function() player.set_input_layout(0x10000) end
				},
				{
					string = L("Side By Side"),
					checked = setting.InputLayout == 0,
					on_command = function() player.set_input_layout(0) end
				},
				{
					string = L("Top Bottom"),
					checked = setting.InputLayout == 1,
					on_command = function() player.set_input_layout(1) end
				},
				{
					string = L("Monoscopic"),
					checked = setting.InputLayout == 2,
					on_command = function() player.set_input_layout(2) end
				},
			}
		},
		{
			string = L("Viewing Device"),
			sub_items = 
			{
				{
					string = L("Nvidia 3D Vision"),
					checked = setting.OutputMode == 0,
					on_command = function() player.set_output_mode(0) end,
				},
				{
					string = L("AMD HD3D"),
					checked = setting.OutputMode == 11,
					on_command = function() player.set_output_mode(11) end,
				},
				{
					string = L("Intel Stereoscopic"),
					checked = setting.OutputMode == 12,
					on_command = function() player.set_output_mode(12) end,
				},
				{
					string = L("IZ3D Displayer"),
					checked = setting.OutputMode == 5,
					on_command = function() player.set_output_mode(5) end,
				},
				{
					string = L("Gerneral 120Hz Glasses"),
					checked = setting.OutputMode == 4,
					on_command = function() player.set_output_mode(4) end,
				},
				{
					string = L("Anaglyph"),
					checked = setting.OutputMode == 2,
					on_command = function() player.set_output_mode(2) end,
				},
				{
					string = L("Monoscopic 2D"),
					checked = setting.OutputMode == 3,
					on_command = function() player.set_output_mode(3) end,
				},
				{
					string = L("Subpixel Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 3,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(3) end,
				},
				{
					seperator = true,
				},
				{
					string = L("3DTV"),
					grayed = true,
				},
				{
					string = L("3DTV - Half Side By Side"),
					checked = setting.OutputMode == 9,
					on_command = function() player.set_output_mode(9) end,
				},
				{
					string = L("3DTV - Half Top Bottom"),
					checked = setting.OutputMode == 10,
					on_command = function() player.set_output_mode(10) end,
				},
				{
					string = L("3DTV - Line Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 1,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(1) end,
				},
				{
					string = L("3DTV - Checkboard Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 2,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(2) end,
				},
				{
					string = L("3DTV - Row Interlace"),
					checked = setting.OutputMode == 1 and setting.MaskMode == 0,
					on_command = function() player.set_output_mode(1) player.set_mask_mode(0) end,
				},
				{
					seperator = true,
				},
				{
					string = L("Dual Projector"),
					grayed = true,
				},
				{
					string = L("Dual Projector - Vertical Span Mode"),
					checked = setting.OutputMode == 8,
					on_command = function() player.set_output_mode(8) end,
				},
				{
					string = L("Dual Projector - Horizontal Span Mode"),
					checked = setting.OutputMode == 7,
					on_command = function() player.set_output_mode(7) end,
				},
				{
					string = L("Dual Projector - Independent Mode"),
					checked = setting.OutputMode == 6,
					on_command = function() player.set_output_mode(6) end,
				},
				{
					seperator = true,
				},
				{
					string = L("Fullscreen Output 1"),
				},
				{
					string = L("Fullscreen Output 2"),
				},
				{
					string = L("HD3D Fullscreen Mode Setting..."),
					on_command = function() player.show_hd3d_fullscreen_mode_dialog() end
				},
			},
		},
		{
			seperator = true,
		},
		{
			string = player.is_playing() and L("Pause\t(Space)") or L("Play\t(Space)"),
			grayed = not player.movie_loaded,
			on_command = function() player.pause() end
		},
		{
			string = L("Fullscreen\t(Alt+Enter)"),
			on_command = function() player.toggle_fullscreen() end
		},
		{
			string = L("Swap Left/Right\t(Tab)"),
			on_command = function() player.set_swapeyes(not player.get_swapeyes()) end
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
	
	-- BluRay 3D
	local bd3d = {}
	m[3].sub_items = bd3d
	local bds = table.pack(player.enum_bd())
	for i=1,#bds/3 do
		local path = bds[i*3-2]
		local volume_name = bds[i*3-1]
		local is_bluray = bds[i*3-0]
		local desc = path ..  (is_bluray and (" (" ..volume_name .. ")") or (volume_name and L(" (No Movie Disc)") or L(" (No Disc)")))
		table.insert(bd3d, {grayed = not(path and volume_name and is_bluray), string= desc, path=path, on_command = function(t) playlist:play(t.path) end})
	end
	table.insert(bd3d, {seperator = true})
	table.insert(bd3d, {string = L("Folder..."), on_command = function()
				local folder = ui.OpenFolder()
				if folder then
					playlist:play(folder)
				end
			end})

	-- language
	m[17].sub_items = {}
	for _,v in pairs(core.get_language_list()) do
		table.insert(m[17].sub_items, {checked = v == setting.LCID, string=v, on_command = function(t) core.set_language(t.string) end})
		print(v)
	end
	
	-- monitors
	local f1 = {}
	local f2 = {}
	local monitor = table.pack(player.enum_monitors())
	for i=1,#monitor/5 do
		local check1 = monitor[i*5-4] == setting.Screen1.left and monitor[i*5-3] == setting.Screen1.top and monitor[i*5-2] == setting.Screen1.right and monitor[i*5-1] == setting.Screen1.bottom
		local check2 = monitor[i*5-4] == setting.Screen2.left and monitor[i*5-3] == setting.Screen2.top and monitor[i*5-2] == setting.Screen2.right and monitor[i*5-1] == setting.Screen2.bottom
		table.insert(f1, {checked = check1, string = monitor[i*5], id = i - 1, on_command = function(t) player.set_monitor(0, t.id) end})
		table.insert(f2, {checked = check2, string = monitor[i*5], id = i - 1, on_command = function(t) player.set_monitor(1, t.id) end})
	end
	
	m[10].sub_items[22].sub_items = f1
	m[10].sub_items[23].sub_items = f2
	
	-- subtitle
	local sub = m[8].sub_items
	local subs = table.pack(player.list_subtitle_track())
	if #subs == 0 then
		table.insert(sub, 1, {grayed = true, string = L("No Subtitle")})
	else
		for i=1, #subs/2 do
			table.insert(sub, 1, {string = subs[i*2-1], checked = subs[i*2], id = i-1, on_command = function(t) player.enable_subtitle_track(t.id) end})
		end
	end
	
	
	-- audio
	local aud = m[7].sub_items
	local auds = table.pack(player.list_audio_track())
	if #auds == 0 then
		table.insert(aud, 1, {grayed = true, string = L("No Audio Track")})
	else
		for i=1, #auds/2 do
			table.insert(aud, 1, {string = auds[i*2-1], checked = auds[i*2], id = i-1, on_command = function(t) player.enable_audio_track(t.id) end})
		end
	end
	
	
	m = menu_builder:Create(m)
	m:popup()
	m:destroy()
	
end