#!/bin/sh

. ./setup.sh

prepare_tests "Check content on commit page"

run_test 'generate foo/commit' 'cgit_url "foo/commit" >trash/tmp'
run_test 'find tree link' 'grep -e "<a href=./foo/tree/.>" trash/tmp'
run_test 'find parent link' 'grep -E "<a href=./foo/commit/\?id=.+>" trash/tmp'

run_test 'find commit subject' '
	grep -e "<div class=.commit-subject.>commit 5<" trash/tmp
'

run_test 'find commit msg' 'grep -e "<div class=.commit-msg.></div>" trash/tmp'
run_test 'find diffstat' 'grep -e "<table summary=.diffstat. class=.diffstat.>" trash/tmp'

run_test 'find diff summary' '
	 grep -e "1 files changed, 1 insertions, 0 deletions" trash/tmp
'

run_test 'get root commit' '
	 root=$(cd trash/repos/foo && git rev-list --reverse HEAD | head -1) &&
	 cgit_url "foo/commit&id=$root" >trash/tmp &&
	 grep "</html>" trash/tmp
'

run_test 'root commit contains diffstat' '
	 grep "<a href=./foo/diff/file-1.id=[0-9a-f]\{40\}.>file-1</a>" trash/tmp
'

run_test 'root commit contains diff' '
	 grep ">diff --git a/file-1 b/file-1<" trash/tmp &&
	 grep -e "<div class=.add.>+1</div>" trash/tmp
'

tests_done
