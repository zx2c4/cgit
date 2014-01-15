#!/usr/bin/env python3

# Please prefer the email-gravatar.lua using lua: as a prefix over this script. This
# script is very slow, in comparison.
#
# This script may be used with the email-filter or repo.email-filter settings in cgitrc.
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
# It receives an email address on argv[1] and text on stdin. It prints
# to stdout that text prepended by a gravatar at 10pt.

import sys
import hashlib
import codecs

email = sys.argv[1].lower().strip()
if email[0] == '<':
        email = email[1:]
if email[-1] == '>':
        email = email[0:-1]

page = sys.argv[2]

sys.stdin = codecs.getreader("utf-8")(sys.stdin.detach())
sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())

md5 = hashlib.md5(email.encode()).hexdigest()
text = sys.stdin.read().strip()

print("<img src='//www.gravatar.com/avatar/" + md5 + "?s=13&amp;d=retro' width='13' height='13' alt='Gravatar' /> " + text)
