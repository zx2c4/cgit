CGIT_VERSION = 0.1

INSTALL_DIR = /var/www/htdocs/cgit
CACHE_ROOT = /var/cache/cgit

EXTLIBS = ../git/libgit.a ../git/xdiff/lib.a -lz -lcrypto
OBJECTS = shared.o cache.o parsing.o html.o ui-shared.o ui-repolist.o \
	ui-summary.o ui-log.o ui-view.c ui-tree.c ui-commit.c ui-diff.o

CFLAGS += -Wall

ifdef DEBUG
	CFLAGS += -g
endif

all: cgit

install: all clean-cache
	install cgit $(INSTALL_DIR)/cgit.cgi
	install cgit.css $(INSTALL_DIR)/cgit.css

cgit: cgit.c cgit.h git.h $(OBJECTS)
	$(CC) $(CFLAGS) -DCGIT_VERSION='"$(CGIT_VERSION)"' cgit.c -o cgit \
		$(OBJECTS) $(EXTLIBS)

$(OBJECTS): cgit.h git.h

ui-diff.o: xdiff.h

.PHONY: clean
clean:
	rm -f cgit *.o

clean-cache:
	rm -rf $(CACHE_ROOT)/*
