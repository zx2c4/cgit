/* config.c: parsing of config files
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

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
	int ret = 0, len;
	char line[256];
	const char *value;
	FILE *f = fopen(filename, "r");

	if (!f)
		return -1;

	while((len = read_config_line(f, line, &value, sizeof(line))) > 0)
		(*fn)(line, value);

	fclose(f);
	return ret;
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

	while (*p == '\n')
		p = strchr(p, '\n') + 1;

	t = strchr(p, '\n');
	if (t && *t) {
		ret->subject = substr(p, t);
		p = t + 1;

		while (*p == '\n')
			p = strchr(p, '\n') + 1;
		ret->msg = p;
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

	while (p && (*p == '\n'))
		p = strchr(p, '\n') + 1;
	if (p && *p)
		ret->msg = xstrdup(p);
	free(data);
	return ret;
}
