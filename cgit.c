/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
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
#include "ui-blob.h"
#include "ui-summary.h"
#include "scan-tree.h"

const char *cgit_version = CGIT_VERSION;

static void add_mimetype(const char *name, const char *value)
{
	struct string_list_item *item;

	item = string_list_insert(&ctx.cfg.mimetypes, name);
	item->util = xstrdup(value);
}

static void process_cached_repolist(const char *path);

static void repo_config(struct cgit_repo *repo, const char *name, const char *value)
{
	const char *path;
	struct string_list_item *item;

	if (!strcmp(name, "name"))
		repo->name = xstrdup(value);
	else if (!strcmp(name, "clone-url"))
		repo->clone_url = xstrdup(value);
	else if (!strcmp(name, "desc"))
		repo->desc = xstrdup(value);
	else if (!strcmp(name, "owner"))
		repo->owner = xstrdup(value);
	else if (!strcmp(name, "homepage"))
		repo->homepage = xstrdup(value);
	else if (!strcmp(name, "defbranch"))
		repo->defbranch = xstrdup(value);
	else if (!strcmp(name, "extra-head-content"))
		repo->extra_head_content = xstrdup(value);
	else if (!strcmp(name, "snapshots"))
		repo->snapshots = ctx.cfg.snapshots & cgit_parse_snapshots_mask(value);
	else if (!strcmp(name, "enable-commit-graph"))
		repo->enable_commit_graph = atoi(value);
	else if (!strcmp(name, "enable-log-filecount"))
		repo->enable_log_filecount = atoi(value);
	else if (!strcmp(name, "enable-log-linecount"))
		repo->enable_log_linecount = atoi(value);
	else if (!strcmp(name, "enable-remote-branches"))
		repo->enable_remote_branches = atoi(value);
	else if (!strcmp(name, "enable-subject-links"))
		repo->enable_subject_links = atoi(value);
	else if (!strcmp(name, "enable-html-serving"))
		repo->enable_html_serving = atoi(value);
	else if (!strcmp(name, "branch-sort")) {
		if (!strcmp(value, "age"))
			repo->branch_sort = 1;
		if (!strcmp(value, "name"))
			repo->branch_sort = 0;
	} else if (!strcmp(name, "commit-sort")) {
		if (!strcmp(value, "date"))
			repo->commit_sort = 1;
		if (!strcmp(value, "topo"))
			repo->commit_sort = 2;
	} else if (!strcmp(name, "max-stats"))
		repo->max_stats = cgit_find_stats_period(value, NULL);
	else if (!strcmp(name, "module-link"))
		repo->module_link= xstrdup(value);
	else if (skip_prefix(name, "module-link.", &path)) {
		item = string_list_append(&repo->submodules, xstrdup(path));
		item->util = xstrdup(value);
	} else if (!strcmp(name, "section"))
		repo->section = xstrdup(value);
	else if (!strcmp(name, "snapshot-prefix"))
		repo->snapshot_prefix = xstrdup(value);
	else if (!strcmp(name, "readme") && value != NULL) {
		if (repo->readme.items == ctx.cfg.readme.items)
			memset(&repo->readme, 0, sizeof(repo->readme));
		string_list_append(&repo->readme, xstrdup(value));
	} else if (!strcmp(name, "logo") && value != NULL)
		repo->logo = xstrdup(value);
	else if (!strcmp(name, "logo-link") && value != NULL)
		repo->logo_link = xstrdup(value);
	else if (!strcmp(name, "hide"))
		repo->hide = atoi(value);
	else if (!strcmp(name, "ignore"))
		repo->ignore = atoi(value);
	else if (ctx.cfg.enable_filter_overrides) {
		if (!strcmp(name, "about-filter"))
			repo->about_filter = cgit_new_filter(value, ABOUT);
		else if (!strcmp(name, "commit-filter"))
			repo->commit_filter = cgit_new_filter(value, COMMIT);
		else if (!strcmp(name, "source-filter"))
			repo->source_filter = cgit_new_filter(value, SOURCE);
		else if (!strcmp(name, "email-filter"))
			repo->email_filter = cgit_new_filter(value, EMAIL);
		else if (!strcmp(name, "owner-filter"))
			repo->owner_filter = cgit_new_filter(value, OWNER);
	}
}

static void config_cb(const char *name, const char *value)
{
	const char *arg;

	if (!strcmp(name, "section"))
		ctx.cfg.section = xstrdup(value);
	else if (!strcmp(name, "repo.url"))
		ctx.repo = cgit_add_repo(value);
	else if (ctx.repo && !strcmp(name, "repo.path"))
		ctx.repo->path = trim_end(value, '/');
	else if (ctx.repo && skip_prefix(name, "repo.", &arg))
		repo_config(ctx.repo, arg, value);
	else if (!strcmp(name, "readme"))
		string_list_append(&ctx.cfg.readme, xstrdup(value));
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
	else if (!strcmp(name, "logo-link"))
		ctx.cfg.logo_link = xstrdup(value);
	else if (!strcmp(name, "module-link"))
		ctx.cfg.module_link = xstrdup(value);
	else if (!strcmp(name, "strict-export"))
		ctx.cfg.strict_export = xstrdup(value);
	else if (!strcmp(name, "virtual-root"))
		ctx.cfg.virtual_root = ensure_end(value, '/');
	else if (!strcmp(name, "noplainemail"))
		ctx.cfg.noplainemail = atoi(value);
	else if (!strcmp(name, "noheader"))
		ctx.cfg.noheader = atoi(value);
	else if (!strcmp(name, "snapshots"))
		ctx.cfg.snapshots = cgit_parse_snapshots_mask(value);
	else if (!strcmp(name, "enable-filter-overrides"))
		ctx.cfg.enable_filter_overrides = atoi(value);
	else if (!strcmp(name, "enable-follow-links"))
		ctx.cfg.enable_follow_links = atoi(value);
	else if (!strcmp(name, "enable-http-clone"))
		ctx.cfg.enable_http_clone = atoi(value);
	else if (!strcmp(name, "enable-index-links"))
		ctx.cfg.enable_index_links = atoi(value);
	else if (!strcmp(name, "enable-index-owner"))
		ctx.cfg.enable_index_owner = atoi(value);
	else if (!strcmp(name, "enable-blame"))
		ctx.cfg.enable_blame = atoi(value);
	else if (!strcmp(name, "enable-commit-graph"))
		ctx.cfg.enable_commit_graph = atoi(value);
	else if (!strcmp(name, "enable-log-filecount"))
		ctx.cfg.enable_log_filecount = atoi(value);
	else if (!strcmp(name, "enable-log-linecount"))
		ctx.cfg.enable_log_linecount = atoi(value);
	else if (!strcmp(name, "enable-remote-branches"))
		ctx.cfg.enable_remote_branches = atoi(value);
	else if (!strcmp(name, "enable-subject-links"))
		ctx.cfg.enable_subject_links = atoi(value);
	else if (!strcmp(name, "enable-html-serving"))
		ctx.cfg.enable_html_serving = atoi(value);
	else if (!strcmp(name, "enable-tree-linenumbers"))
		ctx.cfg.enable_tree_linenumbers = atoi(value);
	else if (!strcmp(name, "enable-git-config"))
		ctx.cfg.enable_git_config = atoi(value);
	else if (!strcmp(name, "max-stats"))
		ctx.cfg.max_stats = cgit_find_stats_period(value, NULL);
	else if (!strcmp(name, "cache-size"))
		ctx.cfg.cache_size = atoi(value);
	else if (!strcmp(name, "cache-root"))
		ctx.cfg.cache_root = xstrdup(expand_macros(value));
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
	else if (!strcmp(name, "cache-about-ttl"))
		ctx.cfg.cache_about_ttl = atoi(value);
	else if (!strcmp(name, "cache-snapshot-ttl"))
		ctx.cfg.cache_snapshot_ttl = atoi(value);
	else if (!strcmp(name, "case-sensitive-sort"))
		ctx.cfg.case_sensitive_sort = atoi(value);
	else if (!strcmp(name, "about-filter"))
		ctx.cfg.about_filter = cgit_new_filter(value, ABOUT);
	else if (!strcmp(name, "commit-filter"))
		ctx.cfg.commit_filter = cgit_new_filter(value, COMMIT);
	else if (!strcmp(name, "email-filter"))
		ctx.cfg.email_filter = cgit_new_filter(value, EMAIL);
	else if (!strcmp(name, "owner-filter"))
		ctx.cfg.owner_filter = cgit_new_filter(value, OWNER);
	else if (!strcmp(name, "auth-filter"))
		ctx.cfg.auth_filter = cgit_new_filter(value, AUTH);
	else if (!strcmp(name, "embedded"))
		ctx.cfg.embedded = atoi(value);
	else if (!strcmp(name, "max-atom-items"))
		ctx.cfg.max_atom_items = atoi(value);
	else if (!strcmp(name, "max-message-length"))
		ctx.cfg.max_msg_len = atoi(value);
	else if (!strcmp(name, "max-repodesc-length"))
		ctx.cfg.max_repodesc_len = atoi(value);
	else if (!strcmp(name, "max-blob-size"))
		ctx.cfg.max_blob_size = atoi(value);
	else if (!strcmp(name, "max-repo-count"))
		ctx.cfg.max_repo_count = atoi(value);
	else if (!strcmp(name, "max-commit-count"))
		ctx.cfg.max_commit_count = atoi(value);
	else if (!strcmp(name, "project-list"))
		ctx.cfg.project_list = xstrdup(expand_macros(value));
	else if (!strcmp(name, "scan-path"))
		if (ctx.cfg.cache_size)
			process_cached_repolist(expand_macros(value));
		else if (ctx.cfg.project_list)
			scan_projects(expand_macros(value),
				      ctx.cfg.project_list, repo_config);
		else
			scan_tree(expand_macros(value), repo_config);
	else if (!strcmp(name, "scan-hidden-path"))
		ctx.cfg.scan_hidden_path = atoi(value);
	else if (!strcmp(name, "section-from-path"))
		ctx.cfg.section_from_path = atoi(value);
	else if (!strcmp(name, "repository-sort"))
		ctx.cfg.repository_sort = xstrdup(value);
	else if (!strcmp(name, "section-sort"))
		ctx.cfg.section_sort = atoi(value);
	else if (!strcmp(name, "source-filter"))
		ctx.cfg.source_filter = cgit_new_filter(value, SOURCE);
	else if (!strcmp(name, "summary-log"))
		ctx.cfg.summary_log = atoi(value);
	else if (!strcmp(name, "summary-branches"))
		ctx.cfg.summary_branches = atoi(value);
	else if (!strcmp(name, "summary-tags"))
		ctx.cfg.summary_tags = atoi(value);
	else if (!strcmp(name, "side-by-side-diffs"))
		ctx.cfg.difftype = atoi(value) ? DIFF_SSDIFF : DIFF_UNIFIED;
	else if (!strcmp(name, "agefile"))
		ctx.cfg.agefile = xstrdup(value);
	else if (!strcmp(name, "mimetype-file"))
		ctx.cfg.mimetype_file = xstrdup(value);
	else if (!strcmp(name, "renamelimit"))
		ctx.cfg.renamelimit = atoi(value);
	else if (!strcmp(name, "remove-suffix"))
		ctx.cfg.remove_suffix = atoi(value);
	else if (!strcmp(name, "robots"))
		ctx.cfg.robots = xstrdup(value);
	else if (!strcmp(name, "clone-prefix"))
		ctx.cfg.clone_prefix = xstrdup(value);
	else if (!strcmp(name, "clone-url"))
		ctx.cfg.clone_url = xstrdup(value);
	else if (!strcmp(name, "local-time"))
		ctx.cfg.local_time = atoi(value);
	else if (!strcmp(name, "commit-sort")) {
		if (!strcmp(value, "date"))
			ctx.cfg.commit_sort = 1;
		if (!strcmp(value, "topo"))
			ctx.cfg.commit_sort = 2;
	} else if (!strcmp(name, "branch-sort")) {
		if (!strcmp(value, "age"))
			ctx.cfg.branch_sort = 1;
		if (!strcmp(value, "name"))
			ctx.cfg.branch_sort = 0;
	} else if (skip_prefix(name, "mimetype.", &arg))
		add_mimetype(arg, value);
	else if (!strcmp(name, "include"))
		parse_configfile(expand_macros(value), config_cb);
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
	} else if (!strcmp(name, "s")) {
		ctx.qry.sort = xstrdup(value);
	} else if (!strcmp(name, "showmsg")) {
		ctx.qry.showmsg = atoi(value);
	} else if (!strcmp(name, "period")) {
		ctx.qry.period = xstrdup(value);
	} else if (!strcmp(name, "dt")) {
		ctx.qry.difftype = atoi(value);
		ctx.qry.has_difftype = 1;
	} else if (!strcmp(name, "ss")) {
		/* No longer generated, but there may be links out there. */
		ctx.qry.difftype = atoi(value) ? DIFF_SSDIFF : DIFF_UNIFIED;
		ctx.qry.has_difftype = 1;
	} else if (!strcmp(name, "all")) {
		ctx.qry.show_all = atoi(value);
	} else if (!strcmp(name, "context")) {
		ctx.qry.context = atoi(value);
	} else if (!strcmp(name, "ignorews")) {
		ctx.qry.ignorews = atoi(value);
	} else if (!strcmp(name, "follow")) {
		ctx.qry.follow = atoi(value);
	}
}

static void prepare_context(void)
{
	memset(&ctx, 0, sizeof(ctx));
	ctx.cfg.agefile = "info/web/last-modified";
	ctx.cfg.cache_size = 0;
	ctx.cfg.cache_max_create_time = 5;
	ctx.cfg.cache_root = CGIT_CACHE_ROOT;
	ctx.cfg.cache_about_ttl = 15;
	ctx.cfg.cache_snapshot_ttl = 5;
	ctx.cfg.cache_repo_ttl = 5;
	ctx.cfg.cache_root_ttl = 5;
	ctx.cfg.cache_scanrc_ttl = 15;
	ctx.cfg.cache_dynamic_ttl = 5;
	ctx.cfg.cache_static_ttl = -1;
	ctx.cfg.case_sensitive_sort = 1;
	ctx.cfg.branch_sort = 0;
	ctx.cfg.commit_sort = 0;
	ctx.cfg.css = "/cgit.css";
	ctx.cfg.logo = "/cgit.png";
	ctx.cfg.favicon = "/favicon.ico";
	ctx.cfg.local_time = 0;
	ctx.cfg.enable_http_clone = 1;
	ctx.cfg.enable_index_owner = 1;
	ctx.cfg.enable_tree_linenumbers = 1;
	ctx.cfg.enable_git_config = 0;
	ctx.cfg.max_repo_count = 50;
	ctx.cfg.max_commit_count = 50;
	ctx.cfg.max_lock_attempts = 5;
	ctx.cfg.max_msg_len = 80;
	ctx.cfg.max_repodesc_len = 80;
	ctx.cfg.max_blob_size = 0;
	ctx.cfg.max_stats = 0;
	ctx.cfg.project_list = NULL;
	ctx.cfg.renamelimit = -1;
	ctx.cfg.remove_suffix = 0;
	ctx.cfg.robots = "index, nofollow";
	ctx.cfg.root_title = "Git repository browser";
	ctx.cfg.root_desc = "a fast webinterface for the git dscm";
	ctx.cfg.scan_hidden_path = 0;
	ctx.cfg.script_name = CGIT_SCRIPT_NAME;
	ctx.cfg.section = "";
	ctx.cfg.repository_sort = "name";
	ctx.cfg.section_sort = 1;
	ctx.cfg.summary_branches = 10;
	ctx.cfg.summary_log = 10;
	ctx.cfg.summary_tags = 10;
	ctx.cfg.max_atom_items = 10;
	ctx.cfg.difftype = DIFF_UNIFIED;
	ctx.env.cgit_config = getenv("CGIT_CONFIG");
	ctx.env.http_host = getenv("HTTP_HOST");
	ctx.env.https = getenv("HTTPS");
	ctx.env.no_http = getenv("NO_HTTP");
	ctx.env.path_info = getenv("PATH_INFO");
	ctx.env.query_string = getenv("QUERY_STRING");
	ctx.env.request_method = getenv("REQUEST_METHOD");
	ctx.env.script_name = getenv("SCRIPT_NAME");
	ctx.env.server_name = getenv("SERVER_NAME");
	ctx.env.server_port = getenv("SERVER_PORT");
	ctx.env.http_cookie = getenv("HTTP_COOKIE");
	ctx.env.http_referer = getenv("HTTP_REFERER");
	ctx.env.content_length = getenv("CONTENT_LENGTH") ? strtoul(getenv("CONTENT_LENGTH"), NULL, 10) : 0;
	ctx.env.authenticated = 0;
	ctx.page.mimetype = "text/html";
	ctx.page.charset = PAGE_ENCODING;
	ctx.page.filename = NULL;
	ctx.page.size = 0;
	ctx.page.modified = time(NULL);
	ctx.page.expires = ctx.page.modified;
	ctx.page.etag = NULL;
	string_list_init(&ctx.cfg.mimetypes, 1);
	if (ctx.env.script_name)
		ctx.cfg.script_name = xstrdup(ctx.env.script_name);
	if (ctx.env.query_string)
		ctx.qry.raw = xstrdup(ctx.env.query_string);
	if (!ctx.env.cgit_config)
		ctx.env.cgit_config = CGIT_CONFIG;
}

struct refmatch {
	char *req_ref;
	char *first_ref;
	int match;
};

static int find_current_ref(const char *refname, const struct object_id *oid,
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

static void free_refmatch_inner(struct refmatch *info)
{
	if (info->first_ref)
		free(info->first_ref);
}

static char *find_default_branch(struct cgit_repo *repo)
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
	free_refmatch_inner(&info);

	return ref;
}

static char *guess_defbranch(void)
{
	const char *ref, *refname;
	struct object_id oid;

	ref = resolve_ref_unsafe("HEAD", 0, &oid, NULL);
	if (!ref || !skip_prefix(ref, "refs/heads/", &refname))
		return "master";
	return xstrdup(refname);
}

/* The caller must free filename and ref after calling this. */
static inline void parse_readme(const char *readme, char **filename, char **ref, struct cgit_repo *repo)
{
	const char *colon;

	*filename = NULL;
	*ref = NULL;

	if (!readme || !readme[0])
		return;

	/* Check if the readme is tracked in the git repo. */
	colon = strchr(readme, ':');
	if (colon && strlen(colon) > 1) {
		/* If it starts with a colon, we want to use
		 * the default branch */
		if (colon == readme && repo->defbranch)
			*ref = xstrdup(repo->defbranch);
		else
			*ref = xstrndup(readme, colon - readme);
		readme = colon + 1;
	}

	/* Prepend repo path to relative readme path unless tracked. */
	if (!(*ref) && readme[0] != '/')
		*filename = fmtalloc("%s/%s", repo->path, readme);
	else
		*filename = xstrdup(readme);
}
static void choose_readme(struct cgit_repo *repo)
{
	int found;
	char *filename, *ref;
	struct string_list_item *entry;

	if (!repo->readme.nr)
		return;

	found = 0;
	for_each_string_list_item(entry, &repo->readme) {
		parse_readme(entry->string, &filename, &ref, repo);
		if (!filename) {
			free(filename);
			free(ref);
			continue;
		}
		if (ref) {
			if (cgit_ref_path_exists(filename, ref, 1)) {
				found = 1;
				break;
			}
		}
		else if (!access(filename, R_OK)) {
			found = 1;
			break;
		}
		free(filename);
		free(ref);
	}
	repo->readme.strdup_strings = 1;
	string_list_clear(&repo->readme, 0);
	repo->readme.strdup_strings = 0;
	if (found)
		string_list_append(&repo->readme, filename)->util = ref;
}

static void print_no_repo_clone_urls(const char *url)
{
        html("<tr><td><a rel='vcs-git' href='");
        html_url_path(url);
        html("' title='");
        html_attr(ctx.repo->name);
        html(" Git repository'>");
        html_txt(url);
        html("</a></td></tr>\n");
}

static void prepare_repo_env(int *nongit)
{
	/* The path to the git repository. */
	setenv("GIT_DIR", ctx.repo->path, 1);

	/* Do not look in /etc/ for gitconfig and gitattributes. */
	setenv("GIT_CONFIG_NOSYSTEM", "1", 1);
	setenv("GIT_ATTR_NOSYSTEM", "1", 1);
	unsetenv("HOME");
	unsetenv("XDG_CONFIG_HOME");

	/* Setup the git directory and initialize the notes system. Both of these
	 * load local configuration from the git repository, so we do them both while
	 * the HOME variables are unset. */
	setup_git_directory_gently(nongit);
	init_display_notes(NULL);
}
static int prepare_repo_cmd(int nongit)
{
	struct object_id oid;
	int rc;

	if (nongit) {
		const char *name = ctx.repo->name;
		rc = errno;
		ctx.page.title = fmtalloc("%s - %s", ctx.cfg.root_title,
						"config error");
		ctx.repo = NULL;
		cgit_print_http_headers();
		cgit_print_docstart();
		cgit_print_pageheader();
		cgit_print_error("Failed to open %s: %s", name,
				 rc ? strerror(rc) : "Not a valid git repository");
		cgit_print_docend();
		return 1;
	}
	ctx.page.title = fmtalloc("%s - %s", ctx.repo->name, ctx.repo->desc);

	if (!ctx.repo->defbranch)
		ctx.repo->defbranch = guess_defbranch();

	if (!ctx.qry.head) {
		ctx.qry.nohead = 1;
		ctx.qry.head = find_default_branch(ctx.repo);
	}

	if (!ctx.qry.head) {
		cgit_print_http_headers();
		cgit_print_docstart();
		cgit_print_pageheader();
		cgit_print_error("Repository seems to be empty");
		if (!strcmp(ctx.qry.page, "summary")) {
			html("<table class='list'><tr class='nohover'><td>&nbsp;</td></tr><tr class='nohover'><th class='left'>Clone</th></tr>\n");
			cgit_prepare_repo_env(ctx.repo);
			cgit_add_clone_urls(print_no_repo_clone_urls);
			html("</table>\n");
		}
		cgit_print_docend();
		return 1;
	}

	if (get_oid(ctx.qry.head, &oid)) {
		char *old_head = ctx.qry.head;
		ctx.qry.head = xstrdup(ctx.repo->defbranch);
		cgit_print_error_page(404, "Not found",
				"Invalid branch: %s", old_head);
		free(old_head);
		return 1;
	}
	string_list_sort(&ctx.repo->submodules);
	cgit_prepare_repo_env(ctx.repo);
	choose_readme(ctx.repo);
	return 0;
}

static inline void open_auth_filter(const char *function)
{
	cgit_open_filter(ctx.cfg.auth_filter, function,
		ctx.env.http_cookie ? ctx.env.http_cookie : "",
		ctx.env.request_method ? ctx.env.request_method : "",
		ctx.env.query_string ? ctx.env.query_string : "",
		ctx.env.http_referer ? ctx.env.http_referer : "",
		ctx.env.path_info ? ctx.env.path_info : "",
		ctx.env.http_host ? ctx.env.http_host : "",
		ctx.env.https ? ctx.env.https : "",
		ctx.qry.repo ? ctx.qry.repo : "",
		ctx.qry.page ? ctx.qry.page : "",
		cgit_currentfullurl(),
		cgit_loginurl());
}

/* We intentionally keep this rather small, instead of looping and
 * feeding it to the filter a couple bytes at a time. This way, the
 * filter itself does not need to handle any denial of service or
 * buffer bloat issues. If this winds up being too small, people
 * will complain on the mailing list, and we'll increase it as needed. */
#define MAX_AUTHENTICATION_POST_BYTES 4096
/* The filter is expected to spit out "Status: " and all headers. */
static inline void authenticate_post(void)
{
	char buffer[MAX_AUTHENTICATION_POST_BYTES];
	ssize_t len;

	open_auth_filter("authenticate-post");
	len = ctx.env.content_length;
	if (len > MAX_AUTHENTICATION_POST_BYTES)
		len = MAX_AUTHENTICATION_POST_BYTES;
	if ((len = read(STDIN_FILENO, buffer, len)) < 0)
		die_errno("Could not read POST from stdin");
	if (write(STDOUT_FILENO, buffer, len) < 0)
		die_errno("Could not write POST to stdout");
	cgit_close_filter(ctx.cfg.auth_filter);
	exit(0);
}

static inline void authenticate_cookie(void)
{
	/* If we don't have an auth_filter, consider all cookies valid, and thus return early. */
	if (!ctx.cfg.auth_filter) {
		ctx.env.authenticated = 1;
		return;
	}

	/* If we're having something POST'd to /login, we're authenticating POST,
	 * instead of the cookie, so call authenticate_post and bail out early.
	 * This pattern here should match /?p=login with POST. */
	if (ctx.env.request_method && ctx.qry.page && !ctx.repo && \
	    !strcmp(ctx.env.request_method, "POST") && !strcmp(ctx.qry.page, "login")) {
		authenticate_post();
		return;
	}

	/* If we've made it this far, we're authenticating the cookie for real, so do that. */
	open_auth_filter("authenticate-cookie");
	ctx.env.authenticated = cgit_close_filter(ctx.cfg.auth_filter);
}

static void process_request(void)
{
	struct cgit_cmd *cmd;
	int nongit = 0;

	/* If we're not yet authenticated, no matter what page we're on,
	 * display the authentication body from the auth_filter. This should
	 * never be cached. */
	if (!ctx.env.authenticated) {
		ctx.page.title = "Authentication Required";
		cgit_print_http_headers();
		cgit_print_docstart();
		cgit_print_pageheader();
		open_auth_filter("body");
		cgit_close_filter(ctx.cfg.auth_filter);
		cgit_print_docend();
		return;
	}

	if (ctx.repo)
		prepare_repo_env(&nongit);

	cmd = cgit_get_cmd();
	if (!cmd) {
		ctx.page.title = "cgit error";
		cgit_print_error_page(404, "Not found", "Invalid request");
		return;
	}

	if (!ctx.cfg.enable_http_clone && cmd->is_clone) {
		ctx.page.title = "cgit error";
		cgit_print_error_page(404, "Not found", "Invalid request");
		return;
	}

	if (cmd->want_repo && !ctx.repo) {
		cgit_print_error_page(400, "Bad request",
				"No repository selected");
		return;
	}

	/* If cmd->want_vpath is set, assume ctx.qry.path contains a "virtual"
	 * in-project path limit to be made available at ctx.qry.vpath.
	 * Otherwise, no path limit is in effect (ctx.qry.vpath = NULL).
	 */
	ctx.qry.vpath = cmd->want_vpath ? ctx.qry.path : NULL;

	if (ctx.repo && prepare_repo_cmd(nongit))
		return;

	cmd->fn();
}

static int cmp_repos(const void *a, const void *b)
{
	const struct cgit_repo *ra = a, *rb = b;
	return strcmp(ra->url, rb->url);
}

static char *build_snapshot_setting(int bitmap)
{
	const struct cgit_snapshot_format *f;
	struct strbuf result = STRBUF_INIT;

	for (f = cgit_snapshot_formats; f->suffix; f++) {
		if (cgit_snapshot_format_bit(f) & bitmap) {
			if (result.len)
				strbuf_addch(&result, ' ');
			strbuf_addstr(&result, f->suffix);
		}
	}
	return strbuf_detach(&result, NULL);
}

static char *get_first_line(char *txt)
{
	char *t = xstrdup(txt);
	char *p = strchr(t, '\n');
	if (p)
		*p = '\0';
	return t;
}

static void print_repo(FILE *f, struct cgit_repo *repo)
{
	struct string_list_item *item;
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
	for_each_string_list_item(item, &repo->readme) {
		if (item->util)
			fprintf(f, "repo.readme=%s:%s\n", (char *)item->util, item->string);
		else
			fprintf(f, "repo.readme=%s\n", item->string);
	}
	if (repo->defbranch)
		fprintf(f, "repo.defbranch=%s\n", repo->defbranch);
	if (repo->extra_head_content)
		fprintf(f, "repo.extra-head-content=%s\n", repo->extra_head_content);
	if (repo->module_link)
		fprintf(f, "repo.module-link=%s\n", repo->module_link);
	if (repo->section)
		fprintf(f, "repo.section=%s\n", repo->section);
	if (repo->homepage)
		fprintf(f, "repo.homepage=%s\n", repo->homepage);
	if (repo->clone_url)
		fprintf(f, "repo.clone-url=%s\n", repo->clone_url);
	fprintf(f, "repo.enable-commit-graph=%d\n",
	        repo->enable_commit_graph);
	fprintf(f, "repo.enable-log-filecount=%d\n",
	        repo->enable_log_filecount);
	fprintf(f, "repo.enable-log-linecount=%d\n",
	        repo->enable_log_linecount);
	if (repo->about_filter && repo->about_filter != ctx.cfg.about_filter)
		cgit_fprintf_filter(repo->about_filter, f, "repo.about-filter=");
	if (repo->commit_filter && repo->commit_filter != ctx.cfg.commit_filter)
		cgit_fprintf_filter(repo->commit_filter, f, "repo.commit-filter=");
	if (repo->source_filter && repo->source_filter != ctx.cfg.source_filter)
		cgit_fprintf_filter(repo->source_filter, f, "repo.source-filter=");
	if (repo->email_filter && repo->email_filter != ctx.cfg.email_filter)
		cgit_fprintf_filter(repo->email_filter, f, "repo.email-filter=");
	if (repo->owner_filter && repo->owner_filter != ctx.cfg.owner_filter)
		cgit_fprintf_filter(repo->owner_filter, f, "repo.owner-filter=");
	if (repo->snapshots != ctx.cfg.snapshots) {
		char *tmp = build_snapshot_setting(repo->snapshots);
		fprintf(f, "repo.snapshots=%s\n", tmp ? tmp : "");
		free(tmp);
	}
	if (repo->snapshot_prefix)
		fprintf(f, "repo.snapshot-prefix=%s\n", repo->snapshot_prefix);
	if (repo->max_stats != ctx.cfg.max_stats)
		fprintf(f, "repo.max-stats=%s\n",
		        cgit_find_stats_periodname(repo->max_stats));
	if (repo->logo)
		fprintf(f, "repo.logo=%s\n", repo->logo);
	if (repo->logo_link)
		fprintf(f, "repo.logo-link=%s\n", repo->logo_link);
	fprintf(f, "repo.enable-remote-branches=%d\n", repo->enable_remote_branches);
	fprintf(f, "repo.enable-subject-links=%d\n", repo->enable_subject_links);
	fprintf(f, "repo.enable-html-serving=%d\n", repo->enable_html_serving);
	if (repo->branch_sort == 1)
		fprintf(f, "repo.branch-sort=age\n");
	if (repo->commit_sort) {
		if (repo->commit_sort == 1)
			fprintf(f, "repo.commit-sort=date\n");
		else if (repo->commit_sort == 2)
			fprintf(f, "repo.commit-sort=topo\n");
	}
	fprintf(f, "repo.hide=%d\n", repo->hide);
	fprintf(f, "repo.ignore=%d\n", repo->ignore);
	fprintf(f, "\n");
}

static void print_repolist(FILE *f, struct cgit_repolist *list, int start)
{
	int i;

	for (i = start; i < list->count; i++)
		print_repo(f, &list->repos[i]);
}

/* Scan 'path' for git repositories, save the resulting repolist in 'cached_rc'
 * and return 0 on success.
 */
static int generate_cached_repolist(const char *path, const char *cached_rc)
{
	struct strbuf locked_rc = STRBUF_INIT;
	int result = 0;
	int idx;
	FILE *f;

	strbuf_addf(&locked_rc, "%s.lock", cached_rc);
	f = fopen(locked_rc.buf, "wx");
	if (!f) {
		/* Inform about the error unless the lockfile already existed,
		 * since that only means we've got concurrent requests.
		 */
		result = errno;
		if (result != EEXIST)
			fprintf(stderr, "[cgit] Error opening %s: %s (%d)\n",
				locked_rc.buf, strerror(result), result);
		goto out;
	}
	idx = cgit_repolist.count;
	if (ctx.cfg.project_list)
		scan_projects(path, ctx.cfg.project_list, repo_config);
	else
		scan_tree(path, repo_config);
	print_repolist(f, &cgit_repolist, idx);
	if (rename(locked_rc.buf, cached_rc))
		fprintf(stderr, "[cgit] Error renaming %s to %s: %s (%d)\n",
			locked_rc.buf, cached_rc, strerror(errno), errno);
	fclose(f);
out:
	strbuf_release(&locked_rc);
	return result;
}

static void process_cached_repolist(const char *path)
{
	struct stat st;
	struct strbuf cached_rc = STRBUF_INIT;
	time_t age;
	unsigned long hash;

	hash = hash_str(path);
	if (ctx.cfg.project_list)
		hash += hash_str(ctx.cfg.project_list);
	strbuf_addf(&cached_rc, "%s/rc-%8lx", ctx.cfg.cache_root, hash);

	if (stat(cached_rc.buf, &st)) {
		/* Nothing is cached, we need to scan without forking. And
		 * if we fail to generate a cached repolist, we need to
		 * invoke scan_tree manually.
		 */
		if (generate_cached_repolist(path, cached_rc.buf)) {
			if (ctx.cfg.project_list)
				scan_projects(path, ctx.cfg.project_list,
					      repo_config);
			else
				scan_tree(path, repo_config);
		}
		goto out;
	}

	parse_configfile(cached_rc.buf, config_cb);

	/* If the cached configfile hasn't expired, lets exit now */
	age = time(NULL) - st.st_mtime;
	if (age <= (ctx.cfg.cache_scanrc_ttl * 60))
		goto out;

	/* The cached repolist has been parsed, but it was old. So lets
	 * rescan the specified path and generate a new cached repolist
	 * in a child-process to avoid latency for the current request.
	 */
	if (fork())
		goto out;

	exit(generate_cached_repolist(path, cached_rc.buf));
out:
	strbuf_release(&cached_rc);
}

static void cgit_parse_args(int argc, const char **argv)
{
	int i;
	const char *arg;
	int scan = 0;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--version")) {
			printf("CGit %s | https://git.zx2c4.com/cgit/\n\nCompiled in features:\n", CGIT_VERSION);
#ifdef NO_LUA
			printf("[-] ");
#else
			printf("[+] ");
#endif
			printf("Lua scripting\n");
#ifndef HAVE_LINUX_SENDFILE
			printf("[-] ");
#else
			printf("[+] ");
#endif
			printf("Linux sendfile() usage\n");

			exit(0);
		}
		if (skip_prefix(argv[i], "--cache=", &arg)) {
			ctx.cfg.cache_root = xstrdup(arg);
		} else if (!strcmp(argv[i], "--nohttp")) {
			ctx.env.no_http = "1";
		} else if (skip_prefix(argv[i], "--query=", &arg)) {
			ctx.qry.raw = xstrdup(arg);
		} else if (skip_prefix(argv[i], "--repo=", &arg)) {
			ctx.qry.repo = xstrdup(arg);
		} else if (skip_prefix(argv[i], "--page=", &arg)) {
			ctx.qry.page = xstrdup(arg);
		} else if (skip_prefix(argv[i], "--head=", &arg)) {
			ctx.qry.head = xstrdup(arg);
			ctx.qry.has_symref = 1;
		} else if (skip_prefix(argv[i], "--sha1=", &arg)) {
			ctx.qry.sha1 = xstrdup(arg);
			ctx.qry.has_sha1 = 1;
		} else if (skip_prefix(argv[i], "--ofs=", &arg)) {
			ctx.qry.ofs = atoi(arg);
		} else if (skip_prefix(argv[i], "--scan-tree=", &arg) ||
		           skip_prefix(argv[i], "--scan-path=", &arg)) {
			/*
			 * HACK: The global snapshot bit mask defines the set
			 * of allowed snapshot formats, but the config file
			 * hasn't been parsed yet so the mask is currently 0.
			 * By setting all bits high before scanning we make
			 * sure that any in-repo cgitrc snapshot setting is
			 * respected by scan_tree().
			 *
			 * NOTE: We assume that there aren't more than 8
			 * different snapshot formats supported by cgit...
			 */
			ctx.cfg.snapshots = 0xFF;
			scan++;
			scan_tree(arg, repo_config);
		}
	}
	if (scan) {
		qsort(cgit_repolist.repos, cgit_repolist.count,
			sizeof(struct cgit_repo), cmp_repos);
		print_repolist(stdout, &cgit_repolist, 0);
		exit(0);
	}
}

static int calc_ttl(void)
{
	if (!ctx.repo)
		return ctx.cfg.cache_root_ttl;

	if (!ctx.qry.page)
		return ctx.cfg.cache_repo_ttl;

	if (!strcmp(ctx.qry.page, "about"))
		return ctx.cfg.cache_about_ttl;

	if (!strcmp(ctx.qry.page, "snapshot"))
		return ctx.cfg.cache_snapshot_ttl;

	if (ctx.qry.has_sha1)
		return ctx.cfg.cache_static_ttl;

	if (ctx.qry.has_symref)
		return ctx.cfg.cache_dynamic_ttl;

	return ctx.cfg.cache_repo_ttl;
}

int cmd_main(int argc, const char **argv)
{
	const char *path;
	int err, ttl;

	cgit_init_filters();
	atexit(cgit_cleanup_filters);

	prepare_context();
	cgit_repolist.length = 0;
	cgit_repolist.count = 0;
	cgit_repolist.repos = NULL;

	cgit_parse_args(argc, argv);
	parse_configfile(expand_macros(ctx.env.cgit_config), config_cb);
	ctx.repo = NULL;
	http_parse_querystring(ctx.qry.raw, querystring_cb);

	/* If virtual-root isn't specified in cgitrc, lets pretend
	 * that virtual-root equals SCRIPT_NAME, minus any possibly
	 * trailing slashes.
	 */
	if (!ctx.cfg.virtual_root && ctx.cfg.script_name)
		ctx.cfg.virtual_root = ensure_end(ctx.cfg.script_name, '/');

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
			char *newqry = fmtalloc("%s?%s", path, ctx.qry.raw);
			free(ctx.qry.raw);
			ctx.qry.raw = newqry;
		} else
			ctx.qry.raw = xstrdup(ctx.qry.url);
		cgit_parse_url(ctx.qry.url);
	}

	/* Before we go any further, we set ctx.env.authenticated by checking to see
	 * if the supplied cookie is valid. All cookies are valid if there is no
	 * auth_filter. If there is an auth_filter, the filter decides. */
	authenticate_cookie();

	ttl = calc_ttl();
	if (ttl < 0)
		ctx.page.expires += 10 * 365 * 24 * 60 * 60; /* 10 years */
	else
		ctx.page.expires += ttl * 60;
	if (!ctx.env.authenticated || (ctx.env.request_method && !strcmp(ctx.env.request_method, "HEAD")))
		ctx.cfg.cache_size = 0;
	err = cache_process(ctx.cfg.cache_size, ctx.cfg.cache_root,
			    ctx.qry.raw, ttl, process_request);
	cgit_cleanup_filters();
	if (err)
		cgit_print_error("Error processing page: %s (%d)",
				 strerror(err), err);
	return err;
}
