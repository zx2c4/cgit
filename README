cgit - CGI for Git
==================

This is an attempt to create a fast web interface for the Git SCM, using a
built-in cache to decrease server I/O pressure.

Installation
------------

Building cgit involves building a proper version of Git. How to do this
depends on how you obtained the cgit sources:

a) If you're working in a cloned cgit repository, you first need to
initialize and update the Git submodule:

    $ git submodule init     # register the Git submodule in .git/config
    $ $EDITOR .git/config    # if you want to specify a different url for git
    $ git submodule update   # clone/fetch and checkout correct git version

b) If you're building from a cgit tarball, you can download a proper git
version like this:

    $ make get-git

When either a) or b) has been performed, you can build and install cgit like
this:

    $ make
    $ sudo make install

This will install `cgit.cgi` and `cgit.css` into `/var/www/htdocs/cgit`. You
can configure this location (and a few other things) by providing a `cgit.conf`
file (see the Makefile for details).

If you'd like to compile without Lua support, you may use:

    $ make NO_LUA=1

And if you'd like to specify a Lua implementation, you may use:

    $ make LUA_PKGCONFIG=lua5.1

If this is not specified, the Lua implementation will be auto-detected,
preferring LuaJIT if many are present. Acceptable values are generally "lua",
"luajit", "lua5.1", and "lua5.2".


Dependencies
------------

* libzip
* libcrypto (OpenSSL)
* libssl (OpenSSL)
* optional: luajit or lua, most reliably used when pkg-config is available

Apache configuration
--------------------

A new `Directory` section must probably be added for cgit, possibly something
like this:

    <Directory "/var/www/htdocs/cgit/">
        AllowOverride None
        Options +ExecCGI
        Order allow,deny
        Allow from all
    </Directory>


Runtime configuration
---------------------

The file `/etc/cgitrc` is read by cgit before handling a request. In addition
to runtime parameters, this file may also contain a list of repositories
displayed by cgit (see `cgitrc.5.txt` for further details).

The cache
---------

When cgit is invoked it looks for a cache file matching the request and
returns it to the client. If no such cache file exists (or if it has expired),
the content for the request is written into the proper cache file before the
file is returned.

If the cache file has expired but cgit is unable to obtain a lock for it, the
stale cache file is returned to the client. This is done to favour page
throughput over page freshness.

The generated content contains the complete response to the client, including
the HTTP headers `Modified` and `Expires`.

Online presence
---------------

* The cgit homepage is hosted by cgit at <https://git.zx2c4.com/cgit/about/>

* Patches, bug reports, discussions and support should go to the cgit
  mailing list: <cgit@lists.zx2c4.com>. To sign up, visit
  <https://lists.zx2c4.com/mailman/listinfo/cgit>
