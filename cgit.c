/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

static int cgit_prepare_cache(struct cacheitem *item)
{
	if (!ctx.repo && ctx.qry.repo) {
		char *title = fmt("%s - %s", ctx.cfg.root_title, "Bad request");
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title, 0);
		cgit_print_error(fmt("Unknown repo: %s", ctx.qry.repo));
		cgit_print_docend();
		return 0;
	}

	if (!ctx.repo) {
		item->name = xstrdup(fmt("%s/index.html", ctx.cfg.cache_root));
		item->ttl = ctx.cfg.cache_root_ttl;
		return 1;
	}

	if (!cgit_cmd) {
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

static void cgit_print_repo_page(struct cacheitem *item)
{
	char *title, *tmp;
	int show_search;
	unsigned char sha1[20];

	if (chdir(ctx.repo->path)) {
		title = fmt("%s - %s", ctx.cfg.root_title, "Bad request");
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title, 0);
		cgit_print_error(fmt("Unable to scan repository: %s",
				     strerror(errno)));
		cgit_print_docend();
		return;
	}

	title = fmt("%s - %s", ctx.repo->name, ctx.repo->desc);
	show_search = 0;
	setenv("GIT_DIR", ctx.repo->path, 1);

	if (!ctx.qry.head) {
		ctx.qry.head = xstrdup(find_default_branch(ctx.repo));
		ctx.repo->defbranch = ctx.qry.head;
	}

	if (!ctx.qry.head) {
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title, 0);
		cgit_print_error("Repository seems to be empty");
		cgit_print_docend();
		return;
	}

	if (get_sha1(ctx.qry.head, sha1)) {
		tmp = xstrdup(ctx.qry.head);
		ctx.qry.head = ctx.repo->defbranch;
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title, 0);
		cgit_print_error(fmt("Invalid branch: %s", tmp));
		cgit_print_docend();
		return;
	}

	if ((cgit_cmd == CMD_SNAPSHOT) && ctx.repo->snapshots) {
		cgit_print_snapshot(item, ctx.qry.head, ctx.qry.sha1,
				    cgit_repobasename(ctx.repo->url),
				    ctx.qry.path,
				    ctx.repo->snapshots );
		return;
	}

	if (cgit_cmd == CMD_PATCH) {
		cgit_print_patch(ctx.qry.sha1, item);
		return;
	}

	if (cgit_cmd == CMD_BLOB) {
		cgit_print_blob(item, ctx.qry.sha1, ctx.qry.path);
		return;
	}

	show_search = (cgit_cmd == CMD_LOG);
	cgit_print_docstart(title, item);
	if (!cgit_cmd) {
		cgit_print_pageheader("summary", show_search);
		cgit_print_summary();
		cgit_print_docend();
		return;
	}

	cgit_print_pageheader(ctx.qry.page, show_search);

	switch(cgit_cmd) {
	case CMD_LOG:
		cgit_print_log(ctx.qry.sha1, ctx.qry.ofs,
			       ctx.cfg.max_commit_count, ctx.qry.grep, ctx.qry.search,
			       ctx.qry.path, 1);
		break;
	case CMD_TREE:
		cgit_print_tree(ctx.qry.sha1, ctx.qry.path);
		break;
	case CMD_COMMIT:
		cgit_print_commit(ctx.qry.sha1);
		break;
	case CMD_REFS:
		cgit_print_refs();
		break;
	case CMD_TAG:
		cgit_print_tag(ctx.qry.sha1);
		break;
	case CMD_DIFF:
		cgit_print_diff(ctx.qry.sha1, ctx.qry.sha2, ctx.qry.path);
		break;
	default:
		cgit_print_error("Invalid request");
	}
	cgit_print_docend();
}

static void cgit_fill_cache(struct cacheitem *item, int use_cache)
{
	static char buf[PATH_MAX];
	int stdout2;

	getcwd(buf, sizeof(buf));
	item->st.st_mtime = time(NULL);

	if (use_cache) {
		stdout2 = chk_positive(dup(STDOUT_FILENO),
				       "Preserving STDOUT");
		chk_zero(close(STDOUT_FILENO), "Closing STDOUT");
		chk_positive(dup2(item->fd, STDOUT_FILENO), "Dup2(cachefile)");
	}

	if (ctx.repo)
		cgit_print_repo_page(item);
	else
		cgit_print_repolist(item);

	if (use_cache) {
		chk_zero(close(STDOUT_FILENO), "Close redirected STDOUT");
		chk_positive(dup2(stdout2, STDOUT_FILENO),
			     "Restoring original STDOUT");
		chk_zero(close(stdout2), "Closing temporary STDOUT");
	}

	chdir(buf);
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

	cgit_prepare_context(&ctx);
	htmlfd = STDOUT_FILENO;
	item.st.st_mtime = time(NULL);
	cgit_repolist.length = 0;
	cgit_repolist.count = 0;
	cgit_repolist.repos = NULL;

	cgit_read_config(cgit_config_env ? cgit_config_env : CGIT_CONFIG,
			 cgit_global_config_cb);
	if (getenv("SCRIPT_NAME"))
		ctx.cfg.script_name = xstrdup(getenv("SCRIPT_NAME"));
	if (getenv("QUERY_STRING"))
		ctx.qry.raw = xstrdup(getenv("QUERY_STRING"));
	cgit_parse_args(argc, argv);
	cgit_parse_query(ctx.qry.raw, cgit_querystring_cb);
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
