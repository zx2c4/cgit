#ifndef GIT_H
#define GIT_H


/*
 * from git:git-compat-util.h 
 */


#ifndef FLEX_ARRAY
#if defined(__GNUC__) && (__GNUC__ < 3)
#define FLEX_ARRAY 0
#else
#define FLEX_ARRAY /* empty */
#endif
#endif


#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <regex.h>

/* On most systems <limits.h> would have given us this, but
 * not on some systems (e.g. GNU/Hurd).
 */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef __GNUC__
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif


extern void die(const char *err, ...) NORETURN __attribute__((format (printf, 1, 2)));


static inline char* xstrdup(const char *str)
{
	char *ret = strdup(str);
	if (!ret)
		die("Out of memory, strdup failed");
	return ret;
}

static inline void *xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (!ret && !size)
		ret = malloc(1);
	if (!ret)
		die("Out of memory, malloc failed");
#ifdef XMALLOC_POISON
	memset(ret, 0xA5, size);
#endif
	return ret;
}

static inline void *xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);
	if (!ret && !size)
		ret = realloc(ptr, 1);
	if (!ret)
		die("Out of memory, realloc failed");
	return ret;
}

static inline void *xcalloc(size_t nmemb, size_t size)
{
	void *ret = calloc(nmemb, size);
	if (!ret && (!nmemb || !size))
		ret = calloc(1, 1);
	if (!ret)
		die("Out of memory, calloc failed");
	return ret;
}

static inline ssize_t xread(int fd, void *buf, size_t len)
{
	ssize_t nr;
	while (1) {
		nr = read(fd, buf, len);
		if ((nr < 0) && (errno == EAGAIN || errno == EINTR))
			continue;
		return nr;
	}
}

static inline ssize_t xwrite(int fd, const void *buf, size_t len)
{
	ssize_t nr;
	while (1) {
		nr = write(fd, buf, len);
		if ((nr < 0) && (errno == EAGAIN || errno == EINTR))
			continue;
		return nr;
	}
}




/*
 * from git:cache.h
 */


enum object_type {
	OBJ_NONE = 0,
	OBJ_COMMIT = 1,
	OBJ_TREE = 2,
	OBJ_BLOB = 3,
	OBJ_TAG = 4,
	/* 5 for future expansion */
	OBJ_OFS_DELTA = 6,
	OBJ_REF_DELTA = 7,
	OBJ_BAD,
};


/* Convert to/from hex/sha1 representation */
#define MINIMUM_ABBREV 4
#define DEFAULT_ABBREV 7

extern const unsigned char null_sha1[20];

extern int sha1_object_info(const unsigned char *, char *, unsigned long *);

extern void * read_sha1_file(const unsigned char *sha1, char *type, unsigned long *size);

extern int get_sha1(const char *str, unsigned char *sha1);
extern int get_sha1_hex(const char *hex, unsigned char *sha1);
extern char *sha1_to_hex(const unsigned char *sha1);	/* static buffer result! */

static inline int is_null_sha1(const unsigned char *sha1)
{
	return !memcmp(sha1, null_sha1, 20);
}
static inline int hashcmp(const unsigned char *sha1, const unsigned char *sha2)
{
	return memcmp(sha1, sha2, 20);
}
static inline void hashcpy(unsigned char *sha_dst, const unsigned char *sha_src)
{
	memcpy(sha_dst, sha_src, 20);
}
static inline void hashclr(unsigned char *hash)
{
	memset(hash, 0, 20);
}


/*
 * from git:grep.h
 */

enum grep_pat_token {
        GREP_PATTERN,
        GREP_PATTERN_HEAD,
        GREP_PATTERN_BODY,
        GREP_AND,
        GREP_OPEN_PAREN,
        GREP_CLOSE_PAREN,
        GREP_NOT,
        GREP_OR,
};

enum grep_context {
        GREP_CONTEXT_HEAD,
        GREP_CONTEXT_BODY,
};

struct grep_pat {
        struct grep_pat *next;
        const char *origin;
        int no;
        enum grep_pat_token token;
        const char *pattern;
        regex_t regexp;
};

enum grep_expr_node {
        GREP_NODE_ATOM,
        GREP_NODE_NOT,
        GREP_NODE_AND,
        GREP_NODE_OR,
};

struct grep_opt {
        struct grep_pat *pattern_list;
        struct grep_pat **pattern_tail;
        struct grep_expr *pattern_expression;
        int prefix_length;
        regex_t regexp;
        unsigned linenum:1;
        unsigned invert:1;
        unsigned status_only:1;
        unsigned name_only:1;
        unsigned unmatch_name_only:1;
        unsigned count:1;
        unsigned word_regexp:1;
        unsigned fixed:1;
        unsigned all_match:1;
#define GREP_BINARY_DEFAULT     0
#define GREP_BINARY_NOMATCH     1
#define GREP_BINARY_TEXT        2
        unsigned binary:2;
        unsigned extended:1;
        unsigned relative:1;
        unsigned pathname:1;
        int regflags;
        unsigned pre_context;
        unsigned post_context;
};


extern void compile_grep_patterns(struct grep_opt *opt);
extern void free_grep_patterns(struct grep_opt *opt);


/*
 * from git:object.h 
 */

extern const char *type_names[9];

struct object_list {
	struct object *item;
	struct object_list *next;
};

struct object_refs {
	unsigned count;
	struct object *base;
	struct object *ref[FLEX_ARRAY]; /* more */
};

struct object_array {
	unsigned int nr;
	unsigned int alloc;
	struct object_array_entry {
		struct object *item;
		const char *name;
	} *objects;
};

#define TYPE_BITS   3
#define FLAG_BITS  27

/*
 * The object type is stored in 3 bits.
 */
struct object {
	unsigned parsed : 1;
	unsigned used : 1;
	unsigned type : TYPE_BITS;
	unsigned flags : FLAG_BITS;
	unsigned char sha1[20];
};


/*
 * from git:tree.h
 */

struct tree {
	struct object object;
	void *buffer;
	unsigned long size;
};


struct tree *lookup_tree(const unsigned char *sha1);
int parse_tree_buffer(struct tree *item, void *buffer, unsigned long size);
int parse_tree(struct tree *tree);
struct tree *parse_tree_indirect(const unsigned char *sha1);

typedef int (*read_tree_fn_t)(const unsigned char *, const char *, int, const char *, unsigned int, int);

extern int read_tree_recursive(struct tree *tree,
			       const char *base, int baselen,
			       int stage, const char **match,
			       read_tree_fn_t fn);

extern int read_tree(struct tree *tree, int stage, const char **paths);


/* from git:commit.h */

struct commit_list {
	struct commit *item;
	struct commit_list *next;
};

struct commit {
	struct object object;
	void *util;
	unsigned long date;
	struct commit_list *parents;
	struct tree *tree;
	char *buffer;
};


struct commit *lookup_commit(const unsigned char *sha1);
struct commit *lookup_commit_reference(const unsigned char *sha1);
struct commit *lookup_commit_reference_gently(const unsigned char *sha1,
					      int quiet);

int parse_commit_buffer(struct commit *item, void *buffer, unsigned long size);
int parse_commit(struct commit *item);

struct commit_list * commit_list_insert(struct commit *item, struct commit_list **list_p);
struct commit_list * insert_by_date(struct commit *item, struct commit_list **list);

void free_commit_list(struct commit_list *list);

void sort_by_date(struct commit_list **list);

/* Commit formats */
enum cmit_fmt {
	CMIT_FMT_RAW,
	CMIT_FMT_MEDIUM,
	CMIT_FMT_DEFAULT = CMIT_FMT_MEDIUM,
	CMIT_FMT_SHORT,
	CMIT_FMT_FULL,
	CMIT_FMT_FULLER,
	CMIT_FMT_ONELINE,
	CMIT_FMT_EMAIL,

	CMIT_FMT_UNSPECIFIED,
};

extern unsigned long pretty_print_commit(enum cmit_fmt fmt, const struct commit *, unsigned long len, char *buf, unsigned long space, int abbrev, const char *subject, const char *after_subject, int relative_date);


typedef void (*topo_sort_set_fn_t)(struct commit*, void *data);
typedef void* (*topo_sort_get_fn_t)(struct commit*);



/*
 * from git:tag.h
 */

extern const char *tag_type;

struct tag {
	struct object object;
	struct object *tagged;
	char *tag;
	char *signature; /* not actually implemented */
};

extern struct tag *lookup_tag(const unsigned char *sha1);
extern int parse_tag_buffer(struct tag *item, void *data, unsigned long size);
extern int parse_tag(struct tag *item);
extern struct object *deref_tag(struct object *, const char *, int);


/*
 * from git:diffcore.h
 */

struct diff_filespec {
	unsigned char sha1[20];
	char *path;
	void *data;
	void *cnt_data;
	unsigned long size;
	int xfrm_flags;		 /* for use by the xfrm */
	unsigned short mode;	 /* file mode */
	unsigned sha1_valid : 1; /* if true, use sha1 and trust mode;
				  * if false, use the name and read from
				  * the filesystem.
				  */
#define DIFF_FILE_VALID(spec) (((spec)->mode) != 0)
	unsigned should_free : 1; /* data should be free()'ed */
	unsigned should_munmap : 1; /* data should be munmap()'ed */
};

struct diff_filepair {
	struct diff_filespec *one;
	struct diff_filespec *two;
	unsigned short int score;
	char status; /* M C R N D U (see Documentation/diff-format.txt) */
	unsigned source_stays : 1; /* all of R/C are copies */
	unsigned broken_pair : 1;
	unsigned renamed_pair : 1;
};

#define DIFF_PAIR_UNMERGED(p) \
	(!DIFF_FILE_VALID((p)->one) && !DIFF_FILE_VALID((p)->two))

#define DIFF_PAIR_RENAME(p) ((p)->renamed_pair)

#define DIFF_PAIR_BROKEN(p) \
	( (!DIFF_FILE_VALID((p)->one) != !DIFF_FILE_VALID((p)->two)) && \
	  ((p)->broken_pair != 0) )

#define DIFF_PAIR_TYPE_CHANGED(p) \
	((S_IFMT & (p)->one->mode) != (S_IFMT & (p)->two->mode))

#define DIFF_PAIR_MODE_CHANGED(p) ((p)->one->mode != (p)->two->mode)

extern void diff_free_filepair(struct diff_filepair *);

extern int diff_unmodified_pair(struct diff_filepair *);

struct diff_queue_struct {
	struct diff_filepair **queue;
	int alloc;
	int nr;
};


/*
 *  from git:diff.h
 */


struct rev_info;
struct diff_options;
struct diff_queue_struct;

typedef void (*change_fn_t)(struct diff_options *options,
		 unsigned old_mode, unsigned new_mode,
		 const unsigned char *old_sha1,
		 const unsigned char *new_sha1,
		 const char *base, const char *path);

typedef void (*add_remove_fn_t)(struct diff_options *options,
		    int addremove, unsigned mode,
		    const unsigned char *sha1,
		    const char *base, const char *path);

typedef void (*diff_format_fn_t)(struct diff_queue_struct *q,
		struct diff_options *options, void *data);

#define DIFF_FORMAT_RAW		0x0001
#define DIFF_FORMAT_DIFFSTAT	0x0002
#define DIFF_FORMAT_NUMSTAT	0x0004
#define DIFF_FORMAT_SUMMARY	0x0008
#define DIFF_FORMAT_PATCH	0x0010

/* These override all above */
#define DIFF_FORMAT_NAME	0x0100
#define DIFF_FORMAT_NAME_STATUS	0x0200
#define DIFF_FORMAT_CHECKDIFF	0x0400

/* Same as output_format = 0 but we know that -s flag was given
 * and we should not give default value to output_format.
 */
#define DIFF_FORMAT_NO_OUTPUT	0x0800

#define DIFF_FORMAT_CALLBACK	0x1000

struct diff_options {
	const char *filter;
	const char *orderfile;
	const char *pickaxe;
	const char *single_follow;
	unsigned recursive:1,
		 tree_in_recursive:1,
		 binary:1,
		 text:1,
		 full_index:1,
		 silent_on_remove:1,
		 find_copies_harder:1,
		 color_diff:1,
		 color_diff_words:1;
	int context;
	int break_opt;
	int detect_rename;
	int line_termination;
	int output_format;
	int pickaxe_opts;
	int rename_score;
	int reverse_diff;
	int rename_limit;
	int setup;
	int abbrev;
	const char *msg_sep;
	const char *stat_sep;
	long xdl_opts;

	int stat_width;
	int stat_name_width;

	int nr_paths;
	const char **paths;
	int *pathlens;
	change_fn_t change;
	add_remove_fn_t add_remove;
	diff_format_fn_t format_callback;
	void *format_callback_data;
};

enum color_diff {
	DIFF_RESET = 0,
	DIFF_PLAIN = 1,
	DIFF_METAINFO = 2,
	DIFF_FRAGINFO = 3,
	DIFF_FILE_OLD = 4,
	DIFF_FILE_NEW = 5,
	DIFF_COMMIT = 6,
	DIFF_WHITESPACE = 7,
};


extern int diff_tree_sha1(const unsigned char *old, const unsigned char *new,
			  const char *base, struct diff_options *opt);

extern int diff_root_tree_sha1(const unsigned char *new, const char *base,
			       struct diff_options *opt);

extern int git_diff_ui_config(const char *var, const char *value);
extern void diff_setup(struct diff_options *);
extern int diff_opt_parse(struct diff_options *, const char **, int);
extern int diff_setup_done(struct diff_options *);


extern void diffcore_std(struct diff_options *);
extern void diff_flush(struct diff_options*);


/* diff-raw status letters */
#define DIFF_STATUS_ADDED		'A'
#define DIFF_STATUS_COPIED		'C'
#define DIFF_STATUS_DELETED		'D'
#define DIFF_STATUS_MODIFIED		'M'
#define DIFF_STATUS_RENAMED		'R'
#define DIFF_STATUS_TYPE_CHANGED	'T'
#define DIFF_STATUS_UNKNOWN		'X'
#define DIFF_STATUS_UNMERGED		'U'



/*
 * from git:refs.g
 */

typedef int each_ref_fn(const char *refname, const unsigned char *sha1, int flags, void *cb_data);
extern int head_ref(each_ref_fn, void *);
extern int for_each_ref(each_ref_fn, void *);
extern int for_each_tag_ref(each_ref_fn, void *);
extern int for_each_branch_ref(each_ref_fn, void *);
extern int for_each_remote_ref(each_ref_fn, void *);



/*
 * from git:revision.h
 */

struct rev_info;
struct log_info;

typedef void (prune_fn_t)(struct rev_info *revs, struct commit *commit);

struct rev_info {
	/* Starting list */
	struct commit_list *commits;
	struct object_array pending;

	/* Basic information */
	const char *prefix;
	void *prune_data;
	prune_fn_t *prune_fn;

	/* Traversal flags */
	unsigned int	dense:1,
			no_merges:1,
			no_walk:1,
			remove_empty_trees:1,
			simplify_history:1,
			lifo:1,
			topo_order:1,
			tag_objects:1,
			tree_objects:1,
			blob_objects:1,
			edge_hint:1,
			limited:1,
			unpacked:1, /* see also ignore_packed below */
			boundary:1,
			parents:1;

	/* Diff flags */
	unsigned int	diff:1,
			full_diff:1,
			show_root_diff:1,
			no_commit_id:1,
			verbose_header:1,
			ignore_merges:1,
			combine_merges:1,
			dense_combined_merges:1,
			always_show_header:1;

	/* Format info */
	unsigned int	shown_one:1,
			abbrev_commit:1,
			relative_date:1;

	const char **ignore_packed; /* pretend objects in these are unpacked */
	int num_ignore_packed;

	unsigned int	abbrev;
	enum cmit_fmt	commit_format;
	struct log_info *loginfo;
	int		nr, total;
	const char	*mime_boundary;
	const char	*message_id;
	const char	*ref_message_id;
	const char	*add_signoff;
	const char	*extra_headers;

	/* Filter by commit log message */
	struct grep_opt	*grep_filter;

	/* special limits */
	int max_count;
	unsigned long max_age;
	unsigned long min_age;

	/* diff info for patches and for paths limiting */
	struct diff_options diffopt;
	struct diff_options pruning;

	topo_sort_set_fn_t topo_setter;
	topo_sort_get_fn_t topo_getter;
};


extern void init_revisions(struct rev_info *revs, const char *prefix);
extern int setup_revisions(int argc, const char **argv, struct rev_info *revs, const char *def);
extern int handle_revision_arg(const char *arg, struct rev_info *revs,int flags,int cant_be_filename);

extern void prepare_revision_walk(struct rev_info *revs);
extern struct commit *get_revision(struct rev_info *revs);



/* from git:log-tree.h */

int log_tree_commit(struct rev_info *, struct commit *);



#endif /* GIT_H */
