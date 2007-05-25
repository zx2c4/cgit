/* ui-tree.c: functions for tree output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

char *curr_rev;

static int print_entry(const unsigned char *sha1, const char *base,
		       int baselen, const char *pathname, unsigned int mode,
		       int stage)
{
	char *name;
	enum object_type type;
	unsigned long size = 0;

	name = xstrdup(pathname);
	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD && !S_ISDIRLNK(mode)) {
		htmlf("<tr><td colspan='3'>Bad object: %s %s</td></tr>",
		      name,
		      sha1_to_hex(sha1));
		return 0;
	}
	html("<tr><td class='filemode'>");
	html_filemode(mode);
	html("</td><td ");
	if (S_ISDIRLNK(mode)) {
		htmlf("class='ls-mod'><a href='");
		html_attr(fmt(cgit_repo->module_link,
			      name,
			      sha1_to_hex(sha1)));
	} else if (S_ISDIR(mode)) {
		html("class='ls-dir'><a href='");
		html_attr(cgit_pageurl(cgit_query_repo, "tree",
				       fmt("h=%s&id=%s&path=%s%s/",
					   curr_rev,
					   sha1_to_hex(sha1),
					   cgit_query_path ? cgit_query_path : "",
					   pathname)));
	} else {
		html("class='ls-blob'><a href='");
		html_attr(cgit_pageurl(cgit_query_repo, "view",
				      fmt("h=%s&id=%s&path=%s%s", curr_rev,
					  sha1_to_hex(sha1),
					  cgit_query_path ? cgit_query_path : "",
					  pathname)));
	}
	htmlf("'>%s</a></td>", name);
	htmlf("<td class='filesize'>%li</td>", size);

	html("<td class='links'><a href='");
	html_attr(cgit_pageurl(cgit_query_repo, "log",
			       fmt("h=%s&path=%s%s",
				   curr_rev,
				   cgit_query_path ? cgit_query_path : "",
				   pathname)));
	html("'>history</a></td>");
	html("</tr>\n");
	free(name);
	return 0;
}

void cgit_print_tree(const char *rev, const char *hex, char *path)
{
	struct tree *tree;
	unsigned char sha1[20];
	struct commit *commit;

	curr_rev = xstrdup(rev);
	get_sha1(rev, sha1);
	commit = lookup_commit_reference(sha1);
	if (!commit || parse_commit(commit)) {
		cgit_print_error(fmt("Invalid head: %s", rev));
		return;
	}
	if (!hex)
		hex = sha1_to_hex(commit->tree->object.sha1);

	if (get_sha1_hex(hex, sha1)) {
		cgit_print_error(fmt("Invalid object id: %s", hex));
		return;
	}
	tree = parse_tree_indirect(sha1);
	if (!tree) {
		cgit_print_error(fmt("Not a tree object: %s", hex));
		return;
	}

	html_txt(path);
	html("<table class='list'>\n");
	html("<tr class='nohover'>");
	html("<th class='left'>Mode</th>");
	html("<th class='left'>Name</th>");
	html("<th class='right'>Size</th>");
	html("<th/>");
	html("</tr>\n");
	read_tree_recursive(tree, "", 0, 1, NULL, print_entry);
	html("</table>\n");
}
