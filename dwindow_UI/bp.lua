---------------------------
--  bp.lua
---------------------------

if bp ~= nil then return end
 
local type=type
local tostring=tostring
local print=print
local setmetatable=setmetatable
local getfenv=getfenv
local ipairs=ipairs
local pairs=pairs
local xpcall=xpcall
local error=error
 
local table_insert=table.insert
local table_concat=table.concat
local debug=debug
 
local nil_value={}
 
local function traversal_r(tbl,num)
	num = num or 64
	local ret={}
	local function insert(v)
		table_insert(ret,v)
		if #ret>num then
			error()
		end
	end
	local function traversal(e)
		if e==nil_value or e==nil then
			insert("nil,")
		elseif type(e)=="table" then
			insert("{")
			local maxn=0
			for i,v in ipairs(e) do 
				traversal(v)
				maxn=i
			end
			for k,v in pairs(e) do
				if not (type(k)=="number" and k>0 and k<=maxn) then
					if type(k)=="number" then
						insert("["..k.."]=")
					else
						insert(tostring(k).."=")
					end
					traversal(v)
				end
			end
			insert("}")
		elseif type(e)=="string" then
			insert('"'..e..'",')
		else
			insert(tostring(e))
			insert(",")
		end
	end
 
	local err=xpcall(
		function() traversal(tbl) end,
		function() end
	)
	if not err then
		table_insert(ret,"...")
	end
 
	return table_concat(ret)
end
 
function print_r(tbl,num)
	print(traversal_r(tbl,num))
end
 
local function init_local(tbl,level)
	local n=1
	local index={}
	while true do
		local name,value=debug.getlocal(level,n)
		if not name then
			break
		end
 
		if name~="(*temporary)" then
			if value==nil then
				value=nil_value
			end
			
			tbl[name]=value
			index["."..name]=n
		end
 
		n=n+1
	end
	setmetatable(tbl,{__index=index})
	return tbl
end
 
local function init_upvalue(tbl,func)
	local n=1
	local index={}
	while true do
		local name,value=debug.getupvalue(func,n)
		if not name then
			break
		end
 
		if value==nil then
			value=nil_value
		end
		
		tbl[name]=value
		index["."..name]=n
 
		n=n+1
	end
	setmetatable(tbl,{__index=index})
	return tbl
end
 
 
function dbg(level)
	level=level and level+2 or 2
 
	local lv=init_local({},level+1)
	local func=debug.getinfo(level,"f").func
	local uv=init_upvalue({},func)
	local _L={}
	setmetatable(_L,{
		__index=function(_,k) 
			local ret=lv[k]
			return ret~=nil_value and ret or nil
		end,
		__newindex=function(_,k,v)
			if lv[k] then
				lv[k]= v~= nil and nil_value or v
				debug.setlocal(level+3,lv["."..k],v)
			else
				print("error:invalid local name:",k)
			end
		end,
		__tostring=function(_)
			return traversal_r(lv)
		end
	})
	print("_L=",traversal_r(lv))
	local _U={}
	setmetatable(_U,{
		__index=function(_,k) 
			local ret=uv[k]
			return ret~=nil_value and ret or nil
		end,
		__newindex=function(_,k,v)
			if uv[k] then
				uv[k]= v~= nil and nil_value or v
				debug.setupvalue(func,uv["."..k],v)
			else
				print("error:invalid upvalue name",k)
			end
		end,
		__tostring=function(_)
			return traversal_r(uv)
		end
	})
	print("_U=",traversal_r(uv))
 
	local _G=getfenv(level)
	_G._L,_G._U=_L,_U
	debug.debug()
	_G._L,_G._U=nil,nil
end
 
 
local _bp_list={}
local _bp_desc={}
 
local function bp_list_name(name)
	local t=type(_bp_desc[name])
	print("["..name.."]",_bp_list[name] and "on" or "off")
	if t=="table" then
		for _,v in ipairs(_bp_desc[name]) do
			print("----",v)
		end
	elseif t=="string" then
		print("---",_bp_desc[name])
	end
end
 
local function bp_list(name)
	if name then
		bp_list_name(name)
	else
		for k,v in pairs(_bp_list) do
			bp_list_name(k)
		end
	end
end
 
local _bp_unnamed={}
local _bp_unnamed_desc={}
local _bp_unnamed_index={}
local _bp_index=1
do
	local weak={__mode="kv"}
	setmetatable(_bp_unnamed,weak)
	setmetatable(_bp_unnamed_desc,{__mode="k"})
	setmetatable(_bp_unnamed_index,weak)
end
 
local function bp_add_index()
	local info=debug.getinfo(3,"Slf")
	local func=info.func
	if _bp_unnamed[info.func]==nil then
		local desc=info.source.."("..info.currentline..")"
		_bp_unnamed[func]=true
		_bp_unnamed_desc[func]=desc
		_bp_unnamed_index[func]=_bp_index
		_bp_unnamed_index[_bp_index]=func
		_bp_index=_bp_index+1
	end
	return _bp_unnamed[func],_bp_unnamed_index[func]
end
 
local _bp_named={}
local _bp_named_desc={}
 
local function bp_add_name(name)
	local info=debug.getinfo(3,"Sl")
	local desc=info.source.."("..info.currentline..")"
	if _bp_named[name]==nil then
		_bp_named[name]=false
	end
 
	local tbl=_bp_named_desc[name]
	if tbl then
		tbl[desc] = true
	else 
		_bp_named_desc[name]={ [desc] = true }
	end
 
	return _bp_named[name],name
end
 
local function bp_show(n)
	if type(n)=="number" then
		local func=_bp_unnamed_index[n]
		if func==nil then
			error "invalid break point id"
		end
		print("break point:",n,
			_bp_unnamed[func] and "on" or "off",
			_bp_unnamed_desc[func]
			)
	else
		print("break point:",n,_bp_named[n] and "on" or "off")
		if _bp_named_desc[n] then
			for k,v in pairs(_bp_named_desc[n]) do
				print("",k)
			end
		else
			print("\tundefined")
		end
	end
end
 
local function bp_show_all()
	for k,_ in pairs(_bp_unnamed) do
		bp_show(_bp_unnamed_index[k])
	end
	for k,_ in pairs(_bp_named) do
		bp_show(k)
	end
end
 
function bp(n,m)
	local trigger,name
	if n==nil then
		trigger,name=bp_add_index()
	else 
		trigger,name=bp_add_name(n)
	end
 
	if trigger then
		bp_show(name)
		if debug then
			print("trackback", debug.traceback())
		end
		if dwindow and dwindow.track_back then
			dwindow.track_back()
		end
		if m then dbg(1) end
	else
		_bp_list[name]=false
	end
end
 
function trigger(n,on)
	on = on~=false
	if type(n)=="number" then
		local func=_bp_unnamed_index[n]
		if func==nil then
			error "invalid break point id"
		end
		_bp_unnamed[func]=on
	else 
		_bp_named[n]=on
	end
end
 
function list(v)
	if v then 
		bp_show(v)
	else
		bp_show_all()
	end
end
 
function error_handle(msg)
	print(msg)
	dbg(1)
end


print("BP loaded")