
INSTALL_BIN = /var/www/htdocs/cgit.cgi
INSTALL_CSS = /var/www/htdocs/cgit.css

EXTLIBS = ../git/libgit.a ../git/xdiff/lib.a -lz -lcrypto
OBJECTS = cgit.o config.o html.o cache.o

CFLAGS += -Wall

all: cgit

install: all
	install cgit $(INSTALL_BIN)
	install cgit.css $(INSTALL_CSS)

clean:
	rm -f cgit *.o

cgit: $(OBJECTS)
	$(CC) $(CFLAGS) -o cgit $(OBJECTS) $(EXTLIBS)

$(OBJECTS): cgit.h git.h
