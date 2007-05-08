/* ui-view.c: functions to output _any_ object, given it's sha1
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

void cgit_print_view(const char *hex, char *path)
{
	unsigned char sha1[20];
	enum object_type type;
	unsigned char *buf;
	unsigned long size;

	if (get_sha1_hex(hex, sha1)){
		cgit_print_error(fmt("Bad hex value: %s", hex));
	        return;
	}

	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		cgit_print_error(fmt("Bad object name: %s", hex));
		return;
	}

	buf = read_sha1_file(sha1, &type, &size);
	if (!buf) {
		cgit_print_error(fmt("Error reading object %s", hex));
		return;
	}

	buf[size] = '\0';
	html("<table class='list'>\n");
	html("<tr class='nohover'><th class='left'>");
	if (path)
		htmlf("%s (", path);
	htmlf("%s %s, %li bytes", typename(type), hex, size);
	if (path)
		html(")");
	html("</th></tr>\n");
	html("<tr><td class='blob'>\n");
	html_txt(buf);
	html("\n</td></tr>\n");
	html("</table>\n");
}
