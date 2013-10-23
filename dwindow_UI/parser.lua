function parseURL(url)
	local youku_prefix = "http://v.youku.com/v_show/id_"
	
	if string.find(url, "pan.baidu.com") then
		local str, code = core.http_request(url);
		if code ~= 200 then
			return
		end
		
		for w in string.gmatch(str, "dlink%\\%\":%\\%\"([^,]+)%\\%\",") do
			url = string.gsub(w, "\\\\", "" ) .. "&my12doom=.rmvb"
		end
		
	elseif string.find(url, youku_prefix) then
		local video_id = string.sub(url, youku_prefix:len()+1)
		video_id = string.sub(video_id, 1, math.min(13, video_id:len()))
		local json = json_url2table("https://openapi.youku.com/v2/videos/files.json?client_id=e57bc82b1a9dcd2f&client_secret=a361608273b857415ee91a8285a16b4a&video_id=" .. video_id)
		print("JSON:", json)
		printtable(json)
		url = (json.files["hd2"] or json.files["mp4"] or json.files["3gphd"]).segs[1].url
		print(url)
	end
	
	return url
end

local reset_and_loadfile_core = player.reset_and_loadfile

function player.reset_and_loadfile(file1, file2, ...)
	if file1 then
		file1 = parseURL(file1)
	end
	
	if file2 then
		file2 = parseURL(file2)
	end
	
	return reset_and_loadfile_core(file1, file2, ...)
end