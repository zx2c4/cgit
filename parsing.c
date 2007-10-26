/* config.c: parsing of config files
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include <iconv.h>

#include "cgit.h"

int next_char(FILE *f)
{
	int c = fgetc(f);
	if (c=='\r') {
		c = fgetc(f);
		if (c!='\n') {
			ungetc(c, f);
			c = '\r';
		}
	}
	return c;
}

void skip_line(FILE *f)
{
	int c;

	while((c=next_char(f)) && c!='\n' && c!=EOF)
		;
}

int read_config_line(FILE *f, char *line, const char **value, int bufsize)
{
	int i = 0, isname = 0;

	*value = NULL;
	while(i<bufsize-1) {
		int c = next_char(f);
		if (!isname && (c=='#' || c==';')) {
			skip_line(f);
			continue;
		}
		if (!isname && isspace(c))
			continue;

		if (c=='=' && !*value) {
			line[i] = 0;
			*value = &line[i+1];
		} else if (c=='\n' && !isname) {
			i = 0;
			continue;
		} else if (c=='\n' || c==EOF) {
			line[i] = 0;
			break;
		} else {
			line[i]=c;
		}
		isname = 1;
		i++;
	}
	line[i+1] = 0;
	return i;
}

int cgit_read_config(const char *filename, configfn fn)
{
	static int nesting;
	int len;
	char line[256];
	const char *value;
	FILE *f;

	/* cancel deeply nested include-commands */
	if (nesting > 8)
		return -1;
	if (!(f = fopen(filename, "r")))
		return -1;
	nesting++;
	while((len = read_config_line(f, line, &value, sizeof(line))) > 0)
		(*fn)(line, value);
	nesting--;
	fclose(f);
	return 0;
}

char *convert_query_hexchar(char *txt)
{
	int d1, d2;
	if (strlen(txt) < 3) {
		*txt = '\0';
		return txt-1;
	}
	d1 = hextoint(*(txt+1));
	d2 = hextoint(*(txt+2));
	if (d1<0 || d2<0) {
		strcpy(txt, txt+3);
		return txt-1;
	} else {
		*txt = d1 * 16 + d2;
		strcpy(txt+1, txt+3);
		return txt;
	}
}

int cgit_parse_query(char *txt, configfn fn)
{
	char *t, *value = NULL, c;

	if (!txt)
		return 0;

	t = txt = xstrdup(txt);

	while((c=*t) != '\0') {
		if (c=='=') {
			*t = '\0';
			value = t+1;
		} else if (c=='+') {
			*t = ' ';
		} else if (c=='%') {
			t = convert_query_hexchar(t);
		} else if (c=='&') {
			*t = '\0';
			(*fn)(txt, value);
			txt = t+1;
			value = NULL;
		}
		t++;
	}
	if (t!=txt)
		(*fn)(txt, value);
	return 0;
}

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

	cgit_repo = NULL;
	if (!url || url[0] == '\0')
		return;

	cgit_repo = cgit_get_repoinfo(url);
	if (cgit_repo) {
		cgit_query_repo = cgit_repo->url;
		return;
	}

	cmd = strchr(url, '/');
	while (!cgit_repo && cmd) {
		cmd[0] = '\0';
		cgit_repo = cgit_get_repoinfo(url);
		if (cgit_repo == NULL) {
			cmd[0] = '/';
			cmd = strchr(cmd + 1, '/');
			continue;
		}

		cgit_query_repo = cgit_repo->url;
		p = strchr(cmd + 1, '/');
		if (p) {
			p[0] = '\0';
			if (p[1])
				cgit_query_path = trim_end(p + 1, '/');
		}
		cgit_cmd = cgit_get_cmd_index(cmd + 1);
		cgit_query_page = xstrdup(cmd + 1);
		return;
	}
}

static char *iconv_msg(char *msg, const char *encoding)
{
	iconv_t msg_conv = iconv_open(PAGE_ENCODING, encoding);
	size_t inlen = strlen(msg);
	char *in;
	char *out;
	size_t inleft;
	size_t outleft;
	char *buf;
	char *ret;
	size_t buf_sz;
	int again, fail;

	if(msg_conv == (iconv_t)-1)
		return NULL;

	buf_sz = inlen * 2;
	buf = xmalloc(buf_sz+1);
	do {
		in = msg;
		inleft = inlen;

		out = buf;
		outleft = buf_sz;
		iconv(msg_conv, &in, &inleft, &out, &outleft);

		if(inleft == 0) {
			fail = 0;
			again = 0;
		} else if(inleft != 0 && errno == E2BIG) {
			fail = 0;
			again = 1;

			buf_sz *= 2;
			free(buf);
			buf = xmalloc(buf_sz+1);
		} else {
			fail = 1;
			again = 0;
		}
	} while(again && !fail);

	if(fail) {
		free(buf);
		ret = NULL;
	} else {
		buf = xrealloc(buf, out - buf);
		*out = 0;
		ret = buf;
	}

	iconv_close(msg_conv);

	return ret;
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
		ret->author_date = atol(++t);
		p = strchr(t, '\n') + 1;
	}

	if (!strncmp(p, "committer ", 9)) {
		p += 9;
		t = strchr(p, '<') - 1;
		ret->committer = substr(p, t);
		p = t;
		t = strchr(t, '>') + 1;
		ret->committer_email = substr(p, t);
		ret->committer_date = atol(++t);
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
		t = iconv_msg(ret->subject, ret->msg_encoding);
		if(t) {
			free(ret->subject);
			ret->subject = t;
		}

		t = iconv_msg(ret->msg, ret->msg_encoding);
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
			ret->tagger_date = atol(++t);
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
