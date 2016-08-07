/* configfile.c: parsing of config files
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include <git-compat-util.h>
#include "configfile.h"

static int next_char(FILE *f)
{
	int c = fgetc(f);
	if (c == '\r') {
		c = fgetc(f);
		if (c != '\n') {
			ungetc(c, f);
			c = '\r';
		}
	}
	return c;
}

static void skip_line(FILE *f)
{
	int c;

	while ((c = next_char(f)) && c != '\n' && c != EOF)
		;
}

static int read_config_line(FILE *f, struct strbuf *name, struct strbuf *value)
{
	int c = next_char(f);

	strbuf_reset(name);
	strbuf_reset(value);

	/* Skip comments and preceding spaces. */
	for(;;) {
		if (c == EOF)
			return 0;
		else if (c == '#' || c == ';')
			skip_line(f);
		else if (!isspace(c))
			break;
		c = next_char(f);
	}

	/* Read variable name. */
	while (c != '=') {
		if (c == '\n' || c == EOF)
			return 0;
		strbuf_addch(name, c);
		c = next_char(f);
	}

	/* Read variable value. */
	c = next_char(f);
	while (c != '\n' && c != EOF) {
		strbuf_addch(value, c);
		c = next_char(f);
	}

	return 1;
}

int parse_configfile(const char *filename, configfile_value_fn fn)
{
	static int nesting;
	struct strbuf name = STRBUF_INIT;
	struct strbuf value = STRBUF_INIT;
	FILE *f;

	/* cancel deeply nested include-commands */
	if (nesting > 8)
		return -1;
	if (!(f = fopen(filename, "r")))
		return -1;
	nesting++;
	while (read_config_line(f, &name, &value))
		fn(name.buf, value.buf);
	nesting--;
	fclose(f);
	strbuf_release(&name);
	strbuf_release(&value);
	return 0;
}

