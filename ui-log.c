/* ui-log.c: functions for log output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

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
	files++;
	if (cgit_repo->enable_log_linecount)
		cgit_diff_files(pair->one->sha1, pair->two->sha1, count_lines);
}

void print_commit(struct commit *commit)
{
	struct commitinfo *info;

	info = cgit_parse_commit(commit);
	html("<tr><td>");
	cgit_print_age(commit->date, TM_WEEK * 2, FMT_SHORTDATE);
	html("</td><td>");
	cgit_commit_link(info->subject, NULL, NULL, cgit_query_head,
			 sha1_to_hex(commit->object.sha1));
	if (cgit_repo->enable_log_filecount) {
		files = 0;
		add_lines = 0;
		rem_lines = 0;
		cgit_diff_commit(commit, inspect_files);
		html("</td><td class='right'>");
		htmlf("%d", files);
		if (cgit_repo->enable_log_linecount) {
			html("</td><td class='right'>");
			htmlf("-%d/+%d", rem_lines, add_lines);
		}
	}
	html("</td><td>");
	html_txt(info->author);
	html("</td></tr>\n");
	cgit_free_commitinfo(info);
}


void cgit_print_log(const char *tip, int ofs, int cnt, char *grep, char *pattern, char *path, int pager)
{
	struct rev_info rev;
	struct commit *commit;
	const char *argv[] = {NULL, tip, NULL, NULL, NULL};
	int argc = 2;
	int i;

	if (!tip)
		argv[1] = cgit_query_head;

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
	if (rev.grep_filter) {
		rev.grep_filter->regflags |= REG_ICASE;
		compile_grep_patterns(rev.grep_filter);
	}
	prepare_revision_walk(&rev);

	html("<table class='list nowrap'>");
	html("<tr class='nohover'><th class='left'>Age</th>"
	     "<th class='left'>Message</th>");

	if (cgit_repo->enable_log_filecount) {
		html("<th class='right'>Files</th>");
		if (cgit_repo->enable_log_linecount)
			html("<th class='right'>Lines</th>");
	}
	html("<th class='left'>Author</th></tr>\n");

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
	html("</table>\n");

	if (pager) {
		html("<div class='pager'>");
		if (ofs > 0) {
			cgit_log_link("[prev]", NULL, NULL, cgit_query_head,
				      cgit_query_sha1, cgit_query_path,
				      ofs - cnt, cgit_query_grep,
				      cgit_query_search);
			html("&nbsp;");
		}
		if ((commit = get_revision(&rev)) != NULL) {
			cgit_log_link("[next]", NULL, NULL, cgit_query_head,
				      cgit_query_sha1, cgit_query_path,
				      ofs + cnt, cgit_query_grep,
				      cgit_query_search);
		}
		html("</div>");
	}
}
