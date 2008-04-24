#ifndef UI_DIFF_H
#define UI_DIFF_H

extern void cgit_print_diffstat(const unsigned char *old_sha1,
				const unsigned char *new_sha1);

extern void cgit_print_diff(const char *new_hex, const char *old_hex,
			    const char *prefix);

#endif /* UI_DIFF_H */
