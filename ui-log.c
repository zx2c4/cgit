/* ui-log.c: functions for log output
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-log.h"
#include "html.h"
#include "ui-shared.h"
#include "argv-array.h"

static int files, add_lines, rem_lines, lines_counted;

/*
 * The list of available column colors in the commit graph.
 */
static const char *column_colors_html[] = {
	"<span class='column1'>",
	"<span class='column2'>",
	"<span class='column3'>",
	"<span class='column4'>",
	"<span class='column5'>",
	"<span class='column6'>",
	"</span>",
};

#define COLUMN_COLORS_HTML_MAX (ARRAY_SIZE(column_colors_html) - 1)

static void count_lines(char *line, int size)
{
	if (size <= 0)
		return;

	if (line[0] == '+')
		add_lines++;

	else if (line[0] == '-')
		rem_lines++;
}

static void inspect_files(struct diff_filepair *pair)
{
	unsigned long old_size = 0;
	unsigned long new_size = 0;
	int binary = 0;

	files++;
	if (ctx.repo->enable_log_linecount)
		cgit_diff_files(&pair->one->oid, &pair->two->oid, &old_size,
				&new_size, &binary, 0, ctx.qry.ignorews,
				count_lines);
}

void show_commit_decorations(struct commit *commit)
{
	const struct name_decoration *deco;
	static char buf[1024];

	buf[sizeof(buf) - 1] = 0;
	deco = get_name_decoration(&commit->object);
	if (!deco)
		return;
	html("<span class='decoration'>");
	while (deco) {
		struct object_id peeled;
		int is_annotated = 0;
		strlcpy(buf, prettify_refname(deco->name), sizeof(buf));
		switch(deco->type) {
		case DECORATION_NONE:
			/* If the git-core doesn't recognize it,
			 * don't display anything. */
			break;
		case DECORATION_REF_LOCAL:
			cgit_log_link(buf, NULL, "branch-deco", buf, NULL,
				ctx.qry.vpath, 0, NULL, NULL,
				ctx.qry.showmsg, 0);
			break;
		case DECORATION_REF_TAG:
			if (!peel_ref(deco->name, &peeled))
				is_annotated = !oidcmp(&commit->object.oid, &peeled);
			cgit_tag_link(buf, NULL, is_annotated ? "tag-annotated-deco" : "tag-deco", buf);
			break;
		case DECORATION_REF_REMOTE:
			if (!ctx.repo->enable_remote_branches)
				break;
			cgit_log_link(buf, NULL, "remote-deco", NULL,
				oid_to_hex(&commit->object.oid),
				ctx.qry.vpath, 0, NULL, NULL,
				ctx.qry.showmsg, 0);
			break;
		default:
			cgit_commit_link(buf, NULL, "deco", ctx.qry.head,
					oid_to_hex(&commit->object.oid),
					ctx.qry.vpath);
			break;
		}
		deco = deco->next;
	}
	html("</span>");
}

static void handle_rename(struct diff_filepair *pair)
{
	/*
	 * After we have seen a rename, we generate links to the previous
	 * name of the file so that commit & diff views get fed the path
	 * that is correct for the commit they are showing, avoiding the
	 * need to walk the entire history leading back to every commit we
	 * show in order detect renames.
	 */
	if (0 != strcmp(ctx.qry.vpath, pair->two->path)) {
		free(ctx.qry.vpath);
		ctx.qry.vpath = xstrdup(pair->two->path);
	}
	inspect_files(pair);
}

static int show_commit(struct commit *commit, struct rev_info *revs)
{
	struct commit_list *parents = commit->parents;
	struct commit *parent;
	int found = 0, saved_fmt;
	struct diff_flags saved_flags = revs->diffopt.flags;

	/* Always show if we're not in "follow" mode with a single file. */
	if (!ctx.qry.follow)
		return 1;

	/*
	 * In "follow" mode, we don't show merges.  This is consistent with
	 * "git log --follow -- <file>".
	 */
	if (parents && parents->next)
		return 0;

	/*
	 * If this is the root commit, do what rev_info tells us.
	 */
	if (!parents)
		return revs->show_root_diff;

	/* When we get here we have precisely one parent. */
	parent = parents->item;
	/* If we can't parse the commit, let print_commit() report an error. */
	if (parse_commit(parent))
		return 1;

	files = 0;
	add_lines = 0;
	rem_lines = 0;

	revs->diffopt.flags.recursive = 1;
	diff_tree_oid(&parent->maybe_tree->object.oid,
		      &commit->maybe_tree->object.oid,
		      "", &revs->diffopt);
	diffcore_std(&revs->diffopt);

	found = !diff_queue_is_empty();
	saved_fmt = revs->diffopt.output_format;
	revs->diffopt.output_format = DIFF_FORMAT_CALLBACK;
	revs->diffopt.format_callback = cgit_diff_tree_cb;
	revs->diffopt.format_callback_data = handle_rename;
	diff_flush(&revs->diffopt);
	revs->diffopt.output_format = saved_fmt;
	revs->diffopt.flags = saved_flags;

	lines_counted = 1;
	return found;
}

static void print_commit(struct commit *commit, struct rev_info *revs)
{
	struct commitinfo *info;
	int columns = revs->graph ? 4 : 3;
	struct strbuf graphbuf = STRBUF_INIT;
	struct strbuf msgbuf = STRBUF_INIT;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;

	if (revs->graph) {
		/* Advance graph until current commit */
		while (!graph_next_line(revs->graph, &graphbuf)) {
			/* Print graph segment in otherwise empty table row */
			html("<tr class='nohover'><td class='commitgraph'>");
			html(graphbuf.buf);
			htmlf("</td><td colspan='%d' /></tr>\n", columns);
			strbuf_setlen(&graphbuf, 0);
		}
		/* Current commit's graph segment is now ready in graphbuf */
	}

	info = cgit_parse_commit(commit);
	htmlf("<tr%s>", ctx.qry.showmsg ? " class='logheader'" : "");

	if (revs->graph) {
		/* Print graph segment for current commit */
		html("<td class='commitgraph'>");
		html(graphbuf.buf);
		html("</td>");
		strbuf_setlen(&graphbuf, 0);
	}
	else {
		html("<td>");
		cgit_print_age(info->committer_date, info->committer_tz, TM_WEEK * 2);
		html("</td>");
	}

	htmlf("<td%s>", ctx.qry.showmsg ? " class='logsubject'" : "");
	if (ctx.qry.showmsg) {
		/* line-wrap long commit subjects instead of truncating them */
		size_t subject_len = strlen(info->subject);

		if (subject_len > ctx.cfg.max_msg_len &&
		    ctx.cfg.max_msg_len >= 15) {
			/* symbol for signaling line-wrap (in PAGE_ENCODING) */
			const char wrap_symbol[] = { ' ', 0xE2, 0x86, 0xB5, 0 };
			int i = ctx.cfg.max_msg_len - strlen(wrap_symbol);

			/* Rewind i to preceding space character */
			while (i > 0 && !isspace(info->subject[i]))
				--i;
			if (!i) /* Oops, zero spaces. Reset i */
				i = ctx.cfg.max_msg_len - strlen(wrap_symbol);

			/* add remainder starting at i to msgbuf */
			strbuf_add(&msgbuf, info->subject + i, subject_len - i);
			strbuf_trim(&msgbuf);
			strbuf_add(&msgbuf, "\n\n", 2);

			/* Place wrap_symbol at position i in info->subject */
			strlcpy(info->subject + i, wrap_symbol, subject_len - i + 1);
		}
	}
	cgit_commit_link(info->subject, NULL, NULL, ctx.qry.head,
			 oid_to_hex(&commit->object.oid), ctx.qry.vpath);
	show_commit_decorations(commit);
	html("</td><td>");
	cgit_open_filter(ctx.repo->email_filter, info->author_email, "log");
	html_txt(info->author);
	cgit_close_filter(ctx.repo->email_filter);

	if (revs->graph) {
		html("</td><td>");
		cgit_print_age(info->committer_date, info->committer_tz, TM_WEEK * 2);
	}

	if (!lines_counted && (ctx.repo->enable_log_filecount ||
			       ctx.repo->enable_log_linecount)) {
		files = 0;
		add_lines = 0;
		rem_lines = 0;
		cgit_diff_commit(commit, inspect_files, ctx.qry.vpath);
	}

	if (ctx.repo->enable_log_filecount)
		htmlf("</td><td>%d", files);
	if (ctx.repo->enable_log_linecount)
		htmlf("</td><td><span class='deletions'>-%d</span>/"
			"<span class='insertions'>+%d</span>", rem_lines, add_lines);

	html("</td></tr>\n");

	if ((revs->graph && !graph_is_commit_finished(revs->graph))
			|| ctx.qry.showmsg) { /* Print a second table row */
		html("<tr class='nohover-highlight'>");

		if (ctx.qry.showmsg) {
			/* Concatenate commit message + notes in msgbuf */
			if (info->msg && *(info->msg)) {
				strbuf_addstr(&msgbuf, info->msg);
				strbuf_addch(&msgbuf, '\n');
			}
			format_display_notes(&commit->object.oid,
					     &msgbuf, PAGE_ENCODING, 0);
			strbuf_addch(&msgbuf, '\n');
			strbuf_ltrim(&msgbuf);
		}

		if (revs->graph) {
			int lines = 0;

			/* Calculate graph padding */
			if (ctx.qry.showmsg) {
				/* Count #lines in commit message + notes */
				const char *p = msgbuf.buf;
				lines = 1;
				while ((p = strchr(p, '\n'))) {
					p++;
					lines++;
				}
			}

			/* Print graph padding */
			html("<td class='commitgraph'>");
			while (lines > 0 || !graph_is_commit_finished(revs->graph)) {
				if (graphbuf.len)
					html("\n");
				strbuf_setlen(&graphbuf, 0);
				graph_next_line(revs->graph, &graphbuf);
				html(graphbuf.buf);
				lines--;
			}
			html("</td>\n");
		}
		else
			html("<td/>"); /* Empty 'Age' column */

		/* Print msgbuf into remainder of table row */
		htmlf("<td colspan='%d'%s>\n", columns - (revs->graph ? 1 : 0),
			ctx.qry.showmsg ? " class='logmsg'" : "");
		html_txt(msgbuf.buf);
		html("</td></tr>\n");
	}

	strbuf_release(&msgbuf);
	strbuf_release(&graphbuf);
	cgit_free_commitinfo(info);
}

static const char *disambiguate_ref(const char *ref, int *must_free_result)
{
	struct object_id oid;
	struct strbuf longref = STRBUF_INIT;

	strbuf_addf(&longref, "refs/heads/%s", ref);
	if (get_oid(longref.buf, &oid) == 0) {
		*must_free_result = 1;
		return strbuf_detach(&longref, NULL);
	}

	*must_free_result = 0;
	strbuf_release(&longref);
	return ref;
}

static char *next_token(char **src)
{
	char *result;

	if (!src || !*src)
		return NULL;
	while (isspace(**src))
		(*src)++;
	if (!**src)
		return NULL;
	result = *src;
	while (**src) {
		if (isspace(**src)) {
			**src = '\0';
			(*src)++;
			break;
		}
		(*src)++;
	}
	return result;
}

void cgit_print_log(const char *tip, int ofs, int cnt, char *grep, char *pattern,
		    char *path, int pager, int commit_graph, int commit_sort)
{
	struct rev_info rev;
	struct commit *commit;
	struct argv_array rev_argv = ARGV_ARRAY_INIT;
	int i, columns = commit_graph ? 4 : 3;
	int must_free_tip = 0;

	/* rev_argv.argv[0] will be ignored by setup_revisions */
	argv_array_push(&rev_argv, "log_rev_setup");

	if (!tip)
		tip = ctx.qry.head;
	tip = disambiguate_ref(tip, &must_free_tip);
	argv_array_push(&rev_argv, tip);

	if (grep && pattern && *pattern) {
		pattern = xstrdup(pattern);
		if (!strcmp(grep, "grep") || !strcmp(grep, "author") ||
		    !strcmp(grep, "committer")) {
			argv_array_pushf(&rev_argv, "--%s=%s", grep, pattern);
		} else if (!strcmp(grep, "range")) {
			char *arg;
			/* Split the pattern at whitespace and add each token
			 * as a revision expression. Do not accept other
			 * rev-list options. Also, replace the previously
			 * pushed tip (it's no longer relevant).
			 */
			argv_array_pop(&rev_argv);
			while ((arg = next_token(&pattern))) {
				if (*arg == '-') {
					fprintf(stderr, "Bad range expr: %s\n",
						arg);
					break;
				}
				argv_array_push(&rev_argv, arg);
			}
		}
	}

	if (!path || !ctx.cfg.enable_follow_links) {
		/*
		 * If we don't have a path, "follow" is a no-op so make sure
		 * the variable is set to false to avoid needing to check
		 * both this and whether we have a path everywhere.
		 */
		ctx.qry.follow = 0;
	}

	if (commit_graph && !ctx.qry.follow) {
		argv_array_push(&rev_argv, "--graph");
		argv_array_push(&rev_argv, "--color");
		graph_set_column_colors(column_colors_html,
					COLUMN_COLORS_HTML_MAX);
	}

	if (commit_sort == 1)
		argv_array_push(&rev_argv, "--date-order");
	else if (commit_sort == 2)
		argv_array_push(&rev_argv, "--topo-order");

	if (path && ctx.qry.follow)
		argv_array_push(&rev_argv, "--follow");
	argv_array_push(&rev_argv, "--");
	if (path)
		argv_array_push(&rev_argv, path);

	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.commit_format = CMIT_FMT_DEFAULT;
	rev.verbose_header = 1;
	rev.show_root_diff = 0;
	rev.ignore_missing = 1;
	rev.simplify_history = 1;
	setup_revisions(rev_argv.argc, rev_argv.argv, &rev, NULL);
	load_ref_decorations(NULL, DECORATE_FULL_REFS);
	rev.show_decorations = 1;
	rev.grep_filter.ignore_case = 1;

	rev.diffopt.detect_rename = 1;
	rev.diffopt.rename_limit = ctx.cfg.renamelimit;
	if (ctx.qry.ignorews)
		DIFF_XDL_SET(&rev.diffopt, IGNORE_WHITESPACE);

	compile_grep_patterns(&rev.grep_filter);
	prepare_revision_walk(&rev);

	if (pager) {
		cgit_print_layout_start();
		html("<table class='list nowrap'>");
	}

	html("<tr class='nohover'>");
	if (commit_graph)
		html("<th></th>");
	else
		html("<th class='left'>Age</th>");
	html("<th class='left'>Commit message");
	if (pager) {
		html(" (");
		cgit_log_link(ctx.qry.showmsg ? "Collapse" : "Expand", NULL,
			      NULL, ctx.qry.head, ctx.qry.sha1,
			      ctx.qry.vpath, ctx.qry.ofs, ctx.qry.grep,
			      ctx.qry.search, ctx.qry.showmsg ? 0 : 1,
			      ctx.qry.follow);
		html(")");
	}
	html("</th><th class='left'>Author</th>");
	if (rev.graph)
		html("<th class='left'>Age</th>");
	if (ctx.repo->enable_log_filecount) {
		html("<th class='left'>Files</th>");
		columns++;
	}
	if (ctx.repo->enable_log_linecount) {
		html("<th class='left'>Lines</th>");
		columns++;
	}
	html("</tr>\n");

	if (ofs<0)
		ofs = 0;

	for (i = 0; i < ofs && (commit = get_revision(&rev)) != NULL; /* nop */) {
		if (show_commit(commit, &rev))
			i++;
		free_commit_buffer(commit);
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}

	for (i = 0; i < cnt && (commit = get_revision(&rev)) != NULL; /* nop */) {
		/*
		 * In "follow" mode, we must count the files and lines the
		 * first time we invoke diff on a given commit, and we need
		 * to do that to see if the commit touches the path we care
		 * about, so we do it in show_commit.  Hence we must clear
		 * lines_counted here.
		 *
		 * This has the side effect of avoiding running diff twice
		 * when we are both following renames and showing file
		 * and/or line counts.
		 */
		lines_counted = 0;
		if (show_commit(commit, &rev)) {
			i++;
			print_commit(commit, &rev);
		}
		free_commit_buffer(commit);
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}
	if (pager) {
		html("</table><ul class='pager'>");
		if (ofs > 0) {
			html("<li>");
			cgit_log_link("[prev]", NULL, NULL, ctx.qry.head,
				      ctx.qry.sha1, ctx.qry.vpath,
				      ofs - cnt, ctx.qry.grep,
				      ctx.qry.search, ctx.qry.showmsg,
				      ctx.qry.follow);
			html("</li>");
		}
		if ((commit = get_revision(&rev)) != NULL) {
			html("<li>");
			cgit_log_link("[next]", NULL, NULL, ctx.qry.head,
				      ctx.qry.sha1, ctx.qry.vpath,
				      ofs + cnt, ctx.qry.grep,
				      ctx.qry.search, ctx.qry.showmsg,
				      ctx.qry.follow);
			html("</li>");
		}
		html("</ul>");
		cgit_print_layout_end();
	} else if ((commit = get_revision(&rev)) != NULL) {
		htmlf("<tr class='nohover'><td colspan='%d'>", columns);
		cgit_log_link("[...]", NULL, NULL, ctx.qry.head, NULL,
			      ctx.qry.vpath, 0, NULL, NULL, ctx.qry.showmsg,
			      ctx.qry.follow);
		html("</td></tr>\n");
	}

	/* If we allocated tip then it is safe to cast away const. */
	if (must_free_tip)
		free((char*) tip);
}
