#!/bin/sh

test_description='Check content on summary page'
. ./setup.sh

test_expect_success 'generate foo summary' 'cgit_url "foo" >tmp'
test_expect_success 'find commit 1' 'grep "commit 1" tmp'
test_expect_success 'find commit 5' 'grep "commit 5" tmp'
test_expect_success 'find branch master' 'grep "master" tmp'
test_expect_success 'no tags' '! grep "tags" tmp'
test_expect_success 'clone-url expanded correctly' '
	grep "git://example.org/foo.git" tmp
'

test_expect_success 'generate bar summary' 'cgit_url "bar" >tmp'
test_expect_success 'no commit 45' '! grep "commit 45" tmp'
test_expect_success 'find commit 46' 'grep "commit 46" tmp'
test_expect_success 'find commit 50' 'grep "commit 50" tmp'
test_expect_success 'find branch master' 'grep "master" tmp'
test_expect_success 'no tags' '! grep "tags" tmp'
test_expect_success 'clone-url expanded correctly' '
	grep "git://example.org/bar.git" tmp
'

test_done
