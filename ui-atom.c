/* ui-atom.c: functions for atom feeds
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-atom.h"
#include "html.h"
#include "ui-shared.h"

static void add_entry(struct commit *commit, const char *host)
{
	char delim = '&';
	char *hex;
	char *mail, *t, *t2;
	struct commitinfo *info;

	info = cgit_parse_commit(commit);
	hex = oid_to_hex(&commit->object.oid);
	html("<entry>\n");
	html("<title>");
	html_txt(info->subject);
	html("</title>\n");
	html("<updated>");
	html_txt(show_date(info->committer_date, 0,
                    date_mode_from_type(DATE_ISO8601_STRICT)));
	html("</updated>\n");
	html("<author>\n");
	if (info->author) {
		html("<name>");
		html_txt(info->author);
		html("</name>\n");
	}
	if (info->author_email && !ctx.cfg.noplainemail) {
		mail = xstrdup(info->author_email);
		t = strchr(mail, '<');
		if (t)
			t++;
		else
			t = mail;
		t2 = strchr(t, '>');
		if (t2)
			*t2 = '\0';
		html("<email>");
		html_txt(t);
		html("</email>\n");
		free(mail);
	}
	html("</author>\n");
	html("<published>");
	html_txt(show_date(info->author_date, 0,
                    date_mode_from_type(DATE_ISO8601_STRICT)));
	html("</published>\n");
	if (host) {
		char *pageurl;
		html("<link rel='alternate' type='text/html' href='");
		html(cgit_httpscheme());
		html_attr(host);
		pageurl = cgit_pageurl(ctx.repo->url, "commit", NULL);
		html_attr(pageurl);
		if (ctx.cfg.virtual_root)
			delim = '?';
		html_attrf("%cid=%s", delim, hex);
		html("'/>\n");
		free(pageurl);
	}
	htmlf("<id>%s</id>\n", hex);
	html("<content type='text'>\n");
	html_txt(info->msg);
	html("</content>\n");
	html("<content type='xhtml'>\n");
	html("<div xmlns='http://www.w3.org/1999/xhtml'>\n");
	html("<pre>\n");
	html_txt(info->msg);
	html("</pre>\n");
	html("</div>\n");
	html("</content>\n");
	html("</entry>\n");
	cgit_free_commitinfo(info);
}


void cgit_print_atom(char *tip, const char *path, int max_count)
{
	char *host;
	const char *argv[] = {NULL, tip, NULL, NULL, NULL};
	struct commit *commit;
	struct rev_info rev;
	int argc = 2;

	if (ctx.qry.show_all)
		argv[1] = "--all";
	else if (!tip)
		argv[1] = ctx.qry.head;

	if (path) {
		argv[argc++] = "--";
		argv[argc++] = path;
	}

	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.commit_format = CMIT_FMT_DEFAULT;
	rev.verbose_header = 1;
	rev.show_root_diff = 0;
	rev.max_count = max_count;
	setup_revisions(argc, argv, &rev, NULL);
	prepare_revision_walk(&rev);

	host = cgit_hosturl();
	ctx.page.mimetype = "text/xml";
	ctx.page.charset = "utf-8";
	cgit_print_http_headers();
	html("<feed xmlns='http://www.w3.org/2005/Atom'>\n");
	html("<title>");
	html_txt(ctx.repo->name);
	if (path) {
		html("/");
		html_txt(path);
	}
	if (tip && !ctx.qry.show_all) {
		html(", branch ");
		html_txt(tip);
	}
	html("</title>\n");
	html("<subtitle>");
	html_txt(ctx.repo->desc);
	html("</subtitle>\n");
	if (host) {
		char *repourl = cgit_repourl(ctx.repo->url);
		html("<link rel='alternate' type='text/html' href='");
		html(cgit_httpscheme());
		html_attr(host);
		html_attr(repourl);
		html("'/>\n");
		free(repourl);
	}
	while ((commit = get_revision(&rev)) != NULL) {
		add_entry(commit, host);
		free_commit_buffer(commit);
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}
	html("</feed>\n");
	free(host);
}
