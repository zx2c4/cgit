/* cache.c: cache management
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

const int NOLOCK = -1;

char *cache_safe_filename(const char *unsafe)
{
	static char buf[4][PATH_MAX];
	static int bufidx;
	char *s;
	char c;

	bufidx++;
	bufidx &= 3;
	s = buf[bufidx];

	while(unsafe && (c = *unsafe++) != 0) {
		if (c == '/' || c == ' ' || c == '&' || c == '|' ||
		    c == '>' || c == '<' || c == '.')
			c = '_';
		*s++ = c;
	}
	*s = '\0';
	return buf[bufidx];
}

int cache_exist(struct cacheitem *item)
{
	if (stat(item->name, &item->st)) {
		item->st.st_mtime = 0;
		return 0;
	}
	return 1;
}

int cache_create_dirs()
{
	char *path;

	path = fmt("%s", ctx.cfg.cache_root);
	if (mkdir(path, S_IRWXU) && errno!=EEXIST)
		return 0;

	if (!cgit_repo)
		return 0;

	path = fmt("%s/%s", ctx.cfg.cache_root,
		   cache_safe_filename(cgit_repo->url));

	if (mkdir(path, S_IRWXU) && errno!=EEXIST)
		return 0;

	if (ctx.qry.page) {
		path = fmt("%s/%s/%s", ctx.cfg.cache_root,
			   cache_safe_filename(cgit_repo->url),
			   ctx.qry.page);
		if (mkdir(path, S_IRWXU) && errno!=EEXIST)
			return 0;
	}
	return 1;
}

int cache_refill_overdue(const char *lockfile)
{
	struct stat st;

	if (stat(lockfile, &st))
		return 0;
	else
		return (time(NULL) - st.st_mtime > ctx.cfg.cache_max_create_time);
}

int cache_lock(struct cacheitem *item)
{
	int i = 0;
	char *lockfile = xstrdup(fmt("%s.lock", item->name));

 top:
	if (++i > ctx.cfg.max_lock_attempts)
		die("cache_lock: unable to lock %s: %s",
		    item->name, strerror(errno));

       	item->fd = open(lockfile, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);

	if (item->fd == NOLOCK && errno == ENOENT && cache_create_dirs())
		goto top;

	if (item->fd == NOLOCK && errno == EEXIST &&
	    cache_refill_overdue(lockfile) && !unlink(lockfile))
			goto top;

	free(lockfile);
	return (item->fd > 0);
}

int cache_unlock(struct cacheitem *item)
{
	close(item->fd);
	return (rename(fmt("%s.lock", item->name), item->name) == 0);
}

int cache_cancel_lock(struct cacheitem *item)
{
	return (unlink(fmt("%s.lock", item->name)) == 0);
}

int cache_expired(struct cacheitem *item)
{
	if (item->ttl < 0)
		return 0;
	return item->st.st_mtime + item->ttl * 60 < time(NULL);
}
