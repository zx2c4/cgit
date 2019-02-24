#include "cgit.h"
#include "ui-stats.h"
#include "html.h"
#include "ui-shared.h"

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
	while (tm->tm_mon % 3 != 0)
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

static const struct cgit_period periods[] = {
	{'w', "week", 12, 4, trunc_week, dec_week, inc_week, pretty_week},
	{'m', "month", 12, 4, trunc_month, dec_month, inc_month, pretty_month},
	{'q', "quarter", 12, 4, trunc_quarter, dec_quarter, inc_quarter, pretty_quarter},
	{'y', "year", 12, 4, trunc_year, dec_year, inc_year, pretty_year},
};

/* Given a period code or name, return a period index (1, 2, 3 or 4)
 * and update the period pointer to the correcsponding struct.
 * If no matching code is found, return 0.
 */
int cgit_find_stats_period(const char *expr, const struct cgit_period **period)
{
	int i;
	char code = '\0';

	if (!expr)
		return 0;

	if (strlen(expr) == 1)
		code = expr[0];

	for (i = 0; i < sizeof(periods) / sizeof(periods[0]); i++)
		if (periods[i].code == code || !strcmp(periods[i].name, expr)) {
			if (period)
				*period = &periods[i];
			return i + 1;
		}
	return 0;
}

const char *cgit_find_stats_periodname(int idx)
{
	if (idx > 0 && idx < 4)
		return periods[idx - 1].name;
	else
		return "";
}

static void add_commit(struct string_list *authors, struct commit *commit,
	const struct cgit_period *period)
{
	struct commitinfo *info;
	struct string_list_item *author, *item;
	struct authorstat *authorstat;
	struct string_list *items;
	char *tmp;
	struct tm *date;
	time_t t;
	uintptr_t *counter;

	info = cgit_parse_commit(commit);
	tmp = xstrdup(info->author);
	author = string_list_insert(authors, tmp);
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
	item = string_list_insert(items, tmp);
	counter = (uintptr_t *)&item->util;
	if (*counter)
		free(tmp);
	(*counter)++;

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
static struct string_list collect_stats(const struct cgit_period *period)
{
	struct string_list authors;
	struct rev_info rev;
	struct commit *commit;
	const char *argv[] = {NULL, ctx.qry.head, NULL, NULL, NULL, NULL};
	int argc = 3;
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
	if (ctx.qry.path) {
		argv[3] = "--";
		argv[4] = ctx.qry.path;
		argc += 2;
	}
	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.commit_format = CMIT_FMT_DEFAULT;
	rev.max_parents = 1;
	rev.verbose_header = 1;
	rev.show_root_diff = 0;
	setup_revisions(argc, argv, &rev, NULL);
	prepare_revision_walk(&rev);
	memset(&authors, 0, sizeof(authors));
	while ((commit = get_revision(&rev)) != NULL) {
		add_commit(&authors, commit, period);
		free_commit_buffer(the_repository->parsed_objects, commit);
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}
	return authors;
}

static void print_combined_authorrow(struct string_list *authors, int from,
				     int to, const char *name,
				     const char *leftclass,
				     const char *centerclass,
				     const char *rightclass,
				     const struct cgit_period *period)
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
			date = string_list_lookup(items, tmp);
			if (date)
				subtotal += (uintptr_t)date->util;
		}
		htmlf("<td class='%s'>%ld</td>", centerclass, subtotal);
		total += subtotal;
	}
	htmlf("<td class='%s'>%ld</td></tr>", rightclass, total);
}

static void print_authors(struct string_list *authors, int top,
			  const struct cgit_period *period)
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
			date = string_list_lookup(items, tmp);
			if (!date)
				html("<td>0</td>");
			else {
				htmlf("<td>%lu</td>", (uintptr_t)date->util);
				total += (uintptr_t)date->util;
			}
		}
		htmlf("<td class='sum'>%ld</td></tr>", total);
	}

	if (top < authors->nr)
		print_combined_authorrow(authors, top, authors->nr - 1,
			"Others (%ld)", "left", "", "sum", period);

	print_combined_authorrow(authors, 0, authors->nr - 1, "Total",
		"total", "sum", "sum", period);
	html("</table>");
}

/* Create a sorted string_list with one entry per author. The util-field
 * for each author is another string_list which is used to calculate the
 * number of commits per time-interval.
 */
void cgit_show_stats(void)
{
	struct string_list authors;
	const struct cgit_period *period;
	int top, i;
	const char *code = "w";

	if (ctx.qry.period)
		code = ctx.qry.period;

	i = cgit_find_stats_period(code, &period);
	if (!i) {
		cgit_print_error_page(404, "Not found",
			"Unknown statistics type: %c", code[0]);
		return;
	}
	if (i > ctx.repo->max_stats) {
		cgit_print_error_page(400, "Bad request",
			"Statistics type disabled: %s", period->name);
		return;
	}
	authors = collect_stats(period);
	qsort(authors.items, authors.nr, sizeof(struct string_list_item),
		cmp_total_commits);

	top = ctx.qry.ofs;
	if (!top)
		top = 10;

	cgit_print_layout_start();
	html("<div class='cgit-panel'>");
	html("<b>stat options</b>");
	html("<form method='get'>");
	cgit_add_hidden_formfields(1, 0, "stats");
	html("<table><tr><td colspan='2'/></tr>");
	if (ctx.repo->max_stats > 1) {
		html("<tr><td class='label'>Period:</td>");
		html("<td class='ctrl'><select name='period' onchange='this.form.submit();'>");
		for (i = 0; i < ctx.repo->max_stats; i++)
			html_option(fmt("%c", periods[i].code),
				    periods[i].name, fmt("%c", period->code));
		html("</select></td></tr>");
	}
	html("<tr><td class='label'>Authors:</td>");
	html("<td class='ctrl'><select name='ofs' onchange='this.form.submit();'>");
	html_intoption(10, "10", top);
	html_intoption(25, "25", top);
	html_intoption(50, "50", top);
	html_intoption(100, "100", top);
	html_intoption(-1, "all", top);
	html("</select></td></tr>");
	html("<tr><td/><td class='ctrl'>");
	html("<noscript><input type='submit' value='Reload'/></noscript>");
	html("</td></tr></table>");
	html("</form>");
	html("</div>");
	htmlf("<h2>Commits per author per %s", period->name);
	if (ctx.qry.path) {
		html(" (path '");
		html_txt(ctx.qry.path);
		html("')");
	}
	html("</h2>");
	print_authors(&authors, top, period);
	cgit_print_layout_end();
}

