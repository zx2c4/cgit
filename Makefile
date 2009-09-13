CGIT_VERSION = v0.8.3
CGIT_SCRIPT_NAME = cgit.cgi
CGIT_SCRIPT_PATH = /var/www/htdocs/cgit
CGIT_DATA_PATH = $(CGIT_SCRIPT_PATH)
CGIT_CONFIG = /etc/cgitrc
CACHE_ROOT = /var/cache/cgit
SHA1_HEADER = <openssl/sha.h>
GIT_VER = 1.6.4.3
GIT_URL = http://www.kernel.org/pub/software/scm/git/git-$(GIT_VER).tar.bz2
INSTALL = install

# Define NO_STRCASESTR if you don't have strcasestr.
#
# Define NEEDS_LIBICONV if linking with libc is not enough (eg. Darwin).
#

#-include config.mak

#
# Platform specific tweaks
#

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')
uname_R := $(shell sh -c 'uname -r 2>/dev/null || echo not')

ifeq ($(uname_O),Cygwin)
	NO_STRCASESTR = YesPlease
	NEEDS_LIBICONV = YesPlease
endif

#
# Let the user override the above settings.
#
-include cgit.conf

#
# Define a way to invoke make in subdirs quietly, shamelessly ripped
# from git.git
#
QUIET_SUBDIR0  = +$(MAKE) -C # space to separate -C and subdir
QUIET_SUBDIR1  =

ifneq ($(findstring $(MAKEFLAGS),w),w)
PRINT_DIR = --no-print-directory
else # "make -w"
NO_SUBDIR = :
endif

ifndef V
	QUIET_CC       = @echo '   ' CC $@;
	QUIET_MM       = @echo '   ' MM $@;
	QUIET_SUBDIR0  = +@subdir=
	QUIET_SUBDIR1  = ;$(NO_SUBDIR) echo '   ' SUBDIR $$subdir; \
			 $(MAKE) $(PRINT_DIR) -C $$subdir
endif

#
# Define a pattern rule for automatic dependency building
#
%.d: %.c
	$(QUIET_MM)$(CC) $(CFLAGS) -MM $< | sed -e 's/\($*\)\.o:/\1.o $@:/g' >$@

#
# Define a pattern rule for silent object building
#
%.o: %.c
	$(QUIET_CC)$(CC) -o $*.o -c $(CFLAGS) $<


EXTLIBS = git/libgit.a git/xdiff/lib.a -lz -lcrypto
OBJECTS =
OBJECTS += cache.o
OBJECTS += cgit.o
OBJECTS += cmd.o
OBJECTS += configfile.o
OBJECTS += html.o
OBJECTS += parsing.o
OBJECTS += scan-tree.o
OBJECTS += shared.o
OBJECTS += ui-atom.o
OBJECTS += ui-blob.o
OBJECTS += ui-clone.o
OBJECTS += ui-commit.o
OBJECTS += ui-diff.o
OBJECTS += ui-log.o
OBJECTS += ui-patch.o
OBJECTS += ui-plain.o
OBJECTS += ui-refs.o
OBJECTS += ui-repolist.o
OBJECTS += ui-shared.o
OBJECTS += ui-snapshot.o
OBJECTS += ui-ssdiff.o
OBJECTS += ui-stats.o
OBJECTS += ui-summary.o
OBJECTS += ui-tag.o
OBJECTS += ui-tree.o

ifdef NEEDS_LIBICONV
	EXTLIBS += -liconv
endif


.PHONY: all libgit test install uninstall clean force-version get-git \
	doc man-doc html-doc clean-doc

all: cgit

VERSION: force-version
	@./gen-version.sh "$(CGIT_VERSION)"
-include VERSION


CFLAGS += -g -Wall -Igit
CFLAGS += -DSHA1_HEADER='$(SHA1_HEADER)'
CFLAGS += -DCGIT_VERSION='"$(CGIT_VERSION)"'
CFLAGS += -DCGIT_CONFIG='"$(CGIT_CONFIG)"'
CFLAGS += -DCGIT_SCRIPT_NAME='"$(CGIT_SCRIPT_NAME)"'
CFLAGS += -DCGIT_CACHE_ROOT='"$(CACHE_ROOT)"'

ifdef NO_ICONV
	CFLAGS += -DNO_ICONV
endif
ifdef NO_STRCASESTR
	CFLAGS += -DNO_STRCASESTR
endif

cgit: $(OBJECTS) libgit
	$(QUIET_CC)$(CC) $(CFLAGS) $(LDFLAGS) -o cgit $(OBJECTS) $(EXTLIBS)

cgit.o: VERSION

-include $(OBJECTS:.o=.d)

libgit:
	$(QUIET_SUBDIR0)git $(QUIET_SUBDIR1) NO_CURL=1 libgit.a
	$(QUIET_SUBDIR0)git $(QUIET_SUBDIR1) NO_CURL=1 xdiff/lib.a

test: all
	$(QUIET_SUBDIR0)tests $(QUIET_SUBDIR1) all

install: all
	$(INSTALL) -m 0755 -d $(DESTDIR)$(CGIT_SCRIPT_PATH)
	$(INSTALL) -m 0755 cgit $(DESTDIR)$(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	$(INSTALL) -m 0755 -d $(DESTDIR)$(CGIT_DATA_PATH)
	$(INSTALL) -m 0644 cgit.css $(DESTDIR)$(CGIT_DATA_PATH)/cgit.css
	$(INSTALL) -m 0644 cgit.png $(DESTDIR)$(CGIT_DATA_PATH)/cgit.png

uninstall:
	rm -f $(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	rm -f $(CGIT_DATA_PATH)/cgit.css
	rm -f $(CGIT_DATA_PATH)/cgit.png

doc: man-doc html-doc pdf-doc

man-doc: cgitrc.5.txt
	a2x -f manpage cgitrc.5.txt

html-doc: cgitrc.5.txt
	a2x -f xhtml --stylesheet=cgit-doc.css cgitrc.5.txt

pdf-doc: cgitrc.5.txt
	a2x -f pdf cgitrc.5.txt

clean: clean-doc
	rm -f cgit VERSION *.o *.d

clean-doc:
	rm -f cgitrc.5 cgitrc.5.html cgitrc.5.pdf cgitrc.5.xml cgitrc.5.fo

get-git:
	curl $(GIT_URL) | tar -xj && rm -rf git && mv git-$(GIT_VER) git
