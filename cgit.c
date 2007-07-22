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
	if (!cgit_repo && cgit_query_repo) {
		char *title = fmt("%s - %s", cgit_root_title, "Bad request");
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title, 0);
		cgit_print_error(fmt("Unknown repo: %s", cgit_query_repo));
		cgit_print_docend();
		return 0;
	}

	if (!cgit_repo) {
		item->name = xstrdup(fmt("%s/index.html", cgit_cache_root));
		item->ttl = cgit_cache_root_ttl;
		return 1;
	}

	if (!cgit_cmd) {
		item->name = xstrdup(fmt("%s/%s/index.%s.html", cgit_cache_root,
					 cache_safe_filename(cgit_repo->url),
					 cache_safe_filename(cgit_querystring)));
		item->ttl = cgit_cache_repo_ttl;
	} else {
		item->name = xstrdup(fmt("%s/%s/%s/%s.html", cgit_cache_root,
					 cache_safe_filename(cgit_repo->url),
					 cgit_query_page,
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

	if ((cgit_cmd == CMD_SNAPSHOT) && cgit_repo->snapshots) {
		cgit_print_snapshot(item, cgit_query_sha1,
				    cgit_repobasename(cgit_repo->url),
				    cgit_query_name,
				    cgit_repo->snapshots );
		return;
	}

	if (cgit_cmd == CMD_BLOB) {
		cgit_print_blob(item, cgit_query_sha1, cgit_query_path);
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

	cgit_print_pageheader(cgit_query_page, show_search);

	switch(cgit_cmd) {
	case CMD_LOG:
		cgit_print_log(cgit_query_sha1, cgit_query_ofs,
			       cgit_max_commit_count, cgit_query_search,
			       cgit_query_path, 1);
		break;
	case CMD_TREE:
		cgit_print_tree(cgit_query_sha1, cgit_query_path);
		break;
	case CMD_COMMIT:
		cgit_print_commit(cgit_query_sha1);
		break;
	case CMD_TAG:
		cgit_print_tag(cgit_query_sha1);
		break;
	case CMD_DIFF:
		cgit_print_diff(cgit_query_sha1, cgit_query_sha2);
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

	if (cgit_repo)
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
	const char *cgit_config_env = getenv("CGIT_CONFIG");

	htmlfd = STDOUT_FILENO;
	item.st.st_mtime = time(NULL);
	cgit_repolist.length = 0;
	cgit_repolist.count = 0;
	cgit_repolist.repos = NULL;

	cgit_read_config(cgit_config_env ? cgit_config_env : CGIT_CONFIG,
			 cgit_global_config_cb);
	cgit_repo = NULL;
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
