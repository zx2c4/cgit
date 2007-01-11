/* ui-tree.c: functions for tree output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"


static int print_entry(const unsigned char *sha1, const char *base, 
		       int baselen, const char *pathname, unsigned int mode, 
		       int stage)
{
	char *name;
	char type[20];
	unsigned long size;

	if (sha1_object_info(sha1, type, &size)) {
		cgit_print_error(fmt("Bad object name: %s", 
				     sha1_to_hex(sha1)));
		return 0;
	}
	name = xstrdup(pathname);
	html("<tr><td class='filemode'>");
	html_filemode(mode);
	html("</td><td>");
	if (S_ISDIR(mode)) {
		html("<div class='ls-dir'><a href='");
		html_attr(cgit_pageurl(cgit_query_repo, "tree", 
				       fmt("id=%s&path=%s%s/", 
					   sha1_to_hex(sha1),
					   cgit_query_path ? cgit_query_path : "",
					   pathname)));
	} else {
		html("<div class='ls-blob'><a href='");
		html_attr(cgit_pageurl(cgit_query_repo, "view",
				      fmt("id=%s&path=%s%s", sha1_to_hex(sha1),
					  cgit_query_path ? cgit_query_path : "",
					  pathname)));
	}
	html("'>");
	html_txt(name);
	if (S_ISDIR(mode))
		html("/");
	html("</a></div></td>");
	htmlf("<td class='filesize'>%li</td>", size);
	html("</tr>\n");
	free(name);
	return 0;
}

void cgit_print_tree(const char *hex, char *path)
{
	struct tree *tree;
	unsigned char sha1[20];

	if (get_sha1_hex(hex, sha1)) {
		cgit_print_error(fmt("Invalid object id: %s", hex));
		return;
	}
	tree = parse_tree_indirect(sha1);
	if (!tree) {
		cgit_print_error(fmt("Not a tree object: %s", hex));
		return;
	}

	html("<h2>Tree content</h2>\n");
	html_txt(path);
	html("<table class='list'>\n");
	html("<tr>");
	html("<th class='left'>Mode</th>");
	html("<th class='left'>Name</th>");
	html("<th class='right'>Size</th>");
	html("</tr>\n");
	read_tree_recursive(tree, "", 0, 1, NULL, print_entry);
	html("</table>\n");
}
