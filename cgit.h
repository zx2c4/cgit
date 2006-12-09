#ifndef CGIT_H
#define CGIT_H

#include "git.h"
#include <openssl/sha.h>

extern char *fmt(const char *format,...);

extern void html(const char *txt);
extern void htmlf(const char *format,...);
extern void html_txt(char *txt);
extern void html_attr(char *txt);

extern void html_link_open(char *url, char *title, char *class);
extern void html_link_close(void);

typedef void (*configfn)(const char *name, const char *value);

extern int cgit_read_config(const char *filename, configfn fn);

#endif /* CGIT_H */
