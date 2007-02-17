/* ui-diff.c: show diff between two blobs
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "xdiff.h"

char *diff_buffer;
int diff_buffer_size;


/*
 * print a single line returned from xdiff
 */
static void print_line(char *line, int len)
{
	char *class = "ctx";
	char c = line[len-1];

	if (line[0] == '+')
		class = "add";
	else if (line[0] == '-')
		class = "del";
	else if (line[0] == '@')
		class = "hunk";

	htmlf("<div class='%s'>", class);
	line[len-1] = '\0';
	html_txt(line);
	html("</div>");
	line[len-1] = c;
}

/*
 * Receive diff-buffers from xdiff and concatenate them as
 * needed across multiple callbacks.
 *
 * This is basically a copy of xdiff-interface.c/xdiff_outf(),
 * ripped from git and modified to use globals instead of
 * a special callback-struct.
 */
int diff_cb(void *priv_, mmbuffer_t *mb, int nbuf)
{
	int i;

	for (i = 0; i < nbuf; i++) {
		if (mb[i].ptr[mb[i].size-1] != '\n') {
			/* Incomplete line */
			diff_buffer = xrealloc(diff_buffer,
					       diff_buffer_size + mb[i].size);
			memcpy(diff_buffer + diff_buffer_size,
			       mb[i].ptr, mb[i].size);
			diff_buffer_size += mb[i].size;
			continue;
		}

		/* we have a complete line */
		if (!diff_buffer) {
			print_line(mb[i].ptr, mb[i].size);
			continue;
		}
		diff_buffer = xrealloc(diff_buffer,
				       diff_buffer_size + mb[i].size);
		memcpy(diff_buffer + diff_buffer_size, mb[i].ptr, mb[i].size);
		print_line(diff_buffer, diff_buffer_size + mb[i].size);
		free(diff_buffer);
		diff_buffer = NULL;
		diff_buffer_size = 0;
	}
	if (diff_buffer) {
		print_line(diff_buffer, diff_buffer_size);
		free(diff_buffer);
		diff_buffer = NULL;
		diff_buffer_size = 0;
	}
	return 0;
}

static int load_mmfile(mmfile_t *file, const unsigned char *sha1)
{
	char type[20];

	if (is_null_sha1(sha1)) {
		file->ptr = (char *)"";
		file->size = 0;
	} else {
		file->ptr = read_sha1_file(sha1, type, &file->size);
	}
	return 1;
}

static void run_diff(const unsigned char *sha1, const unsigned char *sha2)
{
	mmfile_t file1, file2;
	xpparam_t diff_params;
	xdemitconf_t emit_params;
	xdemitcb_t emit_cb;

	if (!load_mmfile(&file1, sha1) || !load_mmfile(&file2, sha2)) {
		cgit_print_error("Unable to load files for diff");
		return;
	}

	diff_params.flags = XDF_NEED_MINIMAL;

	emit_params.ctxlen = 3;
	emit_params.flags = XDL_EMIT_FUNCNAMES;

	emit_cb.outf = diff_cb;

	xdl_diff(&file1, &file2, &diff_params, &emit_params, &emit_cb);
}



void cgit_print_diff(const char *old_hex, const char *new_hex)
{
	unsigned char sha1[20], sha2[20];

	get_sha1(old_hex, sha1);
	get_sha1(new_hex, sha2);

	html("<table class='diff'><tr><td>");
	run_diff(sha1, sha2);
	html("</td></tr></table>");
}
