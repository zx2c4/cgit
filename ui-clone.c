/* ui-clone.c: functions for http cloning, based on
 * git's http-backend.c by Shawn O. Pearce
 *
 * Copyright (C) 2008 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

static int print_ref_info(const char *refname, const unsigned char *sha1,
                          int flags, void *cb_data)
{
	struct object *obj;

	if (!(obj = parse_object(sha1)))
		return 0;

	if (!strcmp(refname, "HEAD") || !prefixcmp(refname, "refs/heads/"))
		htmlf("%s\t%s\n", sha1_to_hex(sha1), refname);
	else if (!prefixcmp(refname, "refs/tags") && obj->type == OBJ_TAG) {
		if (!(obj = deref_tag(obj, refname, 0)))
			return 0;
		htmlf("%s\t%s\n", sha1_to_hex(sha1), refname);
		htmlf("%s\t%s^{}\n", sha1_to_hex(obj->sha1), refname);
	}
	return 0;
}

static void print_pack_info(struct cgit_context *ctx)
{
	struct packed_git *pack;
	int ofs;

	ctx->page.mimetype = "text/plain";
	ctx->page.filename = "objects/info/packs";
	cgit_print_http_headers(ctx);
	ofs = strlen(ctx->repo->path) + strlen("/objects/pack/");
	prepare_packed_git();
	for (pack = packed_git; pack; pack = pack->next)
		if (pack->pack_local)
			htmlf("P %s\n", pack->pack_name + ofs);
}

static void send_file(struct cgit_context *ctx, char *path)
{
	struct stat st;

	if (stat(path, &st)) {
		switch (errno) {
		case ENOENT:
			html_status(404, "Not found", 0);
			break;
		case EACCES:
			html_status(403, "Forbidden", 0);
			break;
		default:
			html_status(400, "Bad request", 0);
		}
		return;
	}
	ctx->page.mimetype = "application/octet-stream";
	ctx->page.filename = path;
	if (prefixcmp(ctx->repo->path, path))
		ctx->page.filename += strlen(ctx->repo->path) + 1;
	cgit_print_http_headers(ctx);
	html_include(path);
}

void cgit_clone_info(struct cgit_context *ctx)
{
	if (!ctx->qry.path || strcmp(ctx->qry.path, "refs"))
		return;

	ctx->page.mimetype = "text/plain";
	ctx->page.filename = "info/refs";
	cgit_print_http_headers(ctx);
	for_each_ref(print_ref_info, ctx);
}

void cgit_clone_objects(struct cgit_context *ctx)
{
	if (!ctx->qry.path) {
		html_status(400, "Bad request", 0);
		return;
	}

	if (!strcmp(ctx->qry.path, "info/packs")) {
		print_pack_info(ctx);
		return;
	}

	send_file(ctx, git_path("objects/%s", ctx->qry.path));
}

void cgit_clone_head(struct cgit_context *ctx)
{
	send_file(ctx, git_path("%s", "HEAD"));
}
