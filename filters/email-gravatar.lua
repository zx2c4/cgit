-- This script may be used with the email-filter or repo.email-filter settings in cgitrc.
-- It adds gravatar icons to author names. It is designed to be used with the lua:
-- prefix in filters. It is much faster than the corresponding python script.
--
-- Requirements:
-- 	luacrypto >= 0.3
-- 	<http://mkottman.github.io/luacrypto/>
--

local crypto = require("crypto")

function filter_open(email, page)
	buffer = ""
	md5 = crypto.digest("md5", email:sub(2, -2):lower())
end

function filter_close()
	html("<img src='//www.gravatar.com/avatar/" .. md5 .. "?s=13&amp;d=retro' width='13' height='13' alt='Gravatar' /> " .. buffer)
	return 0
end

function filter_write(str)
	buffer = buffer .. str
end


