CGIT_VERSION = v0.5
CGIT_SCRIPT_NAME = cgit.cgi
CGIT_SCRIPT_PATH = /var/www/htdocs/cgit
CGIT_CONFIG = /etc/cgitrc
CACHE_ROOT = /var/cache/cgit
SHA1_HEADER = <openssl/sha.h>
GIT_VER = 1.5.2
GIT_URL = http://www.kernel.org/pub/software/scm/git/git-$(GIT_VER).tar.bz2

#
# Let the user override the above settings.
#
-include cgit.conf


EXTLIBS = git/libgit.a git/xdiff/lib.a -lz -lcrypto
OBJECTS = shared.o cache.o parsing.o html.o ui-shared.o ui-repolist.o \
	ui-summary.o ui-log.o ui-tree.o ui-commit.o ui-diff.o \
	ui-snapshot.o ui-blob.o ui-tag.o


.PHONY: all git install clean distclean force-version get-git

all: cgit git

VERSION: force-version
	@./gen-version.sh "$(CGIT_VERSION)"
-include VERSION


CFLAGS += -g -Wall -Igit
CFLAGS += -DSHA1_HEADER='$(SHA1_HEADER)'
CFLAGS += -DCGIT_VERSION='"$(CGIT_VERSION)"'
CFLAGS += -DCGIT_CONFIG='"$(CGIT_CONFIG)"'
CFLAGS += -DCGIT_SCRIPT_NAME='"$(CGIT_SCRIPT_NAME)"'


cgit: cgit.c $(OBJECTS)
	$(CC) $(CFLAGS) cgit.c -o cgit $(OBJECTS) $(EXTLIBS)

$(OBJECTS): cgit.h git/xdiff/lib.a git/libgit.a VERSION

git/xdiff/lib.a: | git

git/libgit.a: | git

git:
	cd git && $(MAKE) xdiff/lib.a
	cd git && $(MAKE) libgit.a

install: all
	mkdir -p $(CGIT_SCRIPT_PATH)
	install cgit $(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	install cgit.css $(CGIT_SCRIPT_PATH)/cgit.css
	rm -rf $(CACHE_ROOT)/*

uninstall:
	rm -f $(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	rm -f $(CGIT_SCRIPT_PATH)/cgit.css
	rm -rf $(CACHE_ROOT)

clean:
	rm -f cgit VERSION *.o
	cd git && $(MAKE) clean

distclean: clean
	git clean -d -x
	cd git && git clean -d -x

get-git:
	curl $(GIT_URL) | tar -xj && rm -rf git && mv git-$(GIT_VER) git
