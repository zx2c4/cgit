/* cache.c: cache management
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 *
 *
 * The cache is just a directory structure where each file is a cache slot,
 * and each filename is based on the hash of some key (e.g. the cgit url).
 * Each file contains the full key followed by the cached content for that
 * key.
 *
 */

#include "cgit.h"
#include "cache.h"
#include "html.h"
#ifdef HAVE_LINUX_SENDFILE
#include <sys/sendfile.h>
#endif

#define CACHE_BUFSIZE (1024 * 4)

struct cache_slot {
	const char *key;
	size_t keylen;
	int ttl;
	cache_fill_fn fn;
	int cache_fd;
	int lock_fd;
	int stdout_fd;
	const char *cache_name;
	const char *lock_name;
	int match;
	struct stat cache_st;
	int bufsize;
	char buf[CACHE_BUFSIZE];
};

/* Open an existing cache slot and fill the cache buffer with
 * (part of) the content of the cache file. Return 0 on success
 * and errno otherwise.
 */
static int open_slot(struct cache_slot *slot)
{
	char *bufz;
	ssize_t bufkeylen = -1;

	slot->cache_fd = open(slot->cache_name, O_RDONLY);
	if (slot->cache_fd == -1)
		return errno;

	if (fstat(slot->cache_fd, &slot->cache_st))
		return errno;

	slot->bufsize = xread(slot->cache_fd, slot->buf, sizeof(slot->buf));
	if (slot->bufsize < 0)
		return errno;

	bufz = memchr(slot->buf, 0, slot->bufsize);
	if (bufz)
		bufkeylen = bufz - slot->buf;

	if (slot->key)
		slot->match = bufkeylen == slot->keylen &&
		    !memcmp(slot->key, slot->buf, bufkeylen + 1);

	return 0;
}

/* Close the active cache slot */
static int close_slot(struct cache_slot *slot)
{
	int err = 0;
	if (slot->cache_fd > 0) {
		if (close(slot->cache_fd))
			err = errno;
		else
			slot->cache_fd = -1;
	}
	return err;
}

/* Print the content of the active cache slot (but skip the key). */
static int print_slot(struct cache_slot *slot)
{
#ifdef HAVE_LINUX_SENDFILE
	off_t start_off;
	int ret;

	start_off = slot->keylen + 1;

	do {
		ret = sendfile(STDOUT_FILENO, slot->cache_fd, &start_off,
				slot->cache_st.st_size - start_off);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return errno;
		}
		return 0;
	} while (1);
#else
	ssize_t i, j;

	i = lseek(slot->cache_fd, slot->keylen + 1, SEEK_SET);
	if (i != slot->keylen + 1)
		return errno;

	do {
		i = j = xread(slot->cache_fd, slot->buf, sizeof(slot->buf));
		if (i > 0)
			j = xwrite(STDOUT_FILENO, slot->buf, i);
	} while (i > 0 && j == i);

	if (i < 0 || j != i)
		return errno;
	else
		return 0;
#endif
}

/* Check if the slot has expired */
static int is_expired(struct cache_slot *slot)
{
	if (slot->ttl < 0)
		return 0;
	else
		return slot->cache_st.st_mtime + slot->ttl * 60 < time(NULL);
}

/* Check if the slot has been modified since we opened it.
 * NB: If stat() fails, we pretend the file is modified.
 */
static int is_modified(struct cache_slot *slot)
{
	struct stat st;

	if (stat(slot->cache_name, &st))
		return 1;
	return (st.st_ino != slot->cache_st.st_ino ||
		st.st_mtime != slot->cache_st.st_mtime ||
		st.st_size != slot->cache_st.st_size);
}

/* Close an open lockfile */
static int close_lock(struct cache_slot *slot)
{
	int err = 0;
	if (slot->lock_fd > 0) {
		if (close(slot->lock_fd))
			err = errno;
		else
			slot->lock_fd = -1;
	}
	return err;
}

/* Create a lockfile used to store the generated content for a cache
 * slot, and write the slot key + \0 into it.
 * Returns 0 on success and errno otherwise.
 */
static int lock_slot(struct cache_slot *slot)
{
	struct flock lock = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0,
		.l_len = 0,
	};

	slot->lock_fd = open(slot->lock_name, O_RDWR | O_CREAT,
			     S_IRUSR | S_IWUSR);
	if (slot->lock_fd == -1)
		return errno;
	if (fcntl(slot->lock_fd, F_SETLK, &lock) < 0) {
		int saved_errno = errno;
		close(slot->lock_fd);
		slot->lock_fd = -1;
		return saved_errno;
	}
	if (xwrite(slot->lock_fd, slot->key, slot->keylen + 1) < 0)
		return errno;
	return 0;
}

/* Release the current lockfile. If `replace_old_slot` is set the
 * lockfile replaces the old cache slot, otherwise the lockfile is
 * just deleted.
 */
static int unlock_slot(struct cache_slot *slot, int replace_old_slot)
{
	int err;

	if (replace_old_slot)
		err = rename(slot->lock_name, slot->cache_name);
	else
		err = unlink(slot->lock_name);

	/* Restore stdout and close the temporary FD. */
	if (slot->stdout_fd >= 0) {
		dup2(slot->stdout_fd, STDOUT_FILENO);
		close(slot->stdout_fd);
		slot->stdout_fd = -1;
	}

	if (err)
		return errno;

	return 0;
}

/* Generate the content for the current cache slot by redirecting
 * stdout to the lock-fd and invoking the callback function
 */
static int fill_slot(struct cache_slot *slot)
{
	/* Preserve stdout */
	slot->stdout_fd = dup(STDOUT_FILENO);
	if (slot->stdout_fd == -1)
		return errno;

	/* Redirect stdout to lockfile */
	if (dup2(slot->lock_fd, STDOUT_FILENO) == -1)
		return errno;

	/* Generate cache content */
	slot->fn();

	/* Make sure any buffered data is flushed to the file */
	if (fflush(stdout))
		return errno;

	/* update stat info */
	if (fstat(slot->lock_fd, &slot->cache_st))
		return errno;

	return 0;
}

/* Crude implementation of 32-bit FNV-1 hash algorithm,
 * see http://www.isthe.com/chongo/tech/comp/fnv/ for details
 * about the magic numbers.
 */
#define FNV_OFFSET 0x811c9dc5
#define FNV_PRIME  0x01000193

unsigned long hash_str(const char *str)
{
	unsigned long h = FNV_OFFSET;
	unsigned char *s = (unsigned char *)str;

	if (!s)
		return h;

	while (*s) {
		h *= FNV_PRIME;
		h ^= *s++;
	}
	return h;
}

static int process_slot(struct cache_slot *slot)
{
	int err;

	err = open_slot(slot);
	if (!err && slot->match) {
		if (is_expired(slot)) {
			if (!lock_slot(slot)) {
				/* If the cachefile has been replaced between
				 * `open_slot` and `lock_slot`, we'll just
				 * serve the stale content from the original
				 * cachefile. This way we avoid pruning the
				 * newly generated slot. The same code-path
				 * is chosen if fill_slot() fails for some
				 * reason.
				 *
				 * TODO? check if the new slot contains the
				 * same key as the old one, since we would
				 * prefer to serve the newest content.
				 * This will require us to open yet another
				 * file-descriptor and read and compare the
				 * key from the new file, so for now we're
				 * lazy and just ignore the new file.
				 */
				if (is_modified(slot) || fill_slot(slot)) {
					unlock_slot(slot, 0);
					close_lock(slot);
				} else {
					close_slot(slot);
					unlock_slot(slot, 1);
					slot->cache_fd = slot->lock_fd;
				}
			}
		}
		if ((err = print_slot(slot)) != 0) {
			cache_log("[cgit] error printing cache %s: %s (%d)\n",
				  slot->cache_name,
				  strerror(err),
				  err);
		}
		close_slot(slot);
		return err;
	}

	/* If the cache slot does not exist (or its key doesn't match the
	 * current key), lets try to create a new cache slot for this
	 * request. If this fails (for whatever reason), lets just generate
	 * the content without caching it and fool the caller to believe
	 * everything worked out (but print a warning on stdout).
	 */

	close_slot(slot);
	if ((err = lock_slot(slot)) != 0) {
		cache_log("[cgit] Unable to lock slot %s: %s (%d)\n",
			  slot->lock_name, strerror(err), err);
		slot->fn();
		return 0;
	}

	if ((err = fill_slot(slot)) != 0) {
		cache_log("[cgit] Unable to fill slot %s: %s (%d)\n",
			  slot->lock_name, strerror(err), err);
		unlock_slot(slot, 0);
		close_lock(slot);
		slot->fn();
		return 0;
	}
	// We've got a valid cache slot in the lock file, which
	// is about to replace the old cache slot. But if we
	// release the lockfile and then try to open the new cache
	// slot, we might get a race condition with a concurrent
	// writer for the same cache slot (with a different key).
	// Lets avoid such a race by just printing the content of
	// the lock file.
	slot->cache_fd = slot->lock_fd;
	unlock_slot(slot, 1);
	if ((err = print_slot(slot)) != 0) {
		cache_log("[cgit] error printing cache %s: %s (%d)\n",
			  slot->cache_name,
			  strerror(err),
			  err);
	}
	close_slot(slot);
	return err;
}

/* Print cached content to stdout, generate the content if necessary. */
int cache_process(int size, const char *path, const char *key, int ttl,
		  cache_fill_fn fn)
{
	unsigned long hash;
	int i;
	struct strbuf filename = STRBUF_INIT;
	struct strbuf lockname = STRBUF_INIT;
	struct cache_slot slot;
	int result;

	/* If the cache is disabled, just generate the content */
	if (size <= 0 || ttl == 0) {
		fn();
		return 0;
	}

	/* Verify input, calculate filenames */
	if (!path) {
		cache_log("[cgit] Cache path not specified, caching is disabled\n");
		fn();
		return 0;
	}
	if (!key)
		key = "";
	hash = hash_str(key) % size;
	strbuf_addstr(&filename, path);
	strbuf_ensure_end(&filename, '/');
	for (i = 0; i < 8; i++) {
		strbuf_addf(&filename, "%x", (unsigned char)(hash & 0xf));
		hash >>= 4;
	}
	strbuf_addbuf(&lockname, &filename);
	strbuf_addstr(&lockname, ".lock");
	slot.fn = fn;
	slot.ttl = ttl;
	slot.stdout_fd = -1;
	slot.cache_name = filename.buf;
	slot.lock_name = lockname.buf;
	slot.key = key;
	slot.keylen = strlen(key);
	result = process_slot(&slot);

	strbuf_release(&filename);
	strbuf_release(&lockname);
	return result;
}

/* Return a strftime formatted date/time
 * NB: the result from this function is to shared memory
 */
static char *sprintftime(const char *format, time_t time)
{
	static char buf[64];
	struct tm tm;

	if (!time)
		return NULL;
	gmtime_r(&time, &tm);
	strftime(buf, sizeof(buf)-1, format, &tm);
	return buf;
}

int cache_ls(const char *path)
{
	DIR *dir;
	struct dirent *ent;
	int err = 0;
	struct cache_slot slot = { NULL };
	struct strbuf fullname = STRBUF_INIT;
	size_t prefixlen;

	if (!path) {
		cache_log("[cgit] cache path not specified\n");
		return -1;
	}
	dir = opendir(path);
	if (!dir) {
		err = errno;
		cache_log("[cgit] unable to open path %s: %s (%d)\n",
			  path, strerror(err), err);
		return err;
	}
	strbuf_addstr(&fullname, path);
	strbuf_ensure_end(&fullname, '/');
	prefixlen = fullname.len;
	while ((ent = readdir(dir)) != NULL) {
		if (strlen(ent->d_name) != 8)
			continue;
		strbuf_setlen(&fullname, prefixlen);
		strbuf_addstr(&fullname, ent->d_name);
		slot.cache_name = fullname.buf;
		if ((err = open_slot(&slot)) != 0) {
			cache_log("[cgit] unable to open path %s: %s (%d)\n",
				  fullname.buf, strerror(err), err);
			continue;
		}
		htmlf("%s %s %10"PRIuMAX" %s\n",
		      fullname.buf,
		      sprintftime("%Y-%m-%d %H:%M:%S",
				  slot.cache_st.st_mtime),
		      (uintmax_t)slot.cache_st.st_size,
		      slot.buf);
		close_slot(&slot);
	}
	closedir(dir);
	strbuf_release(&fullname);
	return 0;
}

/* Print a message to stdout */
void cache_log(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

