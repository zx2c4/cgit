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

	ctx.repo = NULL;
	if (!url || url[0] == '\0')
		return;

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
		return;
	}
}

static char *substr(const char *head, const char *tail)
{
	char *buf;

	if (tail < head)
		return xstrdup("");
	buf = xmalloc(tail - head + 1);
	strncpy(buf, head, tail - head);
	buf[tail - head] = '\0';
	return buf;
}

static const char *parse_user(const char *t, char **name, char **email, unsigned long *date)
{
	const char *p = t;
	int mode = 1;

	while (p && *p) {
		if (mode == 1 && *p == '<') {
			*name = substr(t, p - 1);
			t = p;
			mode++;
		} else if (mode == 1 && *p == '\n') {
			*name = substr(t, p);
			p++;
			break;
		} else if (mode == 2 && *p == '>') {
			*email = substr(t, p + 1);
			t = p;
			mode++;
		} else if (mode == 2 && *p == '\n') {
			*email = substr(t, p);
			p++;
			break;
		} else if (mode == 3 && isdigit(*p)) {
			*date = atol(p);
			mode++;
		} else if (*p == '\n') {
			p++;
			break;
		}
		p++;
	}
	return p;
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

struct commitinfo *cgit_parse_commit(struct commit *commit)
{
	struct commitinfo *ret;
	const char *p = get_cached_commit_buffer(commit, NULL);
	const char *t;

	ret = xmalloc(sizeof(*ret));
	ret->commit = commit;
	ret->author = NULL;
	ret->author_email = NULL;
	ret->committer = NULL;
	ret->committer_email = NULL;
	ret->subject = NULL;
	ret->msg = NULL;
	ret->msg_encoding = NULL;

	if (p == NULL)
		return ret;

	if (!starts_with(p, "tree "))
		die("Bad commit: %s", sha1_to_hex(commit->object.sha1));
	else
		p += 46; // "tree " + hex[40] + "\n"

	while (starts_with(p, "parent "))
		p += 48; // "parent " + hex[40] + "\n"

	if (p && starts_with(p, "author ")) {
		p = parse_user(p + 7, &ret->author, &ret->author_email,
			&ret->author_date);
	}

	if (p && starts_with(p, "committer ")) {
		p = parse_user(p + 10, &ret->committer, &ret->committer_email,
			&ret->committer_date);
	}

	if (p && starts_with(p, "encoding ")) {
		p += 9;
		t = strchr(p, '\n');
		if (t) {
			ret->msg_encoding = substr(p, t + 1);
			p = t + 1;
		}
	}

	/* if no special encoding is found, assume UTF-8 */
	if (!ret->msg_encoding)
		ret->msg_encoding = xstrdup("UTF-8");

	// skip unknown header fields
	while (p && *p && (*p != '\n')) {
		p = strchr(p, '\n');
		if (p)
			p++;
	}

	// skip empty lines between headers and message
	while (p && *p == '\n')
		p++;

	if (!p)
		return ret;

	t = strchr(p, '\n');
	if (t) {
		ret->subject = substr(p, t);
		p = t + 1;

		while (p && *p == '\n') {
			p = strchr(p, '\n');
			if (p)
				p++;
		}
		if (p)
			ret->msg = xstrdup(p);
	} else
		ret->subject = xstrdup(p);

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
	struct taginfo *ret;

	data = read_sha1_file(tag->object.sha1, &type, &size);
	if (!data || type != OBJ_TAG) {
		free(data);
		return 0;
	}

	ret = xmalloc(sizeof(*ret));
	ret->tagger = NULL;
	ret->tagger_email = NULL;
	ret->tagger_date = 0;
	ret->msg = NULL;

	p = data;

	while (p && *p) {
		if (*p == '\n')
			break;

		if (starts_with(p, "tagger ")) {
			p = parse_user(p + 7, &ret->tagger, &ret->tagger_email,
				&ret->tagger_date);
		} else {
			p = strchr(p, '\n');
			if (p)
				p++;
		}
	}

	// skip empty lines between headers and message
	while (p && *p == '\n')
		p++;

	if (p && *p)
		ret->msg = xstrdup(p);
	free(data);
	return ret;
}
