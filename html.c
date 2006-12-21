/* html.c: helper functions for html output
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

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
	if (len>sizeof(buf[bufidx]))
		die("[html.c] string truncated: %s", format);
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
	while(*t){
		int c = *t;
		if (c=='<' || c=='>' || c=='&') {
			*t = '\0';
			html(txt);
			*t = c;
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
	while(*t && len--){
		int c = *t;
		if (c=='<' || c=='>' || c=='&') {
			*t = '\0';
			html(txt);
			*t = c;
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
	if (t!=txt) {
		char c = *t;
		*t = '\0';
		html(txt);
		*t = c;
	}
	if (len<0)
		html("...");
}

void html_attr(char *txt)
{
	char *t = txt;
	while(*t){
		int c = *t;
		if (c=='<' || c=='>' || c=='\'') {
			*t = '\0';
			html(txt);
			*t = c;
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

void html_filemode(unsigned short mode)
{
	if (S_ISDIR(mode))
		html("d");
	else if (S_ISLNK(mode))
		html("l");
	else
		html("-");
	html_fileperm(mode >> 6);
	html_fileperm(mode >> 3);
	html_fileperm(mode);
}

