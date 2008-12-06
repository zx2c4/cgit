#include "cgit.h"
#include "html.h"
#include <string-list.h>

#define MONTHS 6

struct Period {
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

struct authorstat {
	long total;
	struct string_list list;
};

#define DAY_SECS (60 * 60 * 24)
#define WEEK_SECS (DAY_SECS * 7)

static void trunc_week(struct tm *tm)
{
	time_t t = timegm(tm);
	t -= ((tm->tm_wday + 6) % 7) * DAY_SECS;
	gmtime_r(&t, tm);	
}

static void dec_week(struct tm *tm)
{
	time_t t = timegm(tm);
	t -= WEEK_SECS;
	gmtime_r(&t, tm);	
}

static void inc_week(struct tm *tm)
{
	time_t t = timegm(tm);
	t += WEEK_SECS;
	gmtime_r(&t, tm);	
}

static char *pretty_week(struct tm *tm)
{
	static char buf[10];

	strftime(buf, sizeof(buf), "W%V %G", tm);
	return buf;
}

static void trunc_month(struct tm *tm)
{
	tm->tm_mday = 1;
}

static void dec_month(struct tm *tm)
{
	tm->tm_mon--;
	if (tm->tm_mon < 0) {
		tm->tm_year--;
		tm->tm_mon = 11;
	}
}

static void inc_month(struct tm *tm)
{
	tm->tm_mon++;
	if (tm->tm_mon > 11) {
		tm->tm_year++;
		tm->tm_mon = 0;
	}
}

static char *pretty_month(struct tm *tm)
{
	static const char *months[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	return fmt("%s %d", months[tm->tm_mon], tm->tm_year + 1900);
}

static void trunc_quarter(struct tm *tm)
{
	trunc_month(tm);
	while(tm->tm_mon % 3 != 0)
		dec_month(tm);
}

static void dec_quarter(struct tm *tm)
{
	dec_month(tm);
	dec_month(tm);
	dec_month(tm);
}

static void inc_quarter(struct tm *tm)
{
	inc_month(tm);
	inc_month(tm);
	inc_month(tm);
}

static char *pretty_quarter(struct tm *tm)
{
	return fmt("Q%d %d", tm->tm_mon / 3 + 1, tm->tm_year + 1900);
}

static void trunc_year(struct tm *tm)
{
	trunc_month(tm);
	tm->tm_mon = 0;
}

static void dec_year(struct tm *tm)
{
	tm->tm_year--;
}

static void inc_year(struct tm *tm)
{
	tm->tm_year++;
}

static char *pretty_year(struct tm *tm)
{
	return fmt("%d", tm->tm_year + 1900);
}

struct Period periods[] = {
	{'w', "week", 12, 4, trunc_week, dec_week, inc_week, pretty_week},
	{'m', "month", 12, 4, trunc_month, dec_month, inc_month, pretty_month},
	{'q', "quarter", 12, 4, trunc_quarter, dec_quarter, inc_quarter, pretty_quarter},
	{'y', "year", 12, 4, trunc_year, dec_year, inc_year, pretty_year},
};

static void add_commit(struct string_list *authors, struct commit *commit,
	struct Period *period)
{
	struct commitinfo *info;
	struct string_list_item *author, *item;
	struct authorstat *authorstat;
	struct string_list *items;
	char *tmp;
	struct tm *date;
	time_t t;

	info = cgit_parse_commit(commit);
	tmp = xstrdup(info->author);
	author = string_list_insert(tmp, authors);
	if (!author->util)
		author->util = xcalloc(1, sizeof(struct authorstat));
	else
		free(tmp);
	authorstat = author->util;
	items = &authorstat->list;
	t = info->committer_date;
	date = gmtime(&t);
	period->trunc(date);
	tmp = xstrdup(period->pretty(date));
	item = string_list_insert(tmp, items);
	if (item->util)
		free(tmp);
	item->util++;
	authorstat->total++;
	cgit_free_commitinfo(info);
}

static int cmp_total_commits(const void *a1, const void *a2)
{
	const struct string_list_item *i1 = a1;
	const struct string_list_item *i2 = a2;
	const struct authorstat *auth1 = i1->util;
	const struct authorstat *auth2 = i2->util;

	return auth2->total - auth1->total;
}

/* Walk the commit DAG and collect number of commits per author per
 * timeperiod into a nested string_list collection.
 */
struct string_list collect_stats(struct cgit_context *ctx,
	struct Period *period)
{
	struct string_list authors;
	struct rev_info rev;
	struct commit *commit;
	const char *argv[] = {NULL, ctx->qry.head, NULL, NULL};
	time_t now;
	long i;
	struct tm *tm;
	char tmp[11];

	time(&now);
	tm = gmtime(&now);
	period->trunc(tm);
	for (i = 1; i < period->count; i++)
		period->dec(tm);
	strftime(tmp, sizeof(tmp), "%Y-%m-%d", tm);
	argv[2] = xstrdup(fmt("--since=%s", tmp));
	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.commit_format = CMIT_FMT_DEFAULT;
	rev.no_merges = 1;
	rev.verbose_header = 1;
	rev.show_root_diff = 0;
	setup_revisions(3, argv, &rev, NULL);
	prepare_revision_walk(&rev);
	memset(&authors, 0, sizeof(authors));
	while ((commit = get_revision(&rev)) != NULL) {
		add_commit(&authors, commit, period);
		free(commit->buffer);
		free_commit_list(commit->parents);
	}
	return authors;
}

void print_combined_authorrow(struct string_list *authors, int from, int to,
	const char *name, const char *leftclass, const char *centerclass,
	const char *rightclass, struct Period *period)
{
	struct string_list_item *author;
	struct authorstat *authorstat;
	struct string_list *items;
	struct string_list_item *date;
	time_t now;
	long i, j, total, subtotal;
	struct tm *tm;
	char *tmp;

	time(&now);
	tm = gmtime(&now);
	period->trunc(tm);
	for (i = 1; i < period->count; i++)
		period->dec(tm);

	total = 0;
	htmlf("<tr><td class='%s'>%s</td>", leftclass,
		fmt(name, to - from + 1));
	for (j = 0; j < period->count; j++) {
		tmp = period->pretty(tm);
		period->inc(tm);
		subtotal = 0;
		for (i = from; i <= to; i++) {
			author = &authors->items[i];
			authorstat = author->util;
			items = &authorstat->list;
			date = string_list_lookup(tmp, items);
			if (date)
				subtotal += (size_t)date->util;
		}
		htmlf("<td class='%s'>%d</td>", centerclass, subtotal);
		total += subtotal;
	}
	htmlf("<td class='%s'>%d</td></tr>", rightclass, total);
}

void print_authors(struct string_list *authors, int top, struct Period *period)
{
	struct string_list_item *author;
	struct authorstat *authorstat;
	struct string_list *items;
	struct string_list_item *date;
	time_t now;
	long i, j, total;
	struct tm *tm;
	char *tmp;

	time(&now);
	tm = gmtime(&now);
	period->trunc(tm);
	for (i = 1; i < period->count; i++)
		period->dec(tm);

	html("<table class='stats'><tr><th>Author</th>");
	for (j = 0; j < period->count; j++) {
		tmp = period->pretty(tm);
		htmlf("<th>%s</th>", tmp);
		period->inc(tm);
	}
	html("<th>Total</th></tr>\n");

	if (top <= 0 || top > authors->nr)
		top = authors->nr;

	for (i = 0; i < top; i++) {
		author = &authors->items[i];
		html("<tr><td class='left'>");
		html_txt(author->string);
		html("</td>");
		authorstat = author->util;
		items = &authorstat->list;
		total = 0;
		for (j = 0; j < period->count; j++)
			period->dec(tm);
		for (j = 0; j < period->count; j++) {
			tmp = period->pretty(tm);
			period->inc(tm);
			date = string_list_lookup(tmp, items);
			if (!date)
				html("<td>0</td>");
			else {
				htmlf("<td>%d</td>", date->util);
				total += (size_t)date->util;
			}
		}
		htmlf("<td class='sum'>%d</td></tr>", total);
	}

	if (top < authors->nr)
		print_combined_authorrow(authors, top, authors->nr - 1,
			"Others (%d)", "left", "", "sum", period);

	print_combined_authorrow(authors, 0, authors->nr - 1, "Total",
		"total", "sum", "sum", period);
	html("</table>");
}

/* Create a sorted string_list with one entry per author. The util-field
 * for each author is another string_list which is used to calculate the
 * number of commits per time-interval.
 */
void cgit_show_stats(struct cgit_context *ctx)
{
	struct string_list authors;
	struct Period *period;
	int top, i;

	period = &periods[0];
	if (ctx->qry.period) {
		for (i = 0; i < sizeof(periods) / sizeof(periods[0]); i++)
			if (periods[i].code == ctx->qry.period[0]) {
				period = &periods[i];
				break;
			}
	}
	authors = collect_stats(ctx, period);
	qsort(authors.items, authors.nr, sizeof(struct string_list_item),
		cmp_total_commits);

	top = ctx->qry.ofs;
	if (!top)
		top = 10;
	htmlf("<h2>Commits per author per %s</h2>", period->name);

	html("<form method='get' action='.' style='float: right; text-align: right;'>");
	if (strcmp(ctx->qry.head, ctx->repo->defbranch))
		htmlf("<input type='hidden' name='h' value='%s'/>", ctx->qry.head);
	html("Period: ");
	html("<select name='period' onchange='this.form.submit();'>");
	for (i = 0; i < sizeof(periods) / sizeof(periods[0]); i++)
		htmlf("<option value='%c'%s>%s</option>",
			periods[i].code,
			period == &periods[i] ? " selected" : "",
			periods[i].name);
	html("</select><br/><br/>");
	html("Authors: ");
	html("");
	html("<select name='ofs' onchange='this.form.submit();'>");
	htmlf("<option value='10'%s>10</option>", top == 10 ? " selected" : "");
	htmlf("<option value='25'%s>25</option>", top == 25 ? " selected" : "");
	htmlf("<option value='50'%s>50</option>", top == 50 ? " selected" : "");
	htmlf("<option value='100'%s>100</option>", top == 100 ? " selected" : "");
	htmlf("<option value='-1'%s>All</option>", top == -1 ? " selected" : "");
	html("</select>");
	html("<noscript>&nbsp;&nbsp;<input type='submit' value='Reload'/></noscript>");
	html("</form>");
	print_authors(&authors, top, period);
}

