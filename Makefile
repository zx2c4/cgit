CGIT_VERSION = v0.9.0.3
CGIT_SCRIPT_NAME = cgit.cgi
CGIT_SCRIPT_PATH = /var/www/htdocs/cgit
CGIT_DATA_PATH = $(CGIT_SCRIPT_PATH)
CGIT_CONFIG = /etc/cgitrc
CACHE_ROOT = /var/cache/cgit
prefix = /usr
libdir = $(prefix)/lib
filterdir = $(libdir)/cgit/filters
docdir = $(prefix)/share/doc/cgit
htmldir = $(docdir)
pdfdir = $(docdir)
mandir = $(prefix)/share/man
SHA1_HEADER = <openssl/sha.h>
GIT_VER = 1.7.4
GIT_URL = http://hjemli.net/git/git/snapshot/git-$(GIT_VER).tar.bz2
INSTALL = install
MAN5_TXT = $(wildcard *.5.txt)
MAN_TXT  = $(MAN5_TXT)
DOC_MAN5 = $(patsubst %.txt,%,$(MAN5_TXT))
DOC_HTML = $(patsubst %.txt,%.html,$(MAN_TXT))
DOC_PDF  = $(patsubst %.txt,%.pdf,$(MAN_TXT))

# Define NO_STRCASESTR if you don't have strcasestr.
#
# Define NO_OPENSSL to disable linking with OpenSSL and use bundled SHA1
# implementation (slower).
#
# Define NEEDS_LIBICONV if linking with libc is not enough (eg. Darwin).
#
# Define NO_C99_FORMAT if your formatted IO functions (printf/scanf et.al.)
# do not support the 'size specifiers' introduced by C99, namely ll, hh,
# j, z, t. (representing long long int, char, intmax_t, size_t, ptrdiff_t).
# some C compilers supported these specifiers prior to C99 as an extension.
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
	$(QUIET_MM)$(CC) $(CFLAGS) -MM -MP $< | sed -e 's/\($*\)\.o:/\1.o $@:/g' >$@

#
# Define a pattern rule for silent object building
#
%.o: %.c
	$(QUIET_CC)$(CC) -o $*.o -c $(CFLAGS) $<


EXTLIBS = git/libgit.a git/xdiff/lib.a -lz -lpthread
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
OBJECTS += vector.o

ifdef NEEDS_LIBICONV
	EXTLIBS += -liconv
endif


.PHONY: all libgit test install uninstall clean force-version get-git \
	doc clean-doc install-doc install-man install-html install-pdf \
	uninstall-doc uninstall-man uninstall-html uninstall-pdf

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

GIT_OPTIONS = prefix=/usr

ifdef NO_ICONV
	CFLAGS += -DNO_ICONV
endif
ifdef NO_STRCASESTR
	CFLAGS += -DNO_STRCASESTR
endif
ifdef NO_C99_FORMAT
	CFLAGS += -DNO_C99_FORMAT
endif
ifdef NO_OPENSSL
	CFLAGS += -DNO_OPENSSL
	GIT_OPTIONS += NO_OPENSSL=1
else
	EXTLIBS += -lcrypto
endif

cgit: $(OBJECTS) libgit
	$(QUIET_CC)$(CC) $(CFLAGS) $(LDFLAGS) -o cgit $(OBJECTS) $(EXTLIBS)

cgit.o: VERSION

ifneq "$(MAKECMDGOALS)" "clean"
  -include $(OBJECTS:.o=.d)
endif

libgit:
	$(QUIET_SUBDIR0)git $(QUIET_SUBDIR1) NO_CURL=1 $(GIT_OPTIONS) libgit.a
	$(QUIET_SUBDIR0)git $(QUIET_SUBDIR1) NO_CURL=1 $(GIT_OPTIONS) xdiff/lib.a

test: all
	$(QUIET_SUBDIR0)tests $(QUIET_SUBDIR1) all

install: all
	$(INSTALL) -m 0755 -d $(DESTDIR)$(CGIT_SCRIPT_PATH)
	$(INSTALL) -m 0755 cgit $(DESTDIR)$(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	$(INSTALL) -m 0755 -d $(DESTDIR)$(CGIT_DATA_PATH)
	$(INSTALL) -m 0644 cgit.css $(DESTDIR)$(CGIT_DATA_PATH)/cgit.css
	$(INSTALL) -m 0644 cgit.png $(DESTDIR)$(CGIT_DATA_PATH)/cgit.png
	$(INSTALL) -m 0755 -d $(DESTDIR)$(filterdir)
	$(INSTALL) -m 0755 filters/* $(DESTDIR)$(filterdir)

install-doc: install-man install-html install-pdf

install-man: doc-man
	$(INSTALL) -m 0755 -d $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 0644 $(DOC_MAN5) $(DESTDIR)$(mandir)/man5

install-html: doc-html
	$(INSTALL) -m 0755 -d $(DESTDIR)$(htmldir)
	$(INSTALL) -m 0644 $(DOC_HTML) $(DESTDIR)$(htmldir)

install-pdf: doc-pdf
	$(INSTALL) -m 0755 -d $(DESTDIR)$(pdfdir)
	$(INSTALL) -m 0644 $(DOC_PDF) $(DESTDIR)$(pdfdir)

uninstall:
	rm -f $(DESTDIR)$(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	rm -f $(DESTDIR)$(CGIT_DATA_PATH)/cgit.css
	rm -f $(DESTDIR)$(CGIT_DATA_PATH)/cgit.png

uninstall-doc: uninstall-man uninstall-html uninstall-pdf

uninstall-man:
	@for i in $(DOC_MAN5); do \
	    rm -fv $(DESTDIR)$(mandir)/man5/$$i; \
	done

uninstall-html:
	@for i in $(DOC_HTML); do \
	    rm -fv $(DESTDIR)$(htmldir)/$$i; \
	done

uninstall-pdf:
	@for i in $(DOC_PDF); do \
	    rm -fv $(DESTDIR)$(pdfdir)/$$i; \
	done

doc: doc-man doc-html doc-pdf
doc-man: doc-man5
doc-man5: $(DOC_MAN5)
doc-html: $(DOC_HTML)
doc-pdf: $(DOC_PDF)

%.5 : %.5.txt
	a2x -f manpage $<

$(DOC_HTML): %.html : %.txt
	a2x -f xhtml --stylesheet=cgit-doc.css $<

$(DOC_PDF): %.pdf : %.txt
	a2x -f pdf cgitrc.5.txt

clean: clean-doc
	rm -f cgit VERSION *.o *.d

clean-doc:
	rm -f cgitrc.5 cgitrc.5.html cgitrc.5.pdf cgitrc.5.xml cgitrc.5.fo

get-git:
	curl $(GIT_URL) | tar -xjf - && rm -rf git && mv git-$(GIT_VER) git
