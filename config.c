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
		if (!isname && isblank(c))
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

	while(len = read_config_line(f, line, &value, sizeof(line)))
		(*fn)(line, value);

	fclose(f);
	return ret;
}

