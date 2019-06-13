all::

CGIT_VERSION = v1.2.1
CGIT_SCRIPT_NAME = cgit.cgi
CGIT_SCRIPT_PATH = /var/www/htdocs/cgit
CGIT_DATA_PATH = $(CGIT_SCRIPT_PATH)
CGIT_CONFIG = /etc/cgitrc
CACHE_ROOT = /var/cache/cgit
prefix = /usr/local
libdir = $(prefix)/lib
filterdir = $(libdir)/cgit/filters
docdir = $(prefix)/share/doc/cgit
htmldir = $(docdir)
pdfdir = $(docdir)
mandir = $(prefix)/share/man
SHA1_HEADER = <openssl/sha.h>
GIT_VER = 2.23.0
GIT_URL = https://www.kernel.org/pub/software/scm/git/git-$(GIT_VER).tar.xz
INSTALL = install
COPYTREE = cp -r
MAN5_TXT = $(wildcard *.5.txt)
MAN_TXT  = $(MAN5_TXT)
DOC_MAN5 = $(patsubst %.txt,%,$(MAN5_TXT))
DOC_HTML = $(patsubst %.txt,%.html,$(MAN_TXT))
DOC_PDF  = $(patsubst %.txt,%.pdf,$(MAN_TXT))

ASCIIDOC = asciidoc
ASCIIDOC_EXTRA =
ASCIIDOC_HTML = xhtml11
ASCIIDOC_COMMON = $(ASCIIDOC) $(ASCIIDOC_EXTRA)
TXT_TO_HTML = $(ASCIIDOC_COMMON) -b $(ASCIIDOC_HTML)

# Define NO_C99_FORMAT if your formatted IO functions (printf/scanf et.al.)
# do not support the 'size specifiers' introduced by C99, namely ll, hh,
# j, z, t. (representing long long int, char, intmax_t, size_t, ptrdiff_t).
# some C compilers supported these specifiers prior to C99 as an extension.
#
# Define HAVE_LINUX_SENDFILE to use sendfile()

#-include config.mak

-include git/config.mak.uname
#
# Let the user override the above settings.
#
-include cgit.conf

export CGIT_VERSION CGIT_SCRIPT_NAME CGIT_SCRIPT_PATH CGIT_DATA_PATH CGIT_CONFIG CACHE_ROOT

#
# Define a way to invoke make in subdirs quietly, shamelessly ripped
# from git.git
#
QUIET_SUBDIR0  = +$(MAKE) -C # space to separate -C and subdir
QUIET_SUBDIR1  =

ifneq ($(findstring w,$(MAKEFLAGS)),w)
PRINT_DIR = --no-print-directory
else # "make -w"
NO_SUBDIR = :
endif

ifndef V
	QUIET_SUBDIR0  = +@subdir=
	QUIET_SUBDIR1  = ;$(NO_SUBDIR) echo '   ' SUBDIR $$subdir; \
			 $(MAKE) $(PRINT_DIR) -C $$subdir
	QUIET_TAGS     = @echo '   ' TAGS $@;
	export V
endif

.SUFFIXES:

all:: cgit

cgit:
	$(QUIET_SUBDIR0)git $(QUIET_SUBDIR1) -f ../cgit.mk ../cgit $(EXTRA_GIT_TARGETS) NO_CURL=1

sparse:
	$(QUIET_SUBDIR0)git $(QUIET_SUBDIR1) -f ../cgit.mk NO_CURL=1 cgit-sparse

test:
	@$(MAKE) --no-print-directory cgit EXTRA_GIT_TARGETS=all
	$(QUIET_SUBDIR0)tests $(QUIET_SUBDIR1) all

install: all
	$(INSTALL) -m 0755 -d $(DESTDIR)$(CGIT_SCRIPT_PATH)
	$(INSTALL) -m 0755 cgit $(DESTDIR)$(CGIT_SCRIPT_PATH)/$(CGIT_SCRIPT_NAME)
	$(INSTALL) -m 0755 -d $(DESTDIR)$(CGIT_DATA_PATH)
	$(INSTALL) -m 0644 cgit.css $(DESTDIR)$(CGIT_DATA_PATH)/cgit.css
	$(INSTALL) -m 0644 cgit.png $(DESTDIR)$(CGIT_DATA_PATH)/cgit.png
	$(INSTALL) -m 0644 favicon.ico $(DESTDIR)$(CGIT_DATA_PATH)/favicon.ico
	$(INSTALL) -m 0644 robots.txt $(DESTDIR)$(CGIT_DATA_PATH)/robots.txt
	$(INSTALL) -m 0755 -d $(DESTDIR)$(filterdir)
	$(COPYTREE) filters/* $(DESTDIR)$(filterdir)

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
	rm -f $(DESTDIR)$(CGIT_DATA_PATH)/favicon.ico

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
	$(TXT_TO_HTML) -o $@+ $< && \
	mv $@+ $@

$(DOC_PDF): %.pdf : %.txt
	a2x -f pdf cgitrc.5.txt

clean: clean-doc
	$(RM) cgit VERSION CGIT-CFLAGS *.o tags
	$(RM) -r .deps

cleanall: clean
	$(MAKE) -C git clean

clean-doc:
	$(RM) cgitrc.5 cgitrc.5.html cgitrc.5.pdf cgitrc.5.xml cgitrc.5.fo

get-git:
	curl -L $(GIT_URL) | tar -xJf - && rm -rf git && mv git-$(GIT_VER) git

tags:
	$(QUIET_TAGS)find . -name '*.[ch]' | xargs ctags

.PHONY: all cgit git get-git
.PHONY: clean clean-doc cleanall
.PHONY: doc doc-html doc-man doc-pdf
.PHONY: install install-doc install-html install-man install-pdf
.PHONY: tags test
.PHONY: uninstall uninstall-doc uninstall-html uninstall-man uninstall-pdf
