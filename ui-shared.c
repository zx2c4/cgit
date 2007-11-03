/* ui-shared.c: common web output functions
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

const char cgit_doctype[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";

static char *http_date(time_t t)
{
	static char day[][4] =
		{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	static char month[][4] =
		{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		 "Jul", "Aug", "Sep", "Oct", "Now", "Dec"};
	struct tm *tm = gmtime(&t);
	return fmt("%s, %02d %s %04d %02d:%02d:%02d GMT", day[tm->tm_wday],
		   tm->tm_mday, month[tm->tm_mon], 1900+tm->tm_year,
		   tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static long ttl_seconds(long ttl)
{
	if (ttl<0)
		return 60 * 60 * 24 * 365;
	else
		return ttl * 60;
}

void cgit_print_error(char *msg)
{
	html("<div class='error'>");
	html_txt(msg);
	html("</div>\n");
}

char *cgit_rooturl()
{
	if (cgit_virtual_root)
		return fmt("%s/", cgit_virtual_root);
	else
		return cgit_script_name;
}

char *cgit_repourl(const char *reponame)
{
	if (cgit_virtual_root) {
		return fmt("%s/%s/", cgit_virtual_root, reponame);
	} else {
		return fmt("?r=%s", reponame);
	}
}

char *cgit_fileurl(const char *reponame, const char *pagename,
		   const char *filename, const char *query)
{
	char *tmp;
	char *delim;

	if (cgit_virtual_root) {
		tmp = fmt("%s/%s/%s/%s", cgit_virtual_root, reponame,
			  pagename, (filename ? filename:""));
		delim = "?";
	} else {
		tmp = fmt("?url=%s/%s/%s", reponame, pagename,
			  (filename ? filename : ""));
		delim = "&";
	}
	if (query)
		tmp = fmt("%s%s%s", tmp, delim, query);
	return tmp;
}

char *cgit_pageurl(const char *reponame, const char *pagename,
		   const char *query)
{
	return cgit_fileurl(reponame,pagename,0,query);
}

const char *cgit_repobasename(const char *reponame)
{
	/* I assume we don't need to store more than one repo basename */
	static char rvbuf[1024];
	int p;
	const char *rv;
	strncpy(rvbuf,reponame,sizeof(rvbuf));
	if(rvbuf[sizeof(rvbuf)-1])
		die("cgit_repobasename: truncated repository name '%s'", reponame);
	p = strlen(rvbuf)-1;
	/* strip trailing slashes */
	while(p && rvbuf[p]=='/') rvbuf[p--]=0;
	/* strip trailing .git */
	if(p>=3 && !strncmp(&rvbuf[p-3],".git",4)) {
		p -= 3; rvbuf[p--] = 0;
	}
	/* strip more trailing slashes if any */
	while( p && rvbuf[p]=='/') rvbuf[p--]=0;
	/* find last slash in the remaining string */
	rv = strrchr(rvbuf,'/');
	if(rv)
		return ++rv;
	return rvbuf;
}

char *cgit_currurl()
{
	if (!cgit_virtual_root)
		return cgit_script_name;
	else if (cgit_query_page)
		return fmt("%s/%s/%s/", cgit_virtual_root, cgit_query_repo, cgit_query_page);
	else if (cgit_query_repo)
		return fmt("%s/%s/", cgit_virtual_root, cgit_query_repo);
	else
		return fmt("%s/", cgit_virtual_root);
}

static char *repolink(char *title, char *class, char *page, char *head,
		      char *path)
{
	char *delim = "?";

	html("<a");
	if (title) {
		html(" title='");
		html_attr(title);
		html("'");
	}
	if (class) {
		html(" class='");
		html_attr(class);
		html("'");
	}
	html(" href='");
	if (cgit_virtual_root) {
		html_attr(cgit_virtual_root);
		if (cgit_virtual_root[strlen(cgit_virtual_root) - 1] != '/')
			html("/");
		html_attr(cgit_repo->url);
		if (cgit_repo->url[strlen(cgit_repo->url) - 1] != '/')
			html("/");
		if (page) {
			html(page);
			html("/");
			if (path)
				html_attr(path);
		}
	} else {
		html(cgit_script_name);
		html("?url=");
		html_attr(cgit_repo->url);
		if (cgit_repo->url[strlen(cgit_repo->url) - 1] != '/')
			html("/");
		if (page) {
			html(page);
			html("/");
			if (path)
				html_attr(path);
		}
		delim = "&amp;";
	}
	if (head && strcmp(head, cgit_repo->defbranch)) {
		html(delim);
		html("h=");
		html_attr(head);
		delim = "&amp;";
	}
	return fmt("%s", delim);
}

static void reporevlink(char *page, char *name, char *title, char *class,
			char *head, char *rev, char *path)
{
	char *delim;

	delim = repolink(title, class, page, head, path);
	if (rev && strcmp(rev, cgit_query_head)) {
		html(delim);
		html("id=");
		html_attr(rev);
	}
	html("'>");
	html_txt(name);
	html("</a>");
}

void cgit_tree_link(char *name, char *title, char *class, char *head,
		    char *rev, char *path)
{
	reporevlink("tree", name, title, class, head, rev, path);
}

void cgit_log_link(char *name, char *title, char *class, char *head,
		   char *rev, char *path, int ofs, char *grep, char *pattern)
{
	char *delim;

	delim = repolink(title, class, "log", head, path);
	if (rev && strcmp(rev, cgit_query_head)) {
		html(delim);
		html("id=");
		html_attr(rev);
		delim = "&";
	}
	if (grep && pattern) {
		html(delim);
		html("qt=");
		html_attr(grep);
		delim = "&";
		html(delim);
		html("q=");
		html_attr(pattern);
	}
	if (ofs > 0) {
		html(delim);
		html("ofs=");
		htmlf("%d", ofs);
	}
	html("'>");
	html_txt(name);
	html("</a>");
}

void cgit_commit_link(char *name, char *title, char *class, char *head,
		      char *rev)
{
	if (strlen(name) > cgit_max_msg_len && cgit_max_msg_len >= 15) {
		name[cgit_max_msg_len] = '\0';
		name[cgit_max_msg_len - 1] = '.';
		name[cgit_max_msg_len - 2] = '.';
		name[cgit_max_msg_len - 3] = '.';
	}
	reporevlink("commit", name, title, class, head, rev, NULL);
}

void cgit_refs_link(char *name, char *title, char *class, char *head,
		    char *rev, char *path)
{
	reporevlink("refs", name, title, class, head, rev, path);
}

void cgit_snapshot_link(char *name, char *title, char *class, char *head,
			char *rev, char *archivename)
{
	reporevlink("snapshot", name, title, class, head, rev, archivename);
}

void cgit_diff_link(char *name, char *title, char *class, char *head,
		    char *new_rev, char *old_rev, char *path)
{
	char *delim;

	delim = repolink(title, class, "diff", head, path);
	if (new_rev && strcmp(new_rev, cgit_query_head)) {
		html(delim);
		html("id=");
		html_attr(new_rev);
		delim = "&amp;";
	}
	if (old_rev) {
		html(delim);
		html("id2=");
		html_attr(old_rev);
	}
	html("'>");
	html_txt(name);
	html("</a>");
}

void cgit_object_link(struct object *obj)
{
	char *page, *arg, *url;

	if (obj->type == OBJ_COMMIT) {
                cgit_commit_link(fmt("commit %s", sha1_to_hex(obj->sha1)), NULL, NULL,
				 cgit_query_head, sha1_to_hex(obj->sha1));
		return;
	} else if (obj->type == OBJ_TREE) {
		page = "tree";
		arg = "id";
	} else if (obj->type == OBJ_TAG) {
		page = "tag";
		arg = "id";
	} else {
		page = "blob";
		arg = "id";
	}

	url = cgit_pageurl(cgit_query_repo, page,
			   fmt("%s=%s", arg, sha1_to_hex(obj->sha1)));
	html_link_open(url, NULL, NULL);
	htmlf("%s %s", typename(obj->type),
	      sha1_to_hex(obj->sha1));
	html_link_close();
}

void cgit_print_date(time_t secs, char *format)
{
	char buf[64];
	struct tm *time;

	time = gmtime(&secs);
	strftime(buf, sizeof(buf)-1, format, time);
	html_txt(buf);
}

void cgit_print_age(time_t t, time_t max_relative, char *format)
{
	time_t now, secs;

	time(&now);
	secs = now - t;

	if (secs > max_relative && max_relative >= 0) {
		cgit_print_date(t, format);
		return;
	}

	if (secs < TM_HOUR * 2) {
		htmlf("<span class='age-mins'>%.0f min.</span>",
		      secs * 1.0 / TM_MIN);
		return;
	}
	if (secs < TM_DAY * 2) {
		htmlf("<span class='age-hours'>%.0f hours</span>",
		      secs * 1.0 / TM_HOUR);
		return;
	}
	if (secs < TM_WEEK * 2) {
		htmlf("<span class='age-days'>%.0f days</span>",
		      secs * 1.0 / TM_DAY);
		return;
	}
	if (secs < TM_MONTH * 2) {
		htmlf("<span class='age-weeks'>%.0f weeks</span>",
		      secs * 1.0 / TM_WEEK);
		return;
	}
	if (secs < TM_YEAR * 2) {
		htmlf("<span class='age-months'>%.0f months</span>",
		      secs * 1.0 / TM_MONTH);
		return;
	}
	htmlf("<span class='age-years'>%.0f years</span>",
	      secs * 1.0 / TM_YEAR);
}

void cgit_print_docstart(char *title, struct cacheitem *item)
{
	html("Content-Type: text/html; charset=utf-8\n");
	htmlf("Last-Modified: %s\n", http_date(item->st.st_mtime));
	htmlf("Expires: %s\n", http_date(item->st.st_mtime +
					 ttl_seconds(item->ttl)));
	html("\n");
	html(cgit_doctype);
	html("<html>\n");
	html("<head>\n");
	html("<title>");
	html_txt(title);
	html("</title>\n");
	htmlf("<meta name='generator' content='cgit %s'/>\n", cgit_version);
	html("<link rel='stylesheet' type='text/css' href='");
	html_attr(cgit_css);
	html("'/>\n");
	html("</head>\n");
	html("<body>\n");
}

void cgit_print_docend()
{
	html("</td>\n</tr>\n<table>\n</body>\n</html>\n");
}

int print_branch_option(const char *refname, const unsigned char *sha1,
			int flags, void *cb_data)
{
	char *name = (char *)refname;
	html_option(name, name, cgit_query_head);
	return 0;
}

int print_archive_ref(const char *refname, const unsigned char *sha1,
		       int flags, void *cb_data)
{
	struct tag *tag;
	struct taginfo *info;
	struct object *obj;
	char buf[256], *url;
	unsigned char fileid[20];
	int *header = (int *)cb_data;

	if (prefixcmp(refname, "refs/archives"))
		return 0;
	strncpy(buf, refname+14, sizeof(buf));
	obj = parse_object(sha1);
	if (!obj)
		return 1;
	if (obj->type == OBJ_TAG) {
		tag = lookup_tag(sha1);
		if (!tag || parse_tag(tag) || !(info = cgit_parse_tag(tag)))
			return 0;
		hashcpy(fileid, tag->tagged->sha1);
	} else if (obj->type != OBJ_BLOB) {
		return 0;
	} else {
		hashcpy(fileid, sha1);
	}
	if (!*header) {
		html("<p><h1>download</h1>");
		*header = 1;
	}
	url = cgit_pageurl(cgit_query_repo, "blob",
			   fmt("id=%s&amp;path=%s", sha1_to_hex(fileid),
			       buf));
	html_link_open(url, NULL, "menu");
	html_txt(strlpart(buf, 20));
	html_link_close();
	return 0;
}

void add_hidden_formfields(int incl_head, int incl_search, char *page)
{
	char *url;

	if (!cgit_virtual_root) {
		url = fmt("%s/%s", cgit_query_repo, page);
		if (cgit_query_path)
			url = fmt("%s/%s", url, cgit_query_path);
		html_hidden("url", url);
	}

	if (incl_head && strcmp(cgit_query_head, cgit_repo->defbranch))
		html_hidden("h", cgit_query_head);

	if (cgit_query_sha1)
		html_hidden("id", cgit_query_sha1);
	if (cgit_query_sha2)
		html_hidden("id2", cgit_query_sha2);

	if (incl_search) {
		if (cgit_query_grep)
			html_hidden("qt", cgit_query_grep);
		if (cgit_query_search)
			html_hidden("q", cgit_query_search);
	}
}

void cgit_print_pageheader(char *title, int show_search)
{
	static const char *default_info = "This is cgit, a fast webinterface for git repositories";
	int header = 0;

	html("<div id='sidebar'>\n");
	html("<a href='");
	html_attr(cgit_rooturl());
	htmlf("'><div id='logo'><img src='%s' alt='cgit'/></div></a>\n",
	      cgit_logo);
	html("<div class='infobox'>");
	if (cgit_query_repo) {
		html("<h1>");
		html_txt(strrpart(cgit_repo->name, 20));
		html("</h1>\n");
		html_txt(cgit_repo->desc);
		if (cgit_repo->owner) {
			html("<p>\n<h1>owner</h1>\n");
			html_txt(cgit_repo->owner);
		}
		html("<p>\n<h1>navigate</h1>\n");
		reporevlink(NULL, "summary", NULL, "menu", cgit_query_head,
			    NULL, NULL);
		cgit_log_link("log", NULL, "menu", cgit_query_head, NULL, NULL,
			      0, NULL, NULL);
		cgit_tree_link("tree", NULL, "menu", cgit_query_head,
			       cgit_query_sha1, NULL);
		cgit_commit_link("commit", NULL, "menu", cgit_query_head,
			      cgit_query_sha1);
		cgit_diff_link("diff", NULL, "menu", cgit_query_head,
			       cgit_query_sha1, cgit_query_sha2, NULL);

		for_each_ref(print_archive_ref, &header);

		html("<p>\n<h1>branch</h1>\n");
		html("<form method='get' action=''>\n");
		add_hidden_formfields(0, 1, cgit_query_page);
		html("<table class='grid'><tr><td id='branch-dropdown-cell'>");
		html("<select name='h' onchange='this.form.submit();'>\n");
		for_each_branch_ref(print_branch_option, cgit_query_head);
		html("</select>\n");
		html("</td><td>");
		html("<noscript><input type='submit' id='switch-btn' value='..'></noscript>\n");
		html("</td></tr></table>");
		html("</form>\n");

		html("<p>\n<h1>search</h1>\n");
		html("<form method='get' action='");
		if (cgit_virtual_root)
			html_attr(cgit_fileurl(cgit_query_repo, "log",
					       cgit_query_path, NULL));
		html("'>\n");
		add_hidden_formfields(1, 0, "log");
		html("<select name='qt'>\n");
		html_option("grep", "log msg", cgit_query_grep);
		html_option("author", "author", cgit_query_grep);
		html_option("committer", "committer", cgit_query_grep);
		html("</select>\n");
		html("<input class='txt' type='text' name='q' value='");
		html_attr(cgit_query_search);
		html("'/>\n");
		html("</form>\n");
	} else {
		if (!cgit_index_info || html_include(cgit_index_info))
			html(default_info);
	}

	html("</div>\n");

	html("</div>\n<table class='grid'><tr><td id='content'>\n");
}


void cgit_print_snapshot_start(const char *mimetype, const char *filename,
			       struct cacheitem *item)
{
	htmlf("Content-Type: %s\n", mimetype);
	htmlf("Content-Disposition: inline; filename=\"%s\"\n", filename);
	htmlf("Last-Modified: %s\n", http_date(item->st.st_mtime));
	htmlf("Expires: %s\n", http_date(item->st.st_mtime +
					 ttl_seconds(item->ttl)));
	html("\n");
}

/* vim:set sw=8: */
