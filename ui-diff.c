/* ui-diff.c: show diff between two blobs
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"


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

	subproject = (S_ISDIRLNK(mode1) || S_ISDIRLNK(mode2));
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
		html_txt(path1);
		html("<br/>+++ b/");
		html_txt(path2);
	}
	html("</div>");
}

static void filepair_cb(struct diff_filepair *pair)
{
	header(pair->one->sha1, pair->one->path, pair->one->mode,
	       pair->two->sha1, pair->two->path, pair->two->mode);
	if (S_ISDIRLNK(pair->one->mode) || S_ISDIRLNK(pair->two->mode)) {
		if (S_ISDIRLNK(pair->one->mode))
			print_line(fmt("-Subproject %s", sha1_to_hex(pair->one->sha1)), 52);
		if (S_ISDIRLNK(pair->two->mode))
			print_line(fmt("+Subproject %s", sha1_to_hex(pair->two->sha1)), 52);
		return;
	}
	if (cgit_diff_files(pair->one->sha1, pair->two->sha1, print_line))
		cgit_print_error("Error running diff");
}

void cgit_print_diff(const char *new_rev, const char *old_rev)
{
	unsigned char sha1[20], sha2[20];
	enum object_type type;
	unsigned long size;
	struct commit *commit, *commit2;

	if (!new_rev)
		new_rev = cgit_query_head;
	get_sha1(new_rev, sha1);
	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		cgit_print_error(fmt("Bad object name: %s", new_rev));
		return;
	}
	if (type != OBJ_COMMIT) {
		cgit_print_error(fmt("Unhandled object type: %s",
				     typename(type)));
		return;
	}

	commit = lookup_commit_reference(sha1);
	if (!commit || parse_commit(commit))
		cgit_print_error(fmt("Bad commit: %s", sha1_to_hex(sha1)));

	if (old_rev)
		get_sha1(old_rev, sha2);
	else if (commit->parents && commit->parents->item)
		hashcpy(sha2, commit->parents->item->object.sha1);
	else
		hashclr(sha2);

	if (!is_null_sha1(sha2)) {
		type = sha1_object_info(sha2, &size);
		if (type == OBJ_BAD) {
			cgit_print_error(fmt("Bad object name: %s", sha1_to_hex(sha2)));
			return;
		}
		commit2 = lookup_commit_reference(sha2);
		if (!commit2 || parse_commit(commit2))
			cgit_print_error(fmt("Bad commit: %s", sha1_to_hex(sha2)));
	}

	html("<table class='diff'>");
	html("<tr><td>");
	cgit_diff_tree(sha2, sha1, filepair_cb);
	html("</td></tr>");
	html("</table>");
}
