/* shared.c: global vars + some callback functions
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

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
int   cgit_query_ofs    = 0;

int htmlfd = 0;

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
	if (!strcmp(name,"r")) {
		cgit_query_repo = xstrdup(value);
	} else if (!strcmp(name, "p")) {
		cgit_query_page = xstrdup(value);
	} else if (!strcmp(name, "h")) {
		cgit_query_head = xstrdup(value);
		cgit_query_has_symref = 1;
	} else if (!strcmp(name, "id")) {
		cgit_query_sha1 = xstrdup(value);
		cgit_query_has_sha1 = 1;
	} else if (!strcmp(name, "ofs")) {
		cgit_query_ofs = atoi(value);
	}
}

