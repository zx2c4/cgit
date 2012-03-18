/* shared.c: global vars + some callback functions
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include <stdio.h>

struct cgit_repolist cgit_repolist;
struct cgit_context ctx;

int chk_zero(int result, char *msg)
{
	if (result != 0)
		die("%s: %s", msg, strerror(errno));
	return result;
}

int chk_positive(int result, char *msg)
{
	if (result <= 0)
		die("%s: %s", msg, strerror(errno));
	return result;
}

int chk_non_negative(int result, char *msg)
{
    	if (result < 0)
	    	die("%s: %s",msg, strerror(errno));
	return result;
}

struct cgit_repo *cgit_add_repo(const char *url)
{
	struct cgit_repo *ret;

	if (++cgit_repolist.count > cgit_repolist.length) {
		if (cgit_repolist.length == 0)
			cgit_repolist.length = 8;
		else
			cgit_repolist.length *= 2;
		cgit_repolist.repos = xrealloc(cgit_repolist.repos,
					       cgit_repolist.length *
					       sizeof(struct cgit_repo));
	}

	ret = &cgit_repolist.repos[cgit_repolist.count-1];
	memset(ret, 0, sizeof(struct cgit_repo));
	ret->url = trim_end(url, '/');
	ret->name = ret->url;
	ret->path = NULL;
	ret->desc = "[no description]";
	ret->owner = NULL;
	ret->section = ctx.cfg.section;
	ret->snapshots = ctx.cfg.snapshots;
	ret->enable_commit_graph = ctx.cfg.enable_commit_graph;
	ret->enable_log_filecount = ctx.cfg.enable_log_filecount;
	ret->enable_log_linecount = ctx.cfg.enable_log_linecount;
	ret->enable_remote_branches = ctx.cfg.enable_remote_branches;
	ret->enable_subject_links = ctx.cfg.enable_subject_links;
	ret->max_stats = ctx.cfg.max_stats;
	ret->module_link = ctx.cfg.module_link;
	ret->readme = ctx.cfg.readme;
	ret->mtime = -1;
	ret->about_filter = ctx.cfg.about_filter;
	ret->commit_filter = ctx.cfg.commit_filter;
	ret->source_filter = ctx.cfg.source_filter;
	ret->clone_url = ctx.cfg.clone_url;
	ret->submodules.strdup_strings = 1;
	return ret;
}

struct cgit_repo *cgit_get_repoinfo(const char *url)
{
	int i;
	struct cgit_repo *repo;

	for (i=0; i<cgit_repolist.count; i++) {
		repo = &cgit_repolist.repos[i];
		if (!strcmp(repo->url, url))
			return repo;
	}
	return NULL;
}

void *cgit_free_commitinfo(struct commitinfo *info)
{
	free(info->author);
	free(info->author_email);
	free(info->committer);
	free(info->committer_email);
	free(info->subject);
	free(info->msg);
	free(info->msg_encoding);
	free(info);
	return NULL;
}

char *trim_end(const char *str, char c)
{
	int len;

	if (str == NULL)
		return NULL;
	len = strlen(str);
	while(len > 0 && str[len - 1] == c)
		len--;
	if (len == 0)
		return NULL;
	return xstrndup(str, len);
}

char *strlpart(char *txt, int maxlen)
{
	char *result;

	if (!txt)
		return txt;

	if (strlen(txt) <= maxlen)
		return txt;
	result = xmalloc(maxlen + 1);
	memcpy(result, txt, maxlen - 3);
	result[maxlen-1] = result[maxlen-2] = result[maxlen-3] = '.';
	result[maxlen] = '\0';
	return result;
}

char *strrpart(char *txt, int maxlen)
{
	char *result;

	if (!txt)
		return txt;

	if (strlen(txt) <= maxlen)
		return txt;
	result = xmalloc(maxlen + 1);
	memcpy(result + 3, txt + strlen(txt) - maxlen + 4, maxlen - 3);
	result[0] = result[1] = result[2] = '.';
	return result;
}

void cgit_add_ref(struct reflist *list, struct refinfo *ref)
{
	size_t size;

	if (list->count >= list->alloc) {
		list->alloc += (list->alloc ? list->alloc : 4);
		size = list->alloc * sizeof(struct refinfo *);
		list->refs = xrealloc(list->refs, size);
	}
	list->refs[list->count++] = ref;
}

struct refinfo *cgit_mk_refinfo(const char *refname, const unsigned char *sha1)
{
	struct refinfo *ref;

	ref = xmalloc(sizeof (struct refinfo));
	ref->refname = xstrdup(refname);
	ref->object = parse_object(sha1);
	switch (ref->object->type) {
	case OBJ_TAG:
		ref->tag = cgit_parse_tag((struct tag *)ref->object);
		break;
	case OBJ_COMMIT:
		ref->commit = cgit_parse_commit((struct commit *)ref->object);
		break;
	}
	return ref;
}

int cgit_refs_cb(const char *refname, const unsigned char *sha1, int flags,
		  void *cb_data)
{
	struct reflist *list = (struct reflist *)cb_data;
	struct refinfo *info = cgit_mk_refinfo(refname, sha1);

	if (info)
		cgit_add_ref(list, info);
	return 0;
}

void cgit_diff_tree_cb(struct diff_queue_struct *q,
		       struct diff_options *options, void *data)
{
	int i;

	for (i = 0; i < q->nr; i++) {
		if (q->queue[i]->status == 'U')
			continue;
		((filepair_fn)data)(q->queue[i]);
	}
}

static int load_mmfile(mmfile_t *file, const unsigned char *sha1)
{
	enum object_type type;

	if (is_null_sha1(sha1)) {
		file->ptr = (char *)"";
		file->size = 0;
	} else {
		file->ptr = read_sha1_file(sha1, &type, 
		                           (unsigned long *)&file->size);
	}
	return 1;
}

/*
 * Receive diff-buffers from xdiff and concatenate them as
 * needed across multiple callbacks.
 *
 * This is basically a copy of xdiff-interface.c/xdiff_outf(),
 * ripped from git and modified to use globals instead of
 * a special callback-struct.
 */
char *diffbuf = NULL;
int buflen = 0;

int filediff_cb(void *priv, mmbuffer_t *mb, int nbuf)
{
	int i;

	for (i = 0; i < nbuf; i++) {
		if (mb[i].ptr[mb[i].size-1] != '\n') {
			/* Incomplete line */
			diffbuf = xrealloc(diffbuf, buflen + mb[i].size);
			memcpy(diffbuf + buflen, mb[i].ptr, mb[i].size);
			buflen += mb[i].size;
			continue;
		}

		/* we have a complete line */
		if (!diffbuf) {
			((linediff_fn)priv)(mb[i].ptr, mb[i].size);
			continue;
		}
		diffbuf = xrealloc(diffbuf, buflen + mb[i].size);
		memcpy(diffbuf + buflen, mb[i].ptr, mb[i].size);
		((linediff_fn)priv)(diffbuf, buflen + mb[i].size);
		free(diffbuf);
		diffbuf = NULL;
		buflen = 0;
	}
	if (diffbuf) {
		((linediff_fn)priv)(diffbuf, buflen);
		free(diffbuf);
		diffbuf = NULL;
		buflen = 0;
	}
	return 0;
}

int cgit_diff_files(const unsigned char *old_sha1,
		    const unsigned char *new_sha1, unsigned long *old_size,
		    unsigned long *new_size, int *binary, int context,
		    int ignorews, linediff_fn fn)
{
	mmfile_t file1, file2;
	xpparam_t diff_params;
	xdemitconf_t emit_params;
	xdemitcb_t emit_cb;

	if (!load_mmfile(&file1, old_sha1) || !load_mmfile(&file2, new_sha1))
		return 1;

	*old_size = file1.size;
	*new_size = file2.size;

	if ((file1.ptr && buffer_is_binary(file1.ptr, file1.size)) ||
	    (file2.ptr && buffer_is_binary(file2.ptr, file2.size))) {
		*binary = 1;
		if (file1.size)
			free(file1.ptr);
		if (file2.size)
			free(file2.ptr);
		return 0;
	}

	memset(&diff_params, 0, sizeof(diff_params));
	memset(&emit_params, 0, sizeof(emit_params));
	memset(&emit_cb, 0, sizeof(emit_cb));
	diff_params.flags = XDF_NEED_MINIMAL;
	if (ignorews)
		diff_params.flags |= XDF_IGNORE_WHITESPACE;
	emit_params.ctxlen = context > 0 ? context : 3;
	emit_params.flags = XDL_EMIT_FUNCNAMES;
	emit_cb.outf = filediff_cb;
	emit_cb.priv = fn;
	xdl_diff(&file1, &file2, &diff_params, &emit_params, &emit_cb);
	if (file1.size)
		free(file1.ptr);
	if (file2.size)
		free(file2.ptr);
	return 0;
}

void cgit_diff_tree(const unsigned char *old_sha1,
		    const unsigned char *new_sha1,
		    filepair_fn fn, const char *prefix, int ignorews)
{
	struct diff_options opt;
	int prefixlen;

	diff_setup(&opt);
	opt.output_format = DIFF_FORMAT_CALLBACK;
	opt.detect_rename = 1;
	opt.rename_limit = ctx.cfg.renamelimit;
	DIFF_OPT_SET(&opt, RECURSIVE);
	if (ignorews)
		DIFF_XDL_SET(&opt, IGNORE_WHITESPACE);
	opt.format_callback = cgit_diff_tree_cb;
	opt.format_callback_data = fn;
	if (prefix) {
		opt.nr_paths = 1;
		opt.paths = &prefix;
		prefixlen = strlen(prefix);
		opt.pathlens = &prefixlen;
	}
	diff_setup_done(&opt);

	if (old_sha1 && !is_null_sha1(old_sha1))
		diff_tree_sha1(old_sha1, new_sha1, "", &opt);
	else
		diff_root_tree_sha1(new_sha1, "", &opt);
	diffcore_std(&opt);
	diff_flush(&opt);
}

void cgit_diff_commit(struct commit *commit, filepair_fn fn, const char *prefix)
{
	unsigned char *old_sha1 = NULL;

	if (commit->parents)
		old_sha1 = commit->parents->item->object.sha1;
	cgit_diff_tree(old_sha1, commit->object.sha1, fn, prefix,
		       ctx.qry.ignorews);
}

int cgit_parse_snapshots_mask(const char *str)
{
	const struct cgit_snapshot_format *f;
	static const char *delim = " \t,:/|;";
	int tl, sl, rv = 0;

	/* favor legacy setting */
	if(atoi(str))
		return 1;
	for(;;) {
		str += strspn(str,delim);
		tl = strcspn(str,delim);
		if (!tl)
			break;
		for (f = cgit_snapshot_formats; f->suffix; f++) {
			sl = strlen(f->suffix);
			if((tl == sl && !strncmp(f->suffix, str, tl)) ||
			   (tl == sl-1 && !strncmp(f->suffix+1, str, tl-1))) {
				rv |= f->bit;
				break;
			}
		}
		str += tl;
	}
	return rv;
}

typedef struct {
	char * name;
	char * value;
} cgit_env_var;

void cgit_prepare_repo_env(struct cgit_repo * repo)
{
	cgit_env_var env_vars[] = {
		{ .name = "CGIT_REPO_URL", .value = repo->url },
		{ .name = "CGIT_REPO_NAME", .value = repo->name },
		{ .name = "CGIT_REPO_PATH", .value = repo->path },
		{ .name = "CGIT_REPO_OWNER", .value = repo->owner },
		{ .name = "CGIT_REPO_DEFBRANCH", .value = repo->defbranch },
		{ .name = "CGIT_REPO_SECTION", .value = repo->section },
		{ .name = "CGIT_REPO_CLONE_URL", .value = repo->clone_url }
	};
	int env_var_count = ARRAY_SIZE(env_vars);
	cgit_env_var *p, *q;
	static char *warn = "cgit warning: failed to set env: %s=%s\n";

	p = env_vars;
	q = p + env_var_count;
	for (; p < q; p++)
		if (p->value && setenv(p->name, p->value, 1))
			fprintf(stderr, warn, p->name, p->value);
}

int cgit_open_filter(struct cgit_filter *filter)
{

	filter->old_stdout = chk_positive(dup(STDOUT_FILENO),
		"Unable to duplicate STDOUT");
	chk_zero(pipe(filter->pipe_fh), "Unable to create pipe to subprocess");
	filter->pid = chk_non_negative(fork(), "Unable to create subprocess");
	if (filter->pid == 0) {
		close(filter->pipe_fh[1]);
		chk_non_negative(dup2(filter->pipe_fh[0], STDIN_FILENO),
			"Unable to use pipe as STDIN");
		execvp(filter->cmd, filter->argv);
		die("Unable to exec subprocess %s: %s (%d)", filter->cmd,
			strerror(errno), errno);
	}
	close(filter->pipe_fh[0]);
	chk_non_negative(dup2(filter->pipe_fh[1], STDOUT_FILENO),
		"Unable to use pipe as STDOUT");
	close(filter->pipe_fh[1]);
	return 0;
}

int cgit_close_filter(struct cgit_filter *filter)
{
	chk_non_negative(dup2(filter->old_stdout, STDOUT_FILENO),
		"Unable to restore STDOUT");
	close(filter->old_stdout);
	if (filter->pid < 0)
		return 0;
	waitpid(filter->pid, &filter->exitstatus, 0);
	if (WIFEXITED(filter->exitstatus) && !WEXITSTATUS(filter->exitstatus))
		return 0;
	die("Subprocess %s exited abnormally", filter->cmd);
}

/* Read the content of the specified file into a newly allocated buffer,
 * zeroterminate the buffer and return 0 on success, errno otherwise.
 */
int readfile(const char *path, char **buf, size_t *size)
{
	int fd, e;
	struct stat st;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return errno;
	if (fstat(fd, &st)) {
		e = errno;
		close(fd);
		return e;
	}
	if (!S_ISREG(st.st_mode)) {
		close(fd);
		return EISDIR;
	}
	*buf = xmalloc(st.st_size + 1);
	*size = read_in_full(fd, *buf, st.st_size);
	e = errno;
	(*buf)[*size] = '\0';
	close(fd);
	return (*size == st.st_size ? 0 : e);
}

int is_token_char(char c)
{
	return isalnum(c) || c == '_';
}

/* Replace name with getenv(name), return pointer to zero-terminating char
 */
char *expand_macro(char *name, int maxlength)
{
	char *value;
	int len;

	len = 0;
	value = getenv(name);
	if (value) {
		len = strlen(value);
		if (len > maxlength)
			len = maxlength;
		strncpy(name, value, len);
	}
	return name + len;
}

#define EXPBUFSIZE (1024 * 8)

/* Replace all tokens prefixed by '$' in the specified text with the
 * value of the named environment variable.
 * NB: the return value is a static buffer, i.e. it must be strdup'd
 * by the caller.
 */
char *expand_macros(const char *txt)
{
	static char result[EXPBUFSIZE];
	char *p, *start;
	int len;

	p = result;
	start = NULL;
	while (p < result + EXPBUFSIZE - 1 && txt && *txt) {
		*p = *txt;
		if (start) {
			if (!is_token_char(*txt)) {
				if (p - start > 0) {
					*p = '\0';
					len = result + EXPBUFSIZE - start - 1;
					p = expand_macro(start, len) - 1;
				}
				start = NULL;
				txt--;
			}
			p++;
			txt++;
			continue;
		}
		if (*txt == '$') {
			start = p;
			txt++;
			continue;
		}
		p++;
		txt++;
	}
	*p = '\0';
	if (start && p - start > 0) {
		len = result + EXPBUFSIZE - start - 1;
		p = expand_macro(start, len);
		*p = '\0';
	}
	return result;
}
