/* ui-repolist.c: functions for generating the repolist page
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include <time.h>

#include "cgit.h"
#include "html.h"

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

static void print_modtime(struct cgit_repo *repo)
{
	char *path;
	struct stat s;

	path = fmt("%s/%s", repo->path, ctx.cfg.agefile);
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
	int i, columns = 4;
	char *last_group = NULL;

	if (ctx.cfg.enable_index_links)
		columns++;

	cgit_print_docstart(ctx.cfg.root_title, item);
	cgit_print_pageheader(ctx.cfg.root_title, 0);

	html("<table summary='repository list' class='list nowrap'>");
	if (ctx.cfg.index_header) {
		htmlf("<tr class='nohover'><td colspan='%d' class='include-block'>",
		      columns);
		html_include(ctx.cfg.index_header);
		html("</td></tr>");
	}
	html("<tr class='nohover'>"
	     "<th class='left'>Name</th>"
	     "<th class='left'>Description</th>"
	     "<th class='left'>Owner</th>"
	     "<th class='left'>Idle</th>");
	if (ctx.cfg.enable_index_links)
		html("<th>Links</th>");
	html("</tr>\n");

	for (i=0; i<cgit_repolist.count; i++) {
		ctx.repo = &cgit_repolist.repos[i];
		if ((last_group == NULL && ctx.repo->group != NULL) ||
		    (last_group != NULL && ctx.repo->group == NULL) ||
		    (last_group != NULL && ctx.repo->group != NULL &&
		     strcmp(ctx.repo->group, last_group))) {
			htmlf("<tr class='nohover'><td colspan='%d' class='repogroup'>",
			      columns);
			html_txt(ctx.repo->group);
			html("</td></tr>");
			last_group = ctx.repo->group;
		}
		htmlf("<tr><td class='%s'>",
		      ctx.repo->group ? "sublevel-repo" : "toplevel-repo");
		html_link_open(cgit_repourl(ctx.repo->url), NULL, NULL);
		html_txt(ctx.repo->name);
		html_link_close();
		html("</td><td>");
		html_ntxt(ctx.cfg.max_repodesc_len, ctx.repo->desc);
		html("</td><td>");
		html_txt(ctx.repo->owner);
		html("</td><td>");
		print_modtime(ctx.repo);
		html("</td>");
		if (ctx.cfg.enable_index_links) {
			html("<td>");
			html_link_open(cgit_repourl(ctx.repo->url),
				       NULL, "button");
			html("summary</a>");
			cgit_log_link("log", NULL, "button", NULL, NULL, NULL,
				      0, NULL, NULL);
			cgit_tree_link("tree", NULL, "button", NULL, NULL, NULL);
			html("</td>");
		}
		html("</tr>\n");
	}
	html("</table>");
	cgit_print_docend();
}
