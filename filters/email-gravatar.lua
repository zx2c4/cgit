-- This script may be used with the email-filter or repo.email-filter settings in cgitrc.
-- It adds gravatar icons to author names. It is designed to be used with the lua:
-- prefix in filters. It is much faster than the corresponding python script.
--
-- Requirements:
-- 	luaossl
-- 	<http://25thandclement.com/~william/projects/luaossl.html>
--

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
	md5 = md5_hex(email:sub(2, -2):lower())
end

function filter_close()
	html("<img src='//www.gravatar.com/avatar/" .. md5 .. "?s=13&amp;d=retro' width='13' height='13' alt='Gravatar' /> " .. buffer)
	return 0
end

function filter_write(str)
	buffer = buffer .. str
end


