function filter_open(...)
	buffer = ""
	for i = 1, select("#", ...) do
		buffer = buffer .. select(i, ...) .. " "
	end
end

function filter_close()
	html(buffer)
	return 0
end

function filter_write(str)
	buffer = buffer .. string.upper(str)
end


