#ifndef UI_LOG_H
#define UI_LOG_H

extern void cgit_print_log(const char *tip, int ofs, int cnt, char *grep,
			   char *pattern, char *path, int pager,
			   int commit_graph);
extern void show_commit_decorations(struct commit *commit);

#endif /* UI_LOG_H */
