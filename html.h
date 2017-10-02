#ifndef HTML_H
#define HTML_H

#include "cgit.h"

extern void html_raw(const char *txt, size_t size);
extern void html(const char *txt);

__attribute__((format (printf,1,2)))
extern void htmlf(const char *format,...);

__attribute__((format (printf,1,2)))
extern void html_txtf(const char *format,...);

__attribute__((format (printf,1,0)))
extern void html_vtxtf(const char *format, va_list ap);

__attribute__((format (printf,1,2)))
extern void html_attrf(const char *format,...);

extern void html_txt(const char *txt);
extern ssize_t html_ntxt(const char *txt, size_t len);
extern void html_attr(const char *txt);
extern void html_url_path(const char *txt);
extern void html_url_arg(const char *txt);
extern void html_header_arg_in_quotes(const char *txt);
extern void html_hidden(const char *name, const char *value);
extern void html_option(const char *value, const char *text, const char *selected_value);
extern void html_intoption(int value, const char *text, int selected_value);
extern void html_link_open(const char *url, const char *title, const char *class);
extern void html_link_close(void);
extern void html_fileperm(unsigned short mode);
extern int html_include(const char *filename);

extern void http_parse_querystring(const char *txt, void (*fn)(const char *name, const char *value));

#endif /* HTML_H */
