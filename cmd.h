#ifndef CMD_H
#define CMD_H

typedef void (*cgit_cmd_fn)(struct cgit_context *ctx);

struct cgit_cmd {
	const char *name;
	cgit_cmd_fn fn;
	unsigned int want_repo:1,
		want_layout:1,
		want_vpath:1,
		is_clone:1;
};

extern struct cgit_cmd *cgit_get_cmd(struct cgit_context *ctx);

#endif /* CMD_H */
