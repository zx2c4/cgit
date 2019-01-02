/* ui-summary.c: functions for generating repo summary page
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-summary.h"
#include "html.h"
#include "ui-blob.h"
#include "ui-log.h"
#include "ui-plain.h"
#include "ui-refs.h"
#include "ui-shared.h"

static int urls;

static void print_url(const char *url)
{
	int columns = 3;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;

	if (urls++ == 0) {
		htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
		htmlf("<tr class='nohover'><th class='left' colspan='%d'>Clone</th></tr>\n", columns);
	}

	htmlf("<tr><td colspan='%d'><a rel='vcs-git' href='", columns);
	html_url_path(url);
	html("' title='");
	html_attr(ctx.repo->name);
	html(" Git repository'>");
	html_txt(url);
	html("</a></td></tr>\n");
}

void cgit_print_summary(void)
{
	int columns = 3;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;

	cgit_print_layout_start();
	html("<table summary='repository info' class='list nowrap'>");
	cgit_print_branches(ctx.cfg.summary_branches);
	htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
	cgit_print_tags(ctx.cfg.summary_tags);
	if (ctx.cfg.summary_log > 0) {
		htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
		cgit_print_log(ctx.qry.head, 0, ctx.cfg.summary_log, NULL,
			       NULL, NULL, 0, 0, 0);
	}
	urls = 0;
	cgit_add_clone_urls(print_url);
	html("</table>");
	cgit_print_layout_end();
}

/* The caller must free the return value. */
static char* append_readme_path(const char *filename, const char *ref, const char *path)
{
	char *file, *base_dir, *full_path, *resolved_base = NULL, *resolved_full = NULL;
	/* If a subpath is specified for the about page, make it relative
	 * to the directory containing the configured readme. */

	file = xstrdup(filename);
	base_dir = dirname(file);
	if (!strcmp(base_dir, ".") || !strcmp(base_dir, "..")) {
		if (!ref) {
			free(file);
			return NULL;
		}
		full_path = xstrdup(path);
	} else
		full_path = fmtalloc("%s/%s", base_dir, path);

	if (!ref) {
		resolved_base = realpath(base_dir, NULL);
		resolved_full = realpath(full_path, NULL);
		if (!resolved_base || !resolved_full || !starts_with(resolved_full, resolved_base)) {
			free(full_path);
			full_path = NULL;
		}
	}

	free(file);
	free(resolved_base);
	free(resolved_full);

	return full_path;
}

void cgit_print_repo_readme(const char *path)
{
	char *filename, *ref, *mimetype;
	int free_filename = 0;

	mimetype = get_mimetype_for_filename(path);
	if (mimetype && (!strncmp(mimetype, "image/", 6) || !strncmp(mimetype, "video/", 6))) {
		ctx.page.mimetype = mimetype;
		ctx.page.charset = NULL;
		cgit_print_plain();
		free(mimetype);
		return;
	}
	free(mimetype);

	cgit_print_layout_start();
	if (ctx.repo->readme.nr == 0)
		goto done;

	filename = ctx.repo->readme.items[0].string;
	ref = ctx.repo->readme.items[0].util;

	if (path) {
		free_filename = 1;
		filename = append_readme_path(filename, ref, path);
		if (!filename)
			goto done;
	}

	/* Print the calculated readme, either from the git repo or from the
	 * filesystem, while applying the about-filter.
	 */
	html("<div id='summary'>");
	cgit_open_filter(ctx.repo->about_filter, filename);
	if (ref)
		cgit_print_file(filename, ref, 1);
	else
		html_include(filename);
	cgit_close_filter(ctx.repo->about_filter);

	html("</div>");
	if (free_filename)
		free(filename);

done:
	cgit_print_layout_end();
}
