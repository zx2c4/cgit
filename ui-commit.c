/* ui-commit.c: generate commit view
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

void cgit_print_commit(const char *hex)
{
	struct commit *commit;
	struct commitinfo *info;
	struct commit_list *p;
	unsigned char sha1[20];
	char *query;

	if (get_sha1(hex, sha1)) {
		cgit_print_error(fmt("Bad object id: %s", hex));
		return;
	}
	commit = lookup_commit_reference(sha1);
	if (!commit) {
		cgit_print_error(fmt("Bad commit reference: %s", hex));
		return;
	}
	info = cgit_parse_commit(commit);

	html("<table class='commit-info'>\n");
	html("<tr><th>author</th><td>");
	html_txt(info->author);
	html(" ");
	html_txt(info->author_email);
	html("</td><td class='right'>");
	cgit_print_date(info->author_date);
	html("</td></tr>\n");
	html("<tr><th>committer</th><td>");
	html_txt(info->committer);
	html(" ");
	html_txt(info->committer_email);
	html("</td><td class='right'>");
	cgit_print_date(info->committer_date);
	html("</td></tr>\n");
	html("<tr><th>tree</th><td colspan='2' class='sha1'><a href='");
	query = fmt("id=%s", sha1_to_hex(commit->tree->object.sha1));
	html_attr(cgit_pageurl(cgit_query_repo, "tree", query));
	htmlf("'>%s</a></td></tr>\n", sha1_to_hex(commit->tree->object.sha1));
      	for (p = commit->parents; p ; p = p->next) {
		html("<tr><th>parent</th>"
		     "<td colspan='2' class='sha1'>"
		     "<a href='");
		query = fmt("id=%s", sha1_to_hex(p->item->object.sha1));
		html_attr(cgit_pageurl(cgit_query_repo, "commit", query));
		htmlf("'>%s</a></td></tr>\n", 
		      sha1_to_hex(p->item->object.sha1));
	}
	html("</table>\n");
	html("<div class='commit-subject'>");
	html_txt(info->subject);
	html("</div>");
	html("<div class='commit-msg'>");
	html_txt(info->msg);
	html("</div>");
	cgit_free_commitinfo(info);
}
