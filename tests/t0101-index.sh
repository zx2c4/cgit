#!/bin/sh

. ./setup.sh

prepare_tests "Check content on index page"

run_test 'generate index page' 'cgit_url "" >trash/tmp'
run_test 'find foo repo' 'grep -e "foo" trash/tmp'
run_test 'find bar repo' 'grep -e "bar" trash/tmp'
run_test 'no tree-link' 'grep -ve "foo/tree" trash/tmp'
run_test 'no log-link' 'grep -ve "foo/log" trash/tmp'

tests_done
