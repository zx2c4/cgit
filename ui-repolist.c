/* ui-repolist.c: functions for generating the repolist page
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include <time.h>


time_t read_agefile(char *path)
{
	FILE *f;
	static char buf[64], buf2[64];

	if (!(f = fopen(path, "r")))
		return -1;
	fgets(buf, sizeof(buf), f);
	fclose(f);
	if (parse_date(buf, buf2, sizeof(buf2)))
		return strtoul(buf2, NULL, 10);
	else
		return 0;
}

static void print_modtime(struct repoinfo *repo)
{
	char *path;
	struct stat s;

	path = fmt("%s/%s", repo->path, cgit_agefile);
	if (stat(path, &s) == 0) {
		cgit_print_age(read_agefile(path), -1, NULL);
		return;
	}

	path = fmt("%s/refs/heads/%s", repo->path, repo->defbranch);
	if (stat(path, &s) != 0)
		return;
	cgit_print_age(s.st_mtime, -1, NULL);
}

void cgit_print_repolist(struct cacheitem *item)
{
	int i;
	char *last_group = NULL;

	cgit_print_docstart(cgit_root_title, item);
	cgit_print_pageheader(cgit_root_title, 0);

	html("<table class='list nowrap'>");
	if (cgit_index_header) {
		html("<tr class='nohover'><td colspan='5' class='include-block'>");
		html_include(cgit_index_header);
		html("</td></tr>");
	}
	html("<tr class='nohover'>"
	     "<th class='left'>Name</th>"
	     "<th class='left'>Description</th>"
	     "<th class='left'>Owner</th>"
	     "<th class='left'>Idle</th>"
	     "<th>Links</th></tr>\n");

	for (i=0; i<cgit_repolist.count; i++) {
		cgit_repo = &cgit_repolist.repos[i];
		if ((last_group == NULL && cgit_repo->group != NULL) ||
		    (last_group != NULL && cgit_repo->group == NULL) ||
		    (last_group != NULL && cgit_repo->group != NULL &&
		     strcmp(cgit_repo->group, last_group))) {
			html("<tr class='nohover'><td colspan='4' class='repogroup'>");
			html_txt(cgit_repo->group);
			html("</td></tr>");
			last_group = cgit_repo->group;
		}
		htmlf("<tr><td class='%s'>",
		      cgit_repo->group ? "sublevel-repo" : "toplevel-repo");
		html_link_open(cgit_repourl(cgit_repo->url), NULL, NULL);
		html_txt(cgit_repo->name);
		html_link_close();
		html("</td><td>");
		html_ntxt(cgit_max_repodesc_len, cgit_repo->desc);
		html("</td><td>");
		html_txt(cgit_repo->owner);
		html("</td><td>");
		print_modtime(cgit_repo);
		html("</td><td>");
		html_link_open(cgit_repourl(cgit_repo->url),
			       "Summary", "button");
		html("S</a>");
		cgit_log_link("L", "Log", "button", NULL, NULL, NULL);
		cgit_tree_link("F", "Files", "button", NULL, NULL, NULL);
		html("</td></tr>\n");
	}
	html("</table>");
	cgit_print_docend();
}
