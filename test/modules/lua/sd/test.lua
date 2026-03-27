function descriptor()
	return { title = "My SD's title", capabilities = { "search" } }
end

function signal_test(event_name)
	vlc.msg.dbg("lua test event: " .. event_name)
	libvlc = vlc.object.libvlc()
	vlc.var.trigger_callback(libvlc, "test-lua-" .. event_name)
end

function main()
	signal_test("main")
end

function search(query)
	signal_test("search")
end
