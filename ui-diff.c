static void header(unsigned char *sha1, char *path1, int mode1,
		   unsigned char *sha2, char *path2, int mode2)
	int subproject;
	subproject = (S_ISGITLINK(mode1) || S_ISGITLINK(mode2));

	if (is_null_sha1(sha1))
		path1 = "dev/null";
	if (is_null_sha1(sha2))
		path2 = "dev/null";

	if (mode1 == 0)
		htmlf("<br/>new file mode %.6o", mode2);

	if (mode2 == 0)
		htmlf("<br/>deleted file mode %.6o", mode1);

	if (!subproject) {
		abbrev1 = xstrdup(find_unique_abbrev(sha1, DEFAULT_ABBREV));
		abbrev2 = xstrdup(find_unique_abbrev(sha2, DEFAULT_ABBREV));
		htmlf("<br/>index %s..%s", abbrev1, abbrev2);
		free(abbrev1);
		free(abbrev2);
		if (mode1 != 0 && mode2 != 0) {
			htmlf(" %.6o", mode1);
			if (mode2 != mode1)
				htmlf("..%.6o", mode2);
		}
		html("<br/>--- a/");
		html_txt(path1);
		html("<br/>+++ b/");
		html_txt(path2);
	}
	header(pair->one->sha1, pair->one->path, pair->one->mode,
	       pair->two->sha1, pair->two->path, pair->two->mode);
	if (S_ISGITLINK(pair->one->mode) || S_ISGITLINK(pair->two->mode)) {
		if (S_ISGITLINK(pair->one->mode))
			print_line(fmt("-Subproject %s", sha1_to_hex(pair->one->sha1)), 52);
		if (S_ISGITLINK(pair->two->mode))
			print_line(fmt("+Subproject %s", sha1_to_hex(pair->two->sha1)), 52);
		return;
	}
void cgit_print_diff(const char *new_rev, const char *old_rev)
	struct commit *commit, *commit2;
	if (!new_rev)
		new_rev = cgit_query_head;
	get_sha1(new_rev, sha1);
		cgit_print_error(fmt("Bad object name: %s", new_rev));
		return;
	}
	if (type != OBJ_COMMIT) {
		cgit_print_error(fmt("Unhandled object type: %s",
				     typename(type)));
		return;
	}

	commit = lookup_commit_reference(sha1);
	if (!commit || parse_commit(commit))
		cgit_print_error(fmt("Bad commit: %s", sha1_to_hex(sha1)));

	if (old_rev)
		get_sha1(old_rev, sha2);
	else if (commit->parents && commit->parents->item)
		hashcpy(sha2, commit->parents->item->object.sha1);
	else
		hashclr(sha2);

	if (!is_null_sha1(sha2)) {
			cgit_print_error(fmt("Bad object name: %s", sha1_to_hex(sha2)));
		commit2 = lookup_commit_reference(sha2);
		if (!commit2 || parse_commit(commit2))
			cgit_print_error(fmt("Bad commit: %s", sha1_to_hex(sha2)));
	html("<tr><td>");
	cgit_diff_tree(sha2, sha1, filepair_cb);
	html("</td></tr>");
	html("</table>");