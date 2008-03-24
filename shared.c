/* shared.c: global vars + some callback functions
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

struct cgit_repolist cgit_repolist;
struct cgit_context ctx;
int cgit_cmd;

const char *cgit_version = CGIT_VERSION;

void cgit_prepare_context(struct cgit_context *ctx)
{
	memset(ctx, 0, sizeof(ctx));
	ctx->cfg.agefile = "info/web/last-modified";
	ctx->cfg.cache_dynamic_ttl = 5;
	ctx->cfg.cache_max_create_time = 5;
	ctx->cfg.cache_repo_ttl = 5;
	ctx->cfg.cache_root = CGIT_CACHE_ROOT;
	ctx->cfg.cache_root_ttl = 5;
	ctx->cfg.cache_static_ttl = -1;
	ctx->cfg.css = "/cgit.css";
	ctx->cfg.logo = "/git-logo.png";
	ctx->cfg.max_commit_count = 50;
	ctx->cfg.max_lock_attempts = 5;
	ctx->cfg.max_msg_len = 60;
	ctx->cfg.max_repodesc_len = 60;
	ctx->cfg.module_link = "./?repo=%s&page=commit&id=%s";
	ctx->cfg.renamelimit = -1;
	ctx->cfg.robots = "index, nofollow";
	ctx->cfg.root_title = "Git repository browser";
	ctx->cfg.script_name = CGIT_SCRIPT_NAME;
	ctx->page.mimetype = "text/html";
	ctx->page.charset = PAGE_ENCODING;
	ctx->page.filename = NULL;
}

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

struct cgit_repo *add_repo(const char *url)
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
	ret->url = trim_end(url, '/');
	ret->name = ret->url;
	ret->path = NULL;
	ret->desc = "[no description]";
	ret->owner = NULL;
	ret->group = ctx.cfg.repo_group;
	ret->defbranch = "master";
	ret->snapshots = ctx.cfg.snapshots;
	ret->enable_log_filecount = ctx.cfg.enable_log_filecount;
	ret->enable_log_linecount = ctx.cfg.enable_log_linecount;
	ret->module_link = ctx.cfg.module_link;
	ret->readme = NULL;
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

void cgit_global_config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "root-title"))
		ctx.cfg.root_title = xstrdup(value);
	else if (!strcmp(name, "css"))
		ctx.cfg.css = xstrdup(value);
	else if (!strcmp(name, "logo"))
		ctx.cfg.logo = xstrdup(value);
	else if (!strcmp(name, "index-header"))
		ctx.cfg.index_header = xstrdup(value);
	else if (!strcmp(name, "index-info"))
		ctx.cfg.index_info = xstrdup(value);
	else if (!strcmp(name, "logo-link"))
		ctx.cfg.logo_link = xstrdup(value);
	else if (!strcmp(name, "module-link"))
		ctx.cfg.module_link = xstrdup(value);
	else if (!strcmp(name, "virtual-root")) {
		ctx.cfg.virtual_root = trim_end(value, '/');
		if (!ctx.cfg.virtual_root && (!strcmp(value, "/")))
			ctx.cfg.virtual_root = "";
	} else if (!strcmp(name, "nocache"))
		ctx.cfg.nocache = atoi(value);
	else if (!strcmp(name, "snapshots"))
		ctx.cfg.snapshots = cgit_parse_snapshots_mask(value);
	else if (!strcmp(name, "enable-index-links"))
		ctx.cfg.enable_index_links = atoi(value);
	else if (!strcmp(name, "enable-log-filecount"))
		ctx.cfg.enable_log_filecount = atoi(value);
	else if (!strcmp(name, "enable-log-linecount"))
		ctx.cfg.enable_log_linecount = atoi(value);
	else if (!strcmp(name, "cache-root"))
		ctx.cfg.cache_root = xstrdup(value);
	else if (!strcmp(name, "cache-root-ttl"))
		ctx.cfg.cache_root_ttl = atoi(value);
	else if (!strcmp(name, "cache-repo-ttl"))
		ctx.cfg.cache_repo_ttl = atoi(value);
	else if (!strcmp(name, "cache-static-ttl"))
		ctx.cfg.cache_static_ttl = atoi(value);
	else if (!strcmp(name, "cache-dynamic-ttl"))
		ctx.cfg.cache_dynamic_ttl = atoi(value);
	else if (!strcmp(name, "max-message-length"))
		ctx.cfg.max_msg_len = atoi(value);
	else if (!strcmp(name, "max-repodesc-length"))
		ctx.cfg.max_repodesc_len = atoi(value);
	else if (!strcmp(name, "max-commit-count"))
		ctx.cfg.max_commit_count = atoi(value);
	else if (!strcmp(name, "summary-log"))
		ctx.cfg.summary_log = atoi(value);
	else if (!strcmp(name, "summary-branches"))
		ctx.cfg.summary_branches = atoi(value);
	else if (!strcmp(name, "summary-tags"))
		ctx.cfg.summary_tags = atoi(value);
	else if (!strcmp(name, "agefile"))
		ctx.cfg.agefile = xstrdup(value);
	else if (!strcmp(name, "renamelimit"))
		ctx.cfg.renamelimit = atoi(value);
	else if (!strcmp(name, "robots"))
		ctx.cfg.robots = xstrdup(value);
	else if (!strcmp(name, "clone-prefix"))
		ctx.cfg.clone_prefix = xstrdup(value);
	else if (!strcmp(name, "repo.group"))
		ctx.cfg.repo_group = xstrdup(value);
	else if (!strcmp(name, "repo.url"))
		ctx.repo = add_repo(value);
	else if (!strcmp(name, "repo.name"))
		ctx.repo->name = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.path"))
		ctx.repo->path = trim_end(value, '/');
	else if (ctx.repo && !strcmp(name, "repo.clone-url"))
		ctx.repo->clone_url = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.desc"))
		ctx.repo->desc = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.owner"))
		ctx.repo->owner = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.defbranch"))
		ctx.repo->defbranch = xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.snapshots"))
		ctx.repo->snapshots = ctx.cfg.snapshots & cgit_parse_snapshots_mask(value); /* XXX: &? */
	else if (ctx.repo && !strcmp(name, "repo.enable-log-filecount"))
		ctx.repo->enable_log_filecount = ctx.cfg.enable_log_filecount * atoi(value);
	else if (ctx.repo && !strcmp(name, "repo.enable-log-linecount"))
		ctx.repo->enable_log_linecount = ctx.cfg.enable_log_linecount * atoi(value);
	else if (ctx.repo && !strcmp(name, "repo.module-link"))
		ctx.repo->module_link= xstrdup(value);
	else if (ctx.repo && !strcmp(name, "repo.readme") && value != NULL) {
		if (*value == '/')
			ctx.repo->readme = xstrdup(value);
		else
			ctx.repo->readme = xstrdup(fmt("%s/%s", ctx.repo->path, value));
	} else if (!strcmp(name, "include"))
		cgit_read_config(value, cgit_global_config_cb);
}

void cgit_querystring_cb(const char *name, const char *value)
{
	if (!strcmp(name,"r")) {
		ctx.qry.repo = xstrdup(value);
		ctx.repo = cgit_get_repoinfo(value);
	} else if (!strcmp(name, "p")) {
		ctx.qry.page = xstrdup(value);
	} else if (!strcmp(name, "url")) {
		cgit_parse_url(value);
	} else if (!strcmp(name, "qt")) {
		ctx.qry.grep = xstrdup(value);
	} else if (!strcmp(name, "q")) {
		ctx.qry.search = xstrdup(value);
	} else if (!strcmp(name, "h")) {
		ctx.qry.head = xstrdup(value);
		ctx.qry.has_symref = 1;
	} else if (!strcmp(name, "id")) {
		ctx.qry.sha1 = xstrdup(value);
		ctx.qry.has_sha1 = 1;
	} else if (!strcmp(name, "id2")) {
		ctx.qry.sha2 = xstrdup(value);
		ctx.qry.has_sha1 = 1;
	} else if (!strcmp(name, "ofs")) {
		ctx.qry.ofs = atoi(value);
	} else if (!strcmp(name, "path")) {
		ctx.qry.path = trim_end(value, '/');
	} else if (!strcmp(name, "name")) {
		ctx.qry.name = xstrdup(value);
	}
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

int hextoint(char c)
{
	if (c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	else if (c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	else if (c >= '0' && c <= '9')
		return c - '0';
	else
		return -1;
}

char *trim_end(const char *str, char c)
{
	int len;
	char *s, *t;

	if (str == NULL)
		return NULL;
	t = (char *)str;
	len = strlen(t);
	while(len > 0 && t[len - 1] == c)
		len--;

	if (len == 0)
		return NULL;

	c = t[len];
	t[len] = '\0';
	s = xstrdup(t);
	t[len] = c;
	return s;
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
		     const unsigned char *new_sha1,
		     linediff_fn fn)
{
	mmfile_t file1, file2;
	xpparam_t diff_params;
	xdemitconf_t emit_params;
	xdemitcb_t emit_cb;

	if (!load_mmfile(&file1, old_sha1) || !load_mmfile(&file2, new_sha1))
		return 1;

	diff_params.flags = XDF_NEED_MINIMAL;
	emit_params.ctxlen = 3;
	emit_params.flags = XDL_EMIT_FUNCNAMES;
	emit_params.find_func = NULL;
	emit_cb.outf = filediff_cb;
	emit_cb.priv = fn;
	xdl_diff(&file1, &file2, &diff_params, &emit_params, &emit_cb);
	return 0;
}

void cgit_diff_tree(const unsigned char *old_sha1,
		    const unsigned char *new_sha1,
		    filepair_fn fn, const char *prefix)
{
	struct diff_options opt;
	int ret;
	int prefixlen;

	diff_setup(&opt);
	opt.output_format = DIFF_FORMAT_CALLBACK;
	opt.detect_rename = 1;
	opt.rename_limit = ctx.cfg.renamelimit;
	DIFF_OPT_SET(&opt, RECURSIVE);
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
		ret = diff_tree_sha1(old_sha1, new_sha1, "", &opt);
	else
		ret = diff_root_tree_sha1(new_sha1, "", &opt);
	diffcore_std(&opt);
	diff_flush(&opt);
}

void cgit_diff_commit(struct commit *commit, filepair_fn fn)
{
	unsigned char *old_sha1 = NULL;

	if (commit->parents)
		old_sha1 = commit->parents->item->object.sha1;
	cgit_diff_tree(old_sha1, commit->object.sha1, fn, NULL);
}
