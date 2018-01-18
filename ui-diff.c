/* ui-diff.c: show diff between two blobs
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-diff.h"
#include "html.h"
#include "ui-shared.h"
#include "ui-ssdiff.h"

struct object_id old_rev_oid[1];
struct object_id new_rev_oid[1];

static int files, slots;
static int total_adds, total_rems, max_changes;
static int lines_added, lines_removed;

static struct fileinfo {
	char status;
	struct object_id old_oid[1];
	struct object_id new_oid[1];
	unsigned short old_mode;
	unsigned short new_mode;
	char *old_path;
	char *new_path;
	unsigned int added;
	unsigned int removed;
	unsigned long old_size;
	unsigned long new_size;
	unsigned int binary:1;
} *items;

static int use_ssdiff = 0;
static struct diff_filepair *current_filepair;
static const char *current_prefix;

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
	if (is_null_oid(info->new_oid)) {
		cgit_print_filemode(info->old_mode);
	} else {
		cgit_print_filemode(info->new_mode);
	}

	if (info->old_mode != info->new_mode &&
	    !is_null_oid(info->old_oid) &&
	    !is_null_oid(info->new_oid)) {
		html("<span class='modechange'>[");
		cgit_print_filemode(info->old_mode);
		html("]</span>");
	}
	htmlf("</td><td class='%s'>", class);
	cgit_diff_link(info->new_path, NULL, NULL, ctx.qry.head, ctx.qry.sha1,
		       ctx.qry.sha2, info->new_path);
	if (info->status == DIFF_STATUS_COPIED || info->status == DIFF_STATUS_RENAMED) {
		htmlf(" (%s from ",
		      info->status == DIFF_STATUS_COPIED ? "copied" : "renamed");
		html_txt(info->old_path);
		html(")");
	}
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

static int show_filepair(struct diff_filepair *pair)
{
	/* Always show if we have no limiting prefix. */
	if (!current_prefix)
		return 1;

	/* Show if either path in the pair begins with the prefix. */
	if (starts_with(pair->one->path, current_prefix) ||
	    starts_with(pair->two->path, current_prefix))
		return 1;

	/* Otherwise we don't want to show this filepair. */
	return 0;
}

static void inspect_filepair(struct diff_filepair *pair)
{
	int binary = 0;
	unsigned long old_size = 0;
	unsigned long new_size = 0;

	if (!show_filepair(pair))
		return;

	files++;
	lines_added = 0;
	lines_removed = 0;
	cgit_diff_files(&pair->one->oid, &pair->two->oid, &old_size, &new_size,
			&binary, 0, ctx.qry.ignorews, count_diff_lines);
	if (files >= slots) {
		if (slots == 0)
			slots = 4;
		else
			slots = slots * 2;
		items = xrealloc(items, slots * sizeof(struct fileinfo));
	}
	items[files-1].status = pair->status;
	oidcpy(items[files-1].old_oid, &pair->one->oid);
	oidcpy(items[files-1].new_oid, &pair->two->oid);
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

static void cgit_print_diffstat(const struct object_id *old_oid,
				const struct object_id *new_oid,
				const char *prefix)
{
	int i;

	html("<div class='diffstat-header'>");
	cgit_diff_link("Diffstat", NULL, NULL, ctx.qry.head, ctx.qry.sha1,
		       ctx.qry.sha2, NULL);
	if (prefix) {
		html(" (limited to '");
		html_txt(prefix);
		html("')");
	}
	html("</div>");
	html("<table summary='diffstat' class='diffstat'>");
	max_changes = 0;
	cgit_diff_tree(old_oid, new_oid, inspect_filepair, prefix,
		       ctx.qry.ignorews);
	for (i = 0; i<files; i++)
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

static void header(const struct object_id *oid1, char *path1, int mode1,
		   const struct object_id *oid2, char *path2, int mode2)
{
	char *abbrev1, *abbrev2;
	int subproject;

	subproject = (S_ISGITLINK(mode1) || S_ISGITLINK(mode2));
	html("<div class='head'>");
	html("diff --git a/");
	html_txt(path1);
	html(" b/");
	html_txt(path2);

	if (mode1 == 0)
		htmlf("<br/>new file mode %.6o", mode2);

	if (mode2 == 0)
		htmlf("<br/>deleted file mode %.6o", mode1);

	if (!subproject) {
		abbrev1 = xstrdup(find_unique_abbrev(oid1->hash, DEFAULT_ABBREV));
		abbrev2 = xstrdup(find_unique_abbrev(oid2->hash, DEFAULT_ABBREV));
		htmlf("<br/>index %s..%s", abbrev1, abbrev2);
		free(abbrev1);
		free(abbrev2);
		if (mode1 != 0 && mode2 != 0) {
			htmlf(" %.6o", mode1);
			if (mode2 != mode1)
				htmlf("..%.6o", mode2);
		}
		if (is_null_oid(oid1)) {
			path1 = "dev/null";
			html("<br/>--- /");
		} else
			html("<br/>--- a/");
		if (mode1 != 0)
			cgit_tree_link(path1, NULL, NULL, ctx.qry.head,
				       oid_to_hex(old_rev_oid), path1);
		else
			html_txt(path1);
		if (is_null_oid(oid2)) {
			path2 = "dev/null";
			html("<br/>+++ /");
		} else
			html("<br/>+++ b/");
		if (mode2 != 0)
			cgit_tree_link(path2, NULL, NULL, ctx.qry.head,
				       oid_to_hex(new_rev_oid), path2);
		else
			html_txt(path2);
	}
	html("</div>");
}

static void filepair_cb(struct diff_filepair *pair)
{
	unsigned long old_size = 0;
	unsigned long new_size = 0;
	int binary = 0;
	linediff_fn print_line_fn = print_line;

	if (!show_filepair(pair))
		return;

	current_filepair = pair;
	if (use_ssdiff) {
		cgit_ssdiff_header_begin();
		print_line_fn = cgit_ssdiff_line_cb;
	}
	header(&pair->one->oid, pair->one->path, pair->one->mode,
	       &pair->two->oid, pair->two->path, pair->two->mode);
	if (use_ssdiff)
		cgit_ssdiff_header_end();
	if (S_ISGITLINK(pair->one->mode) || S_ISGITLINK(pair->two->mode)) {
		if (S_ISGITLINK(pair->one->mode))
			print_line_fn(fmt("-Subproject %s", oid_to_hex(&pair->one->oid)), 52);
		if (S_ISGITLINK(pair->two->mode))
			print_line_fn(fmt("+Subproject %s", oid_to_hex(&pair->two->oid)), 52);
		if (use_ssdiff)
			cgit_ssdiff_footer();
		return;
	}
	if (cgit_diff_files(&pair->one->oid, &pair->two->oid, &old_size,
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

void cgit_print_diff_ctrls(void)
{
	int i, curr;

	html("<div class='cgit-panel'>");
	html("<b>diff options</b>");
	html("<form method='get'>");
	cgit_add_hidden_formfields(1, 0, ctx.qry.page);
	html("<table>");
	html("<tr><td colspan='2'/></tr>");
	html("<tr>");
	html("<td class='label'>context:</td>");
	html("<td class='ctrl'>");
	html("<select name='context' onchange='this.form.submit();'>");
	curr = ctx.qry.context;
	if (!curr)
		curr = 3;
	for (i = 1; i <= 10; i++)
		html_intoption(i, fmt("%d", i), curr);
	for (i = 15; i <= 40; i += 5)
		html_intoption(i, fmt("%d", i), curr);
	html("</select>");
	html("</td>");
	html("</tr><tr>");
	html("<td class='label'>space:</td>");
	html("<td class='ctrl'>");
	html("<select name='ignorews' onchange='this.form.submit();'>");
	html_intoption(0, "include", ctx.qry.ignorews);
	html_intoption(1, "ignore", ctx.qry.ignorews);
	html("</select>");
	html("</td>");
	html("</tr><tr>");
	html("<td class='label'>mode:</td>");
	html("<td class='ctrl'>");
	html("<select name='dt' onchange='this.form.submit();'>");
	curr = ctx.qry.has_difftype ? ctx.qry.difftype : ctx.cfg.difftype;
	html_intoption(0, "unified", curr);
	html_intoption(1, "ssdiff", curr);
	html_intoption(2, "stat only", curr);
	html("</select></td></tr>");
	html("<tr><td/><td class='ctrl'>");
	html("<noscript><input type='submit' value='reload'/></noscript>");
	html("</td></tr></table>");
	html("</form>");
	html("</div>");
}

void cgit_print_diff(const char *new_rev, const char *old_rev,
		     const char *prefix, int show_ctrls, int raw)
{
	struct commit *commit, *commit2;
	const struct object_id *old_tree_oid, *new_tree_oid;
	diff_type difftype;

	/*
	 * If "follow" is set then the diff machinery needs to examine the
	 * entire commit to detect renames so we must limit the paths in our
	 * own callbacks and not pass the prefix to the diff machinery.
	 */
	if (ctx.qry.follow && ctx.cfg.enable_follow_links) {
		current_prefix = prefix;
		prefix = "";
	} else {
		current_prefix = NULL;
	}

	if (!new_rev)
		new_rev = ctx.qry.head;
	if (get_oid(new_rev, new_rev_oid)) {
		cgit_print_error_page(404, "Not found",
			"Bad object name: %s", new_rev);
		return;
	}
	commit = lookup_commit_reference(new_rev_oid);
	if (!commit || parse_commit(commit)) {
		cgit_print_error_page(404, "Not found",
			"Bad commit: %s", oid_to_hex(new_rev_oid));
		return;
	}
	new_tree_oid = &commit->tree->object.oid;

	if (old_rev) {
		if (get_oid(old_rev, old_rev_oid)) {
			cgit_print_error_page(404, "Not found",
				"Bad object name: %s", old_rev);
			return;
		}
	} else if (commit->parents && commit->parents->item) {
		oidcpy(old_rev_oid, &commit->parents->item->object.oid);
	} else {
		oidclr(old_rev_oid);
	}

	if (!is_null_oid(old_rev_oid)) {
		commit2 = lookup_commit_reference(old_rev_oid);
		if (!commit2 || parse_commit(commit2)) {
			cgit_print_error_page(404, "Not found",
				"Bad commit: %s", oid_to_hex(old_rev_oid));
			return;
		}
		old_tree_oid = &commit2->tree->object.oid;
	} else {
		old_tree_oid = NULL;
	}

	if (raw) {
		struct diff_options diffopt;

		diff_setup(&diffopt);
		diffopt.output_format = DIFF_FORMAT_PATCH;
		diffopt.flags.recursive = 1;
		diff_setup_done(&diffopt);

		ctx.page.mimetype = "text/plain";
		cgit_print_http_headers();
		if (old_tree_oid) {
			diff_tree_oid(old_tree_oid, new_tree_oid, "",
				       &diffopt);
		} else {
			diff_root_tree_oid(new_tree_oid, "", &diffopt);
		}
		diffcore_std(&diffopt);
		diff_flush(&diffopt);

		return;
	}

	difftype = ctx.qry.has_difftype ? ctx.qry.difftype : ctx.cfg.difftype;
	use_ssdiff = difftype == DIFF_SSDIFF;

	if (show_ctrls) {
		cgit_print_layout_start();
		cgit_print_diff_ctrls();
	}

	/*
	 * Clicking on a link to a file in the diff stat should show a diff
	 * of the file, showing the diff stat limited to a single file is
	 * pretty useless.  All links from this point on will be to
	 * individual files, so we simply reset the difftype in the query
	 * here to avoid propagating DIFF_STATONLY to the individual files.
	 */
	if (difftype == DIFF_STATONLY)
		ctx.qry.difftype = ctx.cfg.difftype;

	cgit_print_diffstat(old_rev_oid, new_rev_oid, prefix);

	if (difftype == DIFF_STATONLY)
		return;

	if (use_ssdiff) {
		html("<table summary='ssdiff' class='ssdiff'>");
	} else {
		html("<table summary='diff' class='diff'>");
		html("<tr><td>");
	}
	cgit_diff_tree(old_rev_oid, new_rev_oid, filepair_cb, prefix,
		       ctx.qry.ignorews);
	if (!use_ssdiff)
		html("</td></tr>");
	html("</table>");

	if (show_ctrls)
		cgit_print_layout_end();
}
