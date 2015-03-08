#ifndef UI_STATS_H
#define UI_STATS_H

#include "cgit.h"

struct cgit_period {
	const char code;
	const char *name;
	int max_periods;
	int count;

	/* Convert a tm value to the first day in the period */
	void (*trunc)(struct tm *tm);

	/* Update tm value to start of next/previous period */
	void (*dec)(struct tm *tm);
	void (*inc)(struct tm *tm);

	/* Pretty-print a tm value */
	char *(*pretty)(struct tm *tm);
};

extern int cgit_find_stats_period(const char *expr, const struct cgit_period **period);
extern const char *cgit_find_stats_periodname(int idx);

extern void cgit_show_stats(void);

#endif /* UI_STATS_H */
