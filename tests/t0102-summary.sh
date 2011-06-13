#!/bin/sh

. ./setup.sh

prepare_tests "Check content on summary page"

run_test 'generate foo summary' 'cgit_url "foo" >trash/tmp'
run_test 'find commit 1' 'grep -e "commit 1" trash/tmp'
run_test 'find commit 5' 'grep -e "commit 5" trash/tmp'
run_test 'find branch master' 'grep -e "master" trash/tmp'
run_test 'no tags' '! grep -e "tags" trash/tmp'
run_test 'clone-url expanded correctly' '
	grep -e "git://example.org/foo.git" trash/tmp
'

run_test 'generate bar summary' 'cgit_url "bar" >trash/tmp'
run_test 'no commit 45' '! grep -e "commit 45" trash/tmp'
run_test 'find commit 46' 'grep -e "commit 46" trash/tmp'
run_test 'find commit 50' 'grep -e "commit 50" trash/tmp'
run_test 'find branch master' 'grep -e "master" trash/tmp'
run_test 'no tags' '! grep -e "tags" trash/tmp'
run_test 'clone-url expanded correctly' '
	grep -e "git://example.org/bar.git" trash/tmp
'

tests_done
