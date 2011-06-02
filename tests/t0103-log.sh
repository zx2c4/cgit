#!/bin/sh

. ./setup.sh

prepare_tests "Check content on log page"

run_test 'generate foo/log' 'cgit_url "foo/log" >trash/tmp'
run_test 'find commit 1' 'grep -e "commit 1" trash/tmp'
run_test 'find commit 5' 'grep -e "commit 5" trash/tmp'

run_test 'generate bar/log' 'cgit_url "bar/log" >trash/tmp'
run_test 'find commit 1' 'grep -e "commit 1" trash/tmp'
run_test 'find commit 50' 'grep -e "commit 50" trash/tmp'

run_test 'generate "with%20space/log?qt=grep&q=commit+1"' '
	cgit_url "with+space/log&qt=grep&q=commit+1" >trash/tmp
'
run_test 'find commit 1' 'grep -e "commit 1" trash/tmp'
run_test 'find link with %20 in path' 'grep -e "/with%20space/log/?qt=grep" trash/tmp'
run_test 'find link with + in arg' 'grep -e "/log/?qt=grep&q=commit+1" trash/tmp'
run_test 'no links with space in path' '! grep -e "href=./with space/" trash/tmp'
run_test 'no links with space in arg' '! grep -e "q=commit 1" trash/tmp'
run_test 'commit 2 is not visible' '! grep -e "commit 2" trash/tmp'

tests_done
