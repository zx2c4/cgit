/* ui-plain.c: functions for output of plain blobs by path
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-plain.h"
#include "html.h"
#include "ui-shared.h"

struct walk_tree_context {
	int match_baselen;
	int match;
};

static int print_object(const struct object_id *oid, const char *path)
{
	enum object_type type;
	char *buf, *mimetype;
	unsigned long size;

	type = oid_object_info(the_repository, oid, &size);
	if (type == OBJ_BAD) {
		cgit_print_error_page(404, "Not found", "Not found");
		return 0;
	}

	buf = read_object_file(oid, &type, &size);
	if (!buf) {
		cgit_print_error_page(404, "Not found", "Not found");
		return 0;
	}

	mimetype = get_mimetype_for_filename(path);
	ctx.page.mimetype = mimetype;

	if (!ctx.repo->enable_html_serving) {
		html("X-Content-Type-Options: nosniff\n");
		html("Content-Security-Policy: default-src 'none'\n");
		if (mimetype) {
			/* Built-in white list allows PDF and everything that isn't text/ and application/ */
			if ((!strncmp(mimetype, "text/", 5) || !strncmp(mimetype, "application/", 12)) && strcmp(mimetype, "application/pdf"))
				ctx.page.mimetype = NULL;
		}
	}

	if (!ctx.page.mimetype) {
		if (buffer_is_binary(buf, size)) {
			ctx.page.mimetype = "application/octet-stream";
			ctx.page.charset = NULL;
		} else {
			ctx.page.mimetype = "text/plain";
		}
	}
	ctx.page.filename = path;
	ctx.page.size = size;
	ctx.page.etag = oid_to_hex(oid);
	cgit_print_http_headers();
	html_raw(buf, size);
	free(mimetype);
	free(buf);
	return 1;
}

static char *buildpath(const char *base, int baselen, const char *path)
{
	if (path[0])
		return fmtalloc("%.*s%s/", baselen, base, path);
	else
		return fmtalloc("%.*s/", baselen, base);
}

static void print_dir(const struct object_id *oid, const char *base,
		      int baselen, const char *path)
{
	char *fullpath, *slash;
	size_t len;

	fullpath = buildpath(base, baselen, path);
	slash = (fullpath[0] == '/' ? "" : "/");
	ctx.page.etag = oid_to_hex(oid);
	cgit_print_http_headers();
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
		else {
			free(fullpath);
			fullpath = NULL;
		}
		html("<li>");
		cgit_plain_link("../", NULL, NULL, ctx.qry.head, ctx.qry.sha1,
				fullpath);
		html("</li>\n");
	}
	free(fullpath);
}

static void print_dir_entry(const struct object_id *oid, const char *base,
			    int baselen, const char *path, unsigned mode)
{
	char *fullpath;

	fullpath = buildpath(base, baselen, path);
	if (!S_ISDIR(mode) && !S_ISGITLINK(mode))
		fullpath[strlen(fullpath) - 1] = 0;
	html("  <li>");
	if (S_ISGITLINK(mode)) {
		cgit_submodule_link(NULL, fullpath, oid_to_hex(oid));
	} else
		cgit_plain_link(path, NULL, NULL, ctx.qry.head, ctx.qry.sha1,
				fullpath);
	html("</li>\n");
	free(fullpath);
}

static void print_dir_tail(void)
{
	html(" </ul>\n</body></html>\n");
}

static int walk_tree(const struct object_id *oid, struct strbuf *base,
		const char *pathname, unsigned mode, int stage, void *cbdata)
{
	struct walk_tree_context *walk_tree_ctx = cbdata;

	if (base->len == walk_tree_ctx->match_baselen) {
		if (S_ISREG(mode) || S_ISLNK(mode)) {
			if (print_object(oid, pathname))
				walk_tree_ctx->match = 1;
		} else if (S_ISDIR(mode)) {
			print_dir(oid, base->buf, base->len, pathname);
			walk_tree_ctx->match = 2;
			return READ_TREE_RECURSIVE;
		}
	} else if (base->len < INT_MAX && (int)base->len > walk_tree_ctx->match_baselen) {
		print_dir_entry(oid, base->buf, base->len, pathname, mode);
		walk_tree_ctx->match = 2;
	} else if (S_ISDIR(mode)) {
		return READ_TREE_RECURSIVE;
	}

	return 0;
}

static int basedir_len(const char *path)
{
	char *p = strrchr(path, '/');
	if (p)
		return p - path + 1;
	return 0;
}

void cgit_print_plain(void)
{
	const char *rev = ctx.qry.sha1;
	struct object_id oid;
	struct commit *commit;
	struct pathspec_item path_items = {
		.match = ctx.qry.path,
		.len = ctx.qry.path ? strlen(ctx.qry.path) : 0
	};
	struct pathspec paths = {
		.nr = 1,
		.items = &path_items
	};
	struct walk_tree_context walk_tree_ctx = {
		.match = 0
	};

	if (!rev)
		rev = ctx.qry.head;

	if (get_oid(rev, &oid)) {
		cgit_print_error_page(404, "Not found", "Not found");
		return;
	}
	commit = lookup_commit_reference(the_repository, &oid);
	if (!commit || parse_commit(commit)) {
		cgit_print_error_page(404, "Not found", "Not found");
		return;
	}
	if (!path_items.match) {
		path_items.match = "";
		walk_tree_ctx.match_baselen = -1;
		print_dir(&commit->maybe_tree->object.oid, "", 0, "");
		walk_tree_ctx.match = 2;
	}
	else
		walk_tree_ctx.match_baselen = basedir_len(path_items.match);
	read_tree_recursive(commit->maybe_tree, "", 0, 0, &paths, walk_tree, &walk_tree_ctx);
	if (!walk_tree_ctx.match)
		cgit_print_error_page(404, "Not found", "Not found");
	else if (walk_tree_ctx.match == 2)
		print_dir_tail();
}
