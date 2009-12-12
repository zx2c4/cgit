/* ui-blob.c: show blob content
 *
 * Copyright (C) 2008 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

static char *match_path;
static unsigned char *matched_sha1;

static int walk_tree(const unsigned char *sha1, const char *base,int baselen,
	const char *pathname, unsigned mode, int stage, void *cbdata) {
	if(strncmp(base,match_path,baselen)
		|| strcmp(match_path+baselen,pathname) )
		return READ_TREE_RECURSIVE;
	memmove(matched_sha1,sha1,20);
	return 0;
}

void cgit_print_blob(const char *hex, char *path, const char *head)
{

	unsigned char sha1[20];
	enum object_type type;
	char *buf;
	unsigned long size;
	struct commit *commit;
	const char *paths[] = {path, NULL};

	if (hex) {
		if (get_sha1_hex(hex, sha1)){
			cgit_print_error(fmt("Bad hex value: %s", hex));
			return;
		}
	} else {
		if (get_sha1(head,sha1)) {
			cgit_print_error(fmt("Bad ref: %s", head));
			return;
		}
	}

	type = sha1_object_info(sha1, &size);

	if((!hex) && type == OBJ_COMMIT && path) {
		commit = lookup_commit_reference(sha1);
		match_path = path;
		matched_sha1 = sha1;
		read_tree_recursive(commit->tree, "", 0, 0, paths, walk_tree, NULL);
		type = sha1_object_info(sha1,&size);
	}

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
	ctx.page.mimetype = ctx.qry.mimetype;
	if (!ctx.page.mimetype) {
		if (buffer_is_binary(buf, size))
			ctx.page.mimetype = "application/octet-stream";
		else
			ctx.page.mimetype = "text/plain";
	}
	ctx.page.filename = path;
	cgit_print_http_headers(&ctx);
	write(htmlfd, buf, size);
}
