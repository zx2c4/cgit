/* ui-commit.c: generate commit view
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"
#include "ui-diff.h"

void cgit_print_commit(char *hex)
{
	struct commit *commit, *parent;
	struct commitinfo *info;
	struct commit_list *p;
	unsigned char sha1[20];
	char *tmp;

	if (!hex)
		hex = ctx.qry.head;

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

	html("<table summary='commit info' class='commit-info'>\n");
	html("<tr><th>author</th><td>");
	html_txt(info->author);
	html(" ");
	html_txt(info->author_email);
	html("</td><td class='right'>");
	cgit_print_date(info->author_date, FMT_LONGDATE);
	html("</td></tr>\n");
	html("<tr><th>committer</th><td>");
	html_txt(info->committer);
	html(" ");
	html_txt(info->committer_email);
	html("</td><td class='right'>");
	cgit_print_date(info->committer_date, FMT_LONGDATE);
	html("</td></tr>\n");
	html("<tr><th>commit</th><td colspan='2' class='sha1'>");
	tmp = sha1_to_hex(commit->object.sha1);
	cgit_commit_link(tmp, NULL, NULL, ctx.qry.head, tmp);
	html(" (");
	cgit_patch_link("patch", NULL, NULL, NULL, tmp);
	html(")</td></tr>\n");
	html("<tr><th>tree</th><td colspan='2' class='sha1'>");
	tmp = xstrdup(hex);
	cgit_tree_link(sha1_to_hex(commit->tree->object.sha1), NULL, NULL,
		       ctx.qry.head, tmp, NULL);
	html("</td></tr>\n");
      	for (p = commit->parents; p ; p = p->next) {
		parent = lookup_commit_reference(p->item->object.sha1);
		if (!parent) {
			html("<tr><td colspan='3'>");
			cgit_print_error("Error reading parent commit");
			html("</td></tr>");
			continue;
		}
		html("<tr><th>parent</th>"
		     "<td colspan='2' class='sha1'>");
		cgit_commit_link(sha1_to_hex(p->item->object.sha1), NULL, NULL,
				 ctx.qry.head, sha1_to_hex(p->item->object.sha1));
		html(" (");
		cgit_diff_link("diff", NULL, NULL, ctx.qry.head, hex,
			       sha1_to_hex(p->item->object.sha1), NULL);
		html(")</td></tr>");
	}
	if (ctx.repo->snapshots) {
		html("<tr><th>download</th><td colspan='2' class='sha1'>");
		cgit_print_snapshot_links(ctx.qry.repo, ctx.qry.head,
					  hex, ctx.repo->snapshots);
		html("</td></tr>");
	}
	html("</table>\n");
	html("<div class='commit-subject'>");
	html_txt(info->subject);
	html("</div>");
	html("<div class='commit-msg'>");
	html_txt(info->msg);
	html("</div>");
	if (!(commit->parents && commit->parents->next &&
	      commit->parents->next->next)) {
		tmp = sha1_to_hex(commit->parents->item->object.sha1);
		cgit_print_diff(ctx.qry.sha1, tmp, NULL);
	}
	cgit_free_commitinfo(info);
}
