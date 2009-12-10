/* ui-plain.c: functions for output of plain blobs by path
 *
 * Copyright (C) 2008 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

char *curr_rev;
char *match_path;
int match;

static void print_object(const unsigned char *sha1, const char *path)
{
	enum object_type type;
	char *buf, *ext;
	unsigned long size;
	struct string_list_item *mime;

	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		html_status(404, "Not found", 0);
		return;
	}

	buf = read_sha1_file(sha1, &type, &size);
	if (!buf) {
		html_status(404, "Not found", 0);
		return;
	}
	ctx.page.mimetype = NULL;
	ext = strrchr(path, '.');
	if (ext && *(++ext)) {
		mime = string_list_lookup(ext, &ctx.cfg.mimetypes);
		if (mime)
			ctx.page.mimetype = (char *)mime->util;
	}
	if (!ctx.page.mimetype) {
		if (buffer_is_binary(buf, size))
			ctx.page.mimetype = "application/octet-stream";
		else
			ctx.page.mimetype = "text/plain";
	}
	ctx.page.filename = fmt("%s", path);
	ctx.page.size = size;
	ctx.page.etag = sha1_to_hex(sha1);
	cgit_print_http_headers(&ctx);
	html_raw(buf, size);
	match = 1;
}

static int walk_tree(const unsigned char *sha1, const char *base, int baselen,
		     const char *pathname, unsigned mode, int stage,
		     void *cbdata)
{
	if (S_ISDIR(mode))
		return READ_TREE_RECURSIVE;

	if (S_ISREG(mode) && !strncmp(base, match_path, baselen) &&
	    !strcmp(pathname, match_path + baselen))
		print_object(sha1, pathname);

	return 0;
}

void cgit_print_plain(struct cgit_context *ctx)
{
	const char *rev = ctx->qry.sha1;
	unsigned char sha1[20];
	struct commit *commit;
	const char *paths[] = {ctx->qry.path, NULL};

	if (!rev)
		rev = ctx->qry.head;

	curr_rev = xstrdup(rev);
	if (get_sha1(rev, sha1)) {
		html_status(404, "Not found", 0);
		return;
	}
	commit = lookup_commit_reference(sha1);
	if (!commit || parse_commit(commit)) {
		html_status(404, "Not found", 0);
		return;
	}
	match_path = ctx->qry.path;
	read_tree_recursive(commit->tree, "", 0, 0, paths, walk_tree, NULL);
	if (!match)
		html_status(404, "Not found", 0);
}
