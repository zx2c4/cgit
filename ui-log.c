/* ui-log.c: functions for log output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

static int get_one_line(char *txt)
{
	char *t;

	for(t=txt; *t != '\n' && t != '\0'; t++)
		;
	*t = '\0';
	return t-txt-1;
}

static void cgit_print_commit_shortlog(struct commit *commit)
{
	char *h, *t, *p; 
	char *tree = NULL, *author = NULL, *subject = NULL;
	int len;
	time_t sec;
	struct tm *time;
	char buf[32];

	h = t = commit->buffer;
	
	if (strncmp(h, "tree ", 5))
		die("Bad commit format: %s", 
		    sha1_to_hex(commit->object.sha1));
	
	len = get_one_line(h);
	tree = h+5;
	h += len + 2;

	while (!strncmp(h, "parent ", 7))
		h += get_one_line(h) + 2;
	
	if (!strncmp(h, "author ", 7)) {
		author = h+7;
		h += get_one_line(h) + 2;
		t = author;
		while(t!=h && *t!='<') 
			t++;
		*t='\0';
		p = t;
		while(--t!=author && *t==' ')
			*t='\0';
		while(++p!=h && *p!='>')
			;
		while(++p!=h && !isdigit(*p))
			;

		t = p;
		while(++p && isdigit(*p))
			;
		*p = '\0';
		sec = atoi(t);
		time = gmtime(&sec);
	}

	while((len = get_one_line(h)) > 0)
		h += len+2;

	h++;
	len = get_one_line(h);

	subject = h;

	html("<tr><td>");
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", time);
	html_txt(buf);
	html("</td><td>");
	char *qry = fmt("id=%s", sha1_to_hex(commit->object.sha1));
	char *url = cgit_pageurl(cgit_query_repo, "view", qry);
	html_link_open(url, NULL, NULL);
	html_txt(subject);
	html_link_close();
	html("</td><td>");
	html_txt(author);
	html("</td></tr>\n");
}

void cgit_print_log(const char *tip, int ofs, int cnt)
{
	struct rev_info rev;
	struct commit *commit;
	const char *argv[2] = {NULL, tip};
	int n = 0;
	
	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.commit_format = CMIT_FMT_DEFAULT;
	rev.verbose_header = 1;
	rev.show_root_diff = 0;
	setup_revisions(2, argv, &rev, NULL);
	prepare_revision_walk(&rev);

	html("<h2>Log</h2>");
	html("<table class='list'>");
	html("<tr><th>Date</th><th>Message</th><th>Author</th></tr>\n");
	while ((commit = get_revision(&rev)) != NULL && n++ < 100) {
		cgit_print_commit_shortlog(commit);
		free(commit->buffer);
		commit->buffer = NULL;
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}
	html("</table>\n");
}

