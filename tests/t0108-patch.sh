#!/bin/sh

. ./setup.sh

prepare_tests "Check content on patch page"

run_test 'generate foo/patch' '
	cgit_query "url=foo/patch" >trash/tmp
'

run_test 'find `From:` line' '
	grep "^From: " trash/tmp
'

run_test 'find `Date:` line' '
	grep "^Date: " trash/tmp
'

run_test 'find `Subject:` line' '
	grep "^Subject: commit 5" trash/tmp
'

run_test 'find `cgit` signature' '
	tail -1 trash/tmp | grep "^cgit"
'

run_test 'find initial commit' '
	root=$(git --git-dir="$PWD/trash/repos/foo/.git" rev-list HEAD | tail -1)
'

run_test 'generate patch for initial commit' '
	cgit_query "url=foo/patch&id=$root" >trash/tmp
'

run_test 'find `cgit` signature' '
	tail -1 trash/tmp | grep "^cgit"
'

tests_done
