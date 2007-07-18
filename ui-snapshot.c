/* ui-snapshot.c: generate snapshot of a commit
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

static const struct snapshot_archive_t {
    	const char *suffix;
	const char *mimetype;
	write_archive_fn_t write_func;
}	snapshot_archives[] = {
	{ ".zip", "application/x-zip", write_zip_archive },
	{ ".tar.gz", "application/x-gzip", write_tar_archive }
};

void cgit_print_snapshot(struct cacheitem *item, const char *hex, 
			 const char *prefix, const char *filename)
{
	int fnl = strlen(filename);
	int f;
    	for(f=0;f<(sizeof(snapshot_archives)/sizeof(*snapshot_archives));++f) {
		const struct snapshot_archive_t* sat = &snapshot_archives[f];
		int sl = strlen(sat->suffix);
		if(fnl<sl || strcmp(&filename[fnl-sl],sat->suffix))
			continue;

		struct archiver_args args;
		struct commit *commit;
		unsigned char sha1[20];

		if(get_sha1(hex, sha1)) {
			cgit_print_error(fmt("Bad object id: %s", hex));
			return;
		}
		commit = lookup_commit_reference(sha1);

		if(!commit) {
			cgit_print_error(fmt("Not a commit reference: %s", hex));
			return;;
		}

		memset(&args,0,sizeof(args));
		args.base = fmt("%s/", prefix);
		args.tree = commit->tree;

		cgit_print_snapshot_start(sat->mimetype, filename, item);
		(*sat->write_func)(&args);
		return;
	}
	cgit_print_error(fmt("Unsupported snapshot format: %s", filename));
}

void cgit_print_snapshot_links(const char *repo,const char *hex)
{
    	char *filename;
	int f;
    	for(f=0;f<(sizeof(snapshot_archives)/sizeof(*snapshot_archives));++f) {
		const struct snapshot_archive_t* sat = &snapshot_archives[f];
		filename = fmt("%s-%s%s",repo,hex,sat->suffix);
		htmlf("<a href='%s'>%s</a><br/>",
			cgit_pageurl(repo,"snapshot",
			    fmt("id=%s&amp;name=%s",hex,filename)), filename);
	}
}
