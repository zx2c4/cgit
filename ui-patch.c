/* ui-patch.c: generate patch view
 *
 * Copyright (C) 2007 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-patch.h"
#include "html.h"
#include "ui-shared.h"

void cgit_print_patch(char *hex, const char *prefix)
{
	struct commit *commit;
	struct commitinfo *info;
	unsigned char sha1[20], old_sha1[20];
	char *patchname;

	if (!hex)
		hex = ctx.qry.head;

	if (get_sha1(hex, sha1)) {
		cgit_print_error("Bad object id: %s", hex);
		return;
	}
	commit = lookup_commit_reference(sha1);
	if (!commit) {
		cgit_print_error("Bad commit reference: %s", hex);
		return;
	}
	info = cgit_parse_commit(commit);

	if (commit->parents && commit->parents->item)
		hashcpy(old_sha1, commit->parents->item->object.sha1);
	else
		hashclr(old_sha1);

	patchname = fmt("%s.patch", sha1_to_hex(sha1));
	ctx.page.mimetype = "text/plain";
	ctx.page.filename = patchname;
	cgit_print_http_headers(&ctx);
	htmlf("From %s Mon Sep 17 00:00:00 2001\n", sha1_to_hex(sha1));
	htmlf("From: %s", info->author);
	if (!ctx.cfg.noplainemail) {
		htmlf(" %s", info->author_email);
	}
	html("\n");
	html("Date: ");
	cgit_print_date(info->author_date, "%a, %d %b %Y %H:%M:%S %z%n", ctx.cfg.local_time);
	htmlf("Subject: %s\n\n", info->subject);
	if (info->msg && *info->msg) {
		htmlf("%s", info->msg);
		if (info->msg[strlen(info->msg) - 1] != '\n')
			html("\n");
	}
	html("---\n");
	if (prefix)
		htmlf("(limited to '%s')\n\n", prefix);
	cgit_diff_tree(old_sha1, sha1, filepair_cb_raw, prefix, 0);
	html("--\n");
	htmlf("cgit %s\n", cgit_version);
	cgit_free_commitinfo(info);
}
