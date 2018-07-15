-- This script may be used with the auth-filter. Be sure to configure it as you wish.
--
-- Requirements:
-- 	luacrypto >= 0.3
-- 	<http://mkottman.github.io/luacrypto/>
-- 	luaposix
-- 	<https://github.com/luaposix/luaposix>
--
local sysstat = require("posix.sys.stat")
local unistd = require("posix.unistd")
local crypto = require("crypto")


--
--
-- Configure these variables for your settings.
--
--

-- A list of password protected repositories along with the users who can access them.
local protected_repos = {
	glouglou	= { laurent = true, jason = true },
	qt		= { jason = true, bob = true }
}

-- A list of users and hashes, generated with `mkpasswd -m sha-512 -R 300000`.
local users = {
	jason		= "$6$rounds=300000$YYJct3n/o.ruYK$HhpSeuCuW1fJkpvMZOZzVizeLsBKcGA/aF2UPuV5v60JyH2MVSG6P511UMTj2F3H75.IT2HIlnvXzNb60FcZH1",
	laurent		= "$6$rounds=300000$dP0KNHwYb3JKigT$pN/LG7rWxQ4HniFtx5wKyJXBJUKP7R01zTNZ0qSK/aivw8ywGAOdfYiIQFqFhZFtVGvr11/7an.nesvm8iJUi.",
	bob		= "$6$rounds=300000$jCLCCt6LUpTz$PI1vvd1yaVYcCzqH8QAJFcJ60b6W/6sjcOsU7mAkNo7IE8FRGW1vkjF8I/T5jt/auv5ODLb1L4S2s.CAyZyUC"
}

-- Set this to a path this script can write to for storing a persistent
-- cookie secret, which should be guarded.
local secret_filename = "/var/cache/cgit/auth-secret"

--
--
-- Authentication functions follow below. Swap these out if you want different authentication semantics.
--
--

-- Sets HTTP cookie headers based on post and sets up redirection.
function authenticate_post()
	local hash = users[post["username"]]
	local redirect = validate_value("redirect", post["redirect"])

	if redirect == nil then
		not_found()
		return 0
	end

	redirect_to(redirect)

	if hash == nil or hash ~= unistd.crypt(post["password"], hash) then
		set_cookie("cgitauth", "")
	else
		-- One week expiration time
		local username = secure_value("username", post["username"], os.time() + 604800)
		set_cookie("cgitauth", username)
	end

	html("\n")
	return 0
end


-- Returns 1 if the cookie is valid and 0 if it is not.
function authenticate_cookie()
	accepted_users = protected_repos[cgit["repo"]]
	if accepted_users == nil then
		-- We return as valid if the repo is not protected.
		return 1
	end

	local username = validate_value("username", get_cookie(http["cookie"], "cgitauth"))
	if username == nil or not accepted_users[username:lower()] then
		return 0
	else
		return 1
	end
end

-- Prints the html for the login form.
function body()
	html("<h2>Authentication Required</h2>")
	html("<form method='post' action='")
	html_attr(cgit["login"])
	html("'>")
	html("<input type='hidden' name='redirect' value='")
	html_attr(secure_value("redirect", cgit["url"], 0))
	html("' />")
	html("<table>")
	html("<tr><td><label for='username'>Username:</label></td><td><input id='username' name='username' autofocus /></td></tr>")
	html("<tr><td><label for='password'>Password:</label></td><td><input id='password' name='password' type='password' /></td></tr>")
	html("<tr><td colspan='2'><input value='Login' type='submit' /></td></tr>")
	html("</table></form>")

	return 0
end



--
--
-- Wrapper around filter API, exposing the http table, the cgit table, and the post table to the above functions.
--
--

local actions = {}
actions["authenticate-post"] = authenticate_post
actions["authenticate-cookie"] = authenticate_cookie
actions["body"] = body

function filter_open(...)
	action = actions[select(1, ...)]

	http = {}
	http["cookie"] = select(2, ...)
	http["method"] = select(3, ...)
	http["query"] = select(4, ...)
	http["referer"] = select(5, ...)
	http["path"] = select(6, ...)
	http["host"] = select(7, ...)
	http["https"] = select(8, ...)

	cgit = {}
	cgit["repo"] = select(9, ...)
	cgit["page"] = select(10, ...)
	cgit["url"] = select(11, ...)
	cgit["login"] = select(12, ...)

end

function filter_close()
	return action()
end

function filter_write(str)
	post = parse_qs(str)
end


--
--
-- Utility functions based on keplerproject/wsapi.
--
--

function url_decode(str)
	if not str then
		return ""
	end
	str = string.gsub(str, "+", " ")
	str = string.gsub(str, "%%(%x%x)", function(h) return string.char(tonumber(h, 16)) end)
	str = string.gsub(str, "\r\n", "\n")
	return str
end

function url_encode(str)
	if not str then
		return ""
	end
	str = string.gsub(str, "\n", "\r\n")
	str = string.gsub(str, "([^%w ])", function(c) return string.format("%%%02X", string.byte(c)) end)
	str = string.gsub(str, " ", "+")
	return str
end

function parse_qs(qs)
	local tab = {}
	for key, val in string.gmatch(qs, "([^&=]+)=([^&=]*)&?") do
		tab[url_decode(key)] = url_decode(val)
	end
	return tab
end

function get_cookie(cookies, name)
	cookies = string.gsub(";" .. cookies .. ";", "%s*;%s*", ";")
	return url_decode(string.match(cookies, ";" .. name .. "=(.-);"))
end


--
--
-- Cookie construction and validation helpers.
--
--

local secret = nil

-- Loads a secret from a file, creates a secret, or returns one from memory.
function get_secret()
	if secret ~= nil then
		return secret
	end
	local secret_file = io.open(secret_filename, "r")
	if secret_file == nil then
		local old_umask = sysstat.umask(63)
		local temporary_filename = secret_filename .. ".tmp." .. crypto.hex(crypto.rand.bytes(16))
		local temporary_file = io.open(temporary_filename, "w")
		if temporary_file == nil then
			os.exit(177)
		end
		temporary_file:write(crypto.hex(crypto.rand.bytes(32)))
		temporary_file:close()
		unistd.link(temporary_filename, secret_filename) -- Intentionally fails in the case that another process is doing the same.
		unistd.unlink(temporary_filename)
		sysstat.umask(old_umask)
		secret_file = io.open(secret_filename, "r")
	end
	if secret_file == nil then
		os.exit(177)
	end
	secret = secret_file:read()
	secret_file:close()
	if secret:len() ~= 64 then
		os.exit(177)
	end
	return secret
end

-- Returns value of cookie if cookie is valid. Otherwise returns nil.
function validate_value(expected_field, cookie)
	local i = 0
	local value = ""
	local field = ""
	local expiration = 0
	local salt = ""
	local hmac = ""

	if cookie == nil or cookie:len() < 3 or cookie:sub(1, 1) == "|" then
		return nil
	end

	for component in string.gmatch(cookie, "[^|]+") do
		if i == 0 then
			field = component
		elseif i == 1 then
			value = component
		elseif i == 2 then
			expiration = tonumber(component)
			if expiration == nil then
				expiration = -1
			end
		elseif i == 3 then
			salt = component
		elseif i == 4 then
			hmac = component
		else
			break
		end
		i = i + 1
	end

	if hmac == nil or hmac:len() == 0 then
		return nil
	end

	-- Lua hashes strings, so these comparisons are time invariant.
	if hmac ~= crypto.hmac.digest("sha256", field .. "|" .. value .. "|" .. tostring(expiration) .. "|" .. salt, get_secret()) then
		return nil
	end

	if expiration == -1 or (expiration ~= 0 and expiration <= os.time()) then
		return nil
	end

	if url_decode(field) ~= expected_field then
		return nil
	end

	return url_decode(value)
end

function secure_value(field, value, expiration)
	if value == nil or value:len() <= 0 then
		return ""
	end

	local authstr = ""
	local salt = crypto.hex(crypto.rand.bytes(16))
	value = url_encode(value)
	field = url_encode(field)
	authstr = field .. "|" .. value .. "|" .. tostring(expiration) .. "|" .. salt
	authstr = authstr .. "|" .. crypto.hmac.digest("sha256", authstr, get_secret())
	return authstr
end

function set_cookie(cookie, value)
	html("Set-Cookie: " .. cookie .. "=" .. value .. "; HttpOnly")
	if http["https"] == "yes" or http["https"] == "on" or http["https"] == "1" then
		html("; secure")
	end
	html("\n")
end

function redirect_to(url)
	html("Status: 302 Redirect\n")
	html("Cache-Control: no-cache, no-store\n")
	html("Location: " .. url .. "\n")
end

function not_found()
	html("Status: 404 Not Found\n")
	html("Cache-Control: no-cache, no-store\n\n")
end
