/* ui-refs.c: browse symbolic refs
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-refs.h"
#include "html.h"
#include "ui-shared.h"

static inline int cmp_age(int age1, int age2)
{
	/* age1 and age2 are assumed to be non-negative */
	return age2 - age1;
}

static int cmp_ref_name(const void *a, const void *b)
{
	struct refinfo *r1 = *(struct refinfo **)a;
	struct refinfo *r2 = *(struct refinfo **)b;

	return strcmp(r1->refname, r2->refname);
}

static int cmp_branch_age(const void *a, const void *b)
{
	struct refinfo *r1 = *(struct refinfo **)a;
	struct refinfo *r2 = *(struct refinfo **)b;

	return cmp_age(r1->commit->committer_date, r2->commit->committer_date);
}

static int get_ref_age(struct refinfo *ref)
{
	if (!ref->object)
		return 0;
	switch (ref->object->type) {
	case OBJ_TAG:
		return ref->tag ? ref->tag->tagger_date : 0;
	case OBJ_COMMIT:
		return ref->commit ? ref->commit->committer_date : 0;
	}
	return 0;
}

static int cmp_tag_age(const void *a, const void *b)
{
	struct refinfo *r1 = *(struct refinfo **)a;
	struct refinfo *r2 = *(struct refinfo **)b;

	return cmp_age(get_ref_age(r1), get_ref_age(r2));
}

static int print_branch(struct refinfo *ref)
{
	struct commitinfo *info = ref->commit;
	char *name = (char *)ref->refname;

	if (!info)
		return 1;
	html("<tr><td>");
	cgit_log_link(name, NULL, NULL, name, NULL, NULL, 0, NULL, NULL,
		      ctx.qry.showmsg, 0);
	html("</td><td>");

	if (ref->object->type == OBJ_COMMIT) {
		cgit_commit_link(info->subject, NULL, NULL, name, NULL, NULL);
		html("</td><td>");
		cgit_open_filter(ctx.repo->email_filter, info->author_email, "refs");
		html_txt(info->author);
		cgit_close_filter(ctx.repo->email_filter);
		html("</td><td colspan='2'>");
		cgit_print_age(info->committer_date, info->committer_tz, -1);
	} else {
		html("</td><td></td><td>");
		cgit_object_link(ref->object);
	}
	html("</td></tr>\n");
	return 0;
}

static void print_tag_header(void)
{
	html("<tr class='nohover'><th class='left'>Tag</th>"
	     "<th class='left'>Download</th>"
	     "<th class='left'>Author</th>"
	     "<th class='left' colspan='2'>Age</th></tr>\n");
}

static int print_tag(struct refinfo *ref)
{
	struct tag *tag = NULL;
	struct taginfo *info = NULL;
	char *name = (char *)ref->refname;
	struct object *obj = ref->object;

	if (obj->type == OBJ_TAG) {
		tag = (struct tag *)obj;
		obj = tag->tagged;
		info = ref->tag;
		if (!info)
			return 1;
	}

	html("<tr><td>");
	cgit_tag_link(name, NULL, NULL, name);
	html("</td><td>");
	if (ctx.repo->snapshots && (obj->type == OBJ_COMMIT))
		cgit_print_snapshot_links(ctx.repo, name, "&nbsp;&nbsp;");
	else
		cgit_object_link(obj);
	html("</td><td>");
	if (info) {
		if (info->tagger) {
			cgit_open_filter(ctx.repo->email_filter, info->tagger_email, "refs");
			html_txt(info->tagger);
			cgit_close_filter(ctx.repo->email_filter);
		}
	} else if (ref->object->type == OBJ_COMMIT) {
		cgit_open_filter(ctx.repo->email_filter, ref->commit->author_email, "refs");
		html_txt(ref->commit->author);
		cgit_close_filter(ctx.repo->email_filter);
	}
	html("</td><td colspan='2'>");
	if (info) {
		if (info->tagger_date > 0)
			cgit_print_age(info->tagger_date, info->tagger_tz, -1);
	} else if (ref->object->type == OBJ_COMMIT) {
		cgit_print_age(ref->commit->commit->date, 0, -1);
	}
	html("</td></tr>\n");

	return 0;
}

static void print_refs_link(const char *path)
{
	html("<tr class='nohover'><td colspan='5'>");
	cgit_refs_link("[...]", NULL, NULL, ctx.qry.head, NULL, path);
	html("</td></tr>");
}

void cgit_print_branches(int maxcount)
{
	struct reflist list;
	int i;

	html("<tr class='nohover'><th class='left'>Branch</th>"
	     "<th class='left'>Commit message</th>"
	     "<th class='left'>Author</th>"
	     "<th class='left' colspan='2'>Age</th></tr>\n");

	list.refs = NULL;
	list.alloc = list.count = 0;
	for_each_branch_ref(cgit_refs_cb, &list);
	if (ctx.repo->enable_remote_branches)
		for_each_remote_ref(cgit_refs_cb, &list);

	if (maxcount == 0 || maxcount > list.count)
		maxcount = list.count;

	qsort(list.refs, list.count, sizeof(*list.refs), cmp_branch_age);
	if (ctx.repo->branch_sort == 0)
		qsort(list.refs, maxcount, sizeof(*list.refs), cmp_ref_name);

	for (i = 0; i < maxcount; i++)
		print_branch(list.refs[i]);

	if (maxcount < list.count)
		print_refs_link("heads");

	cgit_free_reflist_inner(&list);
}

void cgit_print_tags(int maxcount)
{
	struct reflist list;
	int i;

	list.refs = NULL;
	list.alloc = list.count = 0;
	for_each_tag_ref(cgit_refs_cb, &list);
	if (list.count == 0)
		return;
	qsort(list.refs, list.count, sizeof(*list.refs), cmp_tag_age);
	if (!maxcount)
		maxcount = list.count;
	else if (maxcount > list.count)
		maxcount = list.count;
	print_tag_header();
	for (i = 0; i < maxcount; i++)
		print_tag(list.refs[i]);

	if (maxcount < list.count)
		print_refs_link("tags");

	cgit_free_reflist_inner(&list);
}

void cgit_print_refs(void)
{
	cgit_print_layout_start();
	html("<table class='list nowrap'>");

	if (ctx.qry.path && starts_with(ctx.qry.path, "heads"))
		cgit_print_branches(0);
	else if (ctx.qry.path && starts_with(ctx.qry.path, "tags"))
		cgit_print_tags(0);
	else {
		cgit_print_branches(0);
		html("<tr class='nohover'><td colspan='5'>&nbsp;</td></tr>");
		cgit_print_tags(0);
	}
	html("</table>");
	cgit_print_layout_end();
}
