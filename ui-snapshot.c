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

static const struct cgit_snapshot_format *get_format(const char *filename)
{
	const struct cgit_snapshot_format *fmt;
	int fl, sl;

	fl = strlen(filename);
	for(fmt = cgit_snapshot_formats; fmt->suffix; fmt++) {
		sl = strlen(fmt->suffix);
		if (sl >= fl)
			continue;
		if (!strcmp(fmt->suffix, filename + fl - sl))
			return fmt;
	}
	return NULL;
}

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
	if (prefix) {
		args.base = fmt("%s/", prefix);
		args.baselen = strlen(prefix) + 1;
	} else {
		args.base = "";
		args.baselen = 0;
	}
	args.tree = commit->tree;
	args.time = commit->date;
	ctx.page.mimetype = xstrdup(format->mimetype);
	ctx.page.filename = xstrdup(filename);
	cgit_print_http_headers(&ctx);
	format->write_func(&args);
	return 0;
}

/* Try to guess the requested revision from the requested snapshot name.
 * First the format extension is stripped, e.g. "cgit-0.7.2.tar.gz" become
 * "cgit-0.7.2". If this is a valid commit object name we've got a winner.
 * Otherwise, if the snapshot name has a prefix matching the result from
 * repo_basename(), we strip the basename and any following '-' and '_'
 * characters ("cgit-0.7.2" -> "0.7.2") and check the resulting name once
 * more. If this still isn't a valid commit object name, we check if pre-
 * pending a 'v' to the remaining snapshot name ("0.7.2" -> "v0.7.2") gives
 * us something valid.
 */
static const char *get_ref_from_filename(const char *url, const char *filename,
					 const struct cgit_snapshot_format *format)
{
	const char *reponame;
	unsigned char sha1[20];
	char *snapshot;

	snapshot = xstrdup(filename);
	snapshot[strlen(snapshot) - strlen(format->suffix)] = '\0';
	fprintf(stderr, "snapshot=%s\n", snapshot);

	if (get_sha1(snapshot, sha1) == 0)
		return snapshot;

	reponame = cgit_repobasename(url);
	fprintf(stderr, "reponame=%s\n", reponame);
	if (prefixcmp(snapshot, reponame) == 0) {
		snapshot += strlen(reponame);
		while (snapshot && (*snapshot == '-' || *snapshot == '_'))
			snapshot++;
	}

	if (get_sha1(snapshot, sha1) == 0)
		return snapshot;

	snapshot = fmt("v%s", snapshot);
	if (get_sha1(snapshot, sha1) == 0)
		return snapshot;

	return NULL;
}

void cgit_print_snapshot(const char *head, const char *hex,
			 const char *filename, int snapshots, int dwim)
{
	const struct cgit_snapshot_format* f;
	char *prefix = NULL;

	f = get_format(filename);
	if (!f) {
		ctx.page.mimetype = "text/html";
		cgit_print_http_headers(&ctx);
		cgit_print_docstart(&ctx);
		cgit_print_pageheader(&ctx);
		cgit_print_error(fmt("Unsupported snapshot format: %s", filename));
		cgit_print_docend();
		return;
	}

	if (!hex && dwim) {
		hex = get_ref_from_filename(ctx.repo->url, filename, f);
		if (hex != NULL) {
			prefix = xstrdup(filename);
			prefix[strlen(filename) - strlen(f->suffix)] = '\0';
		}
	}

	if (!hex)
		hex = head;

	if (!prefix)
		prefix = xstrdup(cgit_repobasename(ctx.repo->url));

	make_snapshot(f, hex, prefix, filename);
	free(prefix);
}
