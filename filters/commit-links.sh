#!/bin/sh
# This script can be used to generate links in commit messages.
#
# To use this script, refer to this file with either the commit-filter or the
# repo.commit-filter options in cgitrc.

# This expression generates links to commits referenced by their SHA1.
regex=$regex'
s|\b([0-9a-fA-F]{8,40})\b|<a href="./?id=\1">\1</a>|g'
# This expression generates links to a fictional bugtracker.
regex=$regex'
s| #([0-9]+)\b|<a href="http://bugs.example.com/?bug=\1">#\1</a>|g'

sed -re "$regex"
