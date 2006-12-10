/* cache.c: cache management
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

const int NOLOCK = -1;

int cache_lookup(struct cacheitem *item)
{
	if (!cgit_query_repo) {
		item->name = xstrdup(fmt("%s/index.html", cgit_cache_root));
		item->ttl = cgit_cache_root_ttl;
	} else if (!cgit_query_page) {
		item->name = xstrdup(fmt("%s/%s/index.html", cgit_cache_root, 
			   cgit_query_repo));
		item->ttl = cgit_cache_repo_ttl;
	} else {
		item->name = xstrdup(fmt("%s/%s/%s/%s.html", cgit_cache_root, 
			   cgit_query_repo, cgit_query_page, 
			   cgit_querystring));
		if (cgit_query_has_symref)
			item->ttl = cgit_cache_dynamic_ttl;
		else if (cgit_query_has_sha1)
			item->ttl = cgit_cache_static_ttl;
		else
			item->ttl = cgit_cache_repo_ttl;
	}
	if (stat(item->name, &item->st)) {
		item->st.st_mtime = 0;
		return 0;
	}
	return 1;
}

int cache_create_dirs()
{
	char *path;

	if (!cgit_query_repo)
		return 0;

	path = fmt("%s/%s", cgit_cache_root, cgit_query_repo);
	if (mkdir(path, S_IRWXU) && errno!=EEXIST)
		return 0;

	if (cgit_query_page) {
		path = fmt("%s/%s/%s", cgit_cache_root, cgit_query_repo, 
			   cgit_query_page);
		if (mkdir(path, S_IRWXU) && errno!=EEXIST)
			return 0;
	}
	return 1;
}

int cache_lock(struct cacheitem *item)
{
	int ret;
	char *lockfile = fmt("%s.lock", item->name);

 top:  
       	item->fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR|S_IWUSR);
	if (item->fd == NOLOCK && errno == ENOENT && cache_create_dirs())
		goto top;
	if (item->fd == NOLOCK && errno == EEXIST) {
		struct stat st;
		time_t t;
		if (stat(lockfile, &st))
			return ret;
		t = time(NULL);
		if (t-st.st_mtime > cgit_cache_max_create_time && 
		    !unlink(lockfile))
			goto top;
		return 0;
	}
	return (item->fd > 0);
}

int cache_unlock(struct cacheitem *item)
{
	close(item->fd);
	return (rename(fmt("%s.lock", item->name), item->name) == 0);
}

int cache_expired(struct cacheitem *item)
{
	if (item->ttl < 0)
		return 0;
	return item->st.st_mtime + item->ttl * 60 < time(NULL);
}
