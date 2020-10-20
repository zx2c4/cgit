#!/bin/sh
# This script can be used to generate links in commit messages.
#
# To use this script, refer to this file with either the commit-filter or the
# repo.commit-filter options in cgitrc.
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
#

regex=''

# This expression generates links to commits referenced by their SHA1.
regex=$regex'
s|\b([0-9a-fA-F]{7,64})\b|<a href="./?id=\1">\1</a>|g'

# This expression generates links to a fictional bugtracker.
regex=$regex'
s|#([0-9]+)\b|<a href="http://bugs.example.com/?bug=\1">#\1</a>|g'

sed -re "$regex"
