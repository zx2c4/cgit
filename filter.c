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

int cgit_open_filter(struct cgit_filter *filter, ...)
{
	int i;
	va_list ap;

	va_start(ap, filter);
	for (i = 0; i < filter->extra_args; i++)
		filter->argv[i+1] = va_arg(ap, char *);
	va_end(ap);

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


int cgit_close_filter(struct cgit_filter *filter)
{
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

struct cgit_filter *cgit_new_filter(const char *cmd, filter_type filtertype)
{
	struct cgit_filter *f;
	int args_size = 0;

	if (!cmd || !cmd[0])
		return NULL;

	f = xmalloc(sizeof(struct cgit_filter));
	memset(f, 0, sizeof(struct cgit_filter));

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

	f->cmd = xstrdup(cmd);
	args_size = (2 + f->extra_args) * sizeof(char *);
	f->argv = xmalloc(args_size);
	memset(f->argv, 0, args_size);
	f->argv[0] = f->cmd;
	return f;
}
