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

void cgit_print_diff(const char *old_hex, const char *new_hex)
{
	unsigned char sha1[20], sha2[20];

	get_sha1(old_hex, sha1);
	get_sha1(new_hex, sha2);

	html("<table class='diff'><tr><td>");
	if (cgit_diff_files(sha1, sha2, print_line))
		cgit_print_error("Error running diff");
	html("</td></tr></table>");
}
