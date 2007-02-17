/* ui-view.c: functions to output _any_ object, given it's sha1
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

void cgit_print_view(const char *hex)
{
	unsigned char sha1[20];
	char type[20];
	unsigned char *buf;
	unsigned long size;

	if (get_sha1_hex(hex, sha1)){
		cgit_print_error(fmt("Bad hex value: %s", hex));
	        return;
	}

	if (sha1_object_info(sha1, type, &size)){
		cgit_print_error("Bad object name");
		return;
	}

	buf = read_sha1_file(sha1, type, &size);
	if (!buf) {
		cgit_print_error("Error reading object");
		return;
	}

	buf[size] = '\0';
	html("<table class='list'>\n");
	htmlf("<tr class='nohover'><th class='left'>%s %s, %li bytes</th></tr>\n", type, hex, size);
	html("<tr><td class='blob'>\n");
	html_txt(buf);
	html("\n</td></tr>\n");
	html("</table>\n");
}
