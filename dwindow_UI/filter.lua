dshow = {}

--[[ media_type table sample:
{
	major = "{73646976-0000-0010-8000-00AA00389B71}",	-- MEDIATYPE_Video
	sub = "{31637661-0000-0010-8000-00AA00389B71}",		-- FourCC Map 'avc1'
	format = "{F72A76A0-EB0A-11d0-ACE4-0000C0CC16BA}"	-- FORMAT_VideoInfo2
	format_data = "..."									-- binary data of pFormat, normally dshow builder doesn't need this infomation, it is designed for decoder connection
}
]]--

-- pre defined media types guids
local MEDIATYPE_Video = "{73646976-0000-0010-8000-00AA00389B71}"
local MEDIATYPE_Audio = "{73647561-0000-0010-8000-00AA00389B71}"
local MEDIATYPE_Subtitle = "{E487EB08-6B26-4be9-9DD3-993434D313FD}"

-- return: guid
local function fourCC(fourcc)

end

local function get_ext(pathname)
	if pathname == nil then return end
	local t = string.reverse(pathname)
	t = string.sub(t, 1, ((string.find(t, ".")  or t:len())-1))
	return string.reverse(t)
end


-- return the guid of desired splitter, nil if not found, which leads to system-registered filters
function dshow:decide_splitter(filename)
	local ext = get_ext(filename)
	local media_info
	local media_info_result
	local ext_result
	
	-- get media info if not slow extentions like m2ts	
	if setting.dshow.slow_extentions[ext] == nil then
		media_info = player.get_media_info(filename)
	end
	
	-- find splitter by media info
	if media_info and media_info.General and media_info.General.Format then
		media_info_result = setting.dshow.media_info[media_info.General.Format]
	end
	
	-- find splitter by extenstion
	ext_result = setting.dshow.extensions[ext]
	
	-- add your own probing codes here
	
	return media_info_result or ext_result
end

-- inp
-- return guid, catagory
-- guiid : the guid of desired decoder, nil if not determined, which will add all available decoder of that catagory
-- catagory: media catagory, 0: video, 1: audio, 2: subtitle
function dshow:render_pin(media_types, pin_name, filter_guid)
	local guid, catagory
	local guid
	for _,v in ipairs(media_types) do
		if v.major == MEDIATYPE_Video then
			catagory = 0
		elseif v.major == MEDIATYPE_Audio then
			catagory = 1
		elseif v.major == MEDIATYPE_Subtitle then
			catagory = 2
		end
	end
end
