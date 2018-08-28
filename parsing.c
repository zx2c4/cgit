/* parsing.c: parsing of config files
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

/*
 * url syntax: [repo ['/' cmd [ '/' path]]]
 *   repo: any valid repo url, may contain '/'
 *   cmd:  log | commit | diff | tree | view | blob | snapshot
 *   path: any valid path, may contain '/'
 *
 */
void cgit_parse_url(const char *url)
{
	char *c, *cmd, *p;
	struct cgit_repo *repo;

	if (!url || url[0] == '\0')
		return;

	ctx.qry.page = NULL;
	ctx.repo = cgit_get_repoinfo(url);
	if (ctx.repo) {
		ctx.qry.repo = ctx.repo->url;
		return;
	}

	cmd = NULL;
	c = strchr(url, '/');
	while (c) {
		c[0] = '\0';
		repo = cgit_get_repoinfo(url);
		if (repo) {
			ctx.repo = repo;
			cmd = c;
		}
		c[0] = '/';
		c = strchr(c + 1, '/');
	}

	if (ctx.repo) {
		ctx.qry.repo = ctx.repo->url;
		p = strchr(cmd + 1, '/');
		if (p) {
			p[0] = '\0';
			if (p[1])
				ctx.qry.path = trim_end(p + 1, '/');
		}
		if (cmd[1])
			ctx.qry.page = xstrdup(cmd + 1);
	}
}

static char *substr(const char *head, const char *tail)
{
	char *buf;

	if (tail < head)
		return xstrdup("");
	buf = xmalloc(tail - head + 1);
	strlcpy(buf, head, tail - head + 1);
	return buf;
}

static void parse_user(const char *t, char **name, char **email, unsigned long *date, int *tz)
{
	struct ident_split ident;
	unsigned email_len;

	if (!split_ident_line(&ident, t, strchrnul(t, '\n') - t)) {
		*name = substr(ident.name_begin, ident.name_end);

		email_len = ident.mail_end - ident.mail_begin;
		*email = xmalloc(strlen("<") + email_len + strlen(">") + 1);
		xsnprintf(*email, email_len + 3, "<%.*s>", email_len, ident.mail_begin);

		if (ident.date_begin)
			*date = strtoul(ident.date_begin, NULL, 10);
		if (ident.tz_begin)
			*tz = atoi(ident.tz_begin);
	}
}

#ifdef NO_ICONV
#define reencode(a, b, c)
#else
static const char *reencode(char **txt, const char *src_enc, const char *dst_enc)
{
	char *tmp;

	if (!txt)
		return NULL;

	if (!*txt || !src_enc || !dst_enc)
		return *txt;

	/* no encoding needed if src_enc equals dst_enc */
	if (!strcasecmp(src_enc, dst_enc))
		return *txt;

	tmp = reencode_string(*txt, dst_enc, src_enc);
	if (tmp) {
		free(*txt);
		*txt = tmp;
	}
	return *txt;
}
#endif

static const char *next_header_line(const char *p)
{
	p = strchr(p, '\n');
	if (!p)
		return NULL;
	return p + 1;
}

static int end_of_header(const char *p)
{
	return !p || (*p == '\n');
}

struct commitinfo *cgit_parse_commit(struct commit *commit)
{
	const int sha1hex_len = 40;
	struct commitinfo *ret;
	const char *p = get_cached_commit_buffer(commit, NULL);
	const char *t;

	ret = xcalloc(1, sizeof(struct commitinfo));
	ret->commit = commit;

	if (!p)
		return ret;

	if (!skip_prefix(p, "tree ", &p))
		die("Bad commit: %s", oid_to_hex(&commit->object.oid));
	p += sha1hex_len + 1;

	while (skip_prefix(p, "parent ", &p))
		p += sha1hex_len + 1;

	if (p && skip_prefix(p, "author ", &p)) {
		parse_user(p, &ret->author, &ret->author_email,
			&ret->author_date, &ret->author_tz);
		p = next_header_line(p);
	}

	if (p && skip_prefix(p, "committer ", &p)) {
		parse_user(p, &ret->committer, &ret->committer_email,
			&ret->committer_date, &ret->committer_tz);
		p = next_header_line(p);
	}

	if (p && skip_prefix(p, "encoding ", &p)) {
		t = strchr(p, '\n');
		if (t) {
			ret->msg_encoding = substr(p, t + 1);
			p = t + 1;
		}
	}

	if (!ret->msg_encoding)
		ret->msg_encoding = xstrdup("UTF-8");

	while (!end_of_header(p))
		p = next_header_line(p);
	while (p && *p == '\n')
		p++;
	if (!p)
		return ret;

	t = strchrnul(p, '\n');
	ret->subject = substr(p, t);
	while (*t == '\n')
		t++;
	ret->msg = xstrdup(t);

	reencode(&ret->author, ret->msg_encoding, PAGE_ENCODING);
	reencode(&ret->author_email, ret->msg_encoding, PAGE_ENCODING);
	reencode(&ret->committer, ret->msg_encoding, PAGE_ENCODING);
	reencode(&ret->committer_email, ret->msg_encoding, PAGE_ENCODING);
	reencode(&ret->subject, ret->msg_encoding, PAGE_ENCODING);
	reencode(&ret->msg, ret->msg_encoding, PAGE_ENCODING);

	return ret;
}

struct taginfo *cgit_parse_tag(struct tag *tag)
{
	void *data;
	enum object_type type;
	unsigned long size;
	const char *p;
	struct taginfo *ret = NULL;

	data = read_object_file(&tag->object.oid, &type, &size);
	if (!data || type != OBJ_TAG)
		goto cleanup;

	ret = xcalloc(1, sizeof(struct taginfo));

	for (p = data; !end_of_header(p); p = next_header_line(p)) {
		if (skip_prefix(p, "tagger ", &p)) {
			parse_user(p, &ret->tagger, &ret->tagger_email,
				&ret->tagger_date, &ret->tagger_tz);
		}
	}

	while (p && *p == '\n')
		p++;

	if (p && *p)
		ret->msg = xstrdup(p);

cleanup:
	free(data);
	return ret;
}
