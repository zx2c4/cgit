/*
 * Since git has it's own cache.h which we include,
 * lets test on CGIT_CACHE_H to avoid confusion
 */

#ifndef CGIT_CACHE_H
#define CGIT_CACHE_H

struct cacheitem {
	char *name;
	struct stat st;
	int ttl;
	int fd;
};

extern char *cache_safe_filename(const char *unsafe);
extern int cache_lock(struct cacheitem *item);
extern int cache_unlock(struct cacheitem *item);
extern int cache_cancel_lock(struct cacheitem *item);
extern int cache_exist(struct cacheitem *item);
extern int cache_expired(struct cacheitem *item);

#endif /* CGIT_CACHE_H */
