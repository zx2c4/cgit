/* ui-tree.c: functions for tree output
 *
 * Copyright (C) 2006-2017 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-tree.h"
#include "html.h"
#include "ui-shared.h"

struct walk_tree_context {
	char *curr_rev;
	char *match_path;
	int state;
};

static void print_text_buffer(const char *name, char *buf, unsigned long size)
{
	unsigned long lineno, idx;
	const char *numberfmt = "<a id='n%1$d' href='#n%1$d'>%1$d</a>\n";

	html("<table summary='blob content' class='blob'>\n");

	if (ctx.cfg.enable_tree_linenumbers) {
		html("<tr><td class='linenumbers'><pre>");
		idx = 0;
		lineno = 0;

		if (size) {
			htmlf(numberfmt, ++lineno);
			while (idx < size - 1) { // skip absolute last newline
				if (buf[idx] == '\n')
					htmlf(numberfmt, ++lineno);
				idx++;
			}
		}
		html("</pre></td>\n");
	}
	else {
		html("<tr>\n");
	}

	if (ctx.repo->source_filter) {
		char *filter_arg = xstrdup(name);
		html("<td class='lines'><pre><code>");
		cgit_open_filter(ctx.repo->source_filter, filter_arg);
		html_raw(buf, size);
		cgit_close_filter(ctx.repo->source_filter);
		free(filter_arg);
		html("</code></pre></td></tr></table>\n");
		return;
	}

	html("<td class='lines'><pre><code>");
	html_txt(buf);
	html("</code></pre></td></tr></table>\n");
}

#define ROWLEN 32

static void print_binary_buffer(char *buf, unsigned long size)
{
	unsigned long ofs, idx;
	static char ascii[ROWLEN + 1];

	html("<table summary='blob content' class='bin-blob'>\n");
	html("<tr><th>ofs</th><th>hex dump</th><th>ascii</th></tr>");
	for (ofs = 0; ofs < size; ofs += ROWLEN, buf += ROWLEN) {
		htmlf("<tr><td class='right'>%04lx</td><td class='hex'>", ofs);
		for (idx = 0; idx < ROWLEN && ofs + idx < size; idx++)
			htmlf("%*s%02x",
			      idx == 16 ? 4 : 1, "",
			      buf[idx] & 0xff);
		html(" </td><td class='hex'>");
		for (idx = 0; idx < ROWLEN && ofs + idx < size; idx++)
			ascii[idx] = isgraph(buf[idx]) ? buf[idx] : '.';
		ascii[idx] = '\0';
		html_txt(ascii);
		html("</td></tr>\n");
	}
	html("</table>\n");
}

static void print_object(const struct object_id *oid, const char *path, const char *basename, const char *rev)
{
	enum object_type type;
	char *buf;
	unsigned long size;

	type = oid_object_info(the_repository, oid, &size);
	if (type == OBJ_BAD) {
		cgit_print_error_page(404, "Not found",
			"Bad object name: %s", oid_to_hex(oid));
		return;
	}

	buf = read_object_file(oid, &type, &size);
	if (!buf) {
		cgit_print_error_page(500, "Internal server error",
			"Error reading object %s", oid_to_hex(oid));
		return;
	}

	cgit_set_title_from_path(path);

	cgit_print_layout_start();
	htmlf("blob: %s (", oid_to_hex(oid));
	cgit_plain_link("plain", NULL, NULL, ctx.qry.head,
		        rev, path);
	if (ctx.cfg.enable_blame) {
		html(") (");
		cgit_blame_link("blame", NULL, NULL, ctx.qry.head,
			        rev, path);
	}
	html(")\n");

	if (ctx.cfg.max_blob_size && size / 1024 > ctx.cfg.max_blob_size) {
		htmlf("<div class='error'>blob size (%ldKB) exceeds display size limit (%dKB).</div>",
				size / 1024, ctx.cfg.max_blob_size);
		return;
	}

	if (buffer_is_binary(buf, size))
		print_binary_buffer(buf, size);
	else
		print_text_buffer(basename, buf, size);

	free(buf);
}

struct single_tree_ctx {
	struct strbuf *path;
	struct object_id oid;
	char *name;
	size_t count;
};

static int single_tree_cb(const struct object_id *oid, struct strbuf *base,
			  const char *pathname, unsigned mode, int stage,
			  void *cbdata)
{
	struct single_tree_ctx *ctx = cbdata;

	if (++ctx->count > 1)
		return -1;

	if (!S_ISDIR(mode)) {
		ctx->count = 2;
		return -1;
	}

	ctx->name = xstrdup(pathname);
	oidcpy(&ctx->oid, oid);
	strbuf_addf(ctx->path, "/%s", pathname);
	return 0;
}

static void write_tree_link(const struct object_id *oid, char *name,
			    char *rev, struct strbuf *fullpath)
{
	size_t initial_length = fullpath->len;
	struct tree *tree;
	struct single_tree_ctx tree_ctx = {
		.path = fullpath,
		.count = 1,
	};
	struct pathspec paths = {
		.nr = 0
	};

	oidcpy(&tree_ctx.oid, oid);

	while (tree_ctx.count == 1) {
		cgit_tree_link(name, NULL, "ls-dir", ctx.qry.head, rev,
			       fullpath->buf);

		tree = lookup_tree(the_repository, &tree_ctx.oid);
		if (!tree)
			return;

		free(tree_ctx.name);
		tree_ctx.name = NULL;
		tree_ctx.count = 0;

		read_tree_recursive(tree, "", 0, 1, &paths, single_tree_cb,
				    &tree_ctx);

		if (tree_ctx.count != 1)
			break;

		html(" / ");
		name = tree_ctx.name;
	}

	strbuf_setlen(fullpath, initial_length);
}

static int ls_item(const struct object_id *oid, struct strbuf *base,
		const char *pathname, unsigned mode, int stage, void *cbdata)
{
	struct walk_tree_context *walk_tree_ctx = cbdata;
	char *name;
	struct strbuf fullpath = STRBUF_INIT;
	struct strbuf class = STRBUF_INIT;
	enum object_type type;
	unsigned long size = 0;

	name = xstrdup(pathname);
	strbuf_addf(&fullpath, "%s%s%s", ctx.qry.path ? ctx.qry.path : "",
		    ctx.qry.path ? "/" : "", name);

	if (!S_ISGITLINK(mode)) {
		type = oid_object_info(the_repository, oid, &size);
		if (type == OBJ_BAD) {
			htmlf("<tr><td colspan='3'>Bad object: %s %s</td></tr>",
			      name,
			      oid_to_hex(oid));
			free(name);
			return 0;
		}
	}

	html("<tr><td class='ls-mode'>");
	cgit_print_filemode(mode);
	html("</td><td>");
	if (S_ISGITLINK(mode)) {
		cgit_submodule_link("ls-mod", fullpath.buf, oid_to_hex(oid));
	} else if (S_ISDIR(mode)) {
		write_tree_link(oid, name, walk_tree_ctx->curr_rev,
				&fullpath);
	} else {
		char *ext = strrchr(name, '.');
		strbuf_addstr(&class, "ls-blob");
		if (ext)
			strbuf_addf(&class, " %s", ext + 1);
		cgit_tree_link(name, NULL, class.buf, ctx.qry.head,
			       walk_tree_ctx->curr_rev, fullpath.buf);
	}
	htmlf("</td><td class='ls-size'>%li</td>", size);

	html("<td>");
	cgit_log_link("log", NULL, "button", ctx.qry.head,
		      walk_tree_ctx->curr_rev, fullpath.buf, 0, NULL, NULL,
		      ctx.qry.showmsg, 0);
	if (ctx.repo->max_stats)
		cgit_stats_link("stats", NULL, "button", ctx.qry.head,
				fullpath.buf);
	if (!S_ISGITLINK(mode))
		cgit_plain_link("plain", NULL, "button", ctx.qry.head,
				walk_tree_ctx->curr_rev, fullpath.buf);
	if (!S_ISDIR(mode) && ctx.cfg.enable_blame)
		cgit_blame_link("blame", NULL, "button", ctx.qry.head,
				walk_tree_ctx->curr_rev, fullpath.buf);
	html("</td></tr>\n");
	free(name);
	strbuf_release(&fullpath);
	strbuf_release(&class);
	return 0;
}

static void ls_head(void)
{
	cgit_print_layout_start();
	html("<table summary='tree listing' class='list'>\n");
	html("<tr class='nohover'>");
	html("<th class='left'>Mode</th>");
	html("<th class='left'>Name</th>");
	html("<th class='right'>Size</th>");
	html("<th/>");
	html("</tr>\n");
}

static void ls_tail(void)
{
	html("</table>\n");
	cgit_print_layout_end();
}

static void ls_tree(const struct object_id *oid, const char *path, struct walk_tree_context *walk_tree_ctx)
{
	struct tree *tree;
	struct pathspec paths = {
		.nr = 0
	};

	tree = parse_tree_indirect(oid);
	if (!tree) {
		cgit_print_error_page(404, "Not found",
			"Not a tree object: %s", oid_to_hex(oid));
		return;
	}

	ls_head();
	read_tree_recursive(tree, "", 0, 1, &paths, ls_item, walk_tree_ctx);
	ls_tail();
}


static int walk_tree(const struct object_id *oid, struct strbuf *base,
		const char *pathname, unsigned mode, int stage, void *cbdata)
{
	struct walk_tree_context *walk_tree_ctx = cbdata;

	if (walk_tree_ctx->state == 0) {
		struct strbuf buffer = STRBUF_INIT;

		strbuf_addbuf(&buffer, base);
		strbuf_addstr(&buffer, pathname);
		if (strcmp(walk_tree_ctx->match_path, buffer.buf))
			return READ_TREE_RECURSIVE;

		if (S_ISDIR(mode)) {
			walk_tree_ctx->state = 1;
			cgit_set_title_from_path(buffer.buf);
			strbuf_release(&buffer);
			ls_head();
			return READ_TREE_RECURSIVE;
		} else {
			walk_tree_ctx->state = 2;
			print_object(oid, buffer.buf, pathname, walk_tree_ctx->curr_rev);
			strbuf_release(&buffer);
			return 0;
		}
	}
	ls_item(oid, base, pathname, mode, stage, walk_tree_ctx);
	return 0;
}

/*
 * Show a tree or a blob
 *   rev:  the commit pointing at the root tree object
 *   path: path to tree or blob
 */
void cgit_print_tree(const char *rev, char *path)
{
	struct object_id oid;
	struct commit *commit;
	struct pathspec_item path_items = {
		.match = path,
		.len = path ? strlen(path) : 0
	};
	struct pathspec paths = {
		.nr = path ? 1 : 0,
		.items = &path_items
	};
	struct walk_tree_context walk_tree_ctx = {
		.match_path = path,
		.state = 0
	};

	if (!rev)
		rev = ctx.qry.head;

	if (get_oid(rev, &oid)) {
		cgit_print_error_page(404, "Not found",
			"Invalid revision name: %s", rev);
		return;
	}
	commit = lookup_commit_reference(the_repository, &oid);
	if (!commit || parse_commit(commit)) {
		cgit_print_error_page(404, "Not found",
			"Invalid commit reference: %s", rev);
		return;
	}

	walk_tree_ctx.curr_rev = xstrdup(rev);

	if (path == NULL) {
		ls_tree(&commit->maybe_tree->object.oid, NULL, &walk_tree_ctx);
		goto cleanup;
	}

	read_tree_recursive(commit->maybe_tree, "", 0, 0, &paths, walk_tree, &walk_tree_ctx);
	if (walk_tree_ctx.state == 1)
		ls_tail();
	else if (walk_tree_ctx.state == 2)
		cgit_print_layout_end();
	else
		cgit_print_error_page(404, "Not found", "Path not found");

cleanup:
	free(walk_tree_ctx.curr_rev);
}
