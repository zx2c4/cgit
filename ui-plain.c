/* ui-plain.c: functions for output of plain blobs by path
 *
 * Copyright (C) 2008 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include <stdio.h>
#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

int match_baselen;
int match;

static char *get_mimetype_from_file(const char *filename, const char *ext)
{
	static const char *delimiters;
	char *result;
	FILE *fd;
	char line[1024];
	char *mimetype;
	char *token;

	if (!filename)
		return NULL;

	fd = fopen(filename, "r");
	if (!fd)
		return NULL;

	delimiters = " \t\r\n";
	result = NULL;

	/* loop over all lines in the file */
	while (!result && fgets(line, sizeof(line), fd)) {
		mimetype = strtok(line, delimiters);

		/* skip empty lines and comment lines */
		if (!mimetype || (mimetype[0] == '#'))
			continue;

		/* loop over all extensions of mimetype */
		while ((token = strtok(NULL, delimiters))) {
			if (!strcasecmp(ext, token)) {
				result = xstrdup(mimetype);
				break;
			}
		}
	}
	fclose(fd);

	return result;
}

static void print_object(const unsigned char *sha1, const char *path)
{
	enum object_type type;
	char *buf, *ext;
	unsigned long size;
	struct string_list_item *mime;
	int freemime;

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
	freemime = 0;
	if (ext && *(++ext)) {
		mime = string_list_lookup(&ctx.cfg.mimetypes, ext);
		if (mime) {
			ctx.page.mimetype = (char *)mime->util;
		} else {
			ctx.page.mimetype = get_mimetype_from_file(ctx.cfg.mimetype_file, ext);
			if (ctx.page.mimetype)
				freemime = 1;
		}
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
	if (freemime)
		free(ctx.page.mimetype);
}

static char *buildpath(const char *base, int baselen, const char *path)
{
	if (path[0])
		return fmt("%.*s%s/", baselen, base, path);
	else
		return fmt("%.*s/", baselen, base);
}

static void print_dir(const unsigned char *sha1, const char *base,
		      int baselen, const char *path)
{
	char *fullpath, *slash;
	size_t len;

	fullpath = buildpath(base, baselen, path);
	slash = (fullpath[0] == '/' ? "" : "/");
	ctx.page.etag = sha1_to_hex(sha1);
	cgit_print_http_headers(&ctx);
	htmlf("<html><head><title>%s", slash);
	html_txt(fullpath);
	htmlf("</title></head>\n<body>\n<h2>%s", slash);
	html_txt(fullpath);
	html("</h2>\n<ul>\n");
	len = strlen(fullpath);
	if (len > 1) {
		fullpath[len - 1] = 0;
		slash = strrchr(fullpath, '/');
		if (slash)
			*(slash + 1) = 0;
		else
			fullpath = NULL;
		html("<li>");
		cgit_plain_link("../", NULL, NULL, ctx.qry.head, ctx.qry.sha1,
				fullpath);
		html("</li>\n");
	}
	match = 2;
}

static void print_dir_entry(const unsigned char *sha1, const char *base,
			    int baselen, const char *path, unsigned mode)
{
	char *fullpath;

	fullpath = buildpath(base, baselen, path);
	if (!S_ISDIR(mode) && !S_ISGITLINK(mode))
		fullpath[strlen(fullpath) - 1] = 0;
	html("  <li>");
	if (S_ISGITLINK(mode)) {
		cgit_submodule_link(NULL, fullpath, sha1_to_hex(sha1));
	} else
		cgit_plain_link(path, NULL, NULL, ctx.qry.head, ctx.qry.sha1,
				fullpath);
	html("</li>\n");
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
			print_dir(sha1, base, baselen, pathname);
			return READ_TREE_RECURSIVE;
		}
	}
	else if (baselen > match_baselen)
		print_dir_entry(sha1, base, baselen, pathname, mode);
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
		print_dir(commit->tree->object.sha1, "", 0, "");
	}
	else
		match_baselen = basedir_len(paths[0]);
	read_tree_recursive(commit->tree, "", 0, 0, paths, walk_tree, NULL);
	if (!match)
		html_status(404, "Not found", 0);
	else if (match == 2)
		print_dir_tail();
}
