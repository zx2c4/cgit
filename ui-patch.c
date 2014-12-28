/* ui-patch.c: generate patch view
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-patch.h"
#include "html.h"
#include "ui-shared.h"

void cgit_print_patch(const char *new_rev, const char *old_rev,
		      const char *prefix)
{
	struct rev_info rev;
	struct commit *commit;
	unsigned char new_rev_sha1[20], old_rev_sha1[20];
	char rev_range[2 * 40 + 3];
	char *rev_argv[] = { NULL, "--reverse", "--format=email", rev_range };
	char *patchname;

	if (!new_rev)
		new_rev = ctx.qry.head;

	if (get_sha1(new_rev, new_rev_sha1)) {
		cgit_print_error("Bad object id: %s", new_rev);
		return;
	}
	commit = lookup_commit_reference(new_rev_sha1);
	if (!commit) {
		cgit_print_error("Bad commit reference: %s", new_rev);
		return;
	}

	if (old_rev) {
		if (get_sha1(old_rev, old_rev_sha1)) {
			cgit_print_error("Bad object id: %s", old_rev);
			return;
		}
		if (!lookup_commit_reference(old_rev_sha1)) {
			cgit_print_error("Bad commit reference: %s", old_rev);
			return;
		}
	} else if (commit->parents && commit->parents->item) {
		hashcpy(old_rev_sha1, commit->parents->item->object.sha1);
	} else {
		hashclr(old_rev_sha1);
	}

	if (is_null_sha1(old_rev_sha1)) {
		memcpy(rev_range, sha1_to_hex(new_rev_sha1), 41);
	} else {
		sprintf(rev_range, "%s..%s", sha1_to_hex(old_rev_sha1),
			sha1_to_hex(new_rev_sha1));
	}

	patchname = fmt("%s.patch", rev_range);
	ctx.page.mimetype = "text/plain";
	ctx.page.filename = patchname;
	cgit_print_http_headers();

	if (ctx.cfg.noplainemail) {
		rev_argv[2] = "--format=format:From %H Mon Sep 17 00:00:00 "
			      "2001%nFrom: %an%nDate: %aD%n%w(78,0,1)Subject: "
			      "%s%n%n%w(0)%b";
	}

	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.verbose_header = 1;
	rev.diff = 1;
	rev.show_root_diff = 1;
	rev.max_parents = 1;
	rev.diffopt.output_format |= DIFF_FORMAT_DIFFSTAT |
			DIFF_FORMAT_PATCH | DIFF_FORMAT_SUMMARY;
	setup_revisions(ARRAY_SIZE(rev_argv), (const char **)rev_argv, &rev,
			NULL);
	prepare_revision_walk(&rev);

	while ((commit = get_revision(&rev)) != NULL) {
		log_tree_commit(&rev, commit);
		printf("-- \ncgit %s\n\n", cgit_version);
	}

	fflush(stdout);
}
