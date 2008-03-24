/* ui-snapshot.c: generate snapshot of a commit
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "html.h"
#include "ui-shared.h"

static int write_compressed_tar_archive(struct archiver_args *args,const char *filter)
{
	int rw[2];
	pid_t gzpid;
	int stdout2;
	int status;
	int rv;

	stdout2 = chk_non_negative(dup(STDIN_FILENO), "Preserving STDOUT before compressing");
	chk_zero(pipe(rw), "Opening pipe from compressor subprocess");
	gzpid = chk_non_negative(fork(), "Forking compressor subprocess");
	if(gzpid==0) {
		/* child */
		chk_zero(close(rw[1]), "Closing write end of pipe in child");
		chk_zero(close(STDIN_FILENO), "Closing STDIN");
		chk_non_negative(dup2(rw[0],STDIN_FILENO), "Redirecting compressor input to stdin");
		execlp(filter,filter,NULL);
		_exit(-1);
	}
	/* parent */
	chk_zero(close(rw[0]), "Closing read end of pipe");
	chk_non_negative(dup2(rw[1],STDOUT_FILENO), "Redirecting output to compressor");

	rv = write_tar_archive(args);

	chk_zero(close(STDOUT_FILENO), "Closing STDOUT redirected to compressor");
	chk_non_negative(dup2(stdout2,STDOUT_FILENO), "Restoring uncompressed STDOUT");
	chk_zero(close(stdout2), "Closing uncompressed STDOUT");
	chk_zero(close(rw[1]), "Closing write end of pipe in parent");
	chk_positive(waitpid(gzpid,&status,0), "Waiting on compressor process");
	if(! ( WIFEXITED(status) && WEXITSTATUS(status)==0 ) )
		cgit_print_error("Failed to compress archive");

	return rv;
}

static int write_tar_gzip_archive(struct archiver_args *args)
{
	return write_compressed_tar_archive(args,"gzip");
}

static int write_tar_bzip2_archive(struct archiver_args *args)
{
	return write_compressed_tar_archive(args,"bzip2");
}

const struct cgit_snapshot_format cgit_snapshot_formats[] = {
	{ ".zip", "application/x-zip", write_zip_archive, 0x1 },
	{ ".tar.gz", "application/x-tar", write_tar_gzip_archive, 0x2 },
	{ ".tar.bz2", "application/x-tar", write_tar_bzip2_archive, 0x4 },
	{ ".tar", "application/x-tar", write_tar_archive, 0x8 },
	{}
};

static int make_snapshot(const struct cgit_snapshot_format *format,
			 const char *hex, const char *prefix,
			 const char *filename)
{
	struct archiver_args args;
	struct commit *commit;
	unsigned char sha1[20];

	if(get_sha1(hex, sha1)) {
		cgit_print_error(fmt("Bad object id: %s", hex));
		return 1;
	}
	commit = lookup_commit_reference(sha1);
	if(!commit) {
		cgit_print_error(fmt("Not a commit reference: %s", hex));
		return 1;
	}
	memset(&args, 0, sizeof(args));
	args.base = fmt("%s/", prefix);
	args.tree = commit->tree;
	args.time = commit->date;
	ctx.page.mimetype = xstrdup(format->mimetype);
	ctx.page.filename = xstrdup(filename);
	cgit_print_http_headers(&ctx);
	format->write_func(&args);
	return 0;
}

void cgit_print_snapshot(const char *head, const char *hex, const char *prefix,
			 const char *filename, int snapshots)
{
	const struct cgit_snapshot_format* f;
	int sl, fnl;

	fnl = strlen(filename);
	if (!hex)
		hex = head;
	for (f = cgit_snapshot_formats; f->suffix; f++) {
		if (!(snapshots & f->bit))
			continue;
		sl = strlen(f->suffix);
		if(fnl < sl || strcmp(&filename[fnl-sl], f->suffix))
			continue;
		make_snapshot(f, hex, prefix, filename);
		return;
	}
	cgit_print_error(fmt("Unsupported snapshot format: %s", filename));
}
