/* ui-blame.c: functions for blame output
 *
 * Copyright (C) 2006-2017 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-blame.h"
#include "html.h"
#include "ui-shared.h"
#include "argv-array.h"
#include "blame.h"


static char *emit_suspect_detail(struct blame_origin *suspect)
{
	struct commitinfo *info;
	struct strbuf detail = STRBUF_INIT;

	info = cgit_parse_commit(suspect->commit);

	strbuf_addf(&detail, "author  %s", info->author);
	if (!ctx.cfg.noplainemail)
		strbuf_addf(&detail, " %s", info->author_email);
	strbuf_addf(&detail, "  %s\n",
		    show_date(info->author_date, info->author_tz,
				    cgit_date_mode(DATE_ISO8601)));

	strbuf_addf(&detail, "committer  %s", info->committer);
	if (!ctx.cfg.noplainemail)
		strbuf_addf(&detail, " %s", info->committer_email);
	strbuf_addf(&detail, "  %s\n\n",
		    show_date(info->committer_date, info->committer_tz,
				    cgit_date_mode(DATE_ISO8601)));

	strbuf_addstr(&detail, info->subject);

	cgit_free_commitinfo(info);
	return strbuf_detach(&detail, NULL);
}

static void emit_blame_entry_hash(struct blame_entry *ent)
{
	struct blame_origin *suspect = ent->suspect;
	struct object_id *oid = &suspect->commit->object.oid;

	char *detail = emit_suspect_detail(suspect);
	cgit_commit_link(find_unique_abbrev(oid->hash, DEFAULT_ABBREV), detail,
			 NULL, ctx.qry.head, oid_to_hex(oid), suspect->path);
	free(detail);
}

static void emit_blame_entry_linenumber(struct blame_entry *ent)
{
	const char *numberfmt = "<a id='n%1$d' href='#n%1$d'>%1$d</a>\n";

	unsigned long lineno = ent->lno;
	while (lineno < ent->lno + ent->num_lines)
		htmlf(numberfmt, ++lineno);
}

static void emit_blame_entry_line(struct blame_scoreboard *sb,
				  struct blame_entry *ent)
{
	const char *cp, *cpend;

	cp = blame_nth_line(sb, ent->lno);
	cpend = blame_nth_line(sb, ent->lno + ent->num_lines);

	html_ntxt(cp, cpend - cp);
}

static void emit_blame_entry(struct blame_scoreboard *sb,
			     struct blame_entry *ent)
{
	html("<tr><td class='sha1 hashes'>");
	emit_blame_entry_hash(ent);
	html("</td>\n");

	if (ctx.cfg.enable_tree_linenumbers) {
		html("<td class='linenumbers'><pre>");
		emit_blame_entry_linenumber(ent);
		html("</pre></td>\n");
	}

	html("<td class='lines'><pre><code>");
	emit_blame_entry_line(sb, ent);
	html("</code></pre></td></tr>\n");
}

struct walk_tree_context {
	char *curr_rev;
	int match_baselen;
	int state;
};

static void print_object(const unsigned char *sha1, const char *path,
			 const char *basename, const char *rev)
{
	enum object_type type;
	unsigned long size;
	struct argv_array rev_argv = ARGV_ARRAY_INIT;
	struct rev_info revs;
	struct blame_scoreboard sb;
	struct blame_origin *o;
	struct blame_entry *ent = NULL;

	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		cgit_print_error_page(404, "Not found", "Bad object name: %s",
				      sha1_to_hex(sha1));
		return;
	}

	argv_array_push(&rev_argv, "blame");
	argv_array_push(&rev_argv, rev);
	init_revisions(&revs, NULL);
	revs.diffopt.flags.allow_textconv = 1;
	setup_revisions(rev_argv.argc, rev_argv.argv, &revs, NULL);
	init_scoreboard(&sb);
	sb.revs = &revs;
	setup_scoreboard(&sb, path, &o);
	o->suspects = blame_entry_prepend(NULL, 0, sb.num_lines, o);
	prio_queue_put(&sb.commits, o->commit);
	blame_origin_decref(o);
	sb.ent = NULL;
	sb.path = path;
	assign_blame(&sb, 0);
	blame_sort_final(&sb);
	blame_coalesce(&sb);

	cgit_set_title_from_path(path);

	cgit_print_layout_start();
	htmlf("blob: %s (", sha1_to_hex(sha1));
	cgit_plain_link("plain", NULL, NULL, ctx.qry.head, rev, path);
	html(") (");
	cgit_tree_link("tree", NULL, NULL, ctx.qry.head, rev, path);
	html(")\n");

	if (ctx.cfg.max_blob_size && size / 1024 > ctx.cfg.max_blob_size) {
		htmlf("<div class='error'>blob size (%ldKB)"
		      " exceeds display size limit (%dKB).</div>",
		      size / 1024, ctx.cfg.max_blob_size);
		return;
	}

	html("<table class='blame blob'>");
	for (ent = sb.ent; ent; ) {
		struct blame_entry *e = ent->next;
		emit_blame_entry(&sb, ent);
		free(ent);
		ent = e;
	}
	html("</table>\n");
	free((void *)sb.final_buf);

	cgit_print_layout_end();
}

static int walk_tree(const unsigned char *sha1, struct strbuf *base,
		     const char *pathname, unsigned mode, int stage,
		     void *cbdata)
{
	struct walk_tree_context *walk_tree_ctx = cbdata;

	if (base->len == walk_tree_ctx->match_baselen) {
		if (S_ISREG(mode)) {
			struct strbuf buffer = STRBUF_INIT;
			strbuf_addbuf(&buffer, base);
			strbuf_addstr(&buffer, pathname);
			print_object(sha1, buffer.buf, pathname,
				     walk_tree_ctx->curr_rev);
			strbuf_release(&buffer);
			walk_tree_ctx->state = 1;
		} else if (S_ISDIR(mode)) {
			walk_tree_ctx->state = 2;
		}
	} else if (base->len < INT_MAX
			&& (int)base->len > walk_tree_ctx->match_baselen) {
		walk_tree_ctx->state = 2;
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

void cgit_print_blame(void)
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
		.state = 0
	};

	if (!rev)
		rev = ctx.qry.head;

	if (get_oid(rev, &oid)) {
		cgit_print_error_page(404, "Not found",
			"Invalid revision name: %s", rev);
		return;
	}
	commit = lookup_commit_reference(&oid);
	if (!commit || parse_commit(commit)) {
		cgit_print_error_page(404, "Not found",
			"Invalid commit reference: %s", rev);
		return;
	}

	walk_tree_ctx.curr_rev = xstrdup(rev);
	walk_tree_ctx.match_baselen = (path_items.match) ?
				       basedir_len(path_items.match) : -1;

	read_tree_recursive(commit->tree, "", 0, 0, &paths, walk_tree,
		&walk_tree_ctx);
	if (!walk_tree_ctx.state)
		cgit_print_error_page(404, "Not found", "Not found");
	else if (walk_tree_ctx.state == 2)
		cgit_print_error_page(404, "No blame for folders",
			"Blame is not available for folders.");

	free(walk_tree_ctx.curr_rev);
}
