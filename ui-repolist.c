/* ui-repolist.c: functions for generating the repolist page
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

void cgit_print_repolist(struct cacheitem *item)
{
	DIR *d;
	struct dirent *de;
	struct stat st;
	char *name;

	chdir(cgit_root);
	cgit_print_docstart(cgit_root_title, item);
	cgit_print_pageheader(cgit_root_title);

	if (!(d = opendir("."))) {
		cgit_print_error(fmt("Unable to scan repository directory: %s",
				     strerror(errno)));
		cgit_print_docend();
		return;
	}

	html("<h2>Repositories</h2>\n");
	html("<table class='list nowrap'>");
	html("<tr>"
	     "<th class='left'>Name</th>"
	     "<th class='left'>Description</th>"
	     "<th class='left'>Owner</th></tr>\n");
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


