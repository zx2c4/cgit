/* ui-repolist.c: functions for generating the repolist page
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

void cgit_print_repolist(struct cacheitem *item)
{
	struct repoinfo *repo;
	int i;

	cgit_print_docstart(cgit_root_title, item);
	cgit_print_pageheader(cgit_root_title, 0);

	html("<h2>Repositories</h2>\n");
	html("<table class='list nowrap'>");
	html("<tr class='nohover'>"
	     "<th class='left'>Name</th>"
	     "<th class='left'>Description</th>"
	     "<th class='left'>Owner</th></tr>\n");

	for (i=0; i<cgit_repolist.count; i++) {
		repo = &cgit_repolist.repos[i];
		html("<tr><td>");
		html_link_open(cgit_repourl(repo->url), NULL, NULL);
		html_txt(repo->name);
		html_link_close();
		html("</td><td>");
		html_txt(repo->desc);
		html("</td><td>");
		html_txt(repo->owner);
		html("</td></tr>\n");
	}
	html("</table>");
	cgit_print_docend();
}
