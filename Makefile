CGIT_VERSION = 0.2

prefix = /var/www/htdocs/cgit

SHA1_HEADER = <openssl/sha.h>

CACHE_ROOT = /var/cache/cgit
EXTLIBS = git/libgit.a git/xdiff/lib.a -lz -lcrypto
OBJECTS = shared.o cache.o parsing.o html.o ui-shared.o ui-repolist.o \
	ui-summary.o ui-log.o ui-view.o ui-tree.o ui-commit.o ui-diff.o \
	ui-snapshot.o ui-blob.o

CFLAGS += -Wall

ifdef DEBUG
	CFLAGS += -g
endif

CFLAGS += -Igit -DSHA1_HEADER='$(SHA1_HEADER)'




#
# basic build rules
#
all: cgit

cgit: cgit.c cgit.h $(OBJECTS)
	$(CC) $(CFLAGS) -DCGIT_VERSION='"$(CGIT_VERSION)"' cgit.c -o cgit \
		$(OBJECTS) $(EXTLIBS)

$(OBJECTS): cgit.h git/libgit.a

git/libgit.a:
	./submodules.sh -i
	$(MAKE) -C git

#
# phony targets
#
install: all clean-cache
	mkdir -p $(prefix)
	install cgit $(prefix)/cgit.cgi
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
