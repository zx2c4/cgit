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

/* two commit hashes with two dots in between and termination */
#define REV_RANGE_LEN 2 * GIT_MAX_HEXSZ + 3

void cgit_print_patch(const char *new_rev, const char *old_rev,
		      const char *prefix)
{
	struct rev_info rev;
	struct commit *commit;
	struct object_id new_rev_oid, old_rev_oid;
	char rev_range[REV_RANGE_LEN];
	const char *rev_argv[] = { NULL, "--reverse", "--format=email", rev_range, "--", prefix, NULL };
	int rev_argc = ARRAY_SIZE(rev_argv) - 1;
	char *patchname;

	if (!prefix)
		rev_argc--;

	if (!new_rev)
		new_rev = ctx.qry.head;

	if (get_oid(new_rev, &new_rev_oid)) {
		cgit_print_error_page(404, "Not found",
				"Bad object id: %s", new_rev);
		return;
	}
	commit = lookup_commit_reference(the_repository, &new_rev_oid);
	if (!commit) {
		cgit_print_error_page(404, "Not found",
				"Bad commit reference: %s", new_rev);
		return;
	}

	if (old_rev) {
		if (get_oid(old_rev, &old_rev_oid)) {
			cgit_print_error_page(404, "Not found",
					"Bad object id: %s", old_rev);
			return;
		}
		if (!lookup_commit_reference(the_repository, &old_rev_oid)) {
			cgit_print_error_page(404, "Not found",
					"Bad commit reference: %s", old_rev);
			return;
		}
	} else if (commit->parents && commit->parents->item) {
		oidcpy(&old_rev_oid, &commit->parents->item->object.oid);
	} else {
		oidclr(&old_rev_oid);
	}

	if (is_null_oid(&old_rev_oid)) {
		memcpy(rev_range, oid_to_hex(&new_rev_oid), GIT_SHA1_HEXSZ + 1);
	} else {
		xsnprintf(rev_range, REV_RANGE_LEN, "%s..%s", oid_to_hex(&old_rev_oid),
			oid_to_hex(&new_rev_oid));
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
	if (prefix)
		rev.diffopt.stat_sep = fmt("(limited to '%s')\n\n", prefix);
	setup_revisions(rev_argc, rev_argv, &rev, NULL);
	prepare_revision_walk(&rev);

	while ((commit = get_revision(&rev)) != NULL) {
		log_tree_commit(&rev, commit);
		printf("-- \ncgit %s\n\n", cgit_version);
	}
}
