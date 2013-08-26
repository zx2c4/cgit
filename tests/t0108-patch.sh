#!/bin/sh

test_description='Check content on patch page'
. ./setup.sh

test_expect_success 'generate foo/patch' '
	cgit_query "url=foo/patch" >tmp
'

test_expect_success 'find `From:` line' '
	grep "^From: " tmp
'

test_expect_success 'find `Date:` line' '
	grep "^Date: " tmp
'

test_expect_success 'find `Subject:` line' '
	grep "^Subject: commit 5" tmp
'

test_expect_success 'find `cgit` signature' '
	tail -1 tmp | grep "^cgit"
'

test_expect_success 'find initial commit' '
	root=$(git --git-dir="$PWD/repos/foo/.git" rev-list --max-parents=0 HEAD)
'

test_expect_success 'generate patch for initial commit' '
	cgit_query "url=foo/patch&id=$root" >tmp
'

test_expect_success 'find `cgit` signature' '
	tail -1 tmp | grep "^cgit"
'

test_done
