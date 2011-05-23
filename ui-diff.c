/* ui-diff.c: show diff between two blobs
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"
#include "ui-ssdiff.h"

unsigned char old_rev_sha1[20];
unsigned char new_rev_sha1[20];

static int files, slots;
static int total_adds, total_rems, max_changes;
static int lines_added, lines_removed;

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
	unsigned long old_size;
	unsigned long new_size;
	int binary:1;
} *items;

static int use_ssdiff = 0;
static struct diff_filepair *current_filepair;

struct diff_filespec *cgit_get_current_old_file(void)
{
	return current_filepair->one;
}

struct diff_filespec *cgit_get_current_new_file(void)
{
	return current_filepair->two;
}

static void print_fileinfo(struct fileinfo *info)
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
		cgit_print_filemode(info->old_mode);
	} else {
		cgit_print_filemode(info->new_mode);
	}

	if (info->old_mode != info->new_mode &&
	    !is_null_sha1(info->old_sha1) &&
	    !is_null_sha1(info->new_sha1)) {
		html("<span class='modechange'>[");
		cgit_print_filemode(info->old_mode);
		html("]</span>");
	}
	htmlf("</td><td class='%s'>", class);
	cgit_diff_link(info->new_path, NULL, NULL, ctx.qry.head, ctx.qry.sha1,
		       ctx.qry.sha2, info->new_path, 0);
	if (info->status == DIFF_STATUS_COPIED || info->status == DIFF_STATUS_RENAMED)
		htmlf(" (%s from %s)",
		      info->status == DIFF_STATUS_COPIED ? "copied" : "renamed",
		      info->old_path);
	html("</td><td class='right'>");
	if (info->binary) {
		htmlf("bin</td><td class='graph'>%ld -> %ld bytes",
		      info->old_size, info->new_size);
		return;
	}
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

static void count_diff_lines(char *line, int len)
{
	if (line && (len > 0)) {
		if (line[0] == '+')
			lines_added++;
		else if (line[0] == '-')
			lines_removed++;
	}
}

static void inspect_filepair(struct diff_filepair *pair)
{
	int binary = 0;
	unsigned long old_size = 0;
	unsigned long new_size = 0;
	files++;
	lines_added = 0;
	lines_removed = 0;
	cgit_diff_files(pair->one->sha1, pair->two->sha1, &old_size, &new_size,
			&binary, 0, ctx.qry.ignorews, count_diff_lines);
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
	items[files-1].old_size = old_size;
	items[files-1].new_size = new_size;
	items[files-1].binary = binary;
	if (lines_added + lines_removed > max_changes)
		max_changes = lines_added + lines_removed;
	total_adds += lines_added;
	total_rems += lines_removed;
}

void cgit_print_diffstat(const unsigned char *old_sha1,
			 const unsigned char *new_sha1, const char *prefix)
{
	int i, save_context = ctx.qry.context;

	html("<div class='diffstat-header'>");
	cgit_diff_link("Diffstat", NULL, NULL, ctx.qry.head, ctx.qry.sha1,
		       ctx.qry.sha2, NULL, 0);
	if (prefix) {
		html(" (limited to '");
		html_txt(prefix);
		html("')");
	}
	html(" (");
	ctx.qry.context = (save_context > 0 ? save_context : 3) << 1;
	cgit_self_link("more", NULL, NULL, &ctx);
	html("/");
	ctx.qry.context = (save_context > 3 ? save_context : 3) >> 1;
	cgit_self_link("less", NULL, NULL, &ctx);
	ctx.qry.context = save_context;
	html(" context)");
	html(" (");
	ctx.qry.ignorews = (ctx.qry.ignorews + 1) % 2;
	cgit_self_link(ctx.qry.ignorews ? "ignore" : "show", NULL, NULL, &ctx);
	ctx.qry.ignorews = (ctx.qry.ignorews + 1) % 2;
	html(" whitespace changes)");
	html("</div>");
	html("<table summary='diffstat' class='diffstat'>");
	max_changes = 0;
	cgit_diff_tree(old_sha1, new_sha1, inspect_filepair, prefix,
		       ctx.qry.ignorews);
	for(i = 0; i<files; i++)
		print_fileinfo(&items[i]);
	html("</table>");
	html("<div class='diffstat-summary'>");
	htmlf("%d files changed, %d insertions, %d deletions",
	      files, total_adds, total_rems);
	html("</div>");
}


/*
 * print a single line returned from xdiff
 */
static void print_line(char *line, int len)
{
	char *class = "ctx";
	char c = line[len-1];

	if (line[0] == '+')
		class = "add";
	else if (line[0] == '-')
		class = "del";
	else if (line[0] == '@')
		class = "hunk";

	htmlf("<div class='%s'>", class);
	line[len-1] = '\0';
	html_txt(line);
	html("</div>");
	line[len-1] = c;
}

static void header(unsigned char *sha1, char *path1, int mode1,
		   unsigned char *sha2, char *path2, int mode2)
{
	char *abbrev1, *abbrev2;
	int subproject;

	subproject = (S_ISGITLINK(mode1) || S_ISGITLINK(mode2));
	html("<div class='head'>");
	html("diff --git a/");
	html_txt(path1);
	html(" b/");
	html_txt(path2);

	if (is_null_sha1(sha1))
		path1 = "dev/null";
	if (is_null_sha1(sha2))
		path2 = "dev/null";

	if (mode1 == 0)
		htmlf("<br/>new file mode %.6o", mode2);

	if (mode2 == 0)
		htmlf("<br/>deleted file mode %.6o", mode1);

	if (!subproject) {
		abbrev1 = xstrdup(find_unique_abbrev(sha1, DEFAULT_ABBREV));
		abbrev2 = xstrdup(find_unique_abbrev(sha2, DEFAULT_ABBREV));
		htmlf("<br/>index %s..%s", abbrev1, abbrev2);
		free(abbrev1);
		free(abbrev2);
		if (mode1 != 0 && mode2 != 0) {
			htmlf(" %.6o", mode1);
			if (mode2 != mode1)
				htmlf("..%.6o", mode2);
		}
		html("<br/>--- a/");
		if (mode1 != 0)
			cgit_tree_link(path1, NULL, NULL, ctx.qry.head,
				       sha1_to_hex(old_rev_sha1), path1);
		else
			html_txt(path1);
		html("<br/>+++ b/");
		if (mode2 != 0)
			cgit_tree_link(path2, NULL, NULL, ctx.qry.head,
				       sha1_to_hex(new_rev_sha1), path2);
		else
			html_txt(path2);
	}
	html("</div>");
}

static void print_ssdiff_link()
{
	if (!strcmp(ctx.qry.page, "diff")) {
		if (use_ssdiff)
			cgit_diff_link("Unidiff", NULL, NULL, ctx.qry.head,
				       ctx.qry.sha1, ctx.qry.sha2, ctx.qry.path, 1);
		else
			cgit_diff_link("Side-by-side diff", NULL, NULL,
				       ctx.qry.head, ctx.qry.sha1,
				       ctx.qry.sha2, ctx.qry.path, 1);
	}
}

static void filepair_cb(struct diff_filepair *pair)
{
	unsigned long old_size = 0;
	unsigned long new_size = 0;
	int binary = 0;
	linediff_fn print_line_fn = print_line;

	current_filepair = pair;
	if (use_ssdiff) {
		cgit_ssdiff_header_begin();
		print_line_fn = cgit_ssdiff_line_cb;
	}
	header(pair->one->sha1, pair->one->path, pair->one->mode,
	       pair->two->sha1, pair->two->path, pair->two->mode);
	if (use_ssdiff)
		cgit_ssdiff_header_end();
	if (S_ISGITLINK(pair->one->mode) || S_ISGITLINK(pair->two->mode)) {
		if (S_ISGITLINK(pair->one->mode))
			print_line_fn(fmt("-Subproject %s", sha1_to_hex(pair->one->sha1)), 52);
		if (S_ISGITLINK(pair->two->mode))
			print_line_fn(fmt("+Subproject %s", sha1_to_hex(pair->two->sha1)), 52);
		if (use_ssdiff)
			cgit_ssdiff_footer();
		return;
	}
	if (cgit_diff_files(pair->one->sha1, pair->two->sha1, &old_size,
			    &new_size, &binary, ctx.qry.context,
			    ctx.qry.ignorews, print_line_fn))
		cgit_print_error("Error running diff");
	if (binary) {
		if (use_ssdiff)
			html("<tr><td colspan='4'>Binary files differ</td></tr>");
		else
			html("Binary files differ");
	}
	if (use_ssdiff)
		cgit_ssdiff_footer();
}

void cgit_print_diff(const char *new_rev, const char *old_rev, const char *prefix)
{
	enum object_type type;
	unsigned long size;
	struct commit *commit, *commit2;

	if (!new_rev)
		new_rev = ctx.qry.head;
	get_sha1(new_rev, new_rev_sha1);
	type = sha1_object_info(new_rev_sha1, &size);
	if (type == OBJ_BAD) {
		cgit_print_error(fmt("Bad object name: %s", new_rev));
		return;
	}
	commit = lookup_commit_reference(new_rev_sha1);
	if (!commit || parse_commit(commit)) {
		cgit_print_error(fmt("Bad commit: %s", sha1_to_hex(new_rev_sha1)));
		return;
	}

	if (old_rev)
		get_sha1(old_rev, old_rev_sha1);
	else if (commit->parents && commit->parents->item)
		hashcpy(old_rev_sha1, commit->parents->item->object.sha1);
	else
		hashclr(old_rev_sha1);

	if (!is_null_sha1(old_rev_sha1)) {
		type = sha1_object_info(old_rev_sha1, &size);
		if (type == OBJ_BAD) {
			cgit_print_error(fmt("Bad object name: %s", sha1_to_hex(old_rev_sha1)));
			return;
		}
		commit2 = lookup_commit_reference(old_rev_sha1);
		if (!commit2 || parse_commit(commit2)) {
			cgit_print_error(fmt("Bad commit: %s", sha1_to_hex(old_rev_sha1)));
			return;
		}
	}

	if ((ctx.qry.ssdiff && !ctx.cfg.ssdiff) || (!ctx.qry.ssdiff && ctx.cfg.ssdiff))
		use_ssdiff = 1;

	print_ssdiff_link();
	cgit_print_diffstat(old_rev_sha1, new_rev_sha1, prefix);

	if (use_ssdiff) {
		html("<table summary='ssdiff' class='ssdiff'>");
	} else {
		html("<table summary='diff' class='diff'>");
		html("<tr><td>");
	}
	cgit_diff_tree(old_rev_sha1, new_rev_sha1, filepair_cb, prefix,
		       ctx.qry.ignorews);
	if (!use_ssdiff)
		html("</td></tr>");
	html("</table>");
}
