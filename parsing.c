/* config.c: parsing of config files
 *
 * Copyright (C) 2006 Lars Hjemli
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
	char *cmd, *p;

	ctx.repo = NULL;
	if (!url || url[0] == '\0')
		return;

	ctx.repo = cgit_get_repoinfo(url);
	if (ctx.repo) {
		ctx.qry.repo = ctx.repo->url;
		return;
	}

	cmd = strchr(url, '/');
	while (!ctx.repo && cmd) {
		cmd[0] = '\0';
		ctx.repo = cgit_get_repoinfo(url);
		if (ctx.repo == NULL) {
			cmd[0] = '/';
			cmd = strchr(cmd + 1, '/');
			continue;
		}

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

char *substr(const char *head, const char *tail)
{
	char *buf;

	buf = xmalloc(tail - head + 1);
	strncpy(buf, head, tail - head);
	buf[tail - head] = '\0';
	return buf;
}

struct commitinfo *cgit_parse_commit(struct commit *commit)
{
	struct commitinfo *ret;
	char *p = commit->buffer, *t = commit->buffer;

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

	if (strncmp(p, "tree ", 5))
		die("Bad commit: %s", sha1_to_hex(commit->object.sha1));
	else
		p += 46; // "tree " + hex[40] + "\n"

	while (!strncmp(p, "parent ", 7))
		p += 48; // "parent " + hex[40] + "\n"

	if (!strncmp(p, "author ", 7)) {
		p += 7;
		t = strchr(p, '<') - 1;
		ret->author = substr(p, t);
		p = t;
		t = strchr(t, '>') + 1;
		ret->author_email = substr(p, t);
		ret->author_date = atol(t+1);
		p = strchr(t, '\n') + 1;
	}

	if (!strncmp(p, "committer ", 9)) {
		p += 9;
		t = strchr(p, '<') - 1;
		ret->committer = substr(p, t);
		p = t;
		t = strchr(t, '>') + 1;
		ret->committer_email = substr(p, t);
		ret->committer_date = atol(t+1);
		p = strchr(t, '\n') + 1;
	}

	if (!strncmp(p, "encoding ", 9)) {
		p += 9;
		t = strchr(p, '\n') + 1;
		ret->msg_encoding = substr(p, t);
		p = t;
	} else
		ret->msg_encoding = xstrdup(PAGE_ENCODING);

	while (*p && (*p != '\n'))
		p = strchr(p, '\n') + 1; // skip unknown header fields

	while (*p == '\n')
		p = strchr(p, '\n') + 1;

	t = strchr(p, '\n');
	if (t) {
		if (*t == '\0')
			ret->subject = "** empty **";
		else
			ret->subject = substr(p, t);
		p = t + 1;

		while (*p == '\n')
			p = strchr(p, '\n') + 1;
		ret->msg = xstrdup(p);
	} else
		ret->subject = substr(p, p+strlen(p));

	if(strcmp(ret->msg_encoding, PAGE_ENCODING)) {
		t = reencode_string(ret->subject, PAGE_ENCODING,
				    ret->msg_encoding);
		if(t) {
			free(ret->subject);
			ret->subject = t;
		}

		t = reencode_string(ret->msg, PAGE_ENCODING,
				    ret->msg_encoding);
		if(t) {
			free(ret->msg);
			ret->msg = t;
		}
	}

	return ret;
}


struct taginfo *cgit_parse_tag(struct tag *tag)
{
	void *data;
	enum object_type type;
	unsigned long size;
	char *p, *t;
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

		if (!strncmp(p, "tagger ", 7)) {
			p += 7;
			t = strchr(p, '<') - 1;
			ret->tagger = substr(p, t);
			p = t;
			t = strchr(t, '>') + 1;
			ret->tagger_email = substr(p, t);
			ret->tagger_date = atol(t+1);
		}
		p = strchr(p, '\n') + 1;
	}

	while (p && *p && (*p != '\n'))
		p = strchr(p, '\n') + 1; // skip unknown tag fields

	while (p && (*p == '\n'))
		p = strchr(p, '\n') + 1;
	if (p && *p)
		ret->msg = xstrdup(p);
	free(data);
	return ret;
}
