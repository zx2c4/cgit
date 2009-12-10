/* ui-tree.c: functions for tree output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include <ctype.h>
#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

char *curr_rev;
char *match_path;
int header = 0;

static void print_text_buffer(const char *name, char *buf, unsigned long size)
{
	unsigned long lineno, idx;
	const char *numberfmt =
		"<a class='no' id='n%1$d' name='n%1$d' href='#n%1$d'>%1$d</a>\n";

	html("<table summary='blob content' class='blob'>\n");

	if (ctx.cfg.enable_tree_linenumbers) {
		html("<tr><td class='linenumbers'><pre>");
		idx = 0;
		lineno = 0;
	
		if (size) {
			htmlf(numberfmt, ++lineno);
			while(idx < size - 1) { // skip absolute last newline
				if (buf[idx] == '\n')
					htmlf(numberfmt, ++lineno);
				idx++;
			}
		}
		html("</pre></td>\n");
	}
	else {
		html("<tr>\n");
	}

	if (ctx.repo->source_filter) {
		html("<td class='lines'><pre><code>");
		ctx.repo->source_filter->argv[1] = xstrdup(name);
		cgit_open_filter(ctx.repo->source_filter);
		write(STDOUT_FILENO, buf, size);
		cgit_close_filter(ctx.repo->source_filter);
		html("</code></pre></td></tr></table>\n");
		return;
	}

	html("<td class='lines'><pre><code>");
	html_txt(buf);
	html("</code></pre></td></tr></table>\n");
}

#define ROWLEN 32

static void print_binary_buffer(char *buf, unsigned long size)
{
	unsigned long ofs, idx;
	static char ascii[ROWLEN + 1];

	html("<table summary='blob content' class='bin-blob'>\n");
	html("<tr><th>ofs</th><th>hex dump</th><th>ascii</th></tr>");
	for (ofs = 0; ofs < size; ofs += ROWLEN, buf += ROWLEN) {
		htmlf("<tr><td class='right'>%04x</td><td class='hex'>", ofs);
		for (idx = 0; idx < ROWLEN && ofs + idx < size; idx++)
			htmlf("%*s%02x",
			      idx == 16 ? 4 : 1, "",
			      buf[idx] & 0xff);
		html(" </td><td class='hex'>");
		for (idx = 0; idx < ROWLEN && ofs + idx < size; idx++)
			ascii[idx] = isgraph(buf[idx]) ? buf[idx] : '.';
		ascii[idx] = '\0';
		html_txt(ascii);
		html("</td></tr>\n");
	}
	html("</table>\n");
}

static void print_object(const unsigned char *sha1, char *path, const char *basename)
{
	enum object_type type;
	char *buf;
	unsigned long size;

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

	html(" (");
	cgit_plain_link("plain", NULL, NULL, ctx.qry.head,
		        curr_rev, path);
	htmlf(")<br/>blob: %s\n", sha1_to_hex(sha1));

	if (buffer_is_binary(buf, size))
		print_binary_buffer(buf, size);
	else
		print_text_buffer(basename, buf, size);
}


static int ls_item(const unsigned char *sha1, const char *base, int baselen,
		   const char *pathname, unsigned int mode, int stage,
		   void *cbdata)
{
	char *name;
	char *fullpath;
	char *class;
	enum object_type type;
	unsigned long size = 0;

	name = xstrdup(pathname);
	fullpath = fmt("%s%s%s", ctx.qry.path ? ctx.qry.path : "",
		       ctx.qry.path ? "/" : "", name);

	if (!S_ISGITLINK(mode)) {
		type = sha1_object_info(sha1, &size);
		if (type == OBJ_BAD) {
			htmlf("<tr><td colspan='3'>Bad object: %s %s</td></tr>",
			      name,
			      sha1_to_hex(sha1));
			return 0;
		}
	}

	html("<tr><td class='ls-mode'>");
	cgit_print_filemode(mode);
	html("</td><td>");
	if (S_ISGITLINK(mode)) {
		htmlf("<a class='ls-mod' href='");
		html_attr(fmt(ctx.repo->module_link,
			      name,
			      sha1_to_hex(sha1)));
		html("'>");
		html_txt(name);
		html("</a>");
	} else if (S_ISDIR(mode)) {
		cgit_tree_link(name, NULL, "ls-dir", ctx.qry.head,
			       curr_rev, fullpath);
	} else {
		class = strrchr(name, '.');
		if (class != NULL) {
			class = fmt("ls-blob %s", class + 1);
		} else
			class = "ls-blob";
		cgit_tree_link(name, NULL, class, ctx.qry.head,
			       curr_rev, fullpath);
	}
	htmlf("</td><td class='ls-size'>%li</td>", size);

	html("<td>");
	cgit_log_link("log", NULL, "button", ctx.qry.head, curr_rev,
		      fullpath, 0, NULL, NULL, ctx.qry.showmsg);
	if (ctx.repo->max_stats)
		cgit_stats_link("stats", NULL, "button", ctx.qry.head,
				fullpath);
	html("</td></tr>\n");
	free(name);
	return 0;
}

static void ls_head()
{
	html("<table summary='tree listing' class='list'>\n");
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
	read_tree_recursive(tree, "", 0, 1, NULL, ls_item, NULL);
	ls_tail();
}


static int walk_tree(const unsigned char *sha1, const char *base, int baselen,
		     const char *pathname, unsigned mode, int stage,
		     void *cbdata)
{
	static int state;
	static char buffer[PATH_MAX];
	char *url;

	if (state == 0) {
		memcpy(buffer, base, baselen);
		strcpy(buffer+baselen, pathname);
		url = cgit_pageurl(ctx.qry.repo, "tree",
				   fmt("h=%s&amp;path=%s", curr_rev, buffer));
		html("/");
		cgit_tree_link(xstrdup(pathname), NULL, NULL, ctx.qry.head,
			       curr_rev, buffer);

		if (strcmp(match_path, buffer))
			return READ_TREE_RECURSIVE;

		if (S_ISDIR(mode)) {
			state = 1;
			ls_head();
			return READ_TREE_RECURSIVE;
		} else {
			print_object(sha1, buffer, pathname);
			return 0;
		}
	}
	ls_item(sha1, base, baselen, pathname, mode, stage, NULL);
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
		rev = ctx.qry.head;

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
	html_attr(cgit_pageurl(ctx.qry.repo, "tree", fmt("h=%s", rev)));
	html("'>root</a>");

	if (path == NULL) {
		ls_tree(commit->tree->object.sha1, NULL);
		return;
	}

	match_path = path;
	read_tree_recursive(commit->tree, "", 0, 0, paths, walk_tree, NULL);
	ls_tail();
}
