/* ui-blob.c: show blob content
 *
 * Copyright (C) 2008 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"

void cgit_print_blob(const char *hex, char *path)
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
	ctx.page.mimetype = "text/plain";
	ctx.page.filename = path;
	cgit_print_http_headers(&ctx);
	write(htmlfd, buf, size);
}
