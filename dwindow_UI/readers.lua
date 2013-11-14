local dwindow_readers = {}

function core.create_reader(URL)
	for _,v in ipairs(dwindow_readers) do
		if v.probe and v.probe(URL) and v.create then
			return v.create(URL)
		end
	end
end

function core.probe_reader(URL)
	for _,v in ipairs(dwindow_readers) do
		local result = v.probe and v.probe(URL)
		if result then
			return result
		end
	end
end

function core.register_reader(probe_func, create_func)
	table.insert(dwindow_readers, {probe=probe_func, create=create_func})
end


-- for HTTP reader
local function probe_http(URL)
	return URL:lower():find("http://") == 1
end

-- function create_http_reader(URL) already created by C++ core
core.register_reader(probe_http, create_http_reader)