/* ui-tree.c: functions for tree output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

char *curr_rev;
char *match_path;
int header = 0;

static void print_object(const unsigned char *sha1, char *path)
{
	enum object_type type;
	unsigned char *buf;
	unsigned long size, lineno, start, idx;

	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		cgit_print_error(fmt("Bad object name: %s",
				     sha1_to_hex(sha1)));
		return;
	}

	buf = read_sha1_file(sha1, &type, &size);
	if (!buf) {
		cgit_print_error(fmt("Error reading object %s",
				     sha1_to_hex(sha1)));
		return;
	}

	html(" blob: <a href='");
	html_attr(cgit_pageurl(cgit_query_repo, "blob", fmt("id=%s", sha1_to_hex(sha1))));
	htmlf("'>%s</a>",sha1_to_hex(sha1));

	html("<table class='blob'>\n");
	idx = 0;
	start = 0;
	lineno = 0;
	while(idx < size) {
		if (buf[idx] == '\n') {
			buf[idx] = '\0';
			htmlf("<tr><td class='no'><a name='%1$d'>%1$d</a></td><td class='txt'>",
			      ++lineno);
			html_txt(buf + start);
			html("</td></tr>\n");
			start = idx + 1;
		}
		idx++;
	}
	html("</table>\n");
}


static int ls_item(const unsigned char *sha1, const char *base, int baselen,
		   const char *pathname, unsigned int mode, int stage)
{
	char *name;
	char *fullpath;
	enum object_type type;
	unsigned long size = 0;

	name = xstrdup(pathname);
	fullpath = fmt("%s%s%s", cgit_query_path ? cgit_query_path : "",
		       cgit_query_path ? "/" : "", name);

	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD && !S_ISGITLINK(mode)) {
		htmlf("<tr><td colspan='3'>Bad object: %s %s</td></tr>",
		      name,
		      sha1_to_hex(sha1));
		return 0;
	}

	html("<tr><td class='ls-mode'>");
	html_filemode(mode);
	html("</td><td>");
	if (S_ISGITLINK(mode)) {
		htmlf("<a class='ls-mod' href='");
		html_attr(fmt(cgit_repo->module_link,
			      name,
			      sha1_to_hex(sha1)));
		html("'>");
		html_txt(name);
		html("</a>");
	} else if (S_ISDIR(mode)) {
		cgit_tree_link(name, NULL, "ls-dir", cgit_query_head,
			       curr_rev, fullpath);
	} else {
		cgit_tree_link(name, NULL, "ls-blob", cgit_query_head,
			       curr_rev, fullpath);
	}
	htmlf("</td><td class='ls-size'>%li</td>", size);

	html("<td>");
	cgit_log_link("log", NULL, "button", cgit_query_head, curr_rev,
		      fullpath, 0);
	html("</td></tr>\n");
	free(name);
	return 0;
}

static void ls_head()
{
	html("<table class='list'>\n");
	html("<tr class='nohover'>");
	html("<th class='left'>Mode</th>");
	html("<th class='left'>Name</th>");
	html("<th class='right'>Size</th>");
	html("<th/>");
	html("</tr>\n");
	header = 1;
}

static void ls_tail()
{
	if (!header)
		return;
	html("</table>\n");
	header = 0;
}

static void ls_tree(const unsigned char *sha1, char *path)
{
	struct tree *tree;

	tree = parse_tree_indirect(sha1);
	if (!tree) {
		cgit_print_error(fmt("Not a tree object: %s",
				     sha1_to_hex(sha1)));
		return;
	}

	ls_head();
	read_tree_recursive(tree, "", 0, 1, NULL, ls_item);
	ls_tail();
}


static int walk_tree(const unsigned char *sha1, const char *base, int baselen,
		     const char *pathname, unsigned mode, int stage)
{
	static int state;
	static char buffer[PATH_MAX];
	char *url;

	if (state == 0) {
		memcpy(buffer, base, baselen);
		strcpy(buffer+baselen, pathname);
		url = cgit_pageurl(cgit_query_repo, "tree",
				   fmt("h=%s&amp;path=%s", curr_rev, buffer));
		html("/");
		cgit_tree_link(xstrdup(pathname), NULL, NULL, cgit_query_head,
			       curr_rev, buffer);

		if (strcmp(match_path, buffer))
			return READ_TREE_RECURSIVE;

		if (S_ISDIR(mode)) {
			state = 1;
			ls_head();
			return READ_TREE_RECURSIVE;
		} else {
			print_object(sha1, buffer);
			return 0;
		}
	}
	ls_item(sha1, base, baselen, pathname, mode, stage);
	return 0;
}


/*
 * Show a tree or a blob
 *   rev:  the commit pointing at the root tree object
 *   path: path to tree or blob
 */
void cgit_print_tree(const char *rev, char *path)
{
	unsigned char sha1[20];
	struct commit *commit;
	const char *paths[] = {path, NULL};

	if (!rev)
		rev = cgit_query_head;

	curr_rev = xstrdup(rev);
	if (get_sha1(rev, sha1)) {
		cgit_print_error(fmt("Invalid revision name: %s", rev));
		return;
	}
	commit = lookup_commit_reference(sha1);
	if (!commit || parse_commit(commit)) {
		cgit_print_error(fmt("Invalid commit reference: %s", rev));
		return;
	}

	html("path: <a href='");
	html_attr(cgit_pageurl(cgit_query_repo, "tree", fmt("h=%s", rev)));
	html("'>root</a>");

	if (path == NULL) {
		ls_tree(commit->tree->object.sha1, NULL);
		return;
	}

	match_path = path;
	read_tree_recursive(commit->tree, NULL, 0, 0, paths, walk_tree);
	ls_tail();
}
