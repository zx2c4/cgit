#!/bin/sh
# This script can be used to generate links in commit messages - the first
# sed expression generates links to commits referenced by their SHA1, while
# the second expression generates links to a fictional bugtracker.
#
# To use this script, refer to this file with either the commit-filter or the
# repo.commit-filter options in cgitrc.

sed -re '
s|\b([0-9a-fA-F]{8,40})\b|<a href="./?id=\1">\1</a>|g
s| #([0-9]+)\b|<a href="http://bugs.example.com/?bug=\1">#\1</a>|g
'
