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
	if (fgets(buf, sizeof(buf), f) == NULL)
		return -1;
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
	if (repo->url && strcasestr(repo->url, ctx.qry.search))
		return 1;
	if (repo->name && strcasestr(repo->name, ctx.qry.search))
		return 1;
	if (repo->desc && strcasestr(repo->desc, ctx.qry.search))
		return 1;
	if (repo->owner && strcasestr(repo->owner, ctx.qry.search))
		return 1;
	return 0;
}

int is_in_url(struct cgit_repo *repo)
{
	if (!ctx.qry.url)
		return 1;
	if (repo->url && !prefixcmp(repo->url, ctx.qry.url))
		return 1;
	return 0;
}

void print_header(int columns)
{
	html("<tr class='nohover'>"
	     "<th class='left'>Name</th>"
	     "<th class='left'>Description</th>"
	     "<th class='left'>Owner</th>"
	     "<th class='left'><a href=\"?s=1\">Idle</a></th>");
	if (ctx.cfg.enable_index_links)
		html("<th class='left'>Links</th>");
	html("</tr>\n");
}


void print_pager(int items, int pagelen, char *search)
{
	int i;
	html("<div class='pager'>");
	for(i = 0; i * pagelen < items; i++)
		cgit_index_link(fmt("[%d]", i+1), fmt("Page %d", i+1), NULL,
				search, i * pagelen);
	html("</div>");
}

static int cgit_reposort_modtime(const void *a, const void *b)
{
	const struct cgit_repo *r1 = a;
	const struct cgit_repo *r2 = b;
	char *path;
	struct stat s;
	time_t t1, t2;
	path = fmt("%s/%s", r1->path, ctx.cfg.agefile);
	if (stat(path, &s) == 0) {
		t1 = read_agefile(path);
	} else {
		path = fmt("%s/refs/heads/%s", r1->path, r1->defbranch);
		if (stat(path, &s) != 0)
			return 0;
		t1 =s.st_mtime;
	}

	path = fmt("%s/%s", r2->path, ctx.cfg.agefile);
	if (stat(path, &s) == 0) {
		t2 = read_agefile(path);
	} else {
		path = fmt("%s/refs/heads/%s", r2->path, r2->defbranch);
		if (stat(path, &s) != 0)
			return 0;
		t2 =s.st_mtime;
	}
	return t2-t1;
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

	if (ctx.cfg.index_header)
		html_include(ctx.cfg.index_header);

	if(ctx.qry.sort)
		qsort(cgit_repolist.repos,cgit_repolist.count,sizeof(struct cgit_repo),cgit_reposort_modtime);

	html("<table summary='repository list' class='list nowrap'>");
	for (i=0; i<cgit_repolist.count; i++) {
		ctx.repo = &cgit_repolist.repos[i];
		if (!(is_match(ctx.repo) && is_in_url(ctx.repo)))
			continue;
		hits++;
		if (hits <= ctx.qry.ofs)
			continue;
		if (hits > ctx.qry.ofs + ctx.cfg.max_repo_count)
			continue;
		if (!header++)
			print_header(columns);
		if (!ctx.qry.sort &&
		    ((last_group == NULL && ctx.repo->group != NULL) ||
		    (last_group != NULL && ctx.repo->group == NULL) ||
		    (last_group != NULL && ctx.repo->group != NULL &&
		     strcmp(ctx.repo->group, last_group)))) {
			htmlf("<tr class='nohover'><td colspan='%d' class='repogroup'>",
			      columns);
			html_txt(ctx.repo->group);
			html("</td></tr>");
			last_group = ctx.repo->group;
		}
		htmlf("<tr><td class='%s'>",
		      ctx.repo->group ? "sublevel-repo" : "toplevel-repo");
		cgit_summary_link(ctx.repo->name, ctx.repo->name, NULL, NULL);
		html("</td><td>");
		html_link_open(cgit_repourl(ctx.repo->url), NULL, NULL);
		html_ntxt(ctx.cfg.max_repodesc_len, ctx.repo->desc);
		html_link_close();
		html("</td><td>");
		html_txt(ctx.repo->owner);
		html("</td><td>");
		print_modtime(ctx.repo);
		html("</td>");
		if (ctx.cfg.enable_index_links) {
			html("<td>");
			cgit_summary_link("summary", NULL, "button", NULL);
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
	else if (hits > ctx.cfg.max_repo_count)
		print_pager(hits, ctx.cfg.max_repo_count, ctx.qry.search);
	cgit_print_docend();
}

void cgit_print_site_readme()
{
	if (ctx.cfg.root_readme)
		html_include(ctx.cfg.root_readme);
}
