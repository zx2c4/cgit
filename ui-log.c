/* ui-log.c: functions for log output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

int files, add_lines, rem_lines;

void count_lines(char *line, int size)
{
	if (size <= 0)
		return;

	if (line[0] == '+')
		add_lines++;

	else if (line[0] == '-')
		rem_lines++;
}

void inspect_files(struct diff_filepair *pair)
{
	unsigned long old_size = 0;
	unsigned long new_size = 0;
	int binary = 0;

	files++;
	if (ctx.repo->enable_log_linecount)
		cgit_diff_files(pair->one->sha1, pair->two->sha1, &old_size,
				&new_size, &binary, count_lines);
}

void show_commit_decorations(struct commit *commit)
{
	struct name_decoration *deco;
	static char buf[1024];

	buf[sizeof(buf) - 1] = 0;
	deco = lookup_decoration(&name_decoration, &commit->object);
	while (deco) {
		if (!prefixcmp(deco->name, "refs/heads/")) {
			strncpy(buf, deco->name + 11, sizeof(buf) - 1);
			cgit_log_link(buf, NULL, "branch-deco", buf, NULL, NULL,
				0, NULL, NULL, ctx.qry.showmsg);
		}
		else if (!prefixcmp(deco->name, "tag: refs/tags/")) {
			strncpy(buf, deco->name + 15, sizeof(buf) - 1);
			cgit_tag_link(buf, NULL, "tag-deco", ctx.qry.head, buf);
		}
		else if (!prefixcmp(deco->name, "refs/tags/")) {
			strncpy(buf, deco->name + 10, sizeof(buf) - 1);
			cgit_tag_link(buf, NULL, "tag-deco", ctx.qry.head, buf);
		}
		else if (!prefixcmp(deco->name, "refs/remotes/")) {
			strncpy(buf, deco->name + 13, sizeof(buf) - 1);
			cgit_log_link(buf, NULL, "remote-deco", NULL,
				sha1_to_hex(commit->object.sha1), NULL,
				0, NULL, NULL, ctx.qry.showmsg);
		}
		else {
			strncpy(buf, deco->name, sizeof(buf) - 1);
			cgit_commit_link(buf, NULL, "deco", ctx.qry.head,
				sha1_to_hex(commit->object.sha1), 0);
		}
		deco = deco->next;
	}
}

void print_commit(struct commit *commit)
{
	struct commitinfo *info;
	char *tmp;
	int cols = 2;

	info = cgit_parse_commit(commit);
	htmlf("<tr%s><td>",
		ctx.qry.showmsg ? " class='logheader'" : "");
	tmp = fmt("id=%s", sha1_to_hex(commit->object.sha1));
	tmp = cgit_pageurl(ctx.repo->url, "commit", tmp);
	html_link_open(tmp, NULL, NULL);
	cgit_print_age(commit->date, TM_WEEK * 2, FMT_SHORTDATE);
	html_link_close();
	htmlf("</td><td%s>",
		ctx.qry.showmsg ? " class='logsubject'" : "");
	cgit_commit_link(info->subject, NULL, NULL, ctx.qry.head,
			 sha1_to_hex(commit->object.sha1), 0);
	show_commit_decorations(commit);
	html("</td><td>");
	html_txt(info->author);
	if (ctx.repo->enable_log_filecount) {
		files = 0;
		add_lines = 0;
		rem_lines = 0;
		cgit_diff_commit(commit, inspect_files);
		html("</td><td>");
		htmlf("%d", files);
		if (ctx.repo->enable_log_linecount) {
			html("</td><td>");
			htmlf("-%d/+%d", rem_lines, add_lines);
		}
	}
	html("</td></tr>\n");
	if (ctx.qry.showmsg) {
		if (ctx.repo->enable_log_filecount) {
			cols++;
			if (ctx.repo->enable_log_linecount)
				cols++;
		}
		htmlf("<tr class='nohover'><td/><td colspan='%d' class='logmsg'>",
			cols);
		html_txt(info->msg);
		html("</td></tr>\n");
	}
	cgit_free_commitinfo(info);
}

static const char *disambiguate_ref(const char *ref)
{
	unsigned char sha1[20];
	const char *longref;

	longref = fmt("refs/heads/%s", ref);
	if (get_sha1(longref, sha1) == 0)
		return longref;

	return ref;
}

void cgit_print_log(const char *tip, int ofs, int cnt, char *grep, char *pattern,
		    char *path, int pager)
{
	struct rev_info rev;
	struct commit *commit;
	const char *argv[] = {NULL, NULL, NULL, NULL, NULL};
	int argc = 2;
	int i, columns = 3;

	if (!tip)
		tip = ctx.qry.head;

	argv[1] = disambiguate_ref(tip);

	if (grep && pattern && (!strcmp(grep, "grep") ||
				!strcmp(grep, "author") ||
				!strcmp(grep, "committer")))
		argv[argc++] = fmt("--%s=%s", grep, pattern);

	if (path) {
		argv[argc++] = "--";
		argv[argc++] = path;
	}
	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.commit_format = CMIT_FMT_DEFAULT;
	rev.verbose_header = 1;
	rev.show_root_diff = 0;
	setup_revisions(argc, argv, &rev, NULL);
	load_ref_decorations(DECORATE_FULL_REFS);
	rev.show_decorations = 1;
	rev.grep_filter.regflags |= REG_ICASE;
	compile_grep_patterns(&rev.grep_filter);
	prepare_revision_walk(&rev);

	if (pager)
		html("<table class='list nowrap'>");

	html("<tr class='nohover'><th class='left'>Age</th>"
	      "<th class='left'>Commit message");
	if (pager) {
		html(" (");
		cgit_log_link(ctx.qry.showmsg ? "Collapse" : "Expand", NULL,
			      NULL, ctx.qry.head, ctx.qry.sha1,
			      ctx.qry.path, ctx.qry.ofs, ctx.qry.grep,
			      ctx.qry.search, ctx.qry.showmsg ? 0 : 1);
		html(")");
	}
	html("</th><th class='left'>Author</th>");
	if (ctx.repo->enable_log_filecount) {
		html("<th class='left'>Files</th>");
		columns++;
		if (ctx.repo->enable_log_linecount) {
			html("<th class='left'>Lines</th>");
			columns++;
		}
	}
	html("</tr>\n");

	if (ofs<0)
		ofs = 0;

	for (i = 0; i < ofs && (commit = get_revision(&rev)) != NULL; i++) {
		free(commit->buffer);
		commit->buffer = NULL;
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}

	for (i = 0; i < cnt && (commit = get_revision(&rev)) != NULL; i++) {
		print_commit(commit);
		free(commit->buffer);
		commit->buffer = NULL;
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}
	if (pager) {
		htmlf("</table><div class='pager'>",
		     columns);
		if (ofs > 0) {
			cgit_log_link("[prev]", NULL, NULL, ctx.qry.head,
				      ctx.qry.sha1, ctx.qry.path,
				      ofs - cnt, ctx.qry.grep,
				      ctx.qry.search, ctx.qry.showmsg);
			html("&nbsp;");
		}
		if ((commit = get_revision(&rev)) != NULL) {
			cgit_log_link("[next]", NULL, NULL, ctx.qry.head,
				      ctx.qry.sha1, ctx.qry.path,
				      ofs + cnt, ctx.qry.grep,
				      ctx.qry.search, ctx.qry.showmsg);
		}
		html("</div>");
	} else if ((commit = get_revision(&rev)) != NULL) {
		html("<tr class='nohover'><td colspan='3'>");
		cgit_log_link("[...]", NULL, NULL, ctx.qry.head, NULL, NULL, 0,
			      NULL, NULL, ctx.qry.showmsg);
		html("</td></tr>\n");
	}
}
