#ifndef UI_SSDIFF_H
#define UI_SSDIFF_H

/*
 * ssdiff line limits
 */
#ifndef MAX_SSDIFF_M
#define MAX_SSDIFF_M 128
#endif

#ifndef MAX_SSDIFF_N
#define MAX_SSDIFF_N 128
#endif
#define MAX_SSDIFF_SIZE ((MAX_SSDIFF_M) * (MAX_SSDIFF_N))

extern void cgit_ssdiff_print_deferred_lines(void);

extern void cgit_ssdiff_line_cb(char *line, int len);

extern void cgit_ssdiff_header_begin(void);
extern void cgit_ssdiff_header_end(void);

extern void cgit_ssdiff_footer(void);

#endif /* UI_SSDIFF_H */
