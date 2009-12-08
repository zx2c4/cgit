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
#include "ui-stats.h"
#include "scan-tree.h"

const char *cgit_version = CGIT_VERSION;

void add_mimetype(const char *name, const char *value)
{
	struct string_list_item *item;

	item = string_list_insert(xstrdup(name), &ctx.cfg.mimetypes);
	item->util = xstrdup(value);
}

struct cgit_filter *new_filter(const char *cmd, int extra_args)
{
	struct cgit_filter *f;

	if (!cmd || !cmd[0])
		return NULL;

	f = xmalloc(sizeof(struct cgit_filter));
	f->cmd = xstrdup(cmd);
	f->argv = xmalloc((2 + extra_args) * sizeof(char *));
	f->argv[0] = f->cmd;
	f->argv[1] = NULL;
	return f;
}

static void process_cached_repolist(const char *path);

void repo_config(struct cgit_repo *repo, const char *name, const char *value)
{
	if (!strcmp(name, "name"))
		repo->name = xstrdup(value);
	else if (!strcmp(name, "clone-url"))
		repo->clone_url = xstrdup(value);
	else if (!strcmp(name, "desc"))
		repo->desc = xstrdup(value);
	else if (!strcmp(name, "owner"))
		repo->owner = xstrdup(value);
	else if (!strcmp(name, "defbranch"))
		repo->defbranch = xstrdup(value);
	else if (!strcmp(name, "snapshots"))
		repo->snapshots = ctx.cfg.snapshots & cgit_parse_snapshots_mask(value);
	else if (!strcmp(name, "enable-log-filecount"))
		repo->enable_log_filecount = ctx.cfg.enable_log_filecount * atoi(value);
	else if (!strcmp(name, "enable-log-linecount"))
		repo->enable_log_linecount = ctx.cfg.enable_log_linecount * atoi(value);
	else if (!strcmp(name, "max-stats"))
		repo->max_stats = cgit_find_stats_period(value, NULL);
	else if (!strcmp(name, "module-link"))
		repo->module_link= xstrdup(value);
	else if (!strcmp(name, "section"))
		repo->section = xstrdup(value);
	else if (!strcmp(name, "readme") && value != NULL) {
		if (*value == '/')
			repo->readme = xstrdup(value);
		else
			repo->readme = xstrdup(fmt("%s/%s", repo->path, value));
	} else if (ctx.cfg.enable_filter_overrides) {
		if (!strcmp(name, "about-filter"))
			repo->about_filter = new_filter(value, 0);
		else if (!strcmp(name, "commit-filter"))
			repo->commit_filter = new_filter(value, 0);
		else if (!strcmp(name, "source-filter"))
			repo->source_filter = new_filter(value, 1);
	}
}

void config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "section") || !strcmp(name, "repo.group"))
		ctx.cfg.section = xstrdup(value);
	else if (!strcmp(name, "repo.url"))
		ctx.repo = cgit_add_repo(value);
	else if (ctx.repo && !strcmp(name, "repo.path"))
		ctx.repo->path = trim_end(value, '/');
	else if (ctx.repo && !prefixcmp(name, "repo."))
		repo_config(ctx.repo, name + 5, value);
	else if (!strcmp(name, "root-title"))
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
	else if (!strcmp(name, "head-include"))
		ctx.cfg.head_include = xstrdup(value);
	else if (!strcmp(name, "header"))
		ctx.cfg.header = xstrdup(value);
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
	else if (!strcmp(name, "noplainemail"))
		ctx.cfg.noplainemail = atoi(value);
	else if (!strcmp(name, "noheader"))
		ctx.cfg.noheader = atoi(value);
	else if (!strcmp(name, "snapshots"))
		ctx.cfg.snapshots = cgit_parse_snapshots_mask(value);
	else if (!strcmp(name, "enable-filter-overrides"))
		ctx.cfg.enable_filter_overrides = atoi(value);
	else if (!strcmp(name, "enable-index-links"))
		ctx.cfg.enable_index_links = atoi(value);
	else if (!strcmp(name, "enable-log-filecount"))
		ctx.cfg.enable_log_filecount = atoi(value);
	else if (!strcmp(name, "enable-log-linecount"))
		ctx.cfg.enable_log_linecount = atoi(value);
	else if (!strcmp(name, "enable-tree-linenumbers"))
		ctx.cfg.enable_tree_linenumbers = atoi(value);
	else if (!strcmp(name, "max-stats"))
		ctx.cfg.max_stats = cgit_find_stats_period(value, NULL);
	else if (!strcmp(name, "cache-size"))
		ctx.cfg.cache_size = atoi(value);
	else if (!strcmp(name, "cache-root"))
		ctx.cfg.cache_root = xstrdup(value);
	else if (!strcmp(name, "cache-root-ttl"))
		ctx.cfg.cache_root_ttl = atoi(value);
	else if (!strcmp(name, "cache-repo-ttl"))
		ctx.cfg.cache_repo_ttl = atoi(value);
	else if (!strcmp(name, "cache-scanrc-ttl"))
		ctx.cfg.cache_scanrc_ttl = atoi(value);
	else if (!strcmp(name, "cache-static-ttl"))
		ctx.cfg.cache_static_ttl = atoi(value);
	else if (!strcmp(name, "cache-dynamic-ttl"))
		ctx.cfg.cache_dynamic_ttl = atoi(value);
	else if (!strcmp(name, "about-filter"))
		ctx.cfg.about_filter = new_filter(value, 0);
	else if (!strcmp(name, "commit-filter"))
		ctx.cfg.commit_filter = new_filter(value, 0);
	else if (!strcmp(name, "embedded"))
		ctx.cfg.embedded = atoi(value);
	else if (!strcmp(name, "max-message-length"))
		ctx.cfg.max_msg_len = atoi(value);
	else if (!strcmp(name, "max-repodesc-length"))
		ctx.cfg.max_repodesc_len = atoi(value);
	else if (!strcmp(name, "max-repo-count"))
		ctx.cfg.max_repo_count = atoi(value);
	else if (!strcmp(name, "max-commit-count"))
		ctx.cfg.max_commit_count = atoi(value);
	else if (!strcmp(name, "scan-path"))
		if (!ctx.cfg.nocache && ctx.cfg.cache_size)
			process_cached_repolist(value);
		else
			scan_tree(value, repo_config);
	else if (!strcmp(name, "source-filter"))
		ctx.cfg.source_filter = new_filter(value, 1);
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
	else if (!prefixcmp(name, "mimetype."))
		add_mimetype(name + 9, value);
	else if (!strcmp(name, "include"))
		parse_configfile(value, config_cb);
}

static void querystring_cb(const char *name, const char *value)
{
	if (!value)
		value = "";

	if (!strcmp(name,"r")) {
		ctx.qry.repo = xstrdup(value);
		ctx.repo = cgit_get_repoinfo(value);
	} else if (!strcmp(name, "p")) {
		ctx.qry.page = xstrdup(value);
	} else if (!strcmp(name, "url")) {
		if (*value == '/')
			value++;
		ctx.qry.url = xstrdup(value);
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
	} else if (!strcmp(name, "s")){
		ctx.qry.sort = xstrdup(value);
	} else if (!strcmp(name, "showmsg")) {
		ctx.qry.showmsg = atoi(value);
	} else if (!strcmp(name, "period")) {
		ctx.qry.period = xstrdup(value);
	}
}

char *xstrdupn(const char *str)
{
	return (str ? xstrdup(str) : NULL);
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
	ctx->cfg.cache_scanrc_ttl = 15;
	ctx->cfg.cache_static_ttl = -1;
	ctx->cfg.css = "/cgit.css";
	ctx->cfg.logo = "/cgit.png";
	ctx->cfg.local_time = 0;
	ctx->cfg.enable_tree_linenumbers = 1;
	ctx->cfg.max_repo_count = 50;
	ctx->cfg.max_commit_count = 50;
	ctx->cfg.max_lock_attempts = 5;
	ctx->cfg.max_msg_len = 80;
	ctx->cfg.max_repodesc_len = 80;
	ctx->cfg.max_stats = 0;
	ctx->cfg.module_link = "./?repo=%s&page=commit&id=%s";
	ctx->cfg.renamelimit = -1;
	ctx->cfg.robots = "index, nofollow";
	ctx->cfg.root_title = "Git repository browser";
	ctx->cfg.root_desc = "a fast webinterface for the git dscm";
	ctx->cfg.script_name = CGIT_SCRIPT_NAME;
	ctx->cfg.section = "";
	ctx->cfg.summary_branches = 10;
	ctx->cfg.summary_log = 10;
	ctx->cfg.summary_tags = 10;
	ctx->env.cgit_config = xstrdupn(getenv("CGIT_CONFIG"));
	ctx->env.http_host = xstrdupn(getenv("HTTP_HOST"));
	ctx->env.https = xstrdupn(getenv("HTTPS"));
	ctx->env.no_http = xstrdupn(getenv("NO_HTTP"));
	ctx->env.path_info = xstrdupn(getenv("PATH_INFO"));
	ctx->env.query_string = xstrdupn(getenv("QUERY_STRING"));
	ctx->env.request_method = xstrdupn(getenv("REQUEST_METHOD"));
	ctx->env.script_name = xstrdupn(getenv("SCRIPT_NAME"));
	ctx->env.server_name = xstrdupn(getenv("SERVER_NAME"));
	ctx->env.server_port = xstrdupn(getenv("SERVER_PORT"));
	ctx->page.mimetype = "text/html";
	ctx->page.charset = PAGE_ENCODING;
	ctx->page.filename = NULL;
	ctx->page.size = 0;
	ctx->page.modified = time(NULL);
	ctx->page.expires = ctx->page.modified;
	ctx->page.etag = NULL;
	memset(&ctx->cfg.mimetypes, 0, sizeof(struct string_list));
	if (ctx->env.script_name)
		ctx->cfg.script_name = ctx->env.script_name;
	if (ctx->env.query_string)
		ctx->qry.raw = ctx->env.query_string;
	if (!ctx->env.cgit_config)
		ctx->env.cgit_config = CGIT_CONFIG;
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
		ctx->qry.nohead = 1;
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
		ctx->page.status = 404;
		ctx->page.statusmsg = "not found";
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

int cmp_repos(const void *a, const void *b)
{
	const struct cgit_repo *ra = a, *rb = b;
	return strcmp(ra->url, rb->url);
}

char *build_snapshot_setting(int bitmap)
{
	const struct cgit_snapshot_format *f;
	char *result = xstrdup("");
	char *tmp;
	int len;

	for (f = cgit_snapshot_formats; f->suffix; f++) {
		if (f->bit & bitmap) {
			tmp = result;
			result = xstrdup(fmt("%s%s ", tmp, f->suffix));
			free(tmp);
		}
	}
	len = strlen(result);
	if (len)
		result[len - 1] = '\0';
	return result;
}

char *get_first_line(char *txt)
{
	char *t = xstrdup(txt);
	char *p = strchr(t, '\n');
	if (p)
		*p = '\0';
	return t;
}

void print_repo(FILE *f, struct cgit_repo *repo)
{
	fprintf(f, "repo.url=%s\n", repo->url);
	fprintf(f, "repo.name=%s\n", repo->name);
	fprintf(f, "repo.path=%s\n", repo->path);
	if (repo->owner)
		fprintf(f, "repo.owner=%s\n", repo->owner);
	if (repo->desc) {
		char *tmp = get_first_line(repo->desc);
		fprintf(f, "repo.desc=%s\n", tmp);
		free(tmp);
	}
	if (repo->readme)
		fprintf(f, "repo.readme=%s\n", repo->readme);
	if (repo->defbranch)
		fprintf(f, "repo.defbranch=%s\n", repo->defbranch);
	if (repo->module_link)
		fprintf(f, "repo.module-link=%s\n", repo->module_link);
	if (repo->section)
		fprintf(f, "repo.section=%s\n", repo->section);
	if (repo->clone_url)
		fprintf(f, "repo.clone-url=%s\n", repo->clone_url);
	fprintf(f, "repo.enable-log-filecount=%d\n",
	        repo->enable_log_filecount);
	fprintf(f, "repo.enable-log-linecount=%d\n",
	        repo->enable_log_linecount);
	if (repo->about_filter && repo->about_filter != ctx.cfg.about_filter)
		fprintf(f, "repo.about-filter=%s\n", repo->about_filter->cmd);
	if (repo->commit_filter && repo->commit_filter != ctx.cfg.commit_filter)
		fprintf(f, "repo.commit-filter=%s\n", repo->commit_filter->cmd);
	if (repo->source_filter && repo->source_filter != ctx.cfg.source_filter)
		fprintf(f, "repo.source-filter=%s\n", repo->source_filter->cmd);
	if (repo->snapshots != ctx.cfg.snapshots) {
		char *tmp = build_snapshot_setting(repo->snapshots);
		fprintf(f, "repo.snapshots=%s\n", tmp);
		free(tmp);
	}
	if (repo->max_stats != ctx.cfg.max_stats)
		fprintf(f, "repo.max-stats=%s\n",
		        cgit_find_stats_periodname(repo->max_stats));
	fprintf(f, "\n");
}

void print_repolist(FILE *f, struct cgit_repolist *list, int start)
{
	int i;

	for(i = start; i < list->count; i++)
		print_repo(f, &list->repos[i]);
}

/* Scan 'path' for git repositories, save the resulting repolist in 'cached_rc'
 * and return 0 on success.
 */
static int generate_cached_repolist(const char *path, const char *cached_rc)
{
	char *locked_rc;
	int idx;
	FILE *f;

	locked_rc = xstrdup(fmt("%s.lock", cached_rc));
	f = fopen(locked_rc, "wx");
	if (!f) {
		/* Inform about the error unless the lockfile already existed,
		 * since that only means we've got concurrent requests.
		 */
		if (errno != EEXIST)
			fprintf(stderr, "[cgit] Error opening %s: %s (%d)\n",
				locked_rc, strerror(errno), errno);
		return errno;
	}
	idx = cgit_repolist.count;
	scan_tree(path, repo_config);
	print_repolist(f, &cgit_repolist, idx);
	if (rename(locked_rc, cached_rc))
		fprintf(stderr, "[cgit] Error renaming %s to %s: %s (%d)\n",
			locked_rc, cached_rc, strerror(errno), errno);
	fclose(f);
	return 0;
}

static void process_cached_repolist(const char *path)
{
	struct stat st;
	char *cached_rc;
	time_t age;

	cached_rc = xstrdup(fmt("%s/rc-%8x", ctx.cfg.cache_root,
		hash_str(path)));

	if (stat(cached_rc, &st)) {
		/* Nothing is cached, we need to scan without forking. And
		 * if we fail to generate a cached repolist, we need to
		 * invoke scan_tree manually.
		 */
		if (generate_cached_repolist(path, cached_rc))
			scan_tree(path, repo_config);
		return;
	}

	parse_configfile(cached_rc, config_cb);

	/* If the cached configfile hasn't expired, lets exit now */
	age = time(NULL) - st.st_mtime;
	if (age <= (ctx.cfg.cache_scanrc_ttl * 60))
		return;

	/* The cached repolist has been parsed, but it was old. So lets
	 * rescan the specified path and generate a new cached repolist
	 * in a child-process to avoid latency for the current request.
	 */
	if (fork())
		return;

	exit(generate_cached_repolist(path, cached_rc));
}

static void cgit_parse_args(int argc, const char **argv)
{
	int i;
	int scan = 0;

	for (i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "--cache=", 8)) {
			ctx.cfg.cache_root = xstrdup(argv[i]+8);
		}
		if (!strcmp(argv[i], "--nocache")) {
			ctx.cfg.nocache = 1;
		}
		if (!strcmp(argv[i], "--nohttp")) {
			ctx.env.no_http = "1";
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
		if (!strncmp(argv[i], "--scan-tree=", 12) ||
		    !strncmp(argv[i], "--scan-path=", 12)) {
			/* HACK: the global snapshot bitmask defines the
			 * set of allowed snapshot formats, but the config
			 * file hasn't been parsed yet so the mask is
			 * currently 0. By setting all bits high before
			 * scanning we make sure that any in-repo cgitrc
			 * snapshot setting is respected by scan_tree().
			 * BTW: we assume that there'll never be more than
			 * 255 different snapshot formats supported by cgit...
			 */
			ctx.cfg.snapshots = 0xFF;
			scan++;
			scan_tree(argv[i] + 12, repo_config);
		}
	}
	if (scan) {
		qsort(cgit_repolist.repos, cgit_repolist.count,
			sizeof(struct cgit_repo), cmp_repos);
		print_repolist(stdout, &cgit_repolist, 0);
		exit(0);
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
	const char *path;
	char *qry;
	int err, ttl;

	prepare_context(&ctx);
	cgit_repolist.length = 0;
	cgit_repolist.count = 0;
	cgit_repolist.repos = NULL;

	cgit_parse_args(argc, argv);
	parse_configfile(ctx.env.cgit_config, config_cb);
	ctx.repo = NULL;
	http_parse_querystring(ctx.qry.raw, querystring_cb);

	/* If virtual-root isn't specified in cgitrc, lets pretend
	 * that virtual-root equals SCRIPT_NAME.
	 */
	if (!ctx.cfg.virtual_root)
		ctx.cfg.virtual_root = ctx.cfg.script_name;

	/* If no url parameter is specified on the querystring, lets
	 * use PATH_INFO as url. This allows cgit to work with virtual
	 * urls without the need for rewriterules in the webserver (as
	 * long as PATH_INFO is included in the cache lookup key).
	 */
	path = ctx.env.path_info;
	if (!ctx.qry.url && path) {
		if (path[0] == '/')
			path++;
		ctx.qry.url = xstrdup(path);
		if (ctx.qry.raw) {
			qry = ctx.qry.raw;
			ctx.qry.raw = xstrdup(fmt("%s?%s", path, qry));
			free(qry);
		} else
			ctx.qry.raw = xstrdup(ctx.qry.url);
		cgit_parse_url(ctx.qry.url);
	}

	ttl = calc_ttl();
	ctx.page.expires += ttl*60;
	if (ctx.env.request_method && !strcmp(ctx.env.request_method, "HEAD"))
		ctx.cfg.nocache = 1;
	if (ctx.cfg.nocache)
		ctx.cfg.cache_size = 0;
	err = cache_process(ctx.cfg.cache_size, ctx.cfg.cache_root,
			    ctx.qry.raw, ttl, process_request, &ctx);
	if (err)
		cgit_print_error(fmt("Error processing page: %s (%d)",
				     strerror(err), err));
	return err;
}
