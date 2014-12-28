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
	tail -2 tmp | head -1 | grep "^cgit"
'

test_expect_success 'compare with output of git-format-patch(1)' '
	CGIT_VERSION=$(sed -n "s/CGIT_VERSION = //p" ../../VERSION) &&
	git --git-dir="$PWD/repos/foo/.git" format-patch --subject-prefix="" --signature="cgit $CGIT_VERSION" --stdout HEAD^ >tmp2 &&
	strip_headers <tmp >tmp_ &&
	test_cmp tmp_ tmp2
'

test_expect_success 'find initial commit' '
	root=$(git --git-dir="$PWD/repos/foo/.git" rev-list --max-parents=0 HEAD)
'

test_expect_success 'generate patch for initial commit' '
	cgit_query "url=foo/patch&id=$root" >tmp
'

test_expect_success 'find `cgit` signature' '
	tail -2 tmp | head -1 | grep "^cgit"
'

test_expect_success 'generate patches for multiple commits' '
	id=$(git --git-dir="$PWD/repos/foo/.git" rev-parse HEAD) &&
	id2=$(git --git-dir="$PWD/repos/foo/.git" rev-parse HEAD~3) &&
	cgit_query "url=foo/patch&id=$id&id2=$id2" >tmp
'

test_expect_success 'find `cgit` signature' '
	tail -2 tmp | head -1 | grep "^cgit"
'

test_expect_success 'compare with output of git-format-patch(1)' '
	CGIT_VERSION=$(sed -n "s/CGIT_VERSION = //p" ../../VERSION) &&
	git --git-dir="$PWD/repos/foo/.git" format-patch -N --subject-prefix="" --signature="cgit $CGIT_VERSION" --stdout HEAD~3..HEAD >tmp2 &&
	strip_headers <tmp >tmp_ &&
	test_cmp tmp_ tmp2
'

test_done
