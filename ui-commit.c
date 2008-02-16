/* ui-commit.c: generate commit view
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

static int files, slots;
static int total_adds, total_rems, max_changes;
static int lines_added, lines_removed;
static char *curr_rev;

static struct fileinfo {
	char status;
	unsigned char old_sha1[20];
	unsigned char new_sha1[20];
	unsigned short old_mode;
	unsigned short new_mode;
	char *old_path;
	char *new_path;
	unsigned int added;
	unsigned int removed;
} *items;


void print_fileinfo(struct fileinfo *info)
{
	char *class;

	switch (info->status) {
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
		die("bug: unhandled diff status %c", info->status);
	}

	html("<tr>");
	htmlf("<td class='mode'>");
	if (is_null_sha1(info->new_sha1)) {
		html_filemode(info->old_mode);
	} else {
		html_filemode(info->new_mode);
	}

	if (info->old_mode != info->new_mode &&
	    !is_null_sha1(info->old_sha1) &&
	    !is_null_sha1(info->new_sha1)) {
		html("<span class='modechange'>[");
		html_filemode(info->old_mode);
		html("]</span>");
	}
	htmlf("</td><td class='%s'>", class);
	cgit_diff_link(info->new_path, NULL, NULL, ctx.qry.head, curr_rev,
		       NULL, info->new_path);
	if (info->status == DIFF_STATUS_COPIED || info->status == DIFF_STATUS_RENAMED)
		htmlf(" (%s from %s)",
		      info->status == DIFF_STATUS_COPIED ? "copied" : "renamed",
		      info->old_path);
	html("</td><td class='right'>");
	htmlf("%d", info->added + info->removed);
	html("</td><td class='graph'>");
	htmlf("<table summary='file diffstat' width='%d%%'><tr>", (max_changes > 100 ? 100 : max_changes));
	htmlf("<td class='add' style='width: %.1f%%;'/>",
	      info->added * 100.0 / max_changes);
	htmlf("<td class='rem' style='width: %.1f%%;'/>",
	      info->removed * 100.0 / max_changes);
	htmlf("<td class='none' style='width: %.1f%%;'/>",
	      (max_changes - info->removed - info->added) * 100.0 / max_changes);
	html("</tr></table></td></tr>\n");
}

void cgit_count_diff_lines(char *line, int len)
{
	if (line && (len > 0)) {
		if (line[0] == '+')
			lines_added++;
		else if (line[0] == '-')
			lines_removed++;
	}
}

void inspect_filepair(struct diff_filepair *pair)
{
	files++;
	lines_added = 0;
	lines_removed = 0;
	cgit_diff_files(pair->one->sha1, pair->two->sha1, cgit_count_diff_lines);
	if (files >= slots) {
		if (slots == 0)
			slots = 4;
		else
			slots = slots * 2;
		items = xrealloc(items, slots * sizeof(struct fileinfo));
	}
	items[files-1].status = pair->status;
	hashcpy(items[files-1].old_sha1, pair->one->sha1);
	hashcpy(items[files-1].new_sha1, pair->two->sha1);
	items[files-1].old_mode = pair->one->mode;
	items[files-1].new_mode = pair->two->mode;
	items[files-1].old_path = xstrdup(pair->one->path);
	items[files-1].new_path = xstrdup(pair->two->path);
	items[files-1].added = lines_added;
	items[files-1].removed = lines_removed;
	if (lines_added + lines_removed > max_changes)
		max_changes = lines_added + lines_removed;
	total_adds += lines_added;
	total_rems += lines_removed;
}


void cgit_print_commit(char *hex)
{
	struct commit *commit, *parent;
	struct commitinfo *info;
	struct commit_list *p;
	unsigned char sha1[20];
	char *tmp;
	int i;

	if (!hex)
		hex = ctx.qry.head;
	curr_rev = hex;

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
	if (cgit_repo->snapshots) {
		html("<tr><th>download</th><td colspan='2' class='sha1'>");
		cgit_print_snapshot_links(ctx.qry.repo, ctx.qry.head,
					  hex, cgit_repo->snapshots);
		html("</td></tr>");
	}
	html("</table>\n");
	html("<div class='commit-subject'>");
	html_txt(info->subject);
	html("</div>");
	html("<div class='commit-msg'>");
	html_txt(info->msg);
	html("</div>");
	if (!(commit->parents && commit->parents->next && commit->parents->next->next)) {
		html("<div class='diffstat-header'>Diffstat</div>");
		html("<table summary='diffstat' class='diffstat'>");
		max_changes = 0;
		cgit_diff_commit(commit, inspect_filepair);
		for(i = 0; i<files; i++)
			print_fileinfo(&items[i]);
		html("</table>");
		html("<div class='diffstat-summary'>");
		htmlf("%d files changed, %d insertions, %d deletions (",
		      files, total_adds, total_rems);
		cgit_diff_link("show diff", NULL, NULL, ctx.qry.head, hex,
			       NULL, NULL);
		html(")</div>");
	}
	cgit_free_commitinfo(info);
}
