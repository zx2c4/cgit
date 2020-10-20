#!/bin/sh

test_description='Check content on commit page'
. ./setup.sh

test_expect_success 'generate foo/commit' 'cgit_url "foo/commit" >tmp'
test_expect_success 'find tree link' 'grep "<a href=./foo/tree/.>" tmp'
test_expect_success 'find parent link' 'grep -E "<a href=./foo/commit/\?id=.+>" tmp'

test_expect_success 'find commit subject' '
	grep "<div class=.commit-subject.>commit 5<" tmp
'

test_expect_success 'find commit msg' 'grep "<div class=.commit-msg.></div>" tmp'
test_expect_success 'find diffstat' 'grep "<table summary=.diffstat. class=.diffstat.>" tmp'

test_expect_success 'find diff summary' '
	grep "1 files changed, 1 insertions, 0 deletions" tmp
'

test_expect_success 'get root commit' '
	root=$(cd repos/foo && git rev-list --reverse HEAD | head -1) &&
	cgit_url "foo/commit&id=$root" >tmp &&
	grep "</html>" tmp
'

test_expect_success 'root commit contains diffstat' '
	grep "<a href=./foo/diff/file-1.id=[0-9a-f]\{40,64\}.>file-1</a>" tmp
'

test_expect_success 'root commit contains diff' '
	grep ">diff --git a/file-1 b/file-1<" tmp &&
	grep "<div class=.add.>+1</div>" tmp
'

test_done
