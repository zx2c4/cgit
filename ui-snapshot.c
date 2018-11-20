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

	result = write_archive(argv.argc, nargv, NULL, the_repository, NULL, 0);
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
	/* .tar must remain the 0 index */
	{ ".tar",	"application/x-tar",	write_tar_archive	},
	{ ".tar.gz",	"application/x-gzip",	write_tar_gzip_archive	},
	{ ".tar.bz2",	"application/x-bzip2",	write_tar_bzip2_archive	},
	{ ".tar.xz",	"application/x-xz",	write_tar_xz_archive	},
	{ ".zip",	"application/x-zip",	write_zip_archive	},
	{ NULL }
};

static struct notes_tree snapshot_sig_notes[ARRAY_SIZE(cgit_snapshot_formats)];

const struct object_id *cgit_snapshot_get_sig(const char *ref,
					      const struct cgit_snapshot_format *f)
{
	struct notes_tree *tree;
	struct object_id oid;

	if (get_oid(ref, &oid))
		return NULL;

	tree = &snapshot_sig_notes[f - &cgit_snapshot_formats[0]];
	if (!tree->initialized) {
		struct strbuf notes_ref = STRBUF_INIT;

		strbuf_addf(&notes_ref, "refs/notes/signatures/%s",
			    f->suffix + 1);

		init_notes(tree, notes_ref.buf, combine_notes_ignore, 0);
		strbuf_release(&notes_ref);
	}

	return get_note(tree, &oid);
}

static const struct cgit_snapshot_format *get_format(const char *filename)
{
	const struct cgit_snapshot_format *fmt;

	for (fmt = cgit_snapshot_formats; fmt->suffix; fmt++) {
		if (ends_with(filename, fmt->suffix))
			return fmt;
	}
	return NULL;
}

const unsigned cgit_snapshot_format_bit(const struct cgit_snapshot_format *f)
{
	return BIT(f - &cgit_snapshot_formats[0]);
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
	if (!lookup_commit_reference(the_repository, &oid)) {
		cgit_print_error_page(400, "Bad request",
				"Not a commit reference: %s", hex);
		return 1;
	}
	ctx.page.etag = oid_to_hex(&oid);
	ctx.page.mimetype = xstrdup(format->mimetype);
	ctx.page.filename = xstrdup(filename);
	cgit_print_http_headers();
	init_archivers();
	format->write_func(hex, prefix);
	return 0;
}

static int write_sig(const struct cgit_snapshot_format *format,
		     const char *hex, const char *archive,
		     const char *filename)
{
	const struct object_id *note = cgit_snapshot_get_sig(hex, format);
	enum object_type type;
	unsigned long size;
	char *buf;

	if (!note) {
		cgit_print_error_page(404, "Not found",
				"No signature for %s", archive);
		return 0;
	}

	buf = read_object_file(note, &type, &size);
	if (!buf) {
		cgit_print_error_page(404, "Not found", "Not found");
		return 0;
	}

	html("X-Content-Type-Options: nosniff\n");
	html("Content-Security-Policy: default-src 'none'\n");
	ctx.page.etag = oid_to_hex(note);
	ctx.page.mimetype = xstrdup("application/pgp-signature");
	ctx.page.filename = xstrdup(filename);
	cgit_print_http_headers();

	html_raw(buf, size);
	free(buf);
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
	const char *sig_filename = NULL;
	char *adj_filename = NULL;
	char *prefix = NULL;

	if (!filename) {
		cgit_print_error_page(400, "Bad request",
				"No snapshot name specified");
		return;
	}

	if (ends_with(filename, ".asc")) {
		sig_filename = filename;

		/* Strip ".asc" from filename for common format processing */
		adj_filename = xstrdup(filename);
		adj_filename[strlen(adj_filename) - 4] = '\0';
		filename = adj_filename;
	}

	f = get_format(filename);
	if (!f || (!sig_filename && !(ctx.repo->snapshots & cgit_snapshot_format_bit(f)))) {
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

	if (sig_filename)
		write_sig(f, hex, filename, sig_filename);
	else
		make_snapshot(f, hex, prefix, filename);

	free(prefix);
	free(adj_filename);
}
