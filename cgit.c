/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

const char cgit_version[] = CGIT_VERSION;

int htmlfd = 0;

char *cgit_root         = "/usr/src/git";
char *cgit_root_title   = "Git repository browser";
char *cgit_css          = "/cgit.css";
char *cgit_logo         = "/git-logo.png";
char *cgit_logo_link    = "http://www.kernel.org/pub/software/scm/git/docs/";
char *cgit_virtual_root = NULL;

char *cgit_cache_root   = "/var/cache/cgit";

int cgit_max_lock_attempts     =  5;
int cgit_cache_root_ttl        =  5;
int cgit_cache_repo_ttl        =  5;
int cgit_cache_dynamic_ttl     =  5;
int cgit_cache_static_ttl      = -1;
int cgit_cache_max_create_time =  5;

char *cgit_repo_name    = NULL;
char *cgit_repo_desc    = NULL;
char *cgit_repo_owner   = NULL;

int cgit_query_has_symref = 0;
int cgit_query_has_sha1   = 0;

char *cgit_querystring  = NULL;
char *cgit_query_repo   = NULL;
char *cgit_query_page   = NULL;
char *cgit_query_head   = NULL;
char *cgit_query_sha1   = NULL;

struct cacheitem cacheitem;

void cgit_global_config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "root"))
		cgit_root = xstrdup(value);
	else if (!strcmp(name, "root-title"))
		cgit_root_title = xstrdup(value);
	else if (!strcmp(name, "css"))
		cgit_css = xstrdup(value);
	else if (!strcmp(name, "logo"))
		cgit_logo = xstrdup(value);
	else if (!strcmp(name, "logo-link"))
		cgit_logo_link = xstrdup(value);
	else if (!strcmp(name, "virtual-root"))
		cgit_virtual_root = xstrdup(value);
}

void cgit_repo_config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "name"))
		cgit_repo_name = xstrdup(value);
	else if (!strcmp(name, "desc"))
		cgit_repo_desc = xstrdup(value);
	else if (!strcmp(name, "owner"))
		cgit_repo_owner = xstrdup(value);
}

void cgit_querystring_cb(const char *name, const char *value)
{
	if (!strcmp(name,"r"))
		cgit_query_repo = xstrdup(value);
	else if (!strcmp(name, "p"))
		cgit_query_page = xstrdup(value);
	else if (!strcmp(name, "h")) {
		cgit_query_head = xstrdup(value);
		cgit_query_has_symref = 1;
	} else if (!strcmp(name, "id")) {
		cgit_query_sha1 = xstrdup(value);
		cgit_query_has_sha1 = 1;
	}
}

static void cgit_print_object(char *hex)
{
	unsigned char sha1[20];
	//struct object *object;
	char type[20];
	unsigned char *buf;
	unsigned long size;

	if (get_sha1_hex(hex, sha1)){
		cgit_print_error(fmt("Bad hex value: %s", hex));
	        return;
	}

	if (sha1_object_info(sha1, type, NULL)){
		cgit_print_error("Bad object name");
		return;
	}

	buf = read_sha1_file(sha1, type, &size);
	if (!buf) {
		cgit_print_error("Error reading object");
		return;
	}

	buf[size] = '\0';
	html("<h2>Object view</h2>");
	htmlf("sha1=%s<br/>type=%s<br/>size=%i<br/>", hex, type, size);
	html("<pre>");
	html_txt(buf);
	html("</pre>");
}

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
	if (!cgit_query_page)
		cgit_print_summary();
	else if (!strcmp(cgit_query_page, "log")) {
		cgit_print_log(cgit_query_head, 0, 100);
	} else if (!strcmp(cgit_query_page, "view")) {
		cgit_print_object(cgit_query_sha1);
	}
	cgit_print_docend();
}

static void cgit_fill_cache(struct cacheitem *item)
{
	htmlfd = item->fd;
	item->st.st_mtime = time(NULL);
	if (cgit_query_repo)
		cgit_print_repo_page(item);
	else
		cgit_print_repolist(item);
}

static void cgit_refresh_cache(struct cacheitem *item)
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
		if (!cache_exist(item))
			cgit_fill_cache(item);
		cache_unlock(item);
	} else if (cache_expired(item) && cache_lock(item)) {
		if (cache_expired(item))
			cgit_fill_cache(item);
		cache_unlock(item);
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

int main(int argc, const char **argv)
{
	cgit_read_config("/etc/cgitrc", cgit_global_config_cb);
	cgit_querystring = xstrdup(getenv("QUERY_STRING"));
	cgit_parse_query(cgit_querystring, cgit_querystring_cb);
	cgit_refresh_cache(&cacheitem);
	cgit_print_cache(&cacheitem);
	return 0;
}
