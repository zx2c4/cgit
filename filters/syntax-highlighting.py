#!/usr/bin/env python3

# This script uses Pygments and Python3. You must have both installed for this to work.
# http://pygments.org/
# http://python.org/
#
# It may be used with the source-filter or repo.source-filter settings in cgitrc.
#
# The following environment variables can be used to retrieve the configuration
# of the repository for which this script is called:
# CGIT_REPO_URL        ( = repo.url       setting )
# CGIT_REPO_NAME       ( = repo.name      setting )
# CGIT_REPO_PATH       ( = repo.path      setting )
# CGIT_REPO_OWNER      ( = repo.owner     setting )
# CGIT_REPO_DEFBRANCH  ( = repo.defbranch setting )
# CGIT_REPO_SECTION    ( = section        setting )
# CGIT_REPO_CLONE_URL  ( = repo.clone-url setting )


import sys
import cgi
import codecs
from pygments.lexers import get_lexer_for_filename
from pygments import highlight
from pygments.formatters import HtmlFormatter

sys.stdin = codecs.getreader("utf-8")(sys.stdin.detach())
sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())
doc = sys.stdin.read()
try:
	lexer = get_lexer_for_filename(sys.argv[1])
	formatter = HtmlFormatter(style='pastie')
	sys.stdout.write("<style>")
	sys.stdout.write(formatter.get_style_defs('.highlight'))
	sys.stdout.write("</style>")

	highlight(doc, lexer, formatter, sys.stdout)
except:
	sys.stdout.write(str(cgi.escape(doc).encode("ascii", "xmlcharrefreplace"), "ascii"))
