#!/bin/sh

test_description='Check content on log page'
. ./setup.sh

test_expect_success 'generate foo/log' 'cgit_url "foo/log" >tmp'
test_expect_success 'find commit 1' 'grep "commit 1" tmp'
test_expect_success 'find commit 5' 'grep "commit 5" tmp'

test_expect_success 'generate bar/log' 'cgit_url "bar/log" >tmp'
test_expect_success 'find commit 1' 'grep "commit 1" tmp'
test_expect_success 'find commit 50' 'grep "commit 50" tmp'

test_expect_success 'generate "with%20space/log?qt=grep&q=commit+1"' '
	cgit_url "with+space/log&qt=grep&q=commit+1" >tmp
'
test_expect_success 'find commit 1' 'grep "commit 1" tmp'
test_expect_success 'find link with %20 in path' 'grep "/with%20space/log/?qt=grep" tmp'
test_expect_success 'find link with + in arg' 'grep "/log/?qt=grep&amp;q=commit+1" tmp'
test_expect_success 'no links with space in path' '! grep "href=./with space/" tmp'
test_expect_success 'no links with space in arg' '! grep "q=commit 1" tmp'
test_expect_success 'commit 2 is not visible' '! grep "commit 2" tmp'

test_done
