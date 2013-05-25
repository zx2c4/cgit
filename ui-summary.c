/* ui-summary.c: functions for generating repo summary page
 *
 * Copyright (C) 2006 Lars Hjemli
 * Copyright (C) 2010 Jason A. Donenfeld <Jason@zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-summary.h"
#include "html.h"
#include "ui-log.h"
#include "ui-refs.h"
#include "ui-blob.h"

static void print_url(char *base, char *suffix)
{
	int columns = 3;
	struct strbuf basebuf = STRBUF_INIT;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;

	if (!base || !*base)
		return;
	if (suffix && *suffix) {
		strbuf_addf(&basebuf, "%s/%s", base, suffix);
		base = basebuf.buf;
	}
	htmlf("<tr><td colspan='%d'><a href='", columns);
	html_url_path(base);
	html("'>");
	html_txt(base);
	html("</a></td></tr>\n");
	strbuf_release(&basebuf);
}

static void print_urls(char *txt, char *suffix)
{
	char *h = txt, *t, c;
	int urls = 0;
	int columns = 3;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;


	while (h && *h) {
		while (h && *h == ' ')
			h++;
		if (!*h)
			break;
		t = h;
		while (t && *t && *t != ' ')
			t++;
		c = *t;
		*t = 0;
		if (urls++ == 0) {
			htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
			htmlf("<tr><th class='left' colspan='%d'>Clone</th></tr>\n", columns);
		}
		print_url(h, suffix);
		*t = c;
		h = t;
	}
}

void cgit_print_summary()
{
	int columns = 3;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;

	html("<table summary='repository info' class='list nowrap'>");
	cgit_print_branches(ctx.cfg.summary_branches);
	htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
	cgit_print_tags(ctx.cfg.summary_tags);
	if (ctx.cfg.summary_log > 0) {
		htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
		cgit_print_log(ctx.qry.head, 0, ctx.cfg.summary_log, NULL,
			       NULL, NULL, 0, 0, 0);
	}
	if (ctx.repo->clone_url)
		print_urls(expand_macros(ctx.repo->clone_url), NULL);
	else if (ctx.cfg.clone_prefix)
		print_urls(ctx.cfg.clone_prefix, ctx.repo->url);
	html("</table>");
}

/* The caller must free filename and ref after calling this. */
void cgit_parse_readme(const char *readme, const char *path, char **filename, char **ref, struct cgit_repo *repo)
{
	const char *slash, *colon;
	char *resolved_base, *resolved_full;

	*filename = NULL;
	*ref = NULL;

	if (!readme || !(*readme))
		return;

	/* Check if the readme is tracked in the git repo. */
	colon = strchr(readme, ':');
	if (colon && strlen(colon) > 1) {
		/* If it starts with a colon, we want to use
		 * the default branch */
		if (colon == readme && repo->defbranch)
			*ref = xstrdup(repo->defbranch);
		else
			*ref = xstrndup(readme, colon - readme);
		readme = colon + 1;
	}

	/* Prepend repo path to relative readme path unless tracked. */
	if (!(*ref) && *readme != '/')
		readme = fmtalloc("%s/%s", repo->path, readme);

	/* If a subpath is specified for the about page, make it relative
	 * to the directory containing the configured readme. */
	if (path) {
		slash = strrchr(readme, '/');
		if (!slash) {
			if (!colon)
				return;
			slash = colon;
		}
		*filename = xmalloc(slash - readme + 1 + strlen(path) + 1);
		strncpy(*filename, readme, slash - readme + 1);
		if (!(*ref))
			resolved_base = realpath(*filename, NULL);
		strcpy(*filename + (slash - readme + 1), path);
		if (!(*ref))
			resolved_full = realpath(*filename, NULL);
		if (!(*ref) && (!resolved_base || !resolved_full || strstr(resolved_full, resolved_base) != resolved_full)) {
			free(*filename);
			*filename = NULL;
		}
		if (!(*ref)) {
			free(resolved_base);
			free(resolved_full);
		}
	} else
		*filename = xstrdup(readme);
}

void cgit_print_repo_readme(char *path)
{
	char *filename, *ref;
	cgit_parse_readme(ctx.repo->readme, path, &filename, &ref, ctx.repo);

	if (!filename)
		return;

	/* Print the calculated readme, either from the git repo or from the
	 * filesystem, while applying the about-filter.
	 */
	html("<div id='summary'>");
	if (ctx.repo->about_filter) {
		ctx.repo->about_filter->argv[1] = filename;
		cgit_open_filter(ctx.repo->about_filter);
	}
	if (ref)
		cgit_print_file(filename, ref);
	else
		html_include(filename);
	if (ctx.repo->about_filter) {
		cgit_close_filter(ctx.repo->about_filter);
		ctx.repo->about_filter->argv[1] = NULL;
	}
	html("</div>");
	free(filename);
	free(ref);
}
