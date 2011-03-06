#ifndef HTML_H
#define HTML_H

extern int htmlfd;

extern void html_raw(const char *txt, size_t size);
extern void html(const char *txt);

__attribute__((format (printf,1,2)))
extern void htmlf(const char *format,...);

extern void html_status(int code, const char *msg, int more_headers);
extern void html_txt(const char *txt);
extern void html_ntxt(int len, const char *txt);
extern void html_attr(const char *txt);
extern void html_url_path(const char *txt);
extern void html_url_arg(const char *txt);
extern void html_hidden(const char *name, const char *value);
extern void html_option(const char *value, const char *text, const char *selected_value);
extern void html_intoption(int value, const char *text, int selected_value);
extern void html_link_open(const char *url, const char *title, const char *class);
extern void html_link_close(void);
extern void html_fileperm(unsigned short mode);
extern int html_include(const char *filename);

extern int http_parse_querystring(const char *txt, void (*fn)(const char *name, const char *value));

#endif /* HTML_H */
