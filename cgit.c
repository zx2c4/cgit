/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

const char cgit_version[] = CGIT_VERSION;


static struct repoinfo *cgit_get_repoinfo(char *url)
{
	int i;
	struct repoinfo *repo;

	for (i=0; i<cgit_repolist.count; i++) {
		repo = &cgit_repolist.repos[i];
		if (!strcmp(repo->url, url))
			return repo;
	}
	return NULL;
}


static int cgit_prepare_cache(struct cacheitem *item)
{
	if (!cgit_query_repo) {
		item->name = xstrdup(fmt("%s/index.html", cgit_cache_root));
		item->ttl = cgit_cache_root_ttl;
		return 1;
	}
	cgit_repo = cgit_get_repoinfo(cgit_query_repo);
	if (!cgit_repo) {
		char *title = fmt("%s - %s", cgit_root_title, "Bad request");
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title, 0);
		cgit_print_error(fmt("Unknown repo: %s", cgit_query_repo));
		cgit_print_docend();
		return 0;
	}

	if (!cgit_query_page) {
		item->name = xstrdup(fmt("%s/%s/index.html", cgit_cache_root,
			   cgit_repo->url));
		item->ttl = cgit_cache_repo_ttl;
	} else {
		item->name = xstrdup(fmt("%s/%s/%s/%s.html", cgit_cache_root,
			   cgit_repo->url, cgit_query_page,
			   cache_safe_filename(cgit_querystring)));
		if (cgit_query_has_symref)
			item->ttl = cgit_cache_dynamic_ttl;
		else if (cgit_query_has_sha1)
			item->ttl = cgit_cache_static_ttl;
		else
			item->ttl = cgit_cache_repo_ttl;
	}
	return 1;
}

static void cgit_print_repo_page(struct cacheitem *item)
{
	char *title;
	int show_search;

	if (!cgit_query_head)
		cgit_query_head = cgit_repo->defbranch;

	if (chdir(cgit_repo->path)) {
		title = fmt("%s - %s", cgit_root_title, "Bad request");
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title, 0);
		cgit_print_error(fmt("Unable to scan repository: %s",
				     strerror(errno)));
		cgit_print_docend();
		return;
	}

	title = fmt("%s - %s", cgit_repo->name, cgit_repo->desc);
	show_search = 0;
	setenv("GIT_DIR", cgit_repo->path, 1);

	if (cgit_query_page) {
	    if (cgit_repo->snapshots && !strcmp(cgit_query_page, "snapshot")) {
		cgit_print_snapshot(item, cgit_query_sha1, "zip",
				    cgit_repo->url, cgit_query_name);
		return;
	    }
	    if (!strcmp(cgit_query_page, "blob")) {
		    cgit_print_blob(item, cgit_query_sha1, cgit_query_path);
		    return;
	    }
	}

	if (cgit_query_page && !strcmp(cgit_query_page, "log"))
		show_search = 1;

	cgit_print_docstart(title, item);


	if (!cgit_query_page) {
		cgit_print_pageheader("summary", show_search);
		cgit_print_summary();
		cgit_print_docend();
		return;
	}

	cgit_print_pageheader(cgit_query_page, show_search);

	if (!strcmp(cgit_query_page, "log")) {
		cgit_print_log(cgit_query_head, cgit_query_ofs,
			       cgit_max_commit_count, cgit_query_search,
			       cgit_query_path);
	} else if (!strcmp(cgit_query_page, "tree")) {
		cgit_print_tree(cgit_query_head, cgit_query_sha1, cgit_query_path);
	} else if (!strcmp(cgit_query_page, "commit")) {
		cgit_print_commit(cgit_query_head);
	} else if (!strcmp(cgit_query_page, "view")) {
		cgit_print_view(cgit_query_sha1, cgit_query_path);
	} else if (!strcmp(cgit_query_page, "diff")) {
		cgit_print_diff(cgit_query_head, cgit_query_sha1, cgit_query_sha2,
				cgit_query_path);
	} else {
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

	if (cgit_query_repo)
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
	if (++i > cgit_max_lock_attempts) {
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
			cgit_cache_root = xstrdup(argv[i]+8);
		}
		if (!strcmp(argv[i], "--nocache")) {
			cgit_nocache = 1;
		}
		if (!strncmp(argv[i], "--query=", 8)) {
			cgit_querystring = xstrdup(argv[i]+8);
		}
		if (!strncmp(argv[i], "--repo=", 7)) {
			cgit_query_repo = xstrdup(argv[i]+7);
		}
		if (!strncmp(argv[i], "--page=", 7)) {
			cgit_query_page = xstrdup(argv[i]+7);
		}
		if (!strncmp(argv[i], "--head=", 7)) {
			cgit_query_head = xstrdup(argv[i]+7);
			cgit_query_has_symref = 1;
		}
		if (!strncmp(argv[i], "--sha1=", 7)) {
			cgit_query_sha1 = xstrdup(argv[i]+7);
			cgit_query_has_sha1 = 1;
		}
		if (!strncmp(argv[i], "--ofs=", 6)) {
			cgit_query_ofs = atoi(argv[i]+6);
		}
	}
}

int main(int argc, const char **argv)
{
	struct cacheitem item;

	htmlfd = STDOUT_FILENO;
	item.st.st_mtime = time(NULL);
	cgit_repolist.length = 0;
	cgit_repolist.count = 0;
	cgit_repolist.repos = NULL;

	cgit_read_config(CGIT_CONFIG, cgit_global_config_cb);
	if (getenv("SCRIPT_NAME"))
		cgit_script_name = xstrdup(getenv("SCRIPT_NAME"));
	if (getenv("QUERY_STRING"))
		cgit_querystring = xstrdup(getenv("QUERY_STRING"));
	cgit_parse_args(argc, argv);
	cgit_parse_query(cgit_querystring, cgit_querystring_cb);
	if (!cgit_prepare_cache(&item))
		return 0;
	if (cgit_nocache) {
		cgit_fill_cache(&item, 0);
	} else {
		cgit_check_cache(&item);
		cgit_print_cache(&item);
	}
	return 0;
}
