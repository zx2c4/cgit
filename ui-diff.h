#ifndef UI_DIFF_H
#define UI_DIFF_H

extern void cgit_print_diff_ctrls(void);

extern void cgit_print_diff(const char *new_hex, const char *old_hex,
			    const char *prefix, int show_ctrls, int raw);

extern struct diff_filespec *cgit_get_current_old_file(void);
extern struct diff_filespec *cgit_get_current_new_file(void);

extern struct object_id old_rev_oid[1];
extern struct object_id new_rev_oid[1];

#endif /* UI_DIFF_H */
