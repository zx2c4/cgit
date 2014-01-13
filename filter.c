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
#include <dlfcn.h>

static ssize_t (*libc_write)(int fd, const void *buf, size_t count);
static ssize_t (*filter_write)(struct cgit_filter *base, const void *buf, size_t count) = NULL;
static struct cgit_filter *current_write_filter = NULL;

static inline void reap_filter(struct cgit_filter *filter)
{
	if (filter && filter->cleanup)
		filter->cleanup(filter);
}

void cgit_cleanup_filters(void)
{
	int i;
	reap_filter(ctx.cfg.about_filter);
	reap_filter(ctx.cfg.commit_filter);
	reap_filter(ctx.cfg.source_filter);
	for (i = 0; i < cgit_repolist.count; ++i) {
		reap_filter(cgit_repolist.repos[i].about_filter);
		reap_filter(cgit_repolist.repos[i].commit_filter);
		reap_filter(cgit_repolist.repos[i].source_filter);
	}
}

void cgit_init_filters(void)
{
	libc_write = dlsym(RTLD_NEXT, "write");
	if (!libc_write)
		die("Could not locate libc's write function");
}

ssize_t write(int fd, const void *buf, size_t count)
{
	if (fd != STDOUT_FILENO || !filter_write)
		return libc_write(fd, buf, count);
	return filter_write(current_write_filter, buf, count);
}

static inline void hook_write(struct cgit_filter *filter, ssize_t (*new_write)(struct cgit_filter *base, const void *buf, size_t count))
{
	/* We want to avoid buggy nested patterns. */
	assert(filter_write == NULL);
	assert(current_write_filter == NULL);
	current_write_filter = filter;
	filter_write = new_write;
}

static inline void unhook_write()
{
	assert(filter_write != NULL);
	assert(current_write_filter != NULL);
	filter_write = NULL;
	current_write_filter = NULL;
}

static int open_exec_filter(struct cgit_filter *base, va_list ap)
{
	struct cgit_exec_filter *filter = (struct cgit_exec_filter *) base;
	int i;

	for (i = 0; i < filter->base.argument_count; i++)
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
	for (i = 0; i < filter->base.argument_count; i++)
		filter->argv[i+1] = NULL;
	return 0;

}

static void fprintf_exec_filter(struct cgit_filter *base, FILE *f, const char *prefix)
{
	struct cgit_exec_filter *filter = (struct cgit_exec_filter *) base;
	fprintf(f, "%sexec:%s\n", prefix, filter->cmd);
}

static void cleanup_exec_filter(struct cgit_filter *base)
{
	struct cgit_exec_filter *filter = (struct cgit_exec_filter *) base;
	if (filter->argv) {
		free(filter->argv);
		filter->argv = NULL;
	}
	if (filter->cmd) {
		free(filter->cmd);
		filter->cmd = NULL;
	}
}

static struct cgit_filter *new_exec_filter(const char *cmd, int argument_count)
{
	struct cgit_exec_filter *f;
	int args_size = 0;

	f = xmalloc(sizeof(*f));
	/* We leave argv for now and assign it below. */
	cgit_exec_filter_init(f, xstrdup(cmd), NULL);
	f->base.argument_count = argument_count;
	args_size = (2 + argument_count) * sizeof(char *);
	f->argv = xmalloc(args_size);
	memset(f->argv, 0, args_size);
	f->argv[0] = f->cmd;
	return &f->base;
}

void cgit_exec_filter_init(struct cgit_exec_filter *filter, char *cmd, char **argv)
{
	memset(filter, 0, sizeof(*filter));
	filter->base.open = open_exec_filter;
	filter->base.close = close_exec_filter;
	filter->base.fprintf = fprintf_exec_filter;
	filter->base.cleanup = cleanup_exec_filter;
	filter->cmd = cmd;
	filter->argv = argv;
	/* The argument count for open_filter is zero by default, unless called from new_filter, above. */
	filter->base.argument_count = 0;
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



static const struct {
	const char *prefix;
	struct cgit_filter *(*ctor)(const char *cmd, int argument_count);
} filter_specs[] = {
	{ "exec", new_exec_filter },
};

struct cgit_filter *cgit_new_filter(const char *cmd, filter_type filtertype)
{
	char *colon;
	int i;
	size_t len;
	int argument_count;

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

	switch (filtertype) {
		case SOURCE:
		case ABOUT:
			argument_count = 1;
			break;

		case COMMIT:
		default:
			argument_count = 0;
			break;
	}

	/* If no prefix is given, exec filter is the default. */
	if (!colon)
		return new_exec_filter(cmd, argument_count);

	for (i = 0; i < ARRAY_SIZE(filter_specs); i++) {
		if (len == strlen(filter_specs[i].prefix) &&
		    !strncmp(filter_specs[i].prefix, cmd, len))
			return filter_specs[i].ctor(colon + 1, argument_count);
	}

	die("Invalid filter type: %.*s", (int) len, cmd);
}
