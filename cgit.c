/* cgit.c: cgi for the git scm
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

const char cgit_version[] = CGIT_VERSION;

const char cgit_doctype[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";

const char cgit_error[] =
"<div class='error'>%s</div>";

const char cgit_lib_error[] =
"<div class='error'>%s: %s</div>";

int htmlfd = 0;

char *cgit_root         = "/usr/src/git";
char *cgit_root_title   = "Git repository browser";
char *cgit_css          = "/cgit.css";
char *cgit_logo         = "/git-logo.png";
char *cgit_logo_link    = "http://www.kernel.org/pub/software/scm/git/docs/";
char *cgit_virtual_root = NULL;

char *cgit_cache_root   = "/var/cache/cgit";

int cgit_cache_root_ttl        =  5;
int cgit_cache_repo_ttl        =  5;
int cgit_cache_dynamic_ttl     =  5;
int cgit_cache_static_ttl      = -1;
int cgit_cache_max_create_time =  5;

char *cgit_repo_name    = NULL;
char *cgit_repo_desc    = NULL;
char *cgit_repo_owner   = NULL;

int cgit_query_has_symref = 0;
int cgit_query_has_sha1   = 0;

char *cgit_querystring  = NULL;
char *cgit_query_repo   = NULL;
char *cgit_query_page   = NULL;
char *cgit_query_head   = NULL;
char *cgit_query_sha1   = NULL;

struct cacheitem cacheitem;

int cgit_parse_query(char *txt, configfn fn)
{
	char *t, *value = NULL, c;

	if (!txt)
		return 0;

	t = txt = xstrdup(txt);
 
	while((c=*t) != '\0') {
		if (c=='=') {
			*t = '\0';
			value = t+1;
		} else if (c=='&') {
			*t = '\0';
			(*fn)(txt, value);
			txt = t+1;
			value = NULL;
		}
		t++;
	}
	if (t!=txt)
		(*fn)(txt, value);
	return 0;
}

void cgit_global_config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "root"))
		cgit_root = xstrdup(value);
	else if (!strcmp(name, "root-title"))
		cgit_root_title = xstrdup(value);
	else if (!strcmp(name, "css"))
		cgit_css = xstrdup(value);
	else if (!strcmp(name, "logo"))
		cgit_logo = xstrdup(value);
	else if (!strcmp(name, "logo-link"))
		cgit_logo_link = xstrdup(value);
	else if (!strcmp(name, "virtual-root"))
		cgit_virtual_root = xstrdup(value);
}

void cgit_repo_config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "name"))
		cgit_repo_name = xstrdup(value);
	else if (!strcmp(name, "desc"))
		cgit_repo_desc = xstrdup(value);
	else if (!strcmp(name, "owner"))
		cgit_repo_owner = xstrdup(value);
}

void cgit_querystring_cb(const char *name, const char *value)
{
	if (!strcmp(name,"r"))
		cgit_query_repo = xstrdup(value);
	else if (!strcmp(name, "p"))
		cgit_query_page = xstrdup(value);
	else if (!strcmp(name, "h")) {
		cgit_query_head = xstrdup(value);
		cgit_query_has_symref = 1;
	} else if (!strcmp(name, "id")) {
		cgit_query_sha1 = xstrdup(value);
		cgit_query_has_sha1 = 1;
	}
}

char *cgit_repourl(const char *reponame)
{
	if (cgit_virtual_root) {
		return fmt("%s/%s/", cgit_virtual_root, reponame);
	} else {
		return fmt("?r=%s", reponame);
	}
}

char *cgit_pageurl(const char *reponame, const char *pagename, 
		   const char *query)
{
	if (cgit_virtual_root) {
		return fmt("%s/%s/%s/?%s", cgit_virtual_root, reponame, 
			   pagename, query);
	} else {
		return fmt("?r=%s&p=%s&%s", reponame, pagename, query);
	}
}

static int cgit_print_branch_cb(const char *refname, const unsigned char *sha1,
				int flags, void *cb_data)
{
	struct commit *commit;
	char buf[256], *url;

	commit = lookup_commit(sha1);
	if (commit && !parse_commit(commit)){
		html("<tr><td>");
		url = cgit_pageurl(cgit_query_repo, "log", 
				   fmt("h=%s", refname));
		html_link_open(url, NULL, NULL);
		strncpy(buf, refname, sizeof(buf));
		html_txt(buf);
		html_link_close();
		html("</td><td>");
		pretty_print_commit(CMIT_FMT_ONELINE, commit, ~0, buf,
				    sizeof(buf), 0, NULL, NULL, 0);
		html_txt(buf);
		html("</td></tr>\n");
	} else {
		html("<tr><td>");
		html_txt(buf);
		html("</td><td>");
		htmlf("*** bad ref %s", sha1_to_hex(sha1));
		html("</td></tr>\n");
	}
	return 0;
}

/* Sun, 06 Nov 1994 08:49:37 GMT */
static char *http_date(time_t t)
{
	static char day[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	static char month[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				  "Jul", "Aug", "Sep", "Oct", "Now", "Dec"};
	struct tm *tm = gmtime(&t);
	return fmt("%s, %02d %s %04d %02d:%02d:%02d GMT", day[tm->tm_wday],
		   tm->tm_mday, month[tm->tm_mon], 1900+tm->tm_year,
		   tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static int ttl_seconds(int ttl)
{
	if (ttl<0)
		return 60 * 60 * 24 * 365;
	else 
		return ttl * 60;
}

static void cgit_print_docstart(char *title)
{
	html("Content-Type: text/html; charset=utf-8\n");
	htmlf("Last-Modified: %s\n", http_date(cacheitem.st.st_mtime));
	htmlf("Expires: %s\n", http_date(cacheitem.st.st_mtime + 
					 ttl_seconds(cacheitem.ttl)));
	html("\n");
	html(cgit_doctype);
	html("<html>\n");
	html("<head>\n");
	html("<title>");
	html_txt(title);
	html("</title>\n");
	htmlf("<meta name='generator' content='cgit v%s'/>\n", cgit_version);
	html("<link rel='stylesheet' type='text/css' href='");
	html_attr(cgit_css);
	html("'/>\n");
	html("</head>\n");
	html("<body>\n");
}

static void cgit_print_docend()
{
	html("</body>\n</html>\n");
}

static void cgit_print_pageheader(char *title)
{
	html("<div id='header'>");
	htmlf("<a href='%s'>", cgit_logo_link);
	htmlf("<img id='logo' src='%s'/>\n", cgit_logo);
	htmlf("</a>");
	html_txt(title);
	html("</div>");
}

static void cgit_print_repolist()
{
	DIR *d;
	struct dirent *de;
	struct stat st;
	char *name;

	chdir(cgit_root);
	cgit_print_docstart(cgit_root_title);
	cgit_print_pageheader(cgit_root_title);

	if (!(d = opendir("."))) {
		htmlf(cgit_lib_error, "Unable to scan repository directory",
		      strerror(errno));
		cgit_print_docend();
		return;
	}

	html("<h2>Repositories</h2>\n");
	html("<table class='list'>");
	html("<tr><th>Name</th><th>Description</th><th>Owner</th></tr>\n");
	while ((de = readdir(d)) != NULL) {
		if (de->d_name[0] == '.')
			continue;
		if (stat(de->d_name, &st) < 0)
			continue;
		if (!S_ISDIR(st.st_mode))
			continue;

		cgit_repo_name = cgit_repo_desc = cgit_repo_owner = NULL;
		name = fmt("%s/info/cgit", de->d_name);
		if (cgit_read_config(name, cgit_repo_config_cb))
			continue;

		html("<tr><td>");
		html_link_open(cgit_repourl(de->d_name), NULL, NULL);
		html_txt(cgit_repo_name);
		html_link_close();
		html("</td><td>");
		html_txt(cgit_repo_desc);
		html("</td><td>");
		html_txt(cgit_repo_owner);
		html("</td></tr>\n");
	}
	closedir(d);
	html("</table>");
	cgit_print_docend();
}

static void cgit_print_branches()
{
	html("<table class='list'>");
	html("<tr><th>Branch name</th><th>Head commit</th></tr>\n");
	for_each_branch_ref(cgit_print_branch_cb, NULL);
	html("</table>");
}

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

static void cgit_print_log(const char *tip, int ofs, int cnt)
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

static void cgit_print_repo_summary()
{
	html("<h2>");
	html_txt("Repo summary page");
	html("</h2>");
	cgit_print_branches();
}

static void cgit_print_object(char *hex)
{
	unsigned char sha1[20];
	//struct object *object;
	char type[20];
	unsigned char *buf;
	unsigned long size;

	if (get_sha1_hex(hex, sha1)){
		htmlf(cgit_error, "Bad hex value");
	        return;
	}

	if (sha1_object_info(sha1, type, NULL)){
		htmlf(cgit_error, "Bad object name");
		return;
	}

	buf = read_sha1_file(sha1, type, &size);
	if (!buf) {
		htmlf(cgit_error, "Error reading object");
		return;
	}

	buf[size] = '\0';
	html("<h2>Object view</h2>");
	htmlf("sha1=%s<br/>type=%s<br/>size=%i<br/>", hex, type, size);
	html("<pre>");
	html_txt(buf);
	html("</pre>");
}

static void cgit_print_repo_page()
{
	if (chdir(fmt("%s/%s", cgit_root, cgit_query_repo)) || 
	    cgit_read_config("info/cgit", cgit_repo_config_cb)) {
		char *title = fmt("%s - %s", cgit_root_title, "Bad request");
		cgit_print_docstart(title);
		cgit_print_pageheader(title);
		htmlf(cgit_lib_error, "Unable to scan repository",
		      strerror(errno));
		cgit_print_docend();
		return;
	}
	setenv("GIT_DIR", fmt("%s/%s", cgit_root, cgit_query_repo), 1);
	char *title = fmt("%s - %s", cgit_repo_name, cgit_repo_desc);
	cgit_print_docstart(title);
	cgit_print_pageheader(title);
	if (!cgit_query_page)
		cgit_print_repo_summary();
	else if (!strcmp(cgit_query_page, "log")) {
		cgit_print_log(cgit_query_head, 0, 100);
	} else if (!strcmp(cgit_query_page, "view")) {
		cgit_print_object(cgit_query_sha1);
	}
	cgit_print_docend();
}

static void cgit_fill_cache(struct cacheitem *item)
{
	htmlfd = item->fd;
	item->st.st_mtime = time(NULL);
	if (cgit_query_repo)
		cgit_print_repo_page();
	else
		cgit_print_repolist();
}

static void cgit_refresh_cache(struct cacheitem *item)
{
 top:
	if (!cache_lookup(item)) {
		if (cache_lock(item)) {
			cgit_fill_cache(item);
			cache_unlock(item);
		} else {
			sched_yield();
			goto top;
		}
	} else if (cache_expired(item)) {
		if (cache_lock(item)) {
			cgit_fill_cache(item);
			cache_unlock(item);
		}
	}
}

static void cgit_print_cache(struct cacheitem *item)
{
	static char buf[4096];
	ssize_t i;

	int fd = open(item->name, O_RDONLY);
	if (fd<0)
		die("Unable to open cached file %s", item->name);

	while((i=read(fd, buf, sizeof(buf))) > 0)
		write(STDOUT_FILENO, buf, i);

	close(fd);
}

int main(int argc, const char **argv)
{
	cgit_read_config("/etc/cgitrc", cgit_global_config_cb);
	cgit_querystring = xstrdup(getenv("QUERY_STRING"));
	cgit_parse_query(cgit_querystring, cgit_querystring_cb);
	cgit_refresh_cache(&cacheitem);
	cgit_print_cache(&cacheitem);
	return 0;
}
