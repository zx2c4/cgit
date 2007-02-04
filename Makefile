CGIT_VERSION = 0.1

prefix = /var/www/htdocs/cgit
gitsrc = ../git

CACHE_ROOT = /var/cache/cgit
EXTLIBS = $(gitsrc)/libgit.a $(gitsrc)/xdiff/lib.a -lz -lcrypto
OBJECTS = shared.o cache.o parsing.o html.o ui-shared.o ui-repolist.o \
	ui-summary.o ui-log.o ui-view.c ui-tree.c ui-commit.c ui-diff.o

CFLAGS += -Wall

ifdef DEBUG
	CFLAGS += -g
endif

all: cgit

install: all clean-cache
	mkdir -p $(prefix)
	install cgit $(prefix)/cgit.cgi
	install cgit.css $(prefix)/cgit.css

cgit: cgit.c cgit.h git.h $(OBJECTS) $(gitsrc)/libgit.a
	$(CC) $(CFLAGS) -DCGIT_VERSION='"$(CGIT_VERSION)"' cgit.c -o cgit \
		$(OBJECTS) $(EXTLIBS)

$(OBJECTS): cgit.h git.h

ui-diff.o: xdiff.h

$(gitsrc)/libgit.a:
	$(MAKE) -C $(gitsrc)


.PHONY: clean
clean:
	rm -f cgit *.o

clean-cache:
	rm -rf $(CACHE_ROOT)/*
