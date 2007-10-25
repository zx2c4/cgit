/* ui-summary.c: functions for generating repo summary page
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

static int header;

static int cmp_tag_age(void *a, void *b)
{
	struct refinfo *r1 = *(struct refinfo **)a;
	struct refinfo *r2 = *(struct refinfo **)b;

	if (r1->tag->tagger_date != 0 && r2->tag->tagger_date != 0)
		return r2->tag->tagger_date - r1->tag->tagger_date;

	if (r1->tag->tagger_date == 0 && r2->tag->tagger_date == 0)
		return 0;

	if (r1 == 0)
		return +1;

	return -1;
}

static void cgit_print_branch(struct refinfo *ref)
{
	struct commit *commit;
	struct commitinfo *info;
	char *name = (char *)ref->refname;

	commit = lookup_commit(ref->object->sha1);
	// object is not really parsed at this point, because of some fallout
	// from previous calls to git functions in cgit_print_log()
	commit->object.parsed = 0;
	if (commit && !parse_commit(commit)){
		info = cgit_parse_commit(commit);
		html("<tr><td>");
		cgit_log_link(name, NULL, NULL, name, NULL, NULL, 0);
		html("</td><td>");
		cgit_print_age(commit->date, -1, NULL);
		html("</td><td>");
		html_txt(info->author);
		html("</td><td>");
		cgit_commit_link(info->subject, NULL, NULL, name, NULL);
		html("</td></tr>\n");
		cgit_free_commitinfo(info);
	} else {
		html("<tr><td>");
		html_txt(name);
		html("</td><td colspan='3'>");
		htmlf("*** bad ref %s ***", sha1_to_hex(ref->object->sha1));
		html("</td></tr>\n");
	}
}

static void print_tag_header()
{
	html("<tr class='nohover'><th class='left'>Tag</th>"
	     "<th class='left'>Age</th>"
	     "<th class='left'>Author</th>"
	     "<th class='left'>Reference</th></tr>\n");
	header = 1;
}

static int print_tag(struct refinfo *ref)
{
	struct tag *tag;
	struct taginfo *info;
	char *url, *name = (char *)ref->refname;

	if (ref->object->type == OBJ_TAG) {
		tag = lookup_tag(ref->object->sha1);
		if (!tag || parse_tag(tag) || !(info = cgit_parse_tag(tag)))
			return 2;
		html("<tr><td>");
		url = cgit_pageurl(cgit_query_repo, "tag",
				   fmt("id=%s", name));
		html_link_open(url, NULL, NULL);
		html_txt(name);
		html_link_close();
		html("</td><td>");
		if (info->tagger_date > 0)
			cgit_print_age(info->tagger_date, -1, NULL);
		html("</td><td>");
		if (info->tagger)
			html(info->tagger);
		html("</td><td>");
		cgit_object_link(tag->tagged);
		html("</td></tr>\n");
	} else {
		if (!header)
			print_tag_header();
		html("<tr><td>");
		html_txt(name);
		html("</td><td colspan='2'/><td>");
		cgit_object_link(ref->object);
		html("</td></tr>\n");
	}
	return 0;
}

static int cgit_print_archive_cb(const char *refname, const unsigned char *sha1,
				 int flags, void *cb_data)
{
	struct tag *tag;
	struct taginfo *info;
	struct object *obj;
	char buf[256], *url;
	unsigned char fileid[20];

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
	if (!header) {
		html("<table id='downloads'>");
		html("<tr><th>Downloads</th></tr>");
		header = 1;
	}
	html("<tr><td>");
	url = cgit_pageurl(cgit_query_repo, "blob",
			   fmt("id=%s&amp;path=%s", sha1_to_hex(fileid),
			       buf));
	html_link_open(url, NULL, NULL);
	html_txt(buf);
	html_link_close();
	html("</td></tr>");
	return 0;
}

static void cgit_print_branches()
{
	struct reflist list;
	int i;

	html("<tr class='nohover'><th class='left'>Branch</th>"
	     "<th class='left'>Idle</th>"
	     "<th class='left'>Author</th>"
	     "<th class='left'>Head commit</th></tr>\n");

	list.refs = NULL;
	list.alloc = list.count = 0;
	for_each_branch_ref(cgit_refs_cb, &list);
	for(i=0; i<list.count; i++)
		cgit_print_branch(list.refs[i]);
}

static void cgit_print_tags()
{
	struct reflist list;
	int i;

	header = 0;
	list.refs = NULL;
	list.alloc = list.count = 0;
	for_each_tag_ref(cgit_refs_cb, &list);
	if (list.count == 0)
		return;
	qsort(list.refs, list.count, sizeof(*list.refs), cmp_tag_age);
	print_tag_header();
	for(i=0; i<list.count; i++)
		print_tag(list.refs[i]);
}

static void cgit_print_archives()
{
	header = 0;
	for_each_ref(cgit_print_archive_cb, NULL);
	if (header)
		html("</table>");
}

void cgit_print_summary()
{
	html("<div id='summary'>");
	cgit_print_archives();
	html("<h2>");
	html_txt(cgit_repo->name);
	html(" - ");
	html_txt(cgit_repo->desc);
	html("</h2>");
	if (cgit_repo->readme)
		html_include(cgit_repo->readme);
	html("</div>");
	if (cgit_summary_log > 0)
		cgit_print_log(cgit_query_head, 0, cgit_summary_log, NULL, NULL, 0);
	html("<table class='list nowrap'>");
	if (cgit_summary_log > 0)
		html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
	cgit_print_branches();
	html("<tr class='nohover'><td colspan='4'>&nbsp;</td></tr>");
	cgit_print_tags();
	html("</table>");
}
