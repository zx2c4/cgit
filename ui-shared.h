#ifndef UI_SHARED_H
#define UI_SHARED_H

extern char *cgit_httpscheme();
extern char *cgit_hosturl();
extern char *cgit_rooturl();
extern char *cgit_repourl(const char *reponame);
extern char *cgit_fileurl(const char *reponame, const char *pagename,
			  const char *filename, const char *query);
extern char *cgit_pageurl(const char *reponame, const char *pagename,
			  const char *query);

extern void cgit_index_link(char *name, char *title, char *class,
			    char *pattern, int ofs);
extern void cgit_summary_link(char *name, char *title, char *class, char *head);
extern void cgit_tag_link(char *name, char *title, char *class, char *head,
			  char *rev);
extern void cgit_tree_link(char *name, char *title, char *class, char *head,
			   char *rev, char *path);
extern void cgit_plain_link(char *name, char *title, char *class, char *head,
			    char *rev, char *path);
extern void cgit_log_link(char *name, char *title, char *class, char *head,
			  char *rev, char *path, int ofs, char *grep,
			  char *pattern, int showmsg);
extern void cgit_commit_link(char *name, char *title, char *class, char *head,
			     char *rev);
extern void cgit_patch_link(char *name, char *title, char *class, char *head,
			    char *rev);
extern void cgit_refs_link(char *name, char *title, char *class, char *head,
			   char *rev, char *path);
extern void cgit_snapshot_link(char *name, char *title, char *class,
			       char *head, char *rev, char *archivename);
extern void cgit_diff_link(char *name, char *title, char *class, char *head,
			   char *new_rev, char *old_rev, char *path);
extern void cgit_stats_link(char *name, char *title, char *class, char *head,
			    char *path);
extern void cgit_object_link(struct object *obj);

extern void cgit_print_error(char *msg);
extern void cgit_print_date(time_t secs, char *format, int local_time);
extern void cgit_print_age(time_t t, time_t max_relative, char *format);
extern void cgit_print_http_headers(struct cgit_context *ctx);
extern void cgit_print_docstart(struct cgit_context *ctx);
extern void cgit_print_docend();
extern void cgit_print_pageheader(struct cgit_context *ctx);
extern void cgit_print_filemode(unsigned short mode);
extern void cgit_print_snapshot_links(const char *repo, const char *head,
				      const char *hex, int snapshots);
extern void cgit_add_hidden_formfields(int incl_head, int incl_search,
				       char *page);
#endif /* UI_SHARED_H */
