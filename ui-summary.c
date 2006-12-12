/* ui-summary.c: functions for generating repo summary page
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

static int cgit_print_branch_cb(const char *refname, const unsigned char *sha1,
				int flags, void *cb_data)
{
	struct commit *commit;
	char buf[256], *url;

	commit = lookup_commit(sha1);
	if (commit && !parse_commit(commit)){
		html("<tr><td>");
		url = cgit_pageurl(cgit_query_repo, "log", 
				   fmt("h=%s", refname));
		html_link_open(url, NULL, NULL);
		strncpy(buf, refname, sizeof(buf));
		html_txt(buf);
		html_link_close();
		html("</td><td>");
		pretty_print_commit(CMIT_FMT_ONELINE, commit, ~0, buf,
				    sizeof(buf), 0, NULL, NULL, 0);
		html_txt(buf);
		html("</td><td><a href='");
		html_attr(cgit_pageurl(cgit_query_repo, "tree", 
				       fmt("id=%s", 
					   sha1_to_hex(commit->tree->object.sha1))));
		html("'>tree</a>");
		html("</td></tr>\n");
	} else {
		html("<tr><td>");
		html_txt(buf);
		html("</td><td>");
		htmlf("*** bad ref %s", sha1_to_hex(sha1));
		html("</td></tr>\n");
	}
	return 0;
}

static void cgit_print_branches()
{
	html("<table class='list'>");
	html("<tr><th>Branch name</th><th>Latest</th><th>Link</th></tr>\n");
	for_each_branch_ref(cgit_print_branch_cb, NULL);
	html("</table>");
}

void cgit_print_summary()
{
	html("<h2>");
	html_txt("Repo summary page");
	html("</h2>");
	cgit_print_branches();
}
