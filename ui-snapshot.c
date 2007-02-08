/* ui-snapshot.c: generate snapshot of a commit
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

static void cgit_print_zip(struct cacheitem *item, const char *hex, 
			   const char *prefix, const char *filename)
{
	struct archiver_args args;
	struct commit *commit;
	unsigned char sha1[20];

	if (get_sha1(hex, sha1)) {
		cgit_print_error(fmt("Bad object id: %s", hex));
		return;
	}
	commit = lookup_commit_reference(sha1);

	if (!commit) {
		cgit_print_error(fmt("Not a commit reference: %s", hex));
		return;
	}

	memset(&args, 0, sizeof(args));
	args.base = fmt("%s/", prefix);
	args.tree = commit->tree;
	
	cgit_print_snapshot_start("application/x-zip", filename, item);
	write_zip_archive(&args);
}


void cgit_print_snapshot(struct cacheitem *item, const char *hex, 
			 const char *format, const char *prefix,
			 const char *filename)
{
	if (!strcmp(format, "zip"))
		cgit_print_zip(item, hex, prefix, filename);
	else
		cgit_print_error(fmt("Unsupported snapshot format: %s", 
				     format));
}
