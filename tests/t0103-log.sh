#!/bin/sh

. ./setup.sh

prepare_tests "Check content on log page"

run_test 'generate foo/log' 'cgit_url "foo/log" >trash/tmp'
run_test 'find commit 1' 'grep -e "commit 1" trash/tmp'
run_test 'find commit 5' 'grep -e "commit 5" trash/tmp'

run_test 'generate bar/log' 'cgit_url "bar/log" >trash/tmp'
run_test 'find commit 1' 'grep -e "commit 1" trash/tmp'
run_test 'find commit 50' 'grep -e "commit 50" trash/tmp'

tests_done
