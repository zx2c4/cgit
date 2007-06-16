CGIT_VERSION = 0.5

prefix = /var/www/htdocs/cgit

SHA1_HEADER = <openssl/sha.h>
CACHE_ROOT = /var/cache/cgit
CGIT_CONFIG = /etc/cgitrc
CGIT_SCRIPT_NAME = cgit.cgi

#
# Let the user override the above settings.
#
-include cgit.conf

EXTLIBS = git/libgit.a git/xdiff/lib.a -lz -lcrypto
OBJECTS = shared.o cache.o parsing.o html.o ui-shared.o ui-repolist.o \
	ui-summary.o ui-log.o ui-tree.o ui-commit.o ui-diff.o \
	ui-snapshot.o ui-blob.o

CFLAGS += -Wall

ifdef DEBUG
	CFLAGS += -g
endif

CFLAGS += -Igit
CFLAGS += -DSHA1_HEADER='$(SHA1_HEADER)'
CFLAGS += -DCGIT_VERSION='"$(CGIT_VERSION)"'
CFLAGS += -DCGIT_CONFIG='"$(CGIT_CONFIG)"'
CFLAGS += -DCGIT_SCRIPT_NAME='"$(CGIT_SCRIPT_NAME)"'


#
# If make is run on a nongit platform, get the git sources as a tarball.
#
GITVER = $(shell git version 2>/dev/null || echo nogit)
ifeq ($(GITVER),nogit)
GITURL = http://www.kernel.org/pub/software/scm/git/git-1.5.2.tar.bz2
INITGIT = test -e git/git.c || ((curl "$(GITURL)" | tar -xj) && mv git-1.5.2 git)
else
INITGIT = ./submodules.sh -i
endif


#
# basic build rules
#
all: cgit

cgit: cgit.c cgit.h $(OBJECTS)
	$(CC) $(CFLAGS) cgit.c -o cgit $(OBJECTS) $(EXTLIBS)

$(OBJECTS): cgit.h git/libgit.a

git/libgit.a:
	$(INITGIT)
	$(MAKE) -C git

#
# phony targets
#
install: all clean-cache
	mkdir -p $(prefix)
	install cgit $(prefix)/$(CGIT_SCRIPT_NAME)
	install cgit.css $(prefix)/cgit.css

clean-cgit:
	rm -f cgit *.o

distclean-cgit: clean-cgit
	git clean -d -x

clean-sub:
	$(MAKE) -C git clean

distclean-sub: clean-sub
	$(shell cd git && git clean -d -x)

clean-cache:
	rm -rf $(CACHE_ROOT)/*

clean: clean-cgit clean-sub

distclean: distclean-cgit distclean-sub

.PHONY: all install clean clean-cgit clean-sub clean-cache \
	distclean distclean-cgit distclean-sub
