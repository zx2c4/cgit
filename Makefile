CGIT_VERSION = v0.7.2
CGIT_SCRIPT_NAME = cgit.cgi
CGIT_SCRIPT_PATH = /var/www/htdocs/cgit
CGIT_CONFIG = /etc/cgitrc
CACHE_ROOT = /var/cache/cgit
SHA1_HEADER = <openssl/sha.h>
GIT_VER = 1.5.4.1
GIT_URL = http://www.kernel.org/pub/software/scm/git/git-$(GIT_VER).tar.bz2

#
# Let the user override the above settings.
#
-include cgit.conf


#
# Define a pattern rule for automatic dependency building
#
%.d: %.c
	$(CC) $(CFLAGS) -MM $< | sed -e 's/\($*\)\.o:/\1.o $@:/g' >$@


EXTLIBS = git/libgit.a git/xdiff/lib.a -lz -lcrypto
OBJECTS =
OBJECTS += cache.o
OBJECTS += cgit.o
OBJECTS += cmd.o
OBJECTS += html.o
OBJECTS += parsing.o
OBJECTS += shared.o
OBJECTS += ui-blob.o
OBJECTS += ui-commit.o
OBJECTS += ui-diff.o
OBJECTS += ui-log.o
OBJECTS += ui-patch.o
OBJECTS += ui-refs.o
OBJECTS += ui-repolist.o
OBJECTS += ui-shared.o
OBJECTS += ui-snapshot.o
OBJECTS += ui-summary.o
OBJECTS += ui-tag.o
OBJECTS += ui-tree.o

ifdef NEEDS_LIBICONV
	EXTLIBS += -liconv
endif


.PHONY: all git test install clean distclean emptycache force-version get-git

all: cgit git

VERSION: force-version
	@./gen-version.sh "$(CGIT_VERSION)"
-include VERSION


CFLAGS += -g -Wall -Igit
CFLAGS += -DSHA1_HEADER='$(SHA1_HEADER)'
CFLAGS += -DCGIT_VERSION='"$(CGIT_VERSION)"'
CFLAGS += -DCGIT_CONFIG='"$(CGIT_CONFIG)"'
CFLAGS += -DCGIT_SCRIPT_NAME='"$(CGIT_SCRIPT_NAME)"'
CFLAGS += -DCGIT_CACHE_ROOT='"$(CACHE_ROOT)"'


cgit: $(OBJECTS)
	$(CC) $(CFLAGS) -o cgit $(OBJECTS) $(EXTLIBS)

$(OBJECTS): git/xdiff/lib.a git/libgit.a

cgit.o: VERSION

-include $(OBJECTS:.o=.d)

git/xdiff/lib.a: | git

git/libgit.a: | git

git:
	cd git && $(MAKE) xdiff/lib.a
	cd git && $(MAKE) libgit.a

test: all
	$(MAKE) -C tests

install: all
	mkdir -p $(DESTDIR)$(CGIT_SCRIPT_PATH)
	install cgit $(DESTDIR)$(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	install cgit.css $(DESTDIR)$(CGIT_SCRIPT_PATH)/cgit.css
	install cgit.png $(DESTDIR)$(CGIT_SCRIPT_PATH)/cgit.png

uninstall:
	rm -f $(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	rm -f $(CGIT_SCRIPT_PATH)/cgit.css
	rm -f $(CGIT_SCRIPT_PATH)/cgit.png

clean:
	rm -f cgit VERSION *.o *.d
	cd git && $(MAKE) clean

distclean: clean
	git clean -d -x
	cd git && git clean -d -x

emptycache:
	rm -rf $(DESTDIR)$(CACHE_ROOT)/*

get-git:
	curl $(GIT_URL) | tar -xj && rm -rf git && mv git-$(GIT_VER) git
