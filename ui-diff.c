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

static void header(unsigned char *sha1, char *path1,
		   unsigned char *sha2, char *path2)
{
	char *abbrev1, *abbrev2;
	if (is_null_sha1(sha1))
		path1 = "dev/null";
	if (is_null_sha1(sha2))
		path2 = "dev/null";
	html("<tr><td>");
	html("<div class='head'>");
	html("diff --git a/");
	html_txt(path1);
	html(" b/");
	html_txt(path2);
	abbrev1 = xstrdup(find_unique_abbrev(sha1, DEFAULT_ABBREV));
	abbrev2 = xstrdup(find_unique_abbrev(sha2, DEFAULT_ABBREV));
	htmlf("\nindex %s..%s", abbrev1, abbrev2);
	free(abbrev1);
	free(abbrev2);
	html("\n--- a/");
	html_txt(path1);
	html("\n+++ b/");
	html_txt(path2);
	html("</div>");
}

static void filepair_cb(struct diff_filepair *pair)
{
	header(pair->one->sha1, pair->one->path,
	       pair->two->sha1, pair->two->path);
	if (cgit_diff_files(pair->one->sha1, pair->two->sha1, print_line))
		cgit_print_error("Error running diff");
	html("</tr></td>");
}

void cgit_print_diff(const char *head, const char *old_hex, const char *new_hex, char *path)
{
	unsigned char sha1[20], sha2[20];
	enum object_type type;
	unsigned long size;
	struct commit *commit;

	if (head && !old_hex && !new_hex) {
		get_sha1(head, sha1);
		commit = lookup_commit_reference(sha1);
		if (commit && !parse_commit(commit)) {
			html("<table class='diff'>");
			cgit_diff_commit(commit, filepair_cb);
			html("</td></tr></table>");
		}
		return;
	}

	get_sha1(old_hex, sha1);
	get_sha1(new_hex, sha2);

	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		type = sha1_object_info(sha2, &size);
		if (type == OBJ_BAD) {
			cgit_print_error(fmt("Bad object names: %s, %s", old_hex, new_hex));
			return;
		}
	}

	html("<table class='diff'>");
	switch(type) {
	case OBJ_BLOB:
		html("<tr><td>");
		header(sha1, path, sha2, path);
		if (cgit_diff_files(sha1, sha2, print_line))
			cgit_print_error("Error running diff");
		html("</tr></td>");
		break;
	case OBJ_TREE:
		cgit_diff_tree(sha1, sha2, filepair_cb);
		break;
	default:
		cgit_print_error(fmt("Unhandled object type: %s",
				     typename(type)));
		break;
	}
	html("</td></tr></table>");
}
