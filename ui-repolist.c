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
	char *last_group = NULL;

	cgit_print_docstart(cgit_root_title, item);
	cgit_print_pageheader(cgit_root_title, 0);

	html("<table class='list nowrap'>");
	html("<tr class='nohover'>"
	     "<th class='left'>Name</th>"
	     "<th class='left'>Description</th>"
	     "<th class='left'>Owner</th>"
	     "<th class='left'>Links</th></tr>\n");

	for (i=0; i<cgit_repolist.count; i++) {
		repo = &cgit_repolist.repos[i];
		if ((last_group == NULL && repo->group != NULL) ||
		    (last_group != NULL && repo->group == NULL) ||
		    (last_group != NULL && repo->group!= NULL &&
		     strcmp(repo->group, last_group))) {
			html("<tr><td colspan='4' class='repogroup'>");
			html_txt(repo->group);
			html("</td></tr>");
			last_group = repo->group;
		}
		html("<tr><td>");
		html_link_open(cgit_repourl(repo->url), NULL, NULL);
		html_txt(repo->name);
		html_link_close();
		html("</td><td>");
		html_ntxt(cgit_max_repodesc_len, repo->desc);
		html("</td><td>");
		html_txt(repo->owner);
		html("</td><td>");
		html_link_open(cgit_pageurl(repo->name, "commit", NULL),
			       "Commit: display last commit", NULL);
		html("C</a> ");
		html_link_open(cgit_pageurl(repo->name, "diff", NULL),
			       "Diff: see changes introduced by last commit", NULL);
		html("D</a> ");
		html_link_open(cgit_pageurl(repo->name, "log", NULL),
			       "Log: show history of the main branch", NULL);
		html("L</a> ");
		html_link_open(cgit_pageurl(repo->name, "tree", NULL),
			       "Tree: browse the files in the main branch", NULL);
		html("T</a>");
		html("</td></tr>\n");
	}
	html("</table>");
	cgit_print_docend();
}
