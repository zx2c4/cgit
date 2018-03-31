/* ui-snapshot.c: generate snapshot of a commit
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-snapshot.h"
#include "html.h"
#include "ui-shared.h"

static int write_archive_type(const char *format, const char *hex, const char *prefix)
{
	struct argv_array argv = ARGV_ARRAY_INIT;
	const char **nargv;
	int result;
	argv_array_push(&argv, "snapshot");
	argv_array_push(&argv, format);
	if (prefix) {
		struct strbuf buf = STRBUF_INIT;
		strbuf_addstr(&buf, prefix);
		strbuf_addch(&buf, '/');
		argv_array_push(&argv, "--prefix");
		argv_array_push(&argv, buf.buf);
		strbuf_release(&buf);
	}
	argv_array_push(&argv, hex);
	/*
	 * Now we need to copy the pointers to arguments into a new
	 * structure because write_archive will rearrange its arguments
	 * which may result in duplicated/missing entries causing leaks
	 * or double-frees in argv_array_clear.
	 */
	nargv = xmalloc(sizeof(char *) * (argv.argc + 1));
	/* argv_array guarantees a trailing NULL entry. */
	memcpy(nargv, argv.argv, sizeof(char *) * (argv.argc + 1));

	result = write_archive(argv.argc, nargv, NULL, NULL, 0);
	argv_array_clear(&argv);
	free(nargv);
	return result;
}

static int write_tar_archive(const char *hex, const char *prefix)
{
	return write_archive_type("--format=tar", hex, prefix);
}

static int write_zip_archive(const char *hex, const char *prefix)
{
	return write_archive_type("--format=zip", hex, prefix);
}

static int write_compressed_tar_archive(const char *hex,
					const char *prefix,
					char *filter_argv[])
{
	int rv;
	struct cgit_exec_filter f;
	cgit_exec_filter_init(&f, filter_argv[0], filter_argv);

	cgit_open_filter(&f.base);
	rv = write_tar_archive(hex, prefix);
	cgit_close_filter(&f.base);
	return rv;
}

static int write_tar_gzip_archive(const char *hex, const char *prefix)
{
	char *argv[] = { "gzip", "-n", NULL };
	return write_compressed_tar_archive(hex, prefix, argv);
}

static int write_tar_bzip2_archive(const char *hex, const char *prefix)
{
	char *argv[] = { "bzip2", NULL };
	return write_compressed_tar_archive(hex, prefix, argv);
}

static int write_tar_xz_archive(const char *hex, const char *prefix)
{
	char *argv[] = { "xz", NULL };
	return write_compressed_tar_archive(hex, prefix, argv);
}

const struct cgit_snapshot_format cgit_snapshot_formats[] = {
	{ ".zip", "application/x-zip", write_zip_archive, 0x01 },
	{ ".tar.gz", "application/x-gzip", write_tar_gzip_archive, 0x02 },
	{ ".tar.bz2", "application/x-bzip2", write_tar_bzip2_archive, 0x04 },
	{ ".tar", "application/x-tar", write_tar_archive, 0x08 },
	{ ".tar.xz", "application/x-xz", write_tar_xz_archive, 0x10 },
	{ NULL }
};

static const struct cgit_snapshot_format *get_format(const char *filename)
{
	const struct cgit_snapshot_format *fmt;

	for (fmt = cgit_snapshot_formats; fmt->suffix; fmt++) {
		if (ends_with(filename, fmt->suffix))
			return fmt;
	}
	return NULL;
}

static int make_snapshot(const struct cgit_snapshot_format *format,
			 const char *hex, const char *prefix,
			 const char *filename)
{
	struct object_id oid;

	if (get_oid(hex, &oid)) {
		cgit_print_error_page(404, "Not found",
				"Bad object id: %s", hex);
		return 1;
	}
	if (!lookup_commit_reference(&oid)) {
		cgit_print_error_page(400, "Bad request",
				"Not a commit reference: %s", hex);
		return 1;
	}
	ctx.page.etag = oid_to_hex(&oid);
	ctx.page.mimetype = xstrdup(format->mimetype);
	ctx.page.filename = xstrdup(filename);
	cgit_print_http_headers();
	format->write_func(hex, prefix);
	return 0;
}

/* Try to guess the requested revision from the requested snapshot name.
 * First the format extension is stripped, e.g. "cgit-0.7.2.tar.gz" become
 * "cgit-0.7.2". If this is a valid commit object name we've got a winner.
 * Otherwise, if the snapshot name has a prefix matching the result from
 * repo_basename(), we strip the basename and any following '-' and '_'
 * characters ("cgit-0.7.2" -> "0.7.2") and check the resulting name once
 * more. If this still isn't a valid commit object name, we check if pre-
 * pending a 'v' or a 'V' to the remaining snapshot name ("0.7.2" ->
 * "v0.7.2") gives us something valid.
 */
static const char *get_ref_from_filename(const struct cgit_repo *repo,
					 const char *filename,
					 const struct cgit_snapshot_format *format)
{
	const char *reponame;
	struct object_id oid;
	struct strbuf snapshot = STRBUF_INIT;
	int result = 1;

	strbuf_addstr(&snapshot, filename);
	strbuf_setlen(&snapshot, snapshot.len - strlen(format->suffix));

	if (get_oid(snapshot.buf, &oid) == 0)
		goto out;

	reponame = cgit_snapshot_prefix(repo);
	if (starts_with(snapshot.buf, reponame)) {
		const char *new_start = snapshot.buf;
		new_start += strlen(reponame);
		while (new_start && (*new_start == '-' || *new_start == '_'))
			new_start++;
		strbuf_splice(&snapshot, 0, new_start - snapshot.buf, "", 0);
	}

	if (get_oid(snapshot.buf, &oid) == 0)
		goto out;

	strbuf_insert(&snapshot, 0, "v", 1);
	if (get_oid(snapshot.buf, &oid) == 0)
		goto out;

	strbuf_splice(&snapshot, 0, 1, "V", 1);
	if (get_oid(snapshot.buf, &oid) == 0)
		goto out;

	result = 0;
	strbuf_release(&snapshot);

out:
	return result ? strbuf_detach(&snapshot, NULL) : NULL;
}

void cgit_print_snapshot(const char *head, const char *hex,
			 const char *filename, int dwim)
{
	const struct cgit_snapshot_format* f;
	char *prefix = NULL;

	if (!filename) {
		cgit_print_error_page(400, "Bad request",
				"No snapshot name specified");
		return;
	}

	f = get_format(filename);
	if (!f || !(ctx.repo->snapshots & f->bit)) {
		cgit_print_error_page(400, "Bad request",
				"Unsupported snapshot format: %s", filename);
		return;
	}

	if (!hex && dwim) {
		hex = get_ref_from_filename(ctx.repo, filename, f);
		if (hex == NULL) {
			cgit_print_error_page(404, "Not found", "Not found");
			return;
		}
		prefix = xstrdup(filename);
		prefix[strlen(filename) - strlen(f->suffix)] = '\0';
	}

	if (!hex)
		hex = head;

	if (!prefix)
		prefix = xstrdup(cgit_snapshot_prefix(ctx.repo));

	make_snapshot(f, hex, prefix, filename);
	free(prefix);
}
