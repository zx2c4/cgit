#!/bin/sh

. ./setup.sh

prepare_tests "Check content on index page"

run_test 'generate index page' 'cgit_url "" >trash/tmp'
run_test 'find foo repo' 'grep "foo" trash/tmp'
run_test 'find foo description' 'grep "\[no description\]" trash/tmp'
run_test 'find bar repo' 'grep "bar" trash/tmp'
run_test 'find bar description' 'grep "the bar repo" trash/tmp'
run_test 'find foo+bar repo' 'grep ">foo+bar<" trash/tmp'
run_test 'verify foo+bar link' 'grep "/foo+bar/" trash/tmp'
run_test 'verify "with%20space" link' 'grep "/with%20space/" trash/tmp'
run_test 'no tree-link' '! grep "foo/tree" trash/tmp'
run_test 'no log-link' '! grep "foo/log" trash/tmp'

tests_done
