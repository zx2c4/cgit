#!/usr/bin/env python3

# This script uses Pygments and Python3. You must have both installed
# for this to work.
#
# http://pygments.org/
# http://python.org/
#
# It may be used with the source-filter or repo.source-filter settings
# in cgitrc.
#
# The following environment variables can be used to retrieve the
# configuration of the repository for which this script is called:
# CGIT_REPO_URL        ( = repo.url       setting )
# CGIT_REPO_NAME       ( = repo.name      setting )
# CGIT_REPO_PATH       ( = repo.path      setting )
# CGIT_REPO_OWNER      ( = repo.owner     setting )
# CGIT_REPO_DEFBRANCH  ( = repo.defbranch setting )
# CGIT_REPO_SECTION    ( = section        setting )
# CGIT_REPO_CLONE_URL  ( = repo.clone-url setting )


import sys
import io
from pygments import highlight
from pygments.util import ClassNotFound
from pygments.lexers import TextLexer
from pygments.lexers import guess_lexer
from pygments.lexers import guess_lexer_for_filename
from pygments.formatters import HtmlFormatter

# The dark style is automatically selected if the browser is in dark mode
light_style='pastie'
dark_style='monokai'

sys.stdin = io.TextIOWrapper(sys.stdin.buffer, encoding='utf-8', errors='replace')
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
data = sys.stdin.read()
filename = sys.argv[1]
light_formatter = HtmlFormatter(style=light_style, nobackground=True)
dark_formatter = HtmlFormatter(style=dark_style, nobackground=True)

try:
	lexer = guess_lexer_for_filename(filename, data)
except ClassNotFound:
	# check if there is any shebang
	if data[0:2] == '#!':
		lexer = guess_lexer(data)
	else:
		lexer = TextLexer()
except TypeError:
	lexer = TextLexer()

# highlight! :-)
# printout pygments' css definitions as well
sys.stdout.write('<style>')
sys.stdout.write('\n@media only all and (prefers-color-scheme: dark) {\n')
sys.stdout.write(dark_formatter.get_style_defs('.highlight'))
sys.stdout.write('\n}\n@media (prefers-color-scheme: light) {\n')
sys.stdout.write(light_formatter.get_style_defs('.highlight'))
sys.stdout.write('\n}\n')
sys.stdout.write('</style>')
sys.stdout.write(highlight(data, lexer, light_formatter, outfile=None))
