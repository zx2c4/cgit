#include "cgit.h"
#include "ui-ssdiff.h"
#include "html.h"
#include "ui-shared.h"
#include "ui-diff.h"

extern int use_ssdiff;

static int current_old_line, current_new_line;
static int **L = NULL;

struct deferred_lines {
	int line_no;
	char *line;
	struct deferred_lines *next;
};

static struct deferred_lines *deferred_old, *deferred_old_last;
static struct deferred_lines *deferred_new, *deferred_new_last;

static void create_or_reset_lcs_table(void)
{
	int i;

	if (L != NULL) {
		memset(*L, 0, sizeof(int) * MAX_SSDIFF_SIZE);
		return;
	}

	// xcalloc will die if we ran out of memory;
	// not very helpful for debugging
	L = (int**)xcalloc(MAX_SSDIFF_M, sizeof(int *));
	*L = (int*)xcalloc(MAX_SSDIFF_SIZE, sizeof(int));

	for (i = 1; i < MAX_SSDIFF_M; i++) {
		L[i] = *L + i * MAX_SSDIFF_N;
	}
}

static char *longest_common_subsequence(char *A, char *B)
{
	int i, j, ri;
	int m = strlen(A);
	int n = strlen(B);
	int tmp1, tmp2;
	int lcs_length;
	char *result;

	// We bail if the lines are too long
	if (m >= MAX_SSDIFF_M || n >= MAX_SSDIFF_N)
		return NULL;

	create_or_reset_lcs_table();

	for (i = m; i >= 0; i--) {
		for (j = n; j >= 0; j--) {
			if (A[i] == '\0' || B[j] == '\0') {
				L[i][j] = 0;
			} else if (A[i] == B[j]) {
				L[i][j] = 1 + L[i + 1][j + 1];
			} else {
				tmp1 = L[i + 1][j];
				tmp2 = L[i][j + 1];
				L[i][j] = (tmp1 > tmp2 ? tmp1 : tmp2);
			}
		}
	}

	lcs_length = L[0][0];
	result = xmalloc(lcs_length + 2);
	memset(result, 0, sizeof(*result) * (lcs_length + 2));

	ri = 0;
	i = 0;
	j = 0;
	while (i < m && j < n) {
		if (A[i] == B[j]) {
			result[ri] = A[i];
			ri += 1;
			i += 1;
			j += 1;
		} else if (L[i + 1][j] >= L[i][j + 1]) {
			i += 1;
		} else {
			j += 1;
		}
	}

	return result;
}

static int line_from_hunk(char *line, char type)
{
	char *buf1, *buf2;
	int len, res;

	buf1 = strchr(line, type);
	if (buf1 == NULL)
		return 0;
	buf1 += 1;
	buf2 = strchr(buf1, ',');
	if (buf2 == NULL)
		return 0;
	len = buf2 - buf1;
	buf2 = xmalloc(len + 1);
	strlcpy(buf2, buf1, len + 1);
	res = atoi(buf2);
	free(buf2);
	return res;
}

static char *replace_tabs(char *line)
{
	char *prev_buf = line;
	char *cur_buf;
	size_t linelen = strlen(line);
	int n_tabs = 0;
	int i;
	char *result;
	int result_len;

	if (linelen == 0) {
		result = xmalloc(1);
		result[0] = '\0';
		return result;
	}

	for (i = 0; i < linelen; i++) {
		if (line[i] == '\t')
			n_tabs += 1;
	}
	result_len = linelen + n_tabs * 8;
	result = xmalloc(result_len + 1);
	result[0] = '\0';

	for (;;) {
		cur_buf = strchr(prev_buf, '\t');
		if (!cur_buf) {
			strncat(result, prev_buf, result_len);
			break;
		} else {
			strncat(result, prev_buf, cur_buf - prev_buf);
			linelen = strlen(result);
			memset(&result[linelen], ' ', 8 - (linelen % 8));
			result[linelen + 8 - (linelen % 8)] = '\0';
		}
		prev_buf = cur_buf + 1;
	}
	return result;
}

static int calc_deferred_lines(struct deferred_lines *start)
{
	struct deferred_lines *item = start;
	int result = 0;
	while (item) {
		result += 1;
		item = item->next;
	}
	return result;
}

static void deferred_old_add(char *line, int line_no)
{
	struct deferred_lines *item = xmalloc(sizeof(struct deferred_lines));
	item->line = xstrdup(line);
	item->line_no = line_no;
	item->next = NULL;
	if (deferred_old) {
		deferred_old_last->next = item;
		deferred_old_last = item;
	} else {
		deferred_old = deferred_old_last = item;
	}
}

static void deferred_new_add(char *line, int line_no)
{
	struct deferred_lines *item = xmalloc(sizeof(struct deferred_lines));
	item->line = xstrdup(line);
	item->line_no = line_no;
	item->next = NULL;
	if (deferred_new) {
		deferred_new_last->next = item;
		deferred_new_last = item;
	} else {
		deferred_new = deferred_new_last = item;
	}
}

static void print_part_with_lcs(char *class, char *line, char *lcs)
{
	int line_len = strlen(line);
	int i, j;
	char c[2] = " ";
	int same = 1;

	j = 0;
	for (i = 0; i < line_len; i++) {
		c[0] = line[i];
		if (same) {
			if (line[i] == lcs[j])
				j += 1;
			else {
				same = 0;
				htmlf("<span class='%s'>", class);
			}
		} else if (line[i] == lcs[j]) {
			same = 1;
			htmlf("</span>");
			j += 1;
		}
		html_txt(c);
	}
}

static void print_ssdiff_line(char *class,
			      int old_line_no,
			      char *old_line,
			      int new_line_no,
			      char *new_line, int individual_chars)
{
	char *lcs = NULL;

	if (old_line)
		old_line = replace_tabs(old_line + 1);
	if (new_line)
		new_line = replace_tabs(new_line + 1);
	if (individual_chars && old_line && new_line)
		lcs = longest_common_subsequence(old_line, new_line);
	html("<tr>\n");
	if (old_line_no > 0) {
		struct diff_filespec *old_file = cgit_get_current_old_file();
		char *lineno_str = fmt("n%d", old_line_no);
		char *id_str = fmt("id=%s#%s", is_null_oid(&old_file->oid)?"HEAD":oid_to_hex(old_rev_oid), lineno_str);
		char *fileurl = cgit_fileurl(ctx.repo->url, "tree", old_file->path, id_str);
		html("<td class='lineno'><a href='");
		html(fileurl);
		htmlf("' id='%s'>%s</a>", lineno_str, lineno_str + 1);
		html("</td>");
		htmlf("<td class='%s'>", class);
		free(fileurl);
	} else if (old_line)
		htmlf("<td class='lineno'></td><td class='%s'>", class);
	else
		htmlf("<td class='lineno'></td><td class='%s_dark'>", class);
	if (old_line) {
		if (lcs)
			print_part_with_lcs("del", old_line, lcs);
		else
			html_txt(old_line);
	}

	html("</td>\n");
	if (new_line_no > 0) {
		struct diff_filespec *new_file = cgit_get_current_new_file();
		char *lineno_str = fmt("n%d", new_line_no);
		char *id_str = fmt("id=%s#%s", is_null_oid(&new_file->oid)?"HEAD":oid_to_hex(new_rev_oid), lineno_str);
		char *fileurl = cgit_fileurl(ctx.repo->url, "tree", new_file->path, id_str);
		html("<td class='lineno'><a href='");
		html(fileurl);
		htmlf("' id='%s'>%s</a>", lineno_str, lineno_str + 1);
		html("</td>");
		htmlf("<td class='%s'>", class);
		free(fileurl);
	} else if (new_line)
		htmlf("<td class='lineno'></td><td class='%s'>", class);
	else
		htmlf("<td class='lineno'></td><td class='%s_dark'>", class);
	if (new_line) {
		if (lcs)
			print_part_with_lcs("add", new_line, lcs);
		else
			html_txt(new_line);
	}

	html("</td></tr>");
	if (lcs)
		free(lcs);
	if (new_line)
		free(new_line);
	if (old_line)
		free(old_line);
}

static void print_deferred_old_lines(void)
{
	struct deferred_lines *iter_old, *tmp;
	iter_old = deferred_old;
	while (iter_old) {
		print_ssdiff_line("del", iter_old->line_no,
				  iter_old->line, -1, NULL, 0);
		tmp = iter_old->next;
		free(iter_old);
		iter_old = tmp;
	}
}

static void print_deferred_new_lines(void)
{
	struct deferred_lines *iter_new, *tmp;
	iter_new = deferred_new;
	while (iter_new) {
		print_ssdiff_line("add", -1, NULL,
				  iter_new->line_no, iter_new->line, 0);
		tmp = iter_new->next;
		free(iter_new);
		iter_new = tmp;
	}
}

static void print_deferred_changed_lines(void)
{
	struct deferred_lines *iter_old, *iter_new, *tmp;
	int n_old_lines = calc_deferred_lines(deferred_old);
	int n_new_lines = calc_deferred_lines(deferred_new);
	int individual_chars = (n_old_lines == n_new_lines ? 1 : 0);

	iter_old = deferred_old;
	iter_new = deferred_new;
	while (iter_old || iter_new) {
		if (iter_old && iter_new)
			print_ssdiff_line("changed", iter_old->line_no,
					  iter_old->line,
					  iter_new->line_no, iter_new->line,
					  individual_chars);
		else if (iter_old)
			print_ssdiff_line("changed", iter_old->line_no,
					  iter_old->line, -1, NULL, 0);
		else if (iter_new)
			print_ssdiff_line("changed", -1, NULL,
					  iter_new->line_no, iter_new->line, 0);
		if (iter_old) {
			tmp = iter_old->next;
			free(iter_old);
			iter_old = tmp;
		}

		if (iter_new) {
			tmp = iter_new->next;
			free(iter_new);
			iter_new = tmp;
		}
	}
}

void cgit_ssdiff_print_deferred_lines(void)
{
	if (!deferred_old && !deferred_new)
		return;
	if (deferred_old && !deferred_new)
		print_deferred_old_lines();
	else if (!deferred_old && deferred_new)
		print_deferred_new_lines();
	else
		print_deferred_changed_lines();
	deferred_old = deferred_old_last = NULL;
	deferred_new = deferred_new_last = NULL;
}

/*
 * print a single line returned from xdiff
 */
void cgit_ssdiff_line_cb(char *line, int len)
{
	char c = line[len - 1];
	line[len - 1] = '\0';
	if (line[0] == '@') {
		current_old_line = line_from_hunk(line, '-');
		current_new_line = line_from_hunk(line, '+');
	}

	if (line[0] == ' ') {
		if (deferred_old || deferred_new)
			cgit_ssdiff_print_deferred_lines();
		print_ssdiff_line("ctx", current_old_line, line,
				  current_new_line, line, 0);
		current_old_line += 1;
		current_new_line += 1;
	} else if (line[0] == '+') {
		deferred_new_add(line, current_new_line);
		current_new_line += 1;
	} else if (line[0] == '-') {
		deferred_old_add(line, current_old_line);
		current_old_line += 1;
	} else if (line[0] == '@') {
		html("<tr><td colspan='4' class='hunk'>");
		html_txt(line);
		html("</td></tr>");
	} else {
		html("<tr><td colspan='4' class='ctx'>");
		html_txt(line);
		html("</td></tr>");
	}
	line[len - 1] = c;
}

void cgit_ssdiff_header_begin(void)
{
	current_old_line = -1;
	current_new_line = -1;
	html("<tr><td class='space' colspan='4'><div></div></td></tr>");
	html("<tr><td class='head' colspan='4'>");
}

void cgit_ssdiff_header_end(void)
{
	html("</td><tr>");
}

void cgit_ssdiff_footer(void)
{
	if (deferred_old || deferred_new)
		cgit_ssdiff_print_deferred_lines();
	html("<tr><td class='foot' colspan='4'></td></tr>");
}
