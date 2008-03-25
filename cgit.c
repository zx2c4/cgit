/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "cmd.h"
#include "ui-shared.h"

const char *cgit_version = CGIT_VERSION;

void config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "root-title"))
		ctx.cfg.root_title = xstrdup(value);
	else if (!strcmp(name, "css"))
		ctx.cfg.css = xstrdup(value);
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
		cgit_read_config(value, config_cb);
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
	}
}

static void prepare_context(struct cgit_context *ctx)
{
	memset(ctx, 0, sizeof(ctx));
	ctx->cfg.agefile = "info/web/last-modified";
	ctx->cfg.cache_dynamic_ttl = 5;
	ctx->cfg.cache_max_create_time = 5;
	ctx->cfg.cache_repo_ttl = 5;
	ctx->cfg.cache_root = CGIT_CACHE_ROOT;
	ctx->cfg.cache_root_ttl = 5;
	ctx->cfg.cache_static_ttl = -1;
	ctx->cfg.css = "/cgit.css";
	ctx->cfg.logo = "/git-logo.png";
	ctx->cfg.max_commit_count = 50;
	ctx->cfg.max_lock_attempts = 5;
	ctx->cfg.max_msg_len = 60;
	ctx->cfg.max_repodesc_len = 60;
	ctx->cfg.module_link = "./?repo=%s&page=commit&id=%s";
	ctx->cfg.renamelimit = -1;
	ctx->cfg.robots = "index, nofollow";
	ctx->cfg.root_title = "Git repository browser";
	ctx->cfg.script_name = CGIT_SCRIPT_NAME;
	ctx->page.mimetype = "text/html";
	ctx->page.charset = PAGE_ENCODING;
	ctx->page.filename = NULL;
}

static int cgit_prepare_cache(struct cacheitem *item)
{
	if (!ctx.repo && ctx.qry.repo) {
		ctx.page.title = fmt("%s - %s", ctx.cfg.root_title,
				      "Bad request");
		cgit_print_http_headers(&ctx);
		cgit_print_docstart(&ctx);
		cgit_print_pageheader(&ctx);
		cgit_print_error(fmt("Unknown repo: %s", ctx.qry.repo));
		cgit_print_docend();
		return 0;
	}

	if (!ctx.repo) {
		item->name = xstrdup(fmt("%s/index.html", ctx.cfg.cache_root));
		item->ttl = ctx.cfg.cache_root_ttl;
		return 1;
	}

	if (!ctx.qry.page) {
		item->name = xstrdup(fmt("%s/%s/index.%s.html", ctx.cfg.cache_root,
					 cache_safe_filename(ctx.repo->url),
					 cache_safe_filename(ctx.qry.raw)));
		item->ttl = ctx.cfg.cache_repo_ttl;
	} else {
		item->name = xstrdup(fmt("%s/%s/%s/%s.html", ctx.cfg.cache_root,
					 cache_safe_filename(ctx.repo->url),
					 ctx.qry.page,
					 cache_safe_filename(ctx.qry.raw)));
		if (ctx.qry.has_symref)
			item->ttl = ctx.cfg.cache_dynamic_ttl;
		else if (ctx.qry.has_sha1)
			item->ttl = ctx.cfg.cache_static_ttl;
		else
			item->ttl = ctx.cfg.cache_repo_ttl;
	}
	return 1;
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

	info.req_ref = repo->defbranch;
	info.first_ref = NULL;
	info.match = 0;
	for_each_branch_ref(find_current_ref, &info);
	if (info.match)
		return info.req_ref;
	else
		return info.first_ref;
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
		ctx->qry.head = xstrdup(find_default_branch(ctx->repo));
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

static void process_request(struct cgit_context *ctx)
{
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

	if (cmd->want_repo && prepare_repo_cmd(ctx))
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

static long ttl_seconds(long ttl)
{
	if (ttl<0)
		return 60 * 60 * 24 * 365;
	else
		return ttl * 60;
}

static void cgit_fill_cache(struct cacheitem *item, int use_cache)
{
	int stdout2;

	if (use_cache) {
		stdout2 = chk_positive(dup(STDOUT_FILENO),
				       "Preserving STDOUT");
		chk_zero(close(STDOUT_FILENO), "Closing STDOUT");
		chk_positive(dup2(item->fd, STDOUT_FILENO), "Dup2(cachefile)");
	}

	ctx.page.modified = time(NULL);
	ctx.page.expires = ctx.page.modified + ttl_seconds(item->ttl);
	process_request(&ctx);

	if (use_cache) {
		chk_zero(close(STDOUT_FILENO), "Close redirected STDOUT");
		chk_positive(dup2(stdout2, STDOUT_FILENO),
			     "Restoring original STDOUT");
		chk_zero(close(stdout2), "Closing temporary STDOUT");
	}
}

static void cgit_check_cache(struct cacheitem *item)
{
	int i = 0;

 top:
	if (++i > ctx.cfg.max_lock_attempts) {
		die("cgit_refresh_cache: unable to lock %s: %s",
		    item->name, strerror(errno));
	}
	if (!cache_exist(item)) {
		if (!cache_lock(item)) {
			sleep(1);
			goto top;
		}
		if (!cache_exist(item)) {
			cgit_fill_cache(item, 1);
			cache_unlock(item);
		} else {
			cache_cancel_lock(item);
		}
	} else if (cache_expired(item) && cache_lock(item)) {
		if (cache_expired(item)) {
			cgit_fill_cache(item, 1);
			cache_unlock(item);
		} else {
			cache_cancel_lock(item);
		}
	}
}

static void cgit_print_cache(struct cacheitem *item)
{
	static char buf[4096];
	ssize_t i;

	int fd = open(item->name, O_RDONLY);
	if (fd<0)
		die("Unable to open cached file %s", item->name);

	while((i=read(fd, buf, sizeof(buf))) > 0)
		write(STDOUT_FILENO, buf, i);

	close(fd);
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

int main(int argc, const char **argv)
{
	struct cacheitem item;
	const char *cgit_config_env = getenv("CGIT_CONFIG");

	prepare_context(&ctx);
	item.st.st_mtime = time(NULL);
	cgit_repolist.length = 0;
	cgit_repolist.count = 0;
	cgit_repolist.repos = NULL;

	cgit_read_config(cgit_config_env ? cgit_config_env : CGIT_CONFIG,
			 config_cb);
	if (getenv("SCRIPT_NAME"))
		ctx.cfg.script_name = xstrdup(getenv("SCRIPT_NAME"));
	if (getenv("QUERY_STRING"))
		ctx.qry.raw = xstrdup(getenv("QUERY_STRING"));
	cgit_parse_args(argc, argv);
	cgit_parse_query(ctx.qry.raw, querystring_cb);
	if (!cgit_prepare_cache(&item))
		return 0;
	if (ctx.cfg.nocache) {
		cgit_fill_cache(&item, 0);
	} else {
		cgit_check_cache(&item);
		cgit_print_cache(&item);
	}
	return 0;
}
