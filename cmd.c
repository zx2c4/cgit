/* cmd.c: the cgit command dispatcher
 *
 * Copyright (C) 2006-2017 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "cmd.h"
#include "cache.h"
#include "ui-shared.h"
#include "ui-atom.h"
#include "ui-blame.h"
#include "ui-blob.h"
#include "ui-clone.h"
#include "ui-commit.h"
#include "ui-diff.h"
#include "ui-log.h"
#include "ui-patch.h"
#include "ui-plain.h"
#include "ui-refs.h"
#include "ui-repolist.h"
#include "ui-snapshot.h"
#include "ui-stats.h"
#include "ui-summary.h"
#include "ui-tag.h"
#include "ui-tree.h"

static void HEAD_fn(void)
{
	cgit_clone_head();
}

static void atom_fn(void)
{
	cgit_print_atom(ctx.qry.head, ctx.qry.path, ctx.cfg.max_atom_items);
}

static void about_fn(void)
{
	if (ctx.repo) {
		size_t path_info_len = ctx.env.path_info ? strlen(ctx.env.path_info) : 0;
		if (!ctx.qry.path &&
		    ctx.qry.url[strlen(ctx.qry.url) - 1] != '/' &&
		    (!path_info_len || ctx.env.path_info[path_info_len - 1] != '/')) {
			char *currenturl = cgit_currenturl();
			char *redirect = fmtalloc("%s/", currenturl);
			cgit_redirect(redirect, true);
			free(currenturl);
			free(redirect);
		} else if (ctx.repo->readme.nr)
			cgit_print_repo_readme(ctx.qry.path);
		else if (ctx.repo->homepage)
			cgit_redirect(ctx.repo->homepage, false);
		else {
			char *currenturl = cgit_currenturl();
			char *redirect = fmtalloc("%s../", currenturl);
			cgit_redirect(redirect, false);
			free(currenturl);
			free(redirect);
		}
	} else
		cgit_print_site_readme();
}

static void blame_fn(void)
{
	if (ctx.repo->enable_blame)
		cgit_print_blame();
	else
		cgit_print_error_page(403, "Forbidden", "Blame is disabled");
}

static void blob_fn(void)
{
	cgit_print_blob(ctx.qry.sha1, ctx.qry.path, ctx.qry.head, 0);
}

static void commit_fn(void)
{
	cgit_print_commit(ctx.qry.sha1, ctx.qry.path);
}

static void diff_fn(void)
{
	cgit_print_diff(ctx.qry.sha1, ctx.qry.sha2, ctx.qry.path, 1, 0);
}

static void rawdiff_fn(void)
{
	cgit_print_diff(ctx.qry.sha1, ctx.qry.sha2, ctx.qry.path, 1, 1);
}

static void info_fn(void)
{
	cgit_clone_info();
}

static void log_fn(void)
{
	cgit_print_log(ctx.qry.sha1, ctx.qry.ofs, ctx.cfg.max_commit_count,
		       ctx.qry.grep, ctx.qry.search, ctx.qry.path, 1,
		       ctx.repo->enable_commit_graph,
		       ctx.repo->commit_sort);
}

static void ls_cache_fn(void)
{
	ctx.page.mimetype = "text/plain";
	ctx.page.filename = "ls-cache.txt";
	cgit_print_http_headers();
	cache_ls(ctx.cfg.cache_root);
}

static void objects_fn(void)
{
	cgit_clone_objects();
}

static void repolist_fn(void)
{
	cgit_print_repolist();
}

static void patch_fn(void)
{
	cgit_print_patch(ctx.qry.sha1, ctx.qry.sha2, ctx.qry.path);
}

static void plain_fn(void)
{
	cgit_print_plain();
}

static void refs_fn(void)
{
	cgit_print_refs();
}

static void snapshot_fn(void)
{
	cgit_print_snapshot(ctx.qry.head, ctx.qry.sha1, ctx.qry.path,
			    ctx.qry.nohead);
}

static void stats_fn(void)
{
	cgit_show_stats();
}

static void summary_fn(void)
{
	cgit_print_summary();
}

static void tag_fn(void)
{
	cgit_print_tag(ctx.qry.sha1);
}

static void tree_fn(void)
{
	cgit_print_tree(ctx.qry.sha1, ctx.qry.path);
}

#define def_cmd(name, want_repo, want_vpath, is_clone) \
	{#name, name##_fn, want_repo, want_vpath, is_clone}

struct cgit_cmd *cgit_get_cmd(void)
{
	static struct cgit_cmd cmds[] = {
		def_cmd(HEAD, 1, 0, 1),
		def_cmd(atom, 1, 0, 0),
		def_cmd(about, 0, 0, 0),
		def_cmd(blame, 1, 1, 0),
		def_cmd(blob, 1, 0, 0),
		def_cmd(commit, 1, 1, 0),
		def_cmd(diff, 1, 1, 0),
		def_cmd(info, 1, 0, 1),
		def_cmd(log, 1, 1, 0),
		def_cmd(ls_cache, 0, 0, 0),
		def_cmd(objects, 1, 0, 1),
		def_cmd(patch, 1, 1, 0),
		def_cmd(plain, 1, 0, 0),
		def_cmd(rawdiff, 1, 1, 0),
		def_cmd(refs, 1, 0, 0),
		def_cmd(repolist, 0, 0, 0),
		def_cmd(snapshot, 1, 0, 0),
		def_cmd(stats, 1, 1, 0),
		def_cmd(summary, 1, 0, 0),
		def_cmd(tag, 1, 0, 0),
		def_cmd(tree, 1, 1, 0),
	};
	int i;

	if (ctx.qry.page == NULL) {
		if (ctx.repo)
			ctx.qry.page = "summary";
		else
			ctx.qry.page = "repolist";
	}

	for (i = 0; i < sizeof(cmds)/sizeof(*cmds); i++)
		if (!strcmp(ctx.qry.page, cmds[i].name))
			return &cmds[i];
	return NULL;
}
