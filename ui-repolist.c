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
#include "ui-shared.h"

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

int is_match(struct cgit_repo *repo)
{
	if (!ctx.qry.search)
		return 1;
	if (repo->url && strstr(repo->url, ctx.qry.search))
		return 1;
	if (repo->name && strstr(repo->name, ctx.qry.search))
		return 1;
	if (repo->desc && strstr(repo->desc, ctx.qry.search))
		return 1;
	if (repo->owner && strstr(repo->owner, ctx.qry.search))
		return 1;
	return 0;
}

void print_header(int columns)
{
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
		html("<th class='left'>Links</th>");
	html("</tr>\n");
}

void cgit_print_repolist()
{
	int i, columns = 4, hits = 0, header = 0;
	char *last_group = NULL;

	if (ctx.cfg.enable_index_links)
		columns++;

	ctx.page.title = ctx.cfg.root_title;
	cgit_print_http_headers(&ctx);
	cgit_print_docstart(&ctx);
	cgit_print_pageheader(&ctx);

	html("<table summary='repository list' class='list nowrap'>");
	for (i=0; i<cgit_repolist.count; i++) {
		ctx.repo = &cgit_repolist.repos[i];
		if (!is_match(ctx.repo))
			continue;
		if (!header++)
			print_header(columns);
		hits++;
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
	if (!hits)
		cgit_print_error("No repositories found");
	cgit_print_docend();
}
