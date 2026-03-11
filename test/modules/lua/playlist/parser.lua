-- Probe function
function probe()
	vlc.msg.dbg("Initiated probe call")
	vlc.msg.dbg("Access: " .. vlc.access .. " Path:" .. vlc.path)
	return (vlc.access == "file")
end

-- Parse function
function parse()
	vlc.msg.dbg("Initiated parse stream (file)")
	local line = vlc.readline()
	local item = {}
	while line do
		local key, value = string.match(line, '(%w+)%s*=%s*"(.-)"')
		if key and value then
			item[key] = value
		end
		line = vlc.readline()
	end
	return {
		item,
	}
end
