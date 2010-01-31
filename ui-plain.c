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

int match_baselen;
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

static void print_dir(const unsigned char *sha1, const char *path,
		      const char *base)
{
	char *fullpath;
	if (path[0] || base[0])
		fullpath = fmt("/%s%s/", base, path);
	else
		fullpath = "/";
	ctx.page.etag = sha1_to_hex(sha1);
	cgit_print_http_headers(&ctx);
	htmlf("<html><head><title>%s</title></head>\n<body>\n"
	      " <h2>%s</h2>\n <ul>\n", fullpath, fullpath);
	if (path[0] || base[0])
	      html("  <li><a href=\"../\">../</a></li>\n");
	match = 2;
}

static void print_dir_entry(const unsigned char *sha1, const char *path,
			    unsigned mode)
{
	const char *sep = "";
	if (S_ISDIR(mode))
		sep = "/";
	htmlf("  <li><a href=\"%s%s\">%s%s</a></li>\n", path, sep, path, sep);
	match = 2;
}

static void print_dir_tail(void)
{
	html(" </ul>\n</body></html>\n");
}

static int walk_tree(const unsigned char *sha1, const char *base, int baselen,
		     const char *pathname, unsigned mode, int stage,
		     void *cbdata)
{
	if (baselen == match_baselen) {
		if (S_ISREG(mode))
			print_object(sha1, pathname);
		else if (S_ISDIR(mode)) {
			print_dir(sha1, pathname, base);
			return READ_TREE_RECURSIVE;
		}
	}
	else if (baselen > match_baselen)
		print_dir_entry(sha1, pathname, mode);
	else if (S_ISDIR(mode))
		return READ_TREE_RECURSIVE;

	return 0;
}

static int basedir_len(const char *path)
{
	char *p = strrchr(path, '/');
	if (p)
		return p - path + 1;
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

	if (get_sha1(rev, sha1)) {
		html_status(404, "Not found", 0);
		return;
	}
	commit = lookup_commit_reference(sha1);
	if (!commit || parse_commit(commit)) {
		html_status(404, "Not found", 0);
		return;
	}
	if (!paths[0]) {
		paths[0] = "";
		match_baselen = -1;
		print_dir(commit->tree->object.sha1, "", "");
	}
	else
		match_baselen = basedir_len(paths[0]);
	read_tree_recursive(commit->tree, "", 0, 0, paths, walk_tree, NULL);
	if (!match)
		html_status(404, "Not found", 0);
	else if (match == 2)
		print_dir_tail();
}
