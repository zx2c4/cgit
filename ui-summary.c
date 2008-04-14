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

void cgit_print_summary()
{
	if (ctx.repo->readme) {
		html("<div id='summary'>");
		html_include(ctx.repo->readme);
		html("</div>");
	}
	html("<table summary='repository info' class='list nowrap'>");
	cgit_print_branches(ctx.cfg.summary_branches);
	html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
	cgit_print_tags(ctx.cfg.summary_tags);
	if (ctx.cfg.summary_log > 0) {
		html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
		cgit_print_log(ctx.qry.head, 0, ctx.cfg.summary_log, NULL,
			       NULL, NULL, 0);
	}
	html("</table>");
}
