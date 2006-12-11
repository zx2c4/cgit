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
