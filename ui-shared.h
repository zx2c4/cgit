#ifndef UI_SHARED_H
#define UI_SHARED_H

extern const char *cgit_httpscheme(void);
extern char *cgit_hosturl(void);
extern const char *cgit_rooturl(void);
extern char *cgit_currenturl(void);
extern char *cgit_currentfullurl(void);
extern const char *cgit_loginurl(void);
extern char *cgit_repourl(const char *reponame);
extern char *cgit_fileurl(const char *reponame, const char *pagename,
			  const char *filename, const char *query);
extern char *cgit_pageurl(const char *reponame, const char *pagename,
			  const char *query);

extern void cgit_add_clone_urls(void (*fn)(const char *));

extern void cgit_index_link(const char *name, const char *title,
			    const char *class, const char *pattern, const char *sort, int ofs, int always_root);
extern void cgit_repo_link(const char *name, const char *title,
				  const char *class, const char *head);
extern void cgit_summary_link(const char *name, const char *title,
			      const char *class, const char *head);
extern void cgit_tag_link(const char *name, const char *title,
			  const char *class, const char *tag);
extern void cgit_tree_link(const char *name, const char *title,
			   const char *class, const char *head,
			   const char *rev, const char *path);
extern void cgit_plain_link(const char *name, const char *title,
			    const char *class, const char *head,
			    const char *rev, const char *path);
extern void cgit_blame_link(const char *name, const char *title,
			    const char *class, const char *head,
			    const char *rev, const char *path);
extern void cgit_log_link(const char *name, const char *title,
			  const char *class, const char *head, const char *rev,
			  const char *path, int ofs, const char *grep,
			  const char *pattern, int showmsg, int follow);
extern void cgit_commit_link(const char *name, const char *title,
			     const char *class, const char *head,
			     const char *rev, const char *path);
extern void cgit_patch_link(const char *name, const char *title,
			    const char *class, const char *head,
			    const char *rev, const char *path);
extern void cgit_refs_link(const char *name, const char *title,
			   const char *class, const char *head,
			   const char *rev, const char *path);
extern void cgit_snapshot_link(const char *name, const char *title,
			       const char *class, const char *head,
			       const char *rev, const char *archivename);
extern void cgit_diff_link(const char *name, const char *title,
			   const char *class, const char *head,
			   const char *new_rev, const char *old_rev,
			   const char *path);
extern void cgit_stats_link(const char *name, const char *title,
			    const char *class, const char *head,
			    const char *path);
extern void cgit_object_link(struct object *obj);

extern void cgit_submodule_link(const char *class, char *path,
				const char *rev);

extern void cgit_print_layout_start(void);
extern void cgit_print_layout_end(void);

__attribute__((format (printf,1,2)))
extern void cgit_print_error(const char *fmt, ...);
__attribute__((format (printf,1,0)))
extern void cgit_vprint_error(const char *fmt, va_list ap);
extern const struct date_mode *cgit_date_mode(enum date_mode_type type);
extern void cgit_print_age(time_t t, int tz, time_t max_relative);
extern void cgit_print_http_headers(void);
extern void cgit_redirect(const char *url, bool permanent);
extern void cgit_print_docstart(void);
extern void cgit_print_docend(void);
__attribute__((format (printf,3,4)))
extern void cgit_print_error_page(int code, const char *msg, const char *fmt, ...);
extern void cgit_print_pageheader(void);
extern void cgit_print_filemode(unsigned short mode);
extern void cgit_compose_snapshot_prefix(struct strbuf *filename,
					 const char *base, const char *ref);
extern void cgit_print_snapshot_links(const struct cgit_repo *repo,
				      const char *ref, const char *separator);
extern const char *cgit_snapshot_prefix(const struct cgit_repo *repo);
extern void cgit_add_hidden_formfields(int incl_head, int incl_search,
				       const char *page);

extern void cgit_set_title_from_path(const char *path);
#endif /* UI_SHARED_H */
