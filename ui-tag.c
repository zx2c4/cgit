/* ui-tag.c: display a tag
 *
 * Copyright (C) 2007 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

static void print_tag_content(char *buf)
{
	char *p;

	if (!buf)
		return;

	html("<div class='commit-subject'>");
	p = strchr(buf, '\n');
	if (p)
		*p = '\0';
	html_txt(buf);
	html("</div>");
	if (p) {
		html("<div class='commit-msg'>");
		html_txt(++p);
		html("</div>");
	}
}

void cgit_print_tag(char *revname)
{
	unsigned char sha1[20];
	struct object *obj;
	struct tag *tag;
	struct taginfo *info;

	if (!revname)
		revname = ctx.qry.head;

	if (get_sha1(fmt("refs/tags/%s", revname), sha1)) {
		cgit_print_error(fmt("Bad tag reference: %s", revname));
		return;
	}
	obj = parse_object(sha1);
	if (!obj) {
		cgit_print_error(fmt("Bad object id: %s", sha1_to_hex(sha1)));
		return;
	}
	if (obj->type == OBJ_TAG) {
		tag = lookup_tag(sha1);
		if (!tag || parse_tag(tag) || !(info = cgit_parse_tag(tag))) {
			cgit_print_error(fmt("Bad tag object: %s", revname));
			return;
		}
		html("<table class='commit-info'>\n");
		htmlf("<tr><td>Tag name</td><td>");
		html_txt(revname);
		htmlf(" (%s)</td></tr>\n", sha1_to_hex(sha1));
		if (info->tagger_date > 0) {
			html("<tr><td>Tag date</td><td>");
			cgit_print_date(info->tagger_date, FMT_LONGDATE, ctx.cfg.local_time);
			html("</td></tr>\n");
		}
		if (info->tagger) {
			html("<tr><td>Tagged by</td><td>");
			html_txt(info->tagger);
			if (info->tagger_email) {
				html(" ");
				html_txt(info->tagger_email);
			}
			html("</td></tr>\n");
		}
		html("<tr><td>Tagged object</td><td>");
		cgit_object_link(tag->tagged);
		html("</td></tr>\n");
		html("</table>\n");
		print_tag_content(info->msg);
	} else {
		html("<table class='commit-info'>\n");
		htmlf("<tr><td>Tag name</td><td>");
		html_txt(revname);
		html("</td></tr>\n");
		html("<tr><td>Tagged object</td><td>");
		cgit_object_link(obj);
		html("</td></tr>\n");
		html("</table>\n");
        }
	return;
}
