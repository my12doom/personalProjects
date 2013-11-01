local parse_cache = {}

function parseURL(url)
	if parse_cache[url] then return parse_cache[url] end
	
	local out = url
	
	local youku_prefix = "http://v.youku.com/v_show/id_"
	
	if string.find(url, "pan.baidu.com") then
		local str, code = core.http_request(url);
		if code ~= 200 then
			return
		end
		
		for w in string.gmatch(str, "dlink%\\%\":%\\%\"([^,]+)%\\%\",") do
			out = string.gsub(w, "\\\\", "" ) .. "&my12doom=.rmvb"
		end
		
	elseif string.find(url, youku_prefix) then
		local video_id = string.sub(url, youku_prefix:len()+1)
		video_id = string.sub(video_id, 1, math.min(13, video_id:len()))
		local json = json_url2table("https://openapi.youku.com/v2/videos/files.json?client_id=e57bc82b1a9dcd2f&client_secret=a361608273b857415ee91a8285a16b4a&video_id=" .. video_id)
		print("JSON:", json)
		printtable(json)
		out = (json.files["hd2"] or json.files["mp4"] or json.files["3gphd"]).segs --[1].url
		print(out)
	end
	
	parse_cache[url] = out
	
	return out
end
