#!/bin/sh

test_description='Check content on rawdiff page'
. ./setup.sh

test_expect_success 'generate foo/rawdiff' '
	cgit_query "url=foo/rawdiff" >tmp
'

test_expect_success 'compare with output of git-diff(1)' '
	git --git-dir="$PWD/repos/foo/.git" diff HEAD^.. >tmp2 &&
	sed "1,4d" tmp >tmp_ &&
	cmp tmp_ tmp2
'

test_expect_success 'find initial commit' '
	root=$(git --git-dir="$PWD/repos/foo/.git" rev-list --max-parents=0 HEAD)
'

test_expect_success 'generate diff for initial commit' '
	cgit_query "url=foo/rawdiff&id=$root" >tmp
'

test_expect_success 'compare with output of git-diff-tree(1)' '
	git --git-dir="$PWD/repos/foo/.git" diff-tree -p --no-commit-id --root "$root" >tmp2 &&
	sed "1,4d" tmp >tmp_ &&
	cmp tmp_ tmp2
'

test_expect_success 'generate diff for multiple commits' '
	id=$(git --git-dir="$PWD/repos/foo/.git" rev-parse HEAD) &&
	id2=$(git --git-dir="$PWD/repos/foo/.git" rev-parse HEAD~3) &&
	cgit_query "url=foo/rawdiff&id=$id&id2=$id2" >tmp
'

test_expect_success 'compare with output of git-diff(1)' '
	git --git-dir="$PWD/repos/foo/.git" diff HEAD~3..HEAD >tmp2 &&
	sed "1,4d" tmp >tmp_ &&
	cmp tmp_ tmp2
'

test_done
