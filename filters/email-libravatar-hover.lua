local digest = require("openssl.digest")

function md5_hex(input)
	local b = digest.new("md5"):final(input)
	local x = ""
	for i = 1, #b do
		x = x .. string.format("%.2x", string.byte(b, i))
	end
	return x
end

function filter_open(email, page)
	buffer = ""
	hexdigest = md5_hex(email:sub(2, -2):lower())
end

function filter_close()
	baseurl = os.getenv("HTTPS") and "https://seccdn.libravatar.org/" or "http://cdn.libravatar.org/"
	html("<span class='libravatar'><img class='inline' src='" .. baseurl .. "avatar/" .. hexdigest .. "?s=13&amp;d=retro' /><img class='onhover' src='" .. baseurl .. "avatar/" .. hexdigest .. "?s=128&amp;d=retro' /></span>" .. buffer)
	return 0
end

function filter_write(str)
	buffer = buffer .. str
end
