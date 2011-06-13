/* ui-summary.c: functions for generating repo summary page
 *
 * Copyright (C) 2006 Lars Hjemli
 * Copyright (C) 2010 Jason A. Donenfeld <Jason@zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-log.h"
#include "ui-refs.h"
#include "ui-blob.h"

int urls = 0;

static void print_url(char *base, char *suffix)
{
	if (!base || !*base)
		return;
	if (urls++ == 0) {
		html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
		html("<tr><th class='left' colspan='4'>Clone</th></tr>\n");
	}
	if (suffix && *suffix)
		base = fmt("%s/%s", base, suffix);
	html("<tr><td colspan='4'><a href='");
	html_url_path(base);
	html("'>");
	html_txt(base);
	html("</a></td></tr>\n");
}

static void print_urls(char *txt, char *suffix)
{
	char *h = txt, *t, c;

	while (h && *h) {
		while (h && *h == ' ')
			h++;
		t = h;
		while (t && *t && *t != ' ')
			t++;
		c = *t;
		*t = 0;
		print_url(h, suffix);
		*t = c;
		h = t;
	}
}

void cgit_print_summary()
{
	html("<table summary='repository info' class='list nowrap'>");
	cgit_print_branches(ctx.cfg.summary_branches);
	html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
	cgit_print_tags(ctx.cfg.summary_tags);
	if (ctx.cfg.summary_log > 0) {
		html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
		cgit_print_log(ctx.qry.head, 0, ctx.cfg.summary_log, NULL,
			       NULL, NULL, 0, 0);
	}
	if (ctx.repo->clone_url)
		print_urls(expand_macros(ctx.repo->clone_url), NULL);
	else if (ctx.cfg.clone_prefix)
		print_urls(ctx.cfg.clone_prefix, ctx.repo->url);
	html("</table>");
}

void cgit_print_repo_readme(char *path)
{
	char *slash, *tmp, *colon, *ref;

	if (!ctx.repo->readme || !(*ctx.repo->readme))
		return;

	ref = NULL;

	/* Check if the readme is tracked in the git repo. */
	colon = strchr(ctx.repo->readme, ':');
	if (colon && strlen(colon) > 1) {
		*colon = '\0';
		ref = ctx.repo->readme;
		ctx.repo->readme = colon + 1;
		if (!(*ctx.repo->readme))
			return;
	}

	/* Prepend repo path to relative readme path unless tracked. */
	if (!ref && *ctx.repo->readme != '/')
		ctx.repo->readme = xstrdup(fmt("%s/%s", ctx.repo->path,
					       ctx.repo->readme));

	/* If a subpath is specified for the about page, make it relative
	 * to the directory containing the configured readme.
	 */
	if (path) {
		slash = strrchr(ctx.repo->readme, '/');
		if (!slash) {
			if (!colon)
				return;
			slash = colon;
		}
		tmp = xmalloc(slash - ctx.repo->readme + 1 + strlen(path) + 1);
		strncpy(tmp, ctx.repo->readme, slash - ctx.repo->readme + 1);
		strcpy(tmp + (slash - ctx.repo->readme + 1), path);
	} else
		tmp = ctx.repo->readme;

	/* Print the calculated readme, either from the git repo or from the
	 * filesystem, while applying the about-filter.
	 */
	html("<div id='summary'>");
	if (ctx.repo->about_filter)
		cgit_open_filter(ctx.repo->about_filter);
	if (ref)
		cgit_print_file(tmp, ref);
	else
		html_include(tmp);
	if (ctx.repo->about_filter)
		cgit_close_filter(ctx.repo->about_filter);
	html("</div>");
}
