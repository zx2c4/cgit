#ifndef UI_SSDIFF_H
#define UI_SSDIFF_H

extern void cgit_ssdiff_print_deferred_lines();

extern void cgit_ssdiff_line_cb(char *line, int len);

extern void cgit_ssdiff_header_begin();
extern void cgit_ssdiff_header_end();

extern void cgit_ssdiff_footer();

#endif /* UI_SSDIFF_H */
