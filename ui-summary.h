#ifndef UI_SUMMARY_H
#define UI_SUMMARY_H

extern void cgit_parse_readme(const char *readme, const char *path, char **filename, char **ref, struct cgit_repo *repo);
extern void cgit_print_summary();
extern void cgit_print_repo_readme(char *path);

#endif /* UI_SUMMARY_H */
