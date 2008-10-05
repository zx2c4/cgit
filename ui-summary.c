/* ui-summary.c: functions for generating repo summary page
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-log.h"
#include "ui-refs.h"

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
			       NULL, NULL, 0);
	}
	if (ctx.repo->clone_url)
		print_urls(ctx.repo->clone_url, NULL);
	else if (ctx.cfg.clone_prefix)
		print_urls(ctx.cfg.clone_prefix, ctx.repo->url);
	html("</table>");
}

void cgit_print_repo_readme()
{
	if (ctx.repo->readme) {
		html("<div id='summary'>");
		html_include(ctx.repo->readme);
		html("</div>");
	}
}
