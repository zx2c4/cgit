-- This script may be used with the auth-filter. Be sure to configure it as you wish.
--
-- Requirements:
-- 	luacrypto >= 0.3
-- 	<http://mkottman.github.io/luacrypto/>
--


--
--
-- Configure these variables for your settings.
--
--

local protected_repos = {
	glouglou	= { laurent = true, jason = true },
	qt		= { jason = true, bob = true }
}

local users = {
	jason		= "secretpassword",
	laurent		= "s3cr3t",
	bob		= "ilikelua"
}

local secret = "BE SURE TO CUSTOMIZE THIS STRING TO SOMETHING BIG AND RANDOM"



--
--
-- Authentication functions follow below. Swap these out if you want different authentication semantics.
--
--

-- Sets HTTP cookie headers based on post
function authenticate_post()
	local password = users[post["username"]]
	-- TODO: Implement time invariant string comparison function to mitigate against timing attack.
	if password == nil or password ~= post["password"] then
		construct_cookie("", "cgitauth")
	else
		construct_cookie(post["username"], "cgitauth")
	end
	return 0
end


-- Returns 1 if the cookie is valid and 0 if it is not.
function authenticate_cookie()
	accepted_users = protected_repos[cgit["repo"]]
	if accepted_users == nil then
		-- We return as valid if the repo is not protected.
		return 1
	end

	local username = validate_cookie(get_cookie(http["cookie"], "cgitauth"))
	if username == nil or not accepted_users[username] then
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
	html("<table>")
	html("<tr><td><label for='username'>Username:</label></td><td><input id='username' name='username' autofocus /></td></tr>")
	html("<tr><td><label for='password'>Password:</label></td><td><input id='password' name='password' type='password' /></td></tr>")
	html("<tr><td colspan='2'><input value='Login' type='submit' /></td></tr>")
	html("</table></form>")

	return 0
end


--
--
-- Cookie construction and validation helpers.
--
--

local crypto = require("crypto")

-- Returns username of cookie if cookie is valid. Otherwise returns nil.
function validate_cookie(cookie)
	local i = 0
	local username = ""
	local expiration = 0
	local salt = ""
	local hmac = ""

	if cookie:len() < 3 or cookie:sub(1, 1) == "|" then
		return nil
	end

	for component in string.gmatch(cookie, "[^|]+") do
		if i == 0 then
			username = component
		elseif i == 1 then
			expiration = tonumber(component)
			if expiration == nil then
				expiration = 0
			end
		elseif i == 2 then
			salt = component
		elseif i == 3 then
			hmac = component
		else
			break
		end
		i = i + 1
	end

	if hmac == nil or hmac:len() == 0 then
		return nil
	end

	-- TODO: implement time invariant comparison to prevent against timing attack.
	if hmac ~= crypto.hmac.digest("sha1", username .. "|" .. tostring(expiration) .. "|" .. salt, secret) then
		return nil
	end

	if expiration <= os.time() then
		return nil
	end

	return username:lower()
end

function construct_cookie(username, cookie)
	local authstr = ""
	if username:len() > 0 then
		-- One week expiration time
		local expiration = os.time() + 604800
		local salt = crypto.hex(crypto.rand.bytes(16))

		authstr = username .. "|" .. tostring(expiration) .. "|" .. salt
		authstr = authstr .. "|" .. crypto.hmac.digest("sha1", authstr, secret)
	end

	html("Set-Cookie: " .. cookie .. "=" .. authstr .. "; HttpOnly")
	if http["https"] == "yes" or http["https"] == "on" or http["https"] == "1" then
		html("; secure")
	end
	html("\n")
end

--
--
-- Wrapper around filter API follows below, exposing the http table, the cgit table, and the post table to the above functions.
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

	cgit["login"] = ""
	for _ in cgit["url"]:gfind("/") do
		cgit["login"] = cgit["login"] .. "../"
	end
	cgit["login"] = cgit["login"] .. "?p=login"

end

function filter_close()
	return action()
end

function filter_write(str)
	post = parse_qs(str)
end


--
--
-- Utility functions follow below, based on keplerproject/wsapi.
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
