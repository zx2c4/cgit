/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "cache.h"
#include "cmd.h"
#include "configfile.h"
#include "html.h"
#include "ui-shared.h"

const char *cgit_version = CGIT_VERSION;

void config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "root-title"))
		ctx.cfg.root_title = xstrdup(value);
	else if (!strcmp(name, "root-desc"))
		ctx.cfg.root_desc = xstrdup(value);
	else if (!strcmp(name, "root-readme"))
		ctx.cfg.root_readme = xstrdup(value);
	else if (!strcmp(name, "css"))
		ctx.cfg.css = xstrdup(value);
	else if (!strcmp(name, "favicon"))
		ctx.cfg.favicon = xstrdup(value);
	else if (!strcmp(name, "footer"))
		ctx.cfg.footer = xstrdup(value);
	else if (!strcmp(name, "logo"))
		ctx.cfg.logo = xstrdup(value);
	else if (!strcmp(name, "index-header"))
		ctx.cfg.index_header = xstrdup(value);
	else if (!strcmp(name, "index-info"))
		ctx.cfg.index_info = xstrdup(value);
	else if (!strcmp(name, "logo-link"))
		ctx.cfg.logo_link = xstrdup(value);
	else if (!strcmp(name, "module-link"))
		ctx.cfg.module_link = xstrdup(value);
	else if (!strcmp(name, "virtual-root")) {
		ctx.cfg.virtual_root = trim_end(value, '/');
		if (!ctx.cfg.virtual_root && (!strcmp(value, "/")))
			ctx.cfg.virtual_root = "";
	} else if (!strcmp(name, "nocache"))
		ctx.cfg.nocache = atoi(value);
	else if (!strcmp(name, "snapshots"))
		ctx.cfg.snapshots = cgit_parse_snapshots_mask(value);
	else if (!strcmp(name, "enable-index-links"))
		ctx.cfg.enable_index_links = atoi(value);
	else if (!strcmp(name, "enable-log-filecount"))
		ctx.cfg.enable_log_filecount = atoi(value);
	else if (!strcmp(name, "enable-log-linecount"))
		ctx.cfg.enable_log_linecount = atoi(value);
	else if (!strcmp(name, "cache-size"))
		ctx.cfg.cache_size = atoi(value);
	else if (!strcmp(name, "cache-root"))
		ctx.cfg.cache_root = xstrdup(value);
	else if (!strcmp(name, "cache-root-ttl"))
		ctx.cfg.cache_root_ttl = atoi(value);
	else if (!strcmp(name, "cache-repo-ttl"))
		ctx.cfg.cache_repo_ttl = atoi(value);
	else if (!strcmp(name, "cache-static-ttl"))
		ctx.cfg.cache_static_ttl = atoi(value);
	else if (!strcmp(name, "cache-dynamic-ttl"))
		ctx.cfg.cache_dynamic_ttl = atoi(value);
	else if (!strcmp(name, "max-message-length"))
		ctx.cfg.max_msg_len = atoi(value);
	else if (!strcmp(name, "max-repodesc-length"))
		ctx.cfg.max_repodesc_len = atoi(value);
	else if (!strcmp(name, "max-repo-count"))
		ctx.cfg.max_repo_count = atoi(value);
	else if (!strcmp(name, "max-commit-count"))
		ctx.cfg.max_commit_count = atoi(value);
	else if (!strcmp(name, "summary-log"))
		ctx.cfg.summary_log = atoi(value);
	else if (!strcmp(name, "summary-branches"))
		ctx.cfg.summary_branches = atoi(value);
	else if (!strcmp(name, "summary-tags"))
		ctx.cfg.summary_tags = atoi(value);
	else if (!strcmp(name, "agefile"))
		ctx.cfg.agefile = xstrdup(value);
	else if (!strcmp(name, "renamelimit"))
		ctx.cfg.renamelimit = atoi(value);
	else if (!strcmp(name, "robots"))
		ctx.cfg.robots = xstrdup(value);
	else if (!strcmp(name, "clone-prefix"))
		ctx.cfg.clone_prefix = xstrdup(value);
	else if (!strcmp(name, "local-time"))
		ctx.cfg.local_time = atoi(value);
	else if (!strcmp(name, "repo.group"))
		ctx.cfg.repo_group = xstrdup(value);
	else if (!strcmp(name, "repo.url"))
		ctx.repo = cgit_add_repo(value);
	else if (!strcmp(name, "repo.name"))
		ctx.repo->name = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.path"))
		ctx.repo->path = trim_end(value, '/');
	else if (ctx.repo && !strcmp(name, "repo.clone-url"))
		ctx.repo->clone_url = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.desc"))
		ctx.repo->desc = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.owner"))
		ctx.repo->owner = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.defbranch"))
		ctx.repo->defbranch = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.snapshots"))
		ctx.repo->snapshots = ctx.cfg.snapshots & cgit_parse_snapshots_mask(value); /* XXX: &? */
	else if (ctx.repo && !strcmp(name, "repo.enable-log-filecount"))
		ctx.repo->enable_log_filecount = ctx.cfg.enable_log_filecount * atoi(value);
	else if (ctx.repo && !strcmp(name, "repo.enable-log-linecount"))
		ctx.repo->enable_log_linecount = ctx.cfg.enable_log_linecount * atoi(value);
	else if (ctx.repo && !strcmp(name, "repo.module-link"))
		ctx.repo->module_link= xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.readme") && value != NULL) {
		if (*value == '/')
			ctx.repo->readme = xstrdup(value);
		else
			ctx.repo->readme = xstrdup(fmt("%s/%s", ctx.repo->path, value));
	} else if (!strcmp(name, "include"))
		parse_configfile(value, config_cb);
}

static void querystring_cb(const char *name, const char *value)
{
	if (!strcmp(name,"r")) {
		ctx.qry.repo = xstrdup(value);
		ctx.repo = cgit_get_repoinfo(value);
	} else if (!strcmp(name, "p")) {
		ctx.qry.page = xstrdup(value);
	} else if (!strcmp(name, "url")) {
		cgit_parse_url(value);
	} else if (!strcmp(name, "qt")) {
		ctx.qry.grep = xstrdup(value);
	} else if (!strcmp(name, "q")) {
		ctx.qry.search = xstrdup(value);
	} else if (!strcmp(name, "h")) {
		ctx.qry.head = xstrdup(value);
		ctx.qry.has_symref = 1;
	} else if (!strcmp(name, "id")) {
		ctx.qry.sha1 = xstrdup(value);
		ctx.qry.has_sha1 = 1;
	} else if (!strcmp(name, "id2")) {
		ctx.qry.sha2 = xstrdup(value);
		ctx.qry.has_sha1 = 1;
	} else if (!strcmp(name, "ofs")) {
		ctx.qry.ofs = atoi(value);
	} else if (!strcmp(name, "path")) {
		ctx.qry.path = trim_end(value, '/');
	} else if (!strcmp(name, "name")) {
		ctx.qry.name = xstrdup(value);
	} else if (!strcmp(name, "mimetype")) {
		ctx.qry.mimetype = xstrdup(value);
	}
}

static void prepare_context(struct cgit_context *ctx)
{
	memset(ctx, 0, sizeof(ctx));
	ctx->cfg.agefile = "info/web/last-modified";
	ctx->cfg.nocache = 0;
	ctx->cfg.cache_size = 0;
	ctx->cfg.cache_dynamic_ttl = 5;
	ctx->cfg.cache_max_create_time = 5;
	ctx->cfg.cache_repo_ttl = 5;
	ctx->cfg.cache_root = CGIT_CACHE_ROOT;
	ctx->cfg.cache_root_ttl = 5;
	ctx->cfg.cache_static_ttl = -1;
	ctx->cfg.css = "/cgit.css";
	ctx->cfg.logo = "/git-logo.png";
	ctx->cfg.local_time = 0;
	ctx->cfg.max_repo_count = 50;
	ctx->cfg.max_commit_count = 50;
	ctx->cfg.max_lock_attempts = 5;
	ctx->cfg.max_msg_len = 80;
	ctx->cfg.max_repodesc_len = 80;
	ctx->cfg.module_link = "./?repo=%s&page=commit&id=%s";
	ctx->cfg.renamelimit = -1;
	ctx->cfg.robots = "index, nofollow";
	ctx->cfg.root_title = "Git repository browser";
	ctx->cfg.root_desc = "a fast webinterface for the git dscm";
	ctx->cfg.script_name = CGIT_SCRIPT_NAME;
	ctx->cfg.summary_branches = 10;
	ctx->cfg.summary_log = 10;
	ctx->cfg.summary_tags = 10;
	ctx->page.mimetype = "text/html";
	ctx->page.charset = PAGE_ENCODING;
	ctx->page.filename = NULL;
	ctx->page.modified = time(NULL);
	ctx->page.expires = ctx->page.modified;
}

struct refmatch {
	char *req_ref;
	char *first_ref;
	int match;
};

int find_current_ref(const char *refname, const unsigned char *sha1,
		     int flags, void *cb_data)
{
	struct refmatch *info;

	info = (struct refmatch *)cb_data;
	if (!strcmp(refname, info->req_ref))
		info->match = 1;
	if (!info->first_ref)
		info->first_ref = xstrdup(refname);
	return info->match;
}

char *find_default_branch(struct cgit_repo *repo)
{
	struct refmatch info;
	char *ref;

	info.req_ref = repo->defbranch;
	info.first_ref = NULL;
	info.match = 0;
	for_each_branch_ref(find_current_ref, &info);
	if (info.match)
		ref = info.req_ref;
	else
		ref = info.first_ref;
	if (ref)
		ref = xstrdup(ref);
	return ref;
}

static int prepare_repo_cmd(struct cgit_context *ctx)
{
	char *tmp;
	unsigned char sha1[20];
	int nongit = 0;

	setenv("GIT_DIR", ctx->repo->path, 1);
	setup_git_directory_gently(&nongit);
	if (nongit) {
		ctx->page.title = fmt("%s - %s", ctx->cfg.root_title,
				      "config error");
		tmp = fmt("Not a git repository: '%s'", ctx->repo->path);
		ctx->repo = NULL;
		cgit_print_http_headers(ctx);
		cgit_print_docstart(ctx);
		cgit_print_pageheader(ctx);
		cgit_print_error(tmp);
		cgit_print_docend();
		return 1;
	}
	ctx->page.title = fmt("%s - %s", ctx->repo->name, ctx->repo->desc);

	if (!ctx->qry.head) {
		ctx->qry.head = find_default_branch(ctx->repo);
		ctx->repo->defbranch = ctx->qry.head;
	}

	if (!ctx->qry.head) {
		cgit_print_http_headers(ctx);
		cgit_print_docstart(ctx);
		cgit_print_pageheader(ctx);
		cgit_print_error("Repository seems to be empty");
		cgit_print_docend();
		return 1;
	}

	if (get_sha1(ctx->qry.head, sha1)) {
		tmp = xstrdup(ctx->qry.head);
		ctx->qry.head = ctx->repo->defbranch;
		cgit_print_http_headers(ctx);
		cgit_print_docstart(ctx);
		cgit_print_pageheader(ctx);
		cgit_print_error(fmt("Invalid branch: %s", tmp));
		cgit_print_docend();
		return 1;
	}
	return 0;
}

static void process_request(void *cbdata)
{
	struct cgit_context *ctx = cbdata;
	struct cgit_cmd *cmd;

	cmd = cgit_get_cmd(ctx);
	if (!cmd) {
		ctx->page.title = "cgit error";
		ctx->repo = NULL;
		cgit_print_http_headers(ctx);
		cgit_print_docstart(ctx);
		cgit_print_pageheader(ctx);
		cgit_print_error("Invalid request");
		cgit_print_docend();
		return;
	}

	if (cmd->want_repo && !ctx->repo) {
		cgit_print_http_headers(ctx);
		cgit_print_docstart(ctx);
		cgit_print_pageheader(ctx);
		cgit_print_error(fmt("No repository selected"));
		cgit_print_docend();
		return;
	}

	if (ctx->repo && prepare_repo_cmd(ctx))
		return;

	if (cmd->want_layout) {
		cgit_print_http_headers(ctx);
		cgit_print_docstart(ctx);
		cgit_print_pageheader(ctx);
	}

	cmd->fn(ctx);

	if (cmd->want_layout)
		cgit_print_docend();
}

static void cgit_parse_args(int argc, const char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "--cache=", 8)) {
			ctx.cfg.cache_root = xstrdup(argv[i]+8);
		}
		if (!strcmp(argv[i], "--nocache")) {
			ctx.cfg.nocache = 1;
		}
		if (!strncmp(argv[i], "--query=", 8)) {
			ctx.qry.raw = xstrdup(argv[i]+8);
		}
		if (!strncmp(argv[i], "--repo=", 7)) {
			ctx.qry.repo = xstrdup(argv[i]+7);
		}
		if (!strncmp(argv[i], "--page=", 7)) {
			ctx.qry.page = xstrdup(argv[i]+7);
		}
		if (!strncmp(argv[i], "--head=", 7)) {
			ctx.qry.head = xstrdup(argv[i]+7);
			ctx.qry.has_symref = 1;
		}
		if (!strncmp(argv[i], "--sha1=", 7)) {
			ctx.qry.sha1 = xstrdup(argv[i]+7);
			ctx.qry.has_sha1 = 1;
		}
		if (!strncmp(argv[i], "--ofs=", 6)) {
			ctx.qry.ofs = atoi(argv[i]+6);
		}
	}
}

static int calc_ttl()
{
	if (!ctx.repo)
		return ctx.cfg.cache_root_ttl;

	if (!ctx.qry.page)
		return ctx.cfg.cache_repo_ttl;

	if (ctx.qry.has_symref)
		return ctx.cfg.cache_dynamic_ttl;

	if (ctx.qry.has_sha1)
		return ctx.cfg.cache_static_ttl;

	return ctx.cfg.cache_repo_ttl;
}

int main(int argc, const char **argv)
{
	const char *cgit_config_env = getenv("CGIT_CONFIG");
	int err, ttl;

	prepare_context(&ctx);
	cgit_repolist.length = 0;
	cgit_repolist.count = 0;
	cgit_repolist.repos = NULL;

	parse_configfile(cgit_config_env ? cgit_config_env : CGIT_CONFIG,
			 config_cb);
	ctx.repo = NULL;
	if (getenv("SCRIPT_NAME"))
		ctx.cfg.script_name = xstrdup(getenv("SCRIPT_NAME"));
	if (getenv("QUERY_STRING"))
		ctx.qry.raw = xstrdup(getenv("QUERY_STRING"));
	cgit_parse_args(argc, argv);
	http_parse_querystring(ctx.qry.raw, querystring_cb);

	ttl = calc_ttl();
	ctx.page.expires += ttl*60;
	if (ctx.cfg.nocache)
		ctx.cfg.cache_size = 0;
	err = cache_process(ctx.cfg.cache_size, ctx.cfg.cache_root,
			    ctx.qry.raw, ttl, process_request, &ctx);
	if (err)
		cgit_print_error(fmt("Error processing page: %s (%d)",
				     strerror(err), err));
	return err;
}
