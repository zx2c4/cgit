/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

const char cgit_version[] = CGIT_VERSION;

static void cgit_print_repo_page(struct cacheitem *item)
{
	if (chdir(fmt("%s/%s", cgit_root, cgit_query_repo)) || 
	    cgit_read_config("info/cgit", cgit_repo_config_cb)) {
		char *title = fmt("%s - %s", cgit_root_title, "Bad request");
		cgit_print_docstart(title, item);
		cgit_print_pageheader(title);
		cgit_print_error(fmt("Unable to scan repository: %s",
				     strerror(errno)));
		cgit_print_docend();
		return;
	}
	setenv("GIT_DIR", fmt("%s/%s", cgit_root, cgit_query_repo), 1);
	char *title = fmt("%s - %s", cgit_repo_name, cgit_repo_desc);
	cgit_print_docstart(title, item);
	cgit_print_pageheader(title);
	if (!cgit_query_page) {
		cgit_print_summary();
	} else if (!strcmp(cgit_query_page, "log")) {
		cgit_print_log(cgit_query_head, cgit_query_ofs, 100);
	} else if (!strcmp(cgit_query_page, "tree")) {
		cgit_print_tree(cgit_query_sha1);
	} else if (!strcmp(cgit_query_page, "commit")) {
		cgit_print_commit(cgit_query_sha1);
	} else if (!strcmp(cgit_query_page, "view")) {
		cgit_print_view(cgit_query_sha1);
	}
	cgit_print_docend();
}

static void cgit_fill_cache(struct cacheitem *item)
{
	static char buf[PATH_MAX];

	getcwd(buf, sizeof(buf));
	htmlfd = item->fd;
	item->st.st_mtime = time(NULL);
	if (cgit_query_repo)
		cgit_print_repo_page(item);
	else
		cgit_print_repolist(item);
	chdir(buf);
}

static void cgit_check_cache(struct cacheitem *item)
{
	int i = 0;

	cache_prepare(item);
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
			cgit_fill_cache(item);
			cache_unlock(item);
		} else {
			cache_cancel_lock(item);
		}
	} else if (cache_expired(item) && cache_lock(item)) {
		if (cache_expired(item)) {
			cgit_fill_cache(item);
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
		if (!strncmp(argv[i], "--root=", 7)) {
			cgit_root = xstrdup(argv[i]+7);
		}
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

	cgit_read_config("/etc/cgitrc", cgit_global_config_cb);
	if (getenv("QUERY_STRING"))
		cgit_querystring = xstrdup(getenv("QUERY_STRING"));
	cgit_parse_args(argc, argv);
	cgit_parse_query(cgit_querystring, cgit_querystring_cb);

	if (cgit_nocache) {
		item.fd = STDOUT_FILENO;
		cgit_fill_cache(&item);
	} else {
		cgit_check_cache(&item);
		cgit_print_cache(&item);
	}
	return 0;
}
