/* ui-commit.c: generate commit view
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-commit.h"
#include "html.h"
#include "ui-shared.h"
#include "ui-diff.h"
#include "ui-log.h"

void cgit_print_commit(char *hex, const char *prefix)
{
	struct commit *commit, *parent;
	struct commitinfo *info, *parent_info;
	struct commit_list *p;
	struct strbuf notes = STRBUF_INIT;
	struct object_id oid;
	char *tmp, *tmp2;
	int parents = 0;

	if (!hex)
		hex = ctx.qry.head;

	if (get_oid(hex, &oid)) {
		cgit_print_error_page(400, "Bad request",
				"Bad object id: %s", hex);
		return;
	}
	commit = lookup_commit_reference(the_repository, &oid);
	if (!commit) {
		cgit_print_error_page(404, "Not found",
				"Bad commit reference: %s", hex);
		return;
	}
	info = cgit_parse_commit(commit);

	format_display_notes(&oid, &notes, PAGE_ENCODING, 0);

	load_ref_decorations(NULL, DECORATE_FULL_REFS);

	cgit_print_layout_start();
	cgit_print_diff_ctrls();
	html("<table summary='commit info' class='commit-info'>\n");
	html("<tr><th>author</th><td>");
	cgit_open_filter(ctx.repo->email_filter, info->author_email, "commit");
	html_txt(info->author);
	if (!ctx.cfg.noplainemail) {
		html(" ");
		html_txt(info->author_email);
	}
	cgit_close_filter(ctx.repo->email_filter);
	html("</td><td class='right'>");
	html_txt(show_date(info->author_date, info->author_tz,
				cgit_date_mode(DATE_ISO8601)));
	html("</td></tr>\n");
	html("<tr><th>committer</th><td>");
	cgit_open_filter(ctx.repo->email_filter, info->committer_email, "commit");
	html_txt(info->committer);
	if (!ctx.cfg.noplainemail) {
		html(" ");
		html_txt(info->committer_email);
	}
	cgit_close_filter(ctx.repo->email_filter);
	html("</td><td class='right'>");
	html_txt(show_date(info->committer_date, info->committer_tz,
				cgit_date_mode(DATE_ISO8601)));
	html("</td></tr>\n");
	html("<tr><th>commit</th><td colspan='2' class='sha1'>");
	tmp = oid_to_hex(&commit->object.oid);
	cgit_commit_link(tmp, NULL, NULL, ctx.qry.head, tmp, prefix);
	html(" (");
	cgit_patch_link("patch", NULL, NULL, NULL, tmp, prefix);
	html(")</td></tr>\n");
	html("<tr><th>tree</th><td colspan='2' class='sha1'>");
	tmp = xstrdup(hex);
	cgit_tree_link(oid_to_hex(&commit->maybe_tree->object.oid), NULL, NULL,
		       ctx.qry.head, tmp, NULL);
	if (prefix) {
		html(" /");
		cgit_tree_link(prefix, NULL, NULL, ctx.qry.head, tmp, prefix);
	}
	free(tmp);
	html("</td></tr>\n");
	for (p = commit->parents; p; p = p->next) {
		parent = lookup_commit_reference(the_repository, &p->item->object.oid);
		if (!parent) {
			html("<tr><td colspan='3'>");
			cgit_print_error("Error reading parent commit");
			html("</td></tr>");
			continue;
		}
		html("<tr><th>parent</th>"
		     "<td colspan='2' class='sha1'>");
		tmp = tmp2 = oid_to_hex(&p->item->object.oid);
		if (ctx.repo->enable_subject_links) {
			parent_info = cgit_parse_commit(parent);
			tmp2 = parent_info->subject;
		}
		cgit_commit_link(tmp2, NULL, NULL, ctx.qry.head, tmp, prefix);
		html(" (");
		cgit_diff_link("diff", NULL, NULL, ctx.qry.head, hex,
			       oid_to_hex(&p->item->object.oid), prefix);
		html(")</td></tr>");
		parents++;
	}
	if (ctx.repo->snapshots) {
		html("<tr><th>download</th><td colspan='2' class='sha1'>");
		cgit_print_snapshot_links(ctx.repo, hex, "<br/>");
		html("</td></tr>");
	}
	html("</table>\n");
	html("<div class='commit-subject'>");
	cgit_open_filter(ctx.repo->commit_filter);
	html_txt(info->subject);
	cgit_close_filter(ctx.repo->commit_filter);
	show_commit_decorations(commit);
	html("</div>");
	html("<div class='commit-msg'>");
	cgit_open_filter(ctx.repo->commit_filter);
	html_txt(info->msg);
	cgit_close_filter(ctx.repo->commit_filter);
	html("</div>");
	if (notes.len != 0) {
		html("<div class='notes-header'>Notes</div>");
		html("<div class='notes'>");
		cgit_open_filter(ctx.repo->commit_filter);
		html_txt(notes.buf);
		cgit_close_filter(ctx.repo->commit_filter);
		html("</div>");
		html("<div class='notes-footer'></div>");
	}
	if (parents < 3) {
		if (parents)
			tmp = oid_to_hex(&commit->parents->item->object.oid);
		else
			tmp = NULL;
		cgit_print_diff(ctx.qry.sha1, tmp, prefix, 0, 0);
	}
	strbuf_release(&notes);
	cgit_free_commitinfo(info);
	cgit_print_layout_end();
}
