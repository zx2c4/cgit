/* filter.c: filter framework functions
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static int open_exec_filter(struct cgit_filter *base, va_list ap)
{
	struct cgit_exec_filter *filter = (struct cgit_exec_filter *) base;
	int i;

	for (i = 0; i < filter->extra_args; i++)
		filter->argv[i+1] = va_arg(ap, char *);

	filter->old_stdout = chk_positive(dup(STDOUT_FILENO),
		"Unable to duplicate STDOUT");
	chk_zero(pipe(filter->pipe_fh), "Unable to create pipe to subprocess");
	filter->pid = chk_non_negative(fork(), "Unable to create subprocess");
	if (filter->pid == 0) {
		close(filter->pipe_fh[1]);
		chk_non_negative(dup2(filter->pipe_fh[0], STDIN_FILENO),
			"Unable to use pipe as STDIN");
		execvp(filter->cmd, filter->argv);
		die_errno("Unable to exec subprocess %s", filter->cmd);
	}
	close(filter->pipe_fh[0]);
	chk_non_negative(dup2(filter->pipe_fh[1], STDOUT_FILENO),
		"Unable to use pipe as STDOUT");
	close(filter->pipe_fh[1]);
	return 0;
}

static int close_exec_filter(struct cgit_filter *base)
{
	struct cgit_exec_filter *filter = (struct cgit_exec_filter *) base;
	int i, exit_status;

	chk_non_negative(dup2(filter->old_stdout, STDOUT_FILENO),
		"Unable to restore STDOUT");
	close(filter->old_stdout);
	if (filter->pid < 0)
		goto done;
	waitpid(filter->pid, &exit_status, 0);
	if (WIFEXITED(exit_status) && !WEXITSTATUS(exit_status))
		goto done;
	die("Subprocess %s exited abnormally", filter->cmd);

done:
	for (i = 0; i < filter->extra_args; i++)
		filter->argv[i+1] = NULL;
	return 0;

}

static void fprintf_exec_filter(struct cgit_filter *base, FILE *f, const char *prefix)
{
	struct cgit_exec_filter *filter = (struct cgit_exec_filter *) base;
	fprintf(f, "%sexec:%s\n", prefix, filter->cmd);
}

int cgit_open_filter(struct cgit_filter *filter, ...)
{
	int result;
	va_list ap;
	va_start(ap, filter);
	result = filter->open(filter, ap);
	va_end(ap);
	return result;
}

int cgit_close_filter(struct cgit_filter *filter)
{
	return filter->close(filter);
}

void cgit_fprintf_filter(struct cgit_filter *filter, FILE *f, const char *prefix)
{
	filter->fprintf(filter, f, prefix);
}

void cgit_exec_filter_init(struct cgit_exec_filter *filter, char *cmd, char **argv)
{
	memset(filter, 0, sizeof(*filter));
	filter->base.open = open_exec_filter;
	filter->base.close = close_exec_filter;
	filter->base.fprintf = fprintf_exec_filter;
	filter->cmd = cmd;
	filter->argv = argv;
}

static struct cgit_filter *new_exec_filter(const char *cmd, filter_type filtertype)
{
	struct cgit_exec_filter *f;
	int args_size = 0;

	f = xmalloc(sizeof(*f));
	/* We leave argv for now and assign it below. */
	cgit_exec_filter_init(f, xstrdup(cmd), NULL);

	switch (filtertype) {
		case SOURCE:
		case ABOUT:
			f->extra_args = 1;
			break;

		case COMMIT:
		default:
			f->extra_args = 0;
			break;
	}

	args_size = (2 + f->extra_args) * sizeof(char *);
	f->argv = xmalloc(args_size);
	memset(f->argv, 0, args_size);
	f->argv[0] = f->cmd;
	return &f->base;
}

static const struct {
	const char *prefix;
	struct cgit_filter *(*ctor)(const char *cmd, filter_type filtertype);
} filter_specs[] = {
	{ "exec", new_exec_filter },
};

struct cgit_filter *cgit_new_filter(const char *cmd, filter_type filtertype)
{
	char *colon;
	int i;
	size_t len;
	if (!cmd || !cmd[0])
		return NULL;

	colon = strchr(cmd, ':');
	len = colon - cmd;
	/*
	 * In case we're running on Windows, don't allow a single letter before
	 * the colon.
	 */
	if (len == 1)
		colon = NULL;

	/* If no prefix is given, exec filter is the default. */
	if (!colon)
		return new_exec_filter(cmd, filtertype);

	for (i = 0; i < ARRAY_SIZE(filter_specs); i++) {
		if (len == strlen(filter_specs[i].prefix) &&
		    !strncmp(filter_specs[i].prefix, cmd, len))
			return filter_specs[i].ctor(colon + 1, filtertype);
	}

	die("Invalid filter type: %.*s", (int) len, cmd);
}
