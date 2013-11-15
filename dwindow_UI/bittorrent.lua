-- Lua 5.1+ base64 v3.0 (c) 2009 by Alex Kloss <alexthkloss@web.de>
-- licensed under the terms of the LGPL2

bittorrent = {}

-- character table string
local b='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

-- encoding
function bittorrent.hex2string(data)
    return ((data:gsub('.', function(x) 
        local r,b='',x:byte()
        for i=8,1,-1 do r=r..(b%2^i-b%2^(i-1)>0 and '1' or '0') end
        return r;
    end)..'0000'):gsub('%d%d%d?%d?%d?%d?', function(x)
        if (#x < 6) then return '' end
        local c=0
        for i=1,6 do c=c+(x:sub(i,i)=='1' and 2^(6-i) or 0) end
        return b:sub(c+1,c+1)
    end)..({ '', '==', '=' })[#data%3+1])
end

-- decoding
function bittorrent.string2hex(data)
    data = string.gsub(data, '[^'..b..'=]', '')
    return (data:gsub('.', function(x)
        if (x == '=') then return '' end
        local r,f='',(b:find(x)-1)
        for i=6,1,-1 do r=r..(f%2^i-f%2^(i-1)>0 and '1' or '0') end
        return r;
    end):gsub('%d%d%d?%d?%d?%d?%d?%d?', function(x)
        if (#x ~= 8) then return '' end
        local c=0
        for i=1,8 do c=c+(x:sub(i,i)=='1' and 2^(8-i) or 0) end
        return string.char(c)
    end))
end

-- info_hash: string like "0B1F437BBB3B36F51624E8DB404CB770AEC9D1A7"
-- resume data : byte buff of bencoded resume data
-- return: nil
function bittorrent.save_torrent(info_hash, resume_data)
	setting.torrents = setting.torrents or {}
	setting.torrents[info_hash] = bittorrent.hex2string(resume_data)
end

-- info_hash: string like "0B1F437BBB3B36F51624E8DB404CB770AEC9D1A7"
-- return: byte buff of bencoded resume data if exists, nil if not exists or invalid
function bittorrent.load_torrent(info_hash)
	if type(setting.torrents) == "table" and setting.torrents[info_hash] then
		return bittorrent.string2hex(setting.torrents[info_hash])
	end
end

-- session_state: byte buff of bencoded session state
-- return: nil
function bittorrent.save_session(session_state)
	setting.bittorrent_session_state = bittorrent.hex2string(session_state)
end

-- return: byte buff of bencoded session state if exists, nil if not exists or invalid
function bittorrent.load_session()
	if type(setting.bittorrent_session_state) == "string" then
		return bittorrent.string2hex(setting.bittorrent_session_state)
	end	
end