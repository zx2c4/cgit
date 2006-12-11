CGIT_VERSION = 0.1-pre

INSTALL_BIN = /var/www/htdocs/cgit.cgi
INSTALL_CSS = /var/www/htdocs/cgit.css
CACHE_ROOT = /var/cache/cgit

EXTLIBS = ../git/libgit.a ../git/xdiff/lib.a -lz -lcrypto
OBJECTS = parsing.o html.o cache.o

CFLAGS += -Wall

all: cgit

install: all
	install cgit $(INSTALL_BIN)
	install cgit.css $(INSTALL_CSS)
	rm -rf $(CACHE_ROOT)/*

cgit: cgit.c cgit.h git.h $(OBJECTS)
	$(CC) $(CFLAGS) -DCGIT_VERSION='"$(CGIT_VERSION)"' cgit.c -o cgit \
		$(OBJECTS) $(EXTLIBS)

$(OBJECTS): cgit.h git.h

.PHONY: clean
clean:
	rm -f cgit *.o
