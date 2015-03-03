/* ui-repolist.c: functions for generating the repolist page
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-repolist.h"
#include "html.h"
#include "ui-shared.h"
#include <strings.h>

static time_t read_agefile(char *path)
{
	time_t result;
	size_t size;
	char *buf;
	struct strbuf date_buf = STRBUF_INIT;

	if (readfile(path, &buf, &size))
		return -1;

	if (parse_date(buf, &date_buf) == 0)
		result = strtoul(date_buf.buf, NULL, 10);
	else
		result = 0;
	free(buf);
	strbuf_release(&date_buf);
	return result;
}

static int get_repo_modtime(const struct cgit_repo *repo, time_t *mtime)
{
	struct strbuf path = STRBUF_INIT;
	struct stat s;
	struct cgit_repo *r = (struct cgit_repo *)repo;

	if (repo->mtime != -1) {
		*mtime = repo->mtime;
		return 1;
	}
	strbuf_addf(&path, "%s/%s", repo->path, ctx.cfg.agefile);
	if (stat(path.buf, &s) == 0) {
		*mtime = read_agefile(path.buf);
		if (*mtime) {
			r->mtime = *mtime;
			goto end;
		}
	}

	strbuf_reset(&path);
	strbuf_addf(&path, "%s/refs/heads/%s", repo->path,
		    repo->defbranch ? repo->defbranch : "master");
	if (stat(path.buf, &s) == 0) {
		*mtime = s.st_mtime;
		r->mtime = *mtime;
		goto end;
	}

	strbuf_reset(&path);
	strbuf_addf(&path, "%s/%s", repo->path, "packed-refs");
	if (stat(path.buf, &s) == 0) {
		*mtime = s.st_mtime;
		r->mtime = *mtime;
		goto end;
	}

	*mtime = 0;
	r->mtime = *mtime;
end:
	strbuf_release(&path);
	return (r->mtime != 0);
}

static void print_modtime(struct cgit_repo *repo)
{
	time_t t;
	if (get_repo_modtime(repo, &t))
		cgit_print_age(t, -1, NULL);
}

static int is_match(struct cgit_repo *repo)
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

static int is_in_url(struct cgit_repo *repo)
{
	if (!ctx.qry.url)
		return 1;
	if (repo->url && starts_with(repo->url, ctx.qry.url))
		return 1;
	return 0;
}

static void print_sort_header(const char *title, const char *sort)
{
	html("<th class='left'><a href='");
	html_attr(cgit_currenturl());
	htmlf("?s=%s", sort);
	if (ctx.qry.search) {
		html("&amp;q=");
		html_url_arg(ctx.qry.search);
	}
	htmlf("'>%s</a></th>", title);
}

static void print_header()
{
	html("<tr class='nohover'>");
	print_sort_header("Name", "name");
	print_sort_header("Description", "desc");
	if (ctx.cfg.enable_index_owner)
		print_sort_header("Owner", "owner");
	print_sort_header("Idle", "idle");
	if (ctx.cfg.enable_index_links)
		html("<th class='left'>Links</th>");
	html("</tr>\n");
}


static void print_pager(int items, int pagelen, char *search, char *sort)
{
	int i, ofs;
	char *class = NULL;
	html("<ul class='pager'>");
	for (i = 0, ofs = 0; ofs < items; i++, ofs = i * pagelen) {
		class = (ctx.qry.ofs == ofs) ? "current" : NULL;
		html("<li>");
		cgit_index_link(fmt("[%d]", i + 1), fmt("Page %d", i + 1),
				class, search, sort, ofs, 0);
		html("</li>");
	}
	html("</ul>");
}

static int cmp(const char *s1, const char *s2)
{
	if (s1 && s2) {
		if (ctx.cfg.case_sensitive_sort)
			return strcmp(s1, s2);
		else
			return strcasecmp(s1, s2);
	}
	if (s1 && !s2)
		return -1;
	if (s2 && !s1)
		return 1;
	return 0;
}

static int sort_section(const void *a, const void *b)
{
	const struct cgit_repo *r1 = a;
	const struct cgit_repo *r2 = b;
	int result;
	time_t t;

	result = cmp(r1->section, r2->section);
	if (!result) {
		if (!strcmp(ctx.cfg.repository_sort, "age")) {
			// get_repo_modtime caches the value in r->mtime, so we don't
			// have to worry about inefficiencies here.
			if (get_repo_modtime(r1, &t) && get_repo_modtime(r2, &t))
				result = r2->mtime - r1->mtime;
		}
		if (!result)
			result = cmp(r1->name, r2->name);
	}
	return result;
}

static int sort_name(const void *a, const void *b)
{
	const struct cgit_repo *r1 = a;
	const struct cgit_repo *r2 = b;

	return cmp(r1->name, r2->name);
}

static int sort_desc(const void *a, const void *b)
{
	const struct cgit_repo *r1 = a;
	const struct cgit_repo *r2 = b;

	return cmp(r1->desc, r2->desc);
}

static int sort_owner(const void *a, const void *b)
{
	const struct cgit_repo *r1 = a;
	const struct cgit_repo *r2 = b;

	return cmp(r1->owner, r2->owner);
}

static int sort_idle(const void *a, const void *b)
{
	const struct cgit_repo *r1 = a;
	const struct cgit_repo *r2 = b;
	time_t t1, t2;

	t1 = t2 = 0;
	get_repo_modtime(r1, &t1);
	get_repo_modtime(r2, &t2);
	return t2 - t1;
}

struct sortcolumn {
	const char *name;
	int (*fn)(const void *a, const void *b);
};

struct sortcolumn sortcolumn[] = {
	{"section", sort_section},
	{"name", sort_name},
	{"desc", sort_desc},
	{"owner", sort_owner},
	{"idle", sort_idle},
	{NULL, NULL}
};

static int sort_repolist(char *field)
{
	struct sortcolumn *column;

	for (column = &sortcolumn[0]; column->name; column++) {
		if (strcmp(field, column->name))
			continue;
		qsort(cgit_repolist.repos, cgit_repolist.count,
			sizeof(struct cgit_repo), column->fn);
		return 1;
	}
	return 0;
}


void cgit_print_repolist()
{
	int i, columns = 3, hits = 0, header = 0;
	char *last_section = NULL;
	char *section;
	int sorted = 0;

	if (ctx.cfg.enable_index_links)
		++columns;
	if (ctx.cfg.enable_index_owner)
		++columns;

	ctx.page.title = ctx.cfg.root_title;
	cgit_print_http_headers();
	cgit_print_docstart();
	cgit_print_pageheader();

	if (ctx.cfg.index_header)
		html_include(ctx.cfg.index_header);

	if (ctx.qry.sort)
		sorted = sort_repolist(ctx.qry.sort);
	else if (ctx.cfg.section_sort)
		sort_repolist("section");

	html("<table summary='repository list' class='list nowrap'>");
	for (i = 0; i < cgit_repolist.count; i++) {
		ctx.repo = &cgit_repolist.repos[i];
		if (ctx.repo->hide || ctx.repo->ignore)
			continue;
		if (!(is_match(ctx.repo) && is_in_url(ctx.repo)))
			continue;
		hits++;
		if (hits <= ctx.qry.ofs)
			continue;
		if (hits > ctx.qry.ofs + ctx.cfg.max_repo_count)
			continue;
		if (!header++)
			print_header();
		section = ctx.repo->section;
		if (section && !strcmp(section, ""))
			section = NULL;
		if (!sorted &&
		    ((last_section == NULL && section != NULL) ||
		    (last_section != NULL && section == NULL) ||
		    (last_section != NULL && section != NULL &&
		     strcmp(section, last_section)))) {
			htmlf("<tr class='nohover'><td colspan='%d' class='reposection'>",
			      columns);
			html_txt(section);
			html("</td></tr>");
			last_section = section;
		}
		htmlf("<tr><td class='%s'>",
		      !sorted && section ? "sublevel-repo" : "toplevel-repo");
		cgit_summary_link(ctx.repo->name, ctx.repo->name, NULL, NULL);
		html("</td><td>");
		html_link_open(cgit_repourl(ctx.repo->url), NULL, NULL);
		html_ntxt(ctx.cfg.max_repodesc_len, ctx.repo->desc);
		html_link_close();
		html("</td><td>");
		if (ctx.cfg.enable_index_owner) {
			if (ctx.repo->owner_filter) {
				cgit_open_filter(ctx.repo->owner_filter);
				html_txt(ctx.repo->owner);
				cgit_close_filter(ctx.repo->owner_filter);
			} else {
				html("<a href='");
				html_attr(cgit_currenturl());
				html("?q=");
				html_url_arg(ctx.repo->owner);
				html("'>");
				html_txt(ctx.repo->owner);
				html("</a>");
			}
			html("</td><td>");
		}
		print_modtime(ctx.repo);
		html("</td>");
		if (ctx.cfg.enable_index_links) {
			html("<td>");
			cgit_summary_link("summary", NULL, "button", NULL);
			cgit_log_link("log", NULL, "button", NULL, NULL, NULL,
				      0, NULL, NULL, ctx.qry.showmsg);
			cgit_tree_link("tree", NULL, "button", NULL, NULL, NULL);
			html("</td>");
		}
		html("</tr>\n");
	}
	html("</table>");
	if (!hits)
		cgit_print_error("No repositories found");
	else if (hits > ctx.cfg.max_repo_count)
		print_pager(hits, ctx.cfg.max_repo_count, ctx.qry.search, ctx.qry.sort);
	cgit_print_docend();
}

void cgit_print_site_readme()
{
	if (!ctx.cfg.root_readme)
		return;
	cgit_open_filter(ctx.cfg.about_filter, ctx.cfg.root_readme);
	html_include(ctx.cfg.root_readme);
	cgit_close_filter(ctx.cfg.about_filter);
}
