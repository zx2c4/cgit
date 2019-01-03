-- This script may be used with the auth-filter. Be sure to configure it as you wish.
--
-- Requirements:
-- 	luaossl
-- 	<http://25thandclement.com/~william/projects/luaossl.html>
-- 	lualdap >= 1.2
-- 	<https://git.zx2c4.com/lualdap/about/>
-- 	luaposix
-- 	<https://github.com/luaposix/luaposix>
--
local sysstat = require("posix.sys.stat")
local unistd = require("posix.unistd")
local lualdap = require("lualdap")
local rand = require("openssl.rand")
local hmac = require("openssl.hmac")

--
--
-- Configure these variables for your settings.
--
--

-- A list of password protected repositories, with which gentooAccess
-- group is allowed to access each one.
local protected_repos = {
	glouglou = "infra",
	portage = "dev"
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
	local redirect = validate_value("redirect", post["redirect"])

	if redirect == nil then
		not_found()
		return 0
	end

	redirect_to(redirect)
	
	local groups = gentoo_ldap_user_groups(post["username"], post["password"])
	if groups == nil then
		set_cookie("cgitauth", "")
	else
		-- One week expiration time
		set_cookie("cgitauth", secure_value("gentoogroups", table.concat(groups, ","), os.time() + 604800))
	end

	html("\n")
	return 0
end


-- Returns 1 if the cookie is valid and 0 if it is not.
function authenticate_cookie()
	local required_group = protected_repos[cgit["repo"]]
	if required_group == nil then
		-- We return as valid if the repo is not protected.
		return 1
	end
	
	local user_groups = validate_value("gentoogroups", get_cookie(http["cookie"], "cgitauth"))
	if user_groups == nil or user_groups == "" then
		return 0
	end
	for group in string.gmatch(user_groups, "[^,]+") do
		if group == required_group then
			return 1
		end
	end
	return 0
end

-- Prints the html for the login form.
function body()
	html("<h2>Gentoo LDAP Authentication Required</h2>")
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
-- Gentoo LDAP support.
--
--

function gentoo_ldap_user_groups(username, password)
	-- Ensure the user is alphanumeric
	if username == nil or username:match("%W") then
		return nil
	end

	local who = "uid=" .. username .. ",ou=devs,dc=gentoo,dc=org"

	local ldap, err = lualdap.open_simple {
		uri = "ldap://ldap1.gentoo.org",
		who = who,
		password = password,
		starttls = true,
		certfile = "/var/www/uwsgi/cgit/gentoo-ldap/star.gentoo.org.crt",
		keyfile = "/var/www/uwsgi/cgit/gentoo-ldap/star.gentoo.org.key",
		cacertfile = "/var/www/uwsgi/cgit/gentoo-ldap/ca.pem"
	}
	if ldap == nil then
		return nil
	end

	local group_suffix = ".group"
	local group_suffix_len = group_suffix:len()
	local groups = {}
	for dn, attribs in ldap:search { base = who, scope = "subtree" } do
		local access = attribs["gentooAccess"]
		if dn == who and access ~= nil then
			for i, v in ipairs(access) do
				local vlen = v:len()
				if vlen > group_suffix_len and v:sub(-group_suffix_len) == group_suffix then
					table.insert(groups, v:sub(1, vlen - group_suffix_len))
				end
			end
		end
	end

	ldap:close()

	return groups
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
	return string.match(cookies, ";" .. name .. "=(.-);")
end

function tohex(b)
	local x = ""
	for i = 1, #b do
		x = x .. string.format("%.2x", string.byte(b, i))
	end
	return x
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
		local temporary_filename = secret_filename .. ".tmp." .. tohex(rand.bytes(16))
		local temporary_file = io.open(temporary_filename, "w")
		if temporary_file == nil then
			os.exit(177)
		end
		temporary_file:write(tohex(rand.bytes(32)))
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
	local chmac = ""

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
			chmac = component
		else
			break
		end
		i = i + 1
	end

	if chmac == nil or chmac:len() == 0 then
		return nil
	end

	-- Lua hashes strings, so these comparisons are time invariant.
	if chmac ~= tohex(hmac.new(get_secret(), "sha256"):final(field .. "|" .. value .. "|" .. tostring(expiration) .. "|" .. salt)) then
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
	local salt = tohex(rand.bytes(16))
	value = url_encode(value)
	field = url_encode(field)
	authstr = field .. "|" .. value .. "|" .. tostring(expiration) .. "|" .. salt
	authstr = authstr .. "|" .. tohex(hmac.new(get_secret(), "sha256"):final(authstr))
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
