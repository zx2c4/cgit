#ifndef CGIT_H
#define CGIT_H


#include <git-compat-util.h>
#include <cache.h>
#include <grep.h>
#include <object.h>
#include <tree.h>
#include <commit.h>
#include <tag.h>
#include <diff.h>
#include <diffcore.h>
#include <refs.h>
#include <revision.h>
#include <log-tree.h>
#include <archive.h>
#include <xdiff/xdiff.h>
#include <utf8.h>


/*
 * The valid cgit repo-commands
 */
#define CMD_LOG      1
#define CMD_COMMIT   2
#define CMD_DIFF     3
#define CMD_TREE     4
#define CMD_BLOB     5
#define CMD_SNAPSHOT 6
#define CMD_TAG      7
#define CMD_REFS     8
#define CMD_PATCH    9

/*
 * Dateformats used on misc. pages
 */
#define FMT_LONGDATE "%Y-%m-%d %H:%M:%S"
#define FMT_SHORTDATE "%Y-%m-%d"


/*
 * Limits used for relative dates
 */
#define TM_MIN    60
#define TM_HOUR  (TM_MIN * 60)
#define TM_DAY   (TM_HOUR * 24)
#define TM_WEEK  (TM_DAY * 7)
#define TM_YEAR  (TM_DAY * 365)
#define TM_MONTH (TM_YEAR / 12.0)


/*
 * Default encoding
 */
#define PAGE_ENCODING "UTF-8"

typedef void (*configfn)(const char *name, const char *value);
typedef void (*filepair_fn)(struct diff_filepair *pair);
typedef void (*linediff_fn)(char *line, int len);

struct cacheitem {
	char *name;
	struct stat st;
	int ttl;
	int fd;
};

struct cgit_repo {
	char *url;
	char *name;
	char *path;
	char *desc;
	char *owner;
	char *defbranch;
	char *group;
	char *module_link;
	char *readme;
	char *clone_url;
	int snapshots;
	int enable_log_filecount;
	int enable_log_linecount;
};

struct cgit_repolist {
	int length;
	int count;
	struct cgit_repo *repos;
};

struct commitinfo {
	struct commit *commit;
	char *author;
	char *author_email;
	unsigned long author_date;
	char *committer;
	char *committer_email;
	unsigned long committer_date;
	char *subject;
	char *msg;
	char *msg_encoding;
};

struct taginfo {
	char *tagger;
	char *tagger_email;
	int tagger_date;
	char *msg;
};

struct refinfo {
	const char *refname;
	struct object *object;
	union {
		struct taginfo *tag;
		struct commitinfo *commit;
	};
};

struct reflist {
	struct refinfo **refs;
	int alloc;
	int count;
};

struct cgit_query {
	int has_symref;
	int has_sha1;
	char *raw;
	char *repo;
	char *page;
	char *search;
	char *grep;
	char *head;
	char *sha1;
	char *sha2;
	char *path;
	char *name;
	int   ofs;
};

struct cgit_config {
	char *agefile;
	char *cache_root;
	char *clone_prefix;
	char *css;
	char *index_header;
	char *index_info;
	char *logo;
	char *logo_link;
	char *module_link;
	char *repo_group;
	char *robots;
	char *root_title;
	char *script_name;
	char *virtual_root;
	int cache_dynamic_ttl;
	int cache_max_create_time;
	int cache_repo_ttl;
	int cache_root_ttl;
	int cache_static_ttl;
	int enable_index_links;
	int enable_log_filecount;
	int enable_log_linecount;
	int max_commit_count;
	int max_lock_attempts;
	int max_msg_len;
	int max_repodesc_len;
	int nocache;
	int renamelimit;
	int snapshots;
	int summary_branches;
	int summary_log;
	int summary_tags;
};

struct cgit_context {
	struct cgit_query qry;
	struct cgit_config cfg;
	struct cgit_repo *repo;
};

extern const char *cgit_version;

extern struct cgit_repolist cgit_repolist;
extern struct cgit_context ctx;
extern int cgit_cmd;

extern void cgit_prepare_context(struct cgit_context *ctx);
extern int cgit_get_cmd_index(const char *cmd);
extern struct cgit_repo *cgit_get_repoinfo(const char *url);
extern void cgit_global_config_cb(const char *name, const char *value);
extern void cgit_repo_config_cb(const char *name, const char *value);
extern void cgit_querystring_cb(const char *name, const char *value);

extern int chk_zero(int result, char *msg);
extern int chk_positive(int result, char *msg);
extern int chk_non_negative(int result, char *msg);

extern int hextoint(char c);
extern char *trim_end(const char *str, char c);
extern char *strlpart(char *txt, int maxlen);
extern char *strrpart(char *txt, int maxlen);

extern void cgit_add_ref(struct reflist *list, struct refinfo *ref);
extern int cgit_refs_cb(const char *refname, const unsigned char *sha1,
			int flags, void *cb_data);

extern void *cgit_free_commitinfo(struct commitinfo *info);

extern int cgit_diff_files(const unsigned char *old_sha1,
			   const unsigned char *new_sha1,
			   linediff_fn fn);

extern void cgit_diff_tree(const unsigned char *old_sha1,
			   const unsigned char *new_sha1,
			   filepair_fn fn, const char *prefix);

extern void cgit_diff_commit(struct commit *commit, filepair_fn fn);

extern char *fmt(const char *format,...);

extern int cgit_read_config(const char *filename, configfn fn);
extern int cgit_parse_query(char *txt, configfn fn);
extern struct commitinfo *cgit_parse_commit(struct commit *commit);
extern struct taginfo *cgit_parse_tag(struct tag *tag);
extern void cgit_parse_url(const char *url);

extern char *cache_safe_filename(const char *unsafe);
extern int cache_lock(struct cacheitem *item);
extern int cache_unlock(struct cacheitem *item);
extern int cache_cancel_lock(struct cacheitem *item);
extern int cache_exist(struct cacheitem *item);
extern int cache_expired(struct cacheitem *item);

extern char *cgit_repourl(const char *reponame);
extern char *cgit_fileurl(const char *reponame, const char *pagename,
			  const char *filename, const char *query);
extern char *cgit_pageurl(const char *reponame, const char *pagename,
			  const char *query);

extern const char *cgit_repobasename(const char *reponame);

extern void cgit_tree_link(char *name, char *title, char *class, char *head,
			   char *rev, char *path);
extern void cgit_log_link(char *name, char *title, char *class, char *head,
			  char *rev, char *path, int ofs, char *grep,
			  char *pattern);
extern void cgit_commit_link(char *name, char *title, char *class, char *head,
			     char *rev);
extern void cgit_refs_link(char *name, char *title, char *class, char *head,
			   char *rev, char *path);
extern void cgit_snapshot_link(char *name, char *title, char *class,
			       char *head, char *rev, char *archivename);
extern void cgit_diff_link(char *name, char *title, char *class, char *head,
			   char *new_rev, char *old_rev, char *path);

extern void cgit_object_link(struct object *obj);

extern void cgit_print_error(char *msg);
extern void cgit_print_date(time_t secs, char *format);
extern void cgit_print_age(time_t t, time_t max_relative, char *format);
extern void cgit_print_docstart(char *title, struct cacheitem *item);
extern void cgit_print_docend();
extern void cgit_print_pageheader(char *title, int show_search);
extern void cgit_print_snapshot_start(const char *mimetype,
				      const char *filename,
				      struct cacheitem *item);
extern void cgit_print_filemode(unsigned short mode);
extern void cgit_print_branches(int maxcount);
extern void cgit_print_tags(int maxcount);

extern void cgit_print_repolist(struct cacheitem *item);
extern void cgit_print_summary();
extern void cgit_print_log(const char *tip, int ofs, int cnt, char *grep,
			   char *pattern, char *path, int pager);
extern void cgit_print_blob(struct cacheitem *item, const char *hex, char *path);
extern void cgit_print_tree(const char *rev, char *path);
extern void cgit_print_commit(char *hex);
extern void cgit_print_refs();
extern void cgit_print_tag(char *revname);
extern void cgit_print_diff(const char *new_hex, const char *old_hex, const char *prefix);
extern void cgit_print_patch(char *hex, struct cacheitem *item);
extern void cgit_print_snapshot(struct cacheitem *item, const char *head,
				const char *hex, const char *prefix,
				const char *filename, int snapshot);
extern void cgit_print_snapshot_links(const char *repo, const char *head,
				      const char *hex, int snapshots);
extern int cgit_parse_snapshots_mask(const char *str);

#endif /* CGIT_H */
