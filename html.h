#ifndef HTML_H
#define HTML_H

extern int htmlfd;

extern void html(const char *txt);
extern void htmlf(const char *format,...);
extern void html_txt(char *txt);
extern void html_ntxt(int len, char *txt);
extern void html_attr(char *txt);
extern void html_hidden(char *name, char *value);
extern void html_option(char *value, char *text, char *selected_value);
extern void html_link_open(char *url, char *title, char *class);
extern void html_link_close(void);
extern void html_fileperm(unsigned short mode);
extern int html_include(const char *filename);

extern int http_parse_querystring(char *txt, void (*fn)(const char *name, const char *value));

#endif /* HTML_H */
