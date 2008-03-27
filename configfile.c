/* configfile.c: parsing of config files
 *
 * Copyright (C) 2008 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include <ctype.h>
#include <stdio.h>
#include "configfile.h"

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

int parse_configfile(const char *filename, configfile_value_fn fn)
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
		fn(line, value);
	nesting--;
	fclose(f);
	return 0;
}

