function popup3dvstar()
	local m = 
	{
		{
			string = "打开文件...",
			on_command = function(v)
				playlist:remove_item(v.id)
			end
		},
		{
			string = "打开文件夹...",
			on_command = function(v)
				playlist:play_item(v.id)
			end
		},
		{
			string = "关闭文件",
			grayed = not player.movie_loaded,
			on_command = function(v)
				playlist:clear()
			end
		},
		{
			string = "打开文件夹...",
			seperator = true,
		},
		{
			string = "播放\tSpace",
		},
		{
			string = "暂停\tSpace",
		},
		{
			string = "停止",
		},
		{
			string = "上一曲\tPageUp",
		},
		{
			string = "下一曲\tPageDown",
		},
		{
			string = "播放列表",
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
					string = "清空",
				},
			},
		},		
		{
			seperator = true,
		},
		{
			string = "切换到2D模式",
		},
		{
			string = "切换到3D模式",
		},
		{
			string = "交换左右眼",
		},
		{
			seperator = true,
		},
		{
			string = "高级设置...",
		},
		{
			string = "关于...",
		},
		{
			string = "退出...",
		},
	}
	
	m = menu_builder:Create(m)
	m:popup()
	m:destroy()	
end