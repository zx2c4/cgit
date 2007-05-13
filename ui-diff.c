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

static void filepair_cb(struct diff_filepair *pair)
{
	html("<tr><th>");
	html_txt(pair->two->path);
	html("</th></tr>");
	html("<tr><td>");
	if (cgit_diff_files(pair->one->sha1, pair->two->sha1, print_line))
		cgit_print_error("Error running diff");
	html("</tr></td>");
}

void cgit_print_diff(const char *old_hex, const char *new_hex, char *path)
{
	unsigned char sha1[20], sha2[20];
	enum object_type type;
	unsigned long size;

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
		if (path)
			htmlf("<tr><th>%s</th></tr>", path);
		html("<tr><td>");
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
