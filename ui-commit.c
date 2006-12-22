/* ui-commit.c: generate commit view
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

int files = 0;

void print_filepair(struct diff_filepair *pair)
{
	char *query;
	char *class;
	
	switch (pair->status) {
	case DIFF_STATUS_ADDED:
		class = "add";
		break;
	case DIFF_STATUS_COPIED:
		class = "cpy";
		break;
	case DIFF_STATUS_DELETED:
		class = "del";
		break;
	case DIFF_STATUS_MODIFIED:
		class = "upd";
		break;
	case DIFF_STATUS_RENAMED:
		class = "mov";
		break;
	case DIFF_STATUS_TYPE_CHANGED:
		class = "typ";
		break;
	case DIFF_STATUS_UNKNOWN:
		class = "unk";
		break;
	case DIFF_STATUS_UNMERGED:
		class = "stg";
		break;
	default:
		die("bug: unhandled diff status %c", pair->status);
	}

	html("<tr>");
	htmlf("<td class='mode'>");
	if (is_null_sha1(pair->two->sha1)) {
		html_filemode(pair->one->mode);
	} else {
		html_filemode(pair->two->mode);
	}

	if (pair->one->mode != pair->two->mode && 
	    !is_null_sha1(pair->one->sha1) && 
	    !is_null_sha1(pair->two->sha1)) {
		html("<span class='modechange'>[");
		html_filemode(pair->one->mode);
		html("]</span>");
	}
	htmlf("</td><td class='%s'>", class);
	query = fmt("id=%s&id2=%s", sha1_to_hex(pair->one->sha1), 
		    sha1_to_hex(pair->two->sha1));	
	html_link_open(cgit_pageurl(cgit_query_repo, "diff", query), 
		       NULL, NULL);
	if (pair->status == DIFF_STATUS_COPIED || 
	    pair->status == DIFF_STATUS_RENAMED) {
		html_txt(pair->two->path);
		htmlf("</a> (%s from ", pair->status == DIFF_STATUS_COPIED ? 
		      "copied" : "renamed");
		query = fmt("id=%s", sha1_to_hex(pair->one->sha1));	
		html_link_open(cgit_pageurl(cgit_query_repo, "view", query), 
			       NULL, NULL);
		html_txt(pair->one->path);
		html("</a>)");
	} else {
		html_txt(pair->two->path);
		html("</a>");
	}
	html("<td>");

	//TODO: diffstat graph
	
	html("</td></tr>\n");
	files++;	
}

void diff_format_cb(struct diff_queue_struct *q,
		    struct diff_options *options, void *data)
{
	int i;

	for (i = 0; i < q->nr; i++) {
		if (q->queue[i]->status == 'U')
			continue;
		print_filepair(q->queue[i]);
	}
}

void cgit_diffstat(struct commit *commit)
{
	struct diff_options opt;
	int ret;

	diff_setup(&opt);
	opt.output_format = DIFF_FORMAT_CALLBACK;
	opt.detect_rename = 1;
	opt.recursive = 1;
	opt.format_callback = diff_format_cb;
	diff_setup_done(&opt);
	
	if (commit->parents)
		ret = diff_tree_sha1(commit->parents->item->object.sha1,
				     commit->object.sha1,
				     "", &opt);
	else
		ret = diff_root_tree_sha1(commit->object.sha1, "", &opt);

	diffcore_std(&opt);
	diff_flush(&opt);
}

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
	html("<table class='diffstat'>");
	html("<tr><th colspan='3'>Affected files</tr>\n");
	cgit_diffstat(commit);
	htmlf("<tr><td colspan='3' class='summary'>"
	      "%d file%s changed</td></tr>\n", files, files > 1 ? "s" : "");
	html("</table>");
	cgit_free_commitinfo(info);
}
