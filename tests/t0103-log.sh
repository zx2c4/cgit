#!/bin/sh

. ./setup.sh

prepare_tests "Check content on log page"

run_test 'generate foo/log' 'cgit_url "foo/log" >trash/tmp'
run_test 'find commit 1' 'grep "commit 1" trash/tmp'
run_test 'find commit 5' 'grep "commit 5" trash/tmp'

run_test 'generate bar/log' 'cgit_url "bar/log" >trash/tmp'
run_test 'find commit 1' 'grep "commit 1" trash/tmp'
run_test 'find commit 50' 'grep "commit 50" trash/tmp'

run_test 'generate "with%20space/log?qt=grep&q=commit+1"' '
	cgit_url "with+space/log&qt=grep&q=commit+1" >trash/tmp
'
run_test 'find commit 1' 'grep "commit 1" trash/tmp'
run_test 'find link with %20 in path' 'grep "/with%20space/log/?qt=grep" trash/tmp'
run_test 'find link with + in arg' 'grep "/log/?qt=grep&amp;q=commit+1" trash/tmp'
run_test 'no links with space in path' '! grep "href=./with space/" trash/tmp'
run_test 'no links with space in arg' '! grep "q=commit 1" trash/tmp'
run_test 'commit 2 is not visible' '! grep "commit 2" trash/tmp'

tests_done
