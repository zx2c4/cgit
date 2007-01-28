/* ui-log.c: functions for log output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

void print_commit(struct commit *commit)
{
	char buf[32];
	struct commitinfo *info;
	struct tm *time;

	info = cgit_parse_commit(commit);
	time = gmtime(&commit->date);
	html("<tr><td>");
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", time);
	html_txt(buf);
	html("</td><td>");
	char *qry = fmt("id=%s", sha1_to_hex(commit->object.sha1));
	char *url = cgit_pageurl(cgit_query_repo, "commit", qry);
	html_link_open(url, NULL, NULL);
	html_ntxt(cgit_max_msg_len, info->subject);
	html_link_close();
	html("</td><td>");
	html_txt(info->author);
	html("</td></tr>\n");
	cgit_free_commitinfo(info);
}


void cgit_print_log(const char *tip, int ofs, int cnt, char *grep)
{
	struct rev_info rev;
	struct commit *commit;
	const char *argv[3] = {NULL, tip, NULL};
	int argc = 2;
	int i;
	
	if (grep)
		argv[argc++] = fmt("--grep=%s", grep);
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

	html("<h2>Log</h2>");
	html("<table class='list nowrap'>");
	html("<tr class='nohover'><th class='left'>Date</th>"
	     "<th class='left'>Message</th>"
	     "<th class='left'>Author</th></tr>\n");

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

	html("<div class='pager'>");
	if (ofs > 0) {
		html("&nbsp;<a href='");
		html(cgit_pageurl(cgit_query_repo, cgit_query_page,
				  fmt("h=%s&ofs=%d", tip, ofs-cnt)));
		html("'>[prev]</a>&nbsp;");
       	}

	if ((commit = get_revision(&rev)) != NULL) {
		html("&nbsp;<a href='");
		html(cgit_pageurl(cgit_query_repo, "log",
				  fmt("h=%s&ofs=%d", tip, ofs+cnt)));
		html("'>[next]</a>&nbsp;");
	}
	html("</div>");
}

