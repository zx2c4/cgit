/* html.c: helper functions for html output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int htmlfd = STDOUT_FILENO;

char *fmt(const char *format, ...)
{
	static char buf[8][1024];
	static int bufidx;
	int len;
	va_list args;

	bufidx++;
	bufidx &= 7;

	va_start(args, format);
	len = vsnprintf(buf[bufidx], sizeof(buf[bufidx]), format, args);
	va_end(args);
	if (len>sizeof(buf[bufidx])) {
		fprintf(stderr, "[html.c] string truncated: %s\n", format);
		exit(1);
	}
	return buf[bufidx];
}

void html(const char *txt)
{
	write(htmlfd, txt, strlen(txt));
}

void htmlf(const char *format, ...)
{
	static char buf[65536];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	html(buf);
}

void html_txt(char *txt)
{
	char *t = txt;
	while(t && *t){
		int c = *t;
		if (c=='<' || c=='>' || c=='&') {
			write(htmlfd, txt, t - txt);
			if (c=='>')
				html("&gt;");
			else if (c=='<')
				html("&lt;");
			else if (c=='&')
				html("&amp;");
			txt = t+1;
		}
		t++;
	}
	if (t!=txt)
		html(txt);
}

void html_ntxt(int len, char *txt)
{
	char *t = txt;
	while(t && *t && len--){
		int c = *t;
		if (c=='<' || c=='>' || c=='&') {
			write(htmlfd, txt, t - txt);
			if (c=='>')
				html("&gt;");
			else if (c=='<')
				html("&lt;");
			else if (c=='&')
				html("&amp;");
			txt = t+1;
		}
		t++;
	}
	if (t!=txt)
		write(htmlfd, txt, t - txt);
	if (len<0)
		html("...");
}

void html_attr(char *txt)
{
	char *t = txt;
	while(t && *t){
		int c = *t;
		if (c=='<' || c=='>' || c=='\'') {
			write(htmlfd, txt, t - txt);
			if (c=='>')
				html("&gt;");
			else if (c=='<')
				html("&lt;");
			else if (c=='\'')
				html("&quote;");
			txt = t+1;
		}
		t++;
	}
	if (t!=txt)
		html(txt);
}

void html_hidden(char *name, char *value)
{
	html("<input type='hidden' name='");
	html_attr(name);
	html("' value='");
	html_attr(value);
	html("'/>");
}

void html_option(char *value, char *text, char *selected_value)
{
	html("<option value='");
	html_attr(value);
	html("'");
	if (selected_value && !strcmp(selected_value, value))
		html(" selected='selected'");
	html(">");
	html_txt(text);
	html("</option>\n");
}

void html_link_open(char *url, char *title, char *class)
{
	html("<a href='");
	html_attr(url);
	if (title) {
		html("' title='");
		html_attr(title);
	}
	if (class) {
		html("' class='");
		html_attr(class);
	}
	html("'>");
}

void html_link_close(void)
{
	html("</a>");
}

void html_fileperm(unsigned short mode)
{
	htmlf("%c%c%c", (mode & 4 ? 'r' : '-'),
	      (mode & 2 ? 'w' : '-'), (mode & 1 ? 'x' : '-'));
}

int html_include(const char *filename)
{
	FILE *f;
	char buf[4096];
	size_t len;

	if (!(f = fopen(filename, "r")))
		return -1;
	while((len = fread(buf, 1, 4096, f)) > 0)
		write(htmlfd, buf, len);
	fclose(f);
	return 0;
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

int http_parse_querystring(char *txt, void (*fn)(const char *name, const char *value))
{
	char *t, *value = NULL, c;

	if (!txt)
		return 0;

	t = txt = strdup(txt);
	if (t == NULL) {
		printf("Out of memory\n");
		exit(1);
	}
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
