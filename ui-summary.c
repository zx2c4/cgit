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
	struct commitinfo *info;
	char buf[256], *url;

	strncpy(buf, refname, sizeof(buf));
	commit = lookup_commit(sha1);
	if (commit && !parse_commit(commit)){
		info = cgit_parse_commit(commit);
		html("<tr><td>");
		url = cgit_pageurl(cgit_query_repo, "log", 
				   fmt("h=%s", refname));
		html_link_open(url, NULL, NULL);
		html_txt(buf);
		html_link_close();
		html("</td><td>");
		cgit_print_date(commit->date);
		html("</td><td>");
		html_txt(info->author);
		html("</td><td>");
		url = cgit_pageurl(cgit_query_repo, "commit", 
				   fmt("id=%s", sha1_to_hex(sha1)));
		html_link_open(url, NULL, NULL);
		html_ntxt(80, info->subject);
		html_link_close();
		html("</td></tr>\n");
		cgit_free_commitinfo(info);
	} else {
		html("<tr><td>");
		html_txt(buf);
		html("</td><td colspan='3'>");
		htmlf("*** bad ref %s ***", sha1_to_hex(sha1));
		html("</td></tr>\n");
	}
	return 0;
}

static int cgit_print_tag_cb(const char *refname, const unsigned char *sha1,
				int flags, void *cb_data)
{
	struct tag *tag;
	struct taginfo *info;
	char buf[256], *page, *url;

	strncpy(buf, refname, sizeof(buf));
	tag = lookup_tag(sha1);
	if (tag && !parse_tag(tag) && (info = cgit_parse_tag(tag))){
		html("<tr><td>");
		url = cgit_pageurl(cgit_query_repo, "view", 
				   fmt("id=%s", sha1_to_hex(sha1)));
		html_link_open(url, NULL, NULL);
		html_txt(buf);
		html_link_close();
		html("</td><td>");
		if (info->tagger_date > 0)
			cgit_print_date(info->tagger_date);
		html("</td><td>");
		if (info->tagger)
			html(info->tagger);
		html("</td><td>");
		if (tag->tagged->type == OBJ_COMMIT)
			page = "commit";
		else if (tag->tagged->type == OBJ_TREE)
			page = "tree";
		else
			page = "view";
		
		url = cgit_pageurl(cgit_query_repo, page, 
				   fmt("id=%s", sha1_to_hex(tag->tagged->sha1)));
		html_link_open(url, NULL, NULL);
		htmlf("%s %s", type_names[tag->tagged->type], 
		      sha1_to_hex(tag->tagged->sha1));
		html_link_close();
		html("</td></tr>\n");
	} else {
		html("<tr><td>");
		html_txt(buf);
		html("</td><td colspan='3'>");
		htmlf("*** bad ref %s ***", sha1_to_hex(sha1));
		html("</td></tr>\n");
	}
	return 0;
}

static void cgit_print_branches()
{
	html("<tr><th class='left'>Branch</th>"
	     "<th class='left'>Updated</th>"
	     "<th class='left'>Author</th>"
	     "<th class='left'>Head commit</th></tr>\n");
	for_each_branch_ref(cgit_print_branch_cb, NULL);
}

static void cgit_print_tags()
{
	html("<tr><th class='left'>Tag</th>"
	     "<th class='left'>Created</th>"
	     "<th class='left'>Author</th>"
	     "<th class='left'>Reference</th></tr>\n");
	for_each_tag_ref(cgit_print_tag_cb, NULL);
}

void cgit_print_summary()
{
	html("<h2>");
	html_txt("Repo summary page");
	html("</h2>");
	html("<table class='list nowrap'>");
	cgit_print_branches();
	html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
	cgit_print_tags();
	html("</table>");
}
