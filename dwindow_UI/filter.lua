local config
dshow = {}

--[[ media_type table sample:
{
	major = "{73646976-0000-0010-8000-00AA00389B71}",	-- MEDIATYPE_Video
	sub = "{31637661-0000-0010-8000-00AA00389B71}",		-- FourCC Map 'avc1'
	format = "{F72A76A0-EB0A-11d0-ACE4-0000C0CC16BA}"	-- FORMAT_VideoInfo2
	format_data = "..."									-- binary data of pFormat, normally dshow builder doesn't need this infomation, it is designed for pin connection
}
]]--

-- pre defined media types guids
local MEDIATYPE_Video = "{73646976-0000-0010-8000-00AA00389B71}"
local MEDIATYPE_Audio = "{73647561-0000-0010-8000-00AA00389B71}"
local MEDIATYPE_Subtitle = "{E487EB08-6B26-4be9-9DD3-993434D313FD}"
local GUID_NULL = "{00000000-0000-0000-0000-000000000000}"

-- return: guid
local function fourCC(fourcc)
	if type(fourcc) ~= "string" or fourcc:len() ~= 4 then return GUID_NULL end
	return string.format("{%02X%02X%02X%02X-0000-0010-8000-00AA00389B71}", fourcc:reverse():byte(1,4))
end

local function get_ext(pathname)
	if pathname == nil then return end
	local t = string.reverse(pathname)
	t = string.sub(t, 1, ((string.find(t, "%.")  or t:len())-1))
	return string.reverse(t)
end


-- return the guid of desired splitter, nil if not found, which leads to system-registered filters
function dshow.decide_splitter(filename)
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
	
	-- add special case codes here
	
	return media_info_result or ext_result
end

-- inp
-- return guid, catagory
-- guiid : the guid of desired decoder, table if multiple, nil if not determined which will add all available decoder of that catagory
-- catagory: media catagory, 0: video, 1: audio, 2: subtitle
function dshow.render_pin(media_types, pin_name, filter_guid)
	if #media_types < 1 then return end
	local guid, catagory
	local guid
	local major = media_types[1].major			-- at least 1 media type and assume all media type has same major type
	local tbl = {}
	if major == MEDIATYPE_Video then
		catagory = 0
		
		for k,v in pairs(config.video4cc) do
			tbl[k] = v
		end
		
		if setting.EVR then
			for k,v in pairs(config.video4cc) do
				tbl[k] = v
			end
		end		
		
	elseif major == MEDIATYPE_Audio then
	
		catagory = 1
		
		for k,v in pairs(config.audio4cc) do
			tbl[k] = v
		end
		
	elseif major == MEDIATYPE_Subtitle then
		catagory = 2
	end
	
	for _, mediatype in ipairs(media_types) do
		for k, v in pairs(tbl) do
			if k == filter_guid or fourCC(k) == mediatype.sub or k == mediatype.sub then
				guid = guid or v
			end
		end
	end
	
	-- more codes here
	
	return guid, catagory
end


config =
{
	modules = 
	{
		-- ["CLSID"] = "relative path"
		["{D50F5B87-538B-4ee9-A18B-33783DA63CF1}"]="codec\\mySplitter.ax",
		["{25EAAFCB-DD72-44ce-A4A0-F73E7B065287}"]="codec\\mySplitter.ax",
		["{44BDBC26-FA2C-497b-965C-9BCBEC7FCC56}"]="codec\\mySplitter.ax",
		["{8FD7B1DE-3B84-4817-A96F-4C94728B1AAE}"]="codec\\my12doomSource.ax",
		["{7DE55957-D7D6-4386-9078-0FD2BA96F459}"]="codec\\E3DSource.ax",
		["{09571A4B-F1FE-4C60-9760-DE6D310C7C31}"]="codec\\CoreAVCDecoder.ax",
		["{D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}"]="codec\\CLCvd.ax",
		["{F07E981B-0EC4-4665-A671-C24955D11A38}"]="codec\\CLDemuxer2.ax",
		["{3CCC052E-BDEE-408A-BEA7-90914EF2964B}"]="codec\\MP4Splitter.ax",
		["{F7380D4C-DE45-4F03-9209-15EBA8552463}"]="codec\\AC3File.ax",
		["{765035B3-5944-4A94-806B-20EE3415F26F}"]="codec\\RealMediaSplitter.ax",
		["{941A4793-A705-4312-8DFC-C11CA05F397E}"]="codec\\RealMediaSplitter.ax",
		["{238D0F23-5DC9-45A6-9BE2-666160C324DD}"]="codec\\RealMediaSplitter.ax",
		["{0F40E1E5-4F79-4988-B1A9-CC98794E6B55}"]="codec\\ffdshow.ax",
		["{04FE9017-F873-410e-871E-AB91661A4EF7}"]="codec\\ffdshow.ax",
		["{0B0EFF97-C750-462C-9488-B10E7D87F1A6}"]="codec\\ffdshow.ax",
		["{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}"]="codec\\AVSplitter.ax",
		["{C9ECE7B3-1D8E-41F5-9F24-B255DF16C087}"]="codec\\FLVSplitter.ax",
		["{E1A90E70-EF1D-4326-9D6F-BF1E1AC0A792}"]="codec\\HEVCFLVSPlitter.ax",
		["{658C5E1C-58E1-43CA-9D10-13735D465576}"]="codec\\HEVCDecFltr.dll",
		["{9622477D-E65F-4A5F-BD89-AD75063DDADD}"]="codec\\MXFReader.dll",
		["{3BA3A89F-4A48-4E18-A9A6-0155BCB1A1A2}"]="codec\\mc_dec_j2k_ds.ax",
		["{7DC2B7AA-BCFD-44D2-BD58-E8BD0D2E3ACC}"]="IntelWiDiExtensions.dll",
	},

	extensions = 
	{
		-- ["ext"] = "Splitter CLSID"
		["avi"]="{1B544C20-FD0B-11CE-8C63-00AA0044B51E}",
		["e3d"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["3dv"]="{3CCC052E-BDEE-408A-BEA7-90914EF2964B}",
		["mp4"]="{3CCC052E-BDEE-408A-BEA7-90914EF2964B}",
		["ts"]="{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}",
		["m2ts"]="{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}",
		["ssif"]="{8FD7B1DE-3B84-4817-A96F-4C94728B1AAE}",
		["mpls"]="{8FD7B1DE-3B84-4817-A96F-4C94728B1AAE}",
		["mkv"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["mkv2d"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["mkv3d"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["mk3d"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["mts"]="{8FD7B1DE-3B84-4817-A96F-4C94728B1AAE}",
		["mka"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["aac"]="{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}",
		["ac3"]="{F7380D4C-DE45-4F03-9209-15EBA8552463}",
		["dts"]="{F7380D4C-DE45-4F03-9209-15EBA8552463}",
		["rmvb"]="{765035B3-5944-4A94-806B-20EE3415F26F}",
		["mks"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["png"]="{472EF052-2D21-4742-977C-C02097E5C08E}",
		["jpg"]="{472EF052-2D21-4742-977C-C02097E5C08E}",
		["mpo"]="{472EF052-2D21-4742-977C-C02097E5C08E}",
		["3dp"]="{472EF052-2D21-4742-977C-C02097E5C08E}",
		["bmp"]="{472EF052-2D21-4742-977C-C02097E5C08E}",
		["psd"]="{472EF052-2D21-4742-977C-C02097E5C08E}",
		--"flv"]="{C9ECE7B3-1D8E-41F5-9F24-B255DF16C087}",
		["flv"]="{E1A90E70-EF1D-4326-9D6F-BF1E1AC0A792}",
		["mxf"]="{9622477D-E65F-4A5F-BD89-AD75063DDADD}",
	},

	slow_extentions =
	{
		-- ["slow extension"] = 1
		["ssif"]=1,
		["m2ts"]=1,
		["ts"]=1,
		["tp"]=1,
		["mpls"]=1,
		["mts"]=1,
	},

	media_info =
	{
		-- ["MediaInfo.General.Format"] = "Splitter CLSID"
		["MPEG-4"]="{3CCC052E-BDEE-408A-BEA7-90914EF2964B}",
		["Matroska"]="{7DE55957-D7D6-4386-9078-0FD2BA96F459}",
		["RealMedia"]="{765035B3-5944-4A94-806B-20EE3415F26F}",
		["MPEG-TS"]="{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}",
		["Audio Video Interleave"]="{1B544C20-FD0B-11CE-8C63-00AA0044B51E}",
		["3dv"]="{3CCC052E-BDEE-408A-BEA7-90914EF2964B}",
		["BDAV"]="{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}",
		["AVI"]="{1B544C20-FD0B-11CE-8C63-00AA0044B51E}",
		["Flash Video"]="{E1A90E70-EF1D-4326-9D6F-BF1E1AC0A792}",
		--["Wave"]="wav",
	},

	video4cc = 
	{
		-- H264, CoreAVC
		-- ["FourCC"] = "Decoder Filter CLSID"
		-- ["Splitter CLSID"] = "Decoder Filter CLSID"
		["ccv1"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		["CCV1"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		["avc1"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		["AVC1"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		["h264"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		["H264"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		["x264"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		["X264"]="{09571A4B-F1FE-4C60-9760-DE6D310C7C31}",
		
		-- MPEG-4
		["xvid"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["XVID"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["divx"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["DIVX"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["dx50"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["DX50"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["DIV3"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["DIV4"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["DIV5"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["mp4v"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		["MP4V"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",
		
		-- JPEG 2000
		["MJ2C"]="{3BA3A89F-4A48-4E18-A9A6-0155BCB1A1A2}",
		
		-- Lentoid HEVC Decoder
		["HM91"]="{658C5E1C-58E1-43CA-9D10-13735D465576}",
		["HM10"]="{658C5E1C-58E1-43CA-9D10-13735D465576}",
		
		-- OpenHEVC Decoder
		["HEVC"]="{44BDBC26-FA2C-497b-965C-9BCBEC7FCC56}",
		["HM11"]="{44BDBC26-FA2C-497b-965C-9BCBEC7FCC56}",
		["HM12"]="{44BDBC26-FA2C-497b-965C-9BCBEC7FCC56}",
		
		-- RM Video
		["RV20"]="{238D0F23-5DC9-45A6-9BE2-666160C324DD}",
		["RV30"]="{238D0F23-5DC9-45A6-9BE2-666160C324DD}",
		["RV40"]="{238D0F23-5DC9-45A6-9BE2-666160C324DD}",
		["RV41"]="{238D0F23-5DC9-45A6-9BE2-666160C324DD}",
		["{765035B3-5944-4A94-806B-20EE3415F26F}"]="{238D0F23-5DC9-45A6-9BE2-666160C324DD}",			-- the filter CLSID

		-- others
		["{E06D8026-DB46-11CF-B4D1-00805F6CBBEA}"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",			-- MEDIASUBTYPE_MPEG2_VIDEO
		["{E436EB81-524F-11CE-9F53-0020AF0BA770}"]="{04FE9017-F873-410E-871E-AB91661A4EF7}",			-- MEDIASUBTYPE_MPEG1Payload
	},
	
	video4cc_dxva = 
	{
		["ccv1"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
		["CCV1"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
		["avc1"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
		["AVC1"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
		["h264"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
		["H264"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
		["x264"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
		["X264"]="{0B0EFF97-C750-462C-9488-B10E7D87F1A6}",
	},
	audio4cc =
	{
		["{765035B3-5944-4A94-806B-20EE3415F26F}"]="{941A4793-A705-4312-8DFC-C11CA05F397E}",			-- the filter CLSID		
	},
}