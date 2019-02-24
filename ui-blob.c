/* ui-blob.c: show blob content
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-blob.h"
#include "html.h"
#include "ui-shared.h"

struct walk_tree_context {
	const char *match_path;
	struct object_id *matched_oid;
	unsigned int found_path:1;
	unsigned int file_only:1;
};

static int walk_tree(const struct object_id *oid, struct strbuf *base,
		const char *pathname, unsigned mode, int stage, void *cbdata)
{
	struct walk_tree_context *walk_tree_ctx = cbdata;

	if (walk_tree_ctx->file_only && !S_ISREG(mode))
		return READ_TREE_RECURSIVE;
	if (strncmp(base->buf, walk_tree_ctx->match_path, base->len)
		|| strcmp(walk_tree_ctx->match_path + base->len, pathname))
		return READ_TREE_RECURSIVE;
	oidcpy(walk_tree_ctx->matched_oid, oid);
	walk_tree_ctx->found_path = 1;
	return 0;
}

int cgit_ref_path_exists(const char *path, const char *ref, int file_only)
{
	struct object_id oid;
	unsigned long size;
	struct pathspec_item path_items = {
		.match = xstrdup(path),
		.len = strlen(path)
	};
	struct pathspec paths = {
		.nr = 1,
		.items = &path_items
	};
	struct walk_tree_context walk_tree_ctx = {
		.match_path = path,
		.matched_oid = &oid,
		.found_path = 0,
		.file_only = file_only
	};

	if (get_oid(ref, &oid))
		goto done;
	if (oid_object_info(the_repository, &oid, &size) != OBJ_COMMIT)
		goto done;
	read_tree_recursive(the_repository, lookup_commit_reference(the_repository, &oid)->maybe_tree,
		"", 0, 0, &paths, walk_tree, &walk_tree_ctx);

done:
	free(path_items.match);
	return walk_tree_ctx.found_path;
}

int cgit_print_file(char *path, const char *head, int file_only)
{
	struct object_id oid;
	enum object_type type;
	char *buf;
	unsigned long size;
	struct commit *commit;
	struct pathspec_item path_items = {
		.match = path,
		.len = strlen(path)
	};
	struct pathspec paths = {
		.nr = 1,
		.items = &path_items
	};
	struct walk_tree_context walk_tree_ctx = {
		.match_path = path,
		.matched_oid = &oid,
		.found_path = 0,
		.file_only = file_only
	};

	if (get_oid(head, &oid))
		return -1;
	type = oid_object_info(the_repository, &oid, &size);
	if (type == OBJ_COMMIT) {
		commit = lookup_commit_reference(the_repository, &oid);
		read_tree_recursive(the_repository, commit->maybe_tree,
			"", 0, 0, &paths, walk_tree, &walk_tree_ctx);
		if (!walk_tree_ctx.found_path)
			return -1;
		type = oid_object_info(the_repository, &oid, &size);
	}
	if (type == OBJ_BAD)
		return -1;
	buf = read_object_file(&oid, &type, &size);
	if (!buf)
		return -1;
	buf[size] = '\0';
	html_raw(buf, size);
	free(buf);
	return 0;
}

void cgit_print_blob(const char *hex, char *path, const char *head, int file_only)
{
	struct object_id oid;
	enum object_type type;
	char *buf;
	unsigned long size;
	struct commit *commit;
	struct pathspec_item path_items = {
		.match = path,
		.len = path ? strlen(path) : 0
	};
	struct pathspec paths = {
		.nr = 1,
		.items = &path_items
	};
	struct walk_tree_context walk_tree_ctx = {
		.match_path = path,
		.matched_oid = &oid,
		.found_path = 0,
		.file_only = file_only
	};

	if (hex) {
		if (get_oid_hex(hex, &oid)) {
			cgit_print_error_page(400, "Bad request",
					"Bad hex value: %s", hex);
			return;
		}
	} else {
		if (get_oid(head, &oid)) {
			cgit_print_error_page(404, "Not found",
					"Bad ref: %s", head);
			return;
		}
	}

	type = oid_object_info(the_repository, &oid, &size);

	if ((!hex) && type == OBJ_COMMIT && path) {
		commit = lookup_commit_reference(the_repository, &oid);
		read_tree_recursive(the_repository, commit->maybe_tree,
			"", 0, 0, &paths, walk_tree, &walk_tree_ctx);
		type = oid_object_info(the_repository, &oid, &size);
	}

	if (type == OBJ_BAD) {
		cgit_print_error_page(404, "Not found",
				"Bad object name: %s", hex);
		return;
	}

	buf = read_object_file(&oid, &type, &size);
	if (!buf) {
		cgit_print_error_page(500, "Internal server error",
				"Error reading object %s", hex);
		return;
	}

	buf[size] = '\0';
	if (buffer_is_binary(buf, size))
		ctx.page.mimetype = "application/octet-stream";
	else
		ctx.page.mimetype = "text/plain";
	ctx.page.filename = path;

	html("X-Content-Type-Options: nosniff\n");
	html("Content-Security-Policy: default-src 'none'\n");
	cgit_print_http_headers();
	html_raw(buf, size);
	free(buf);
}
