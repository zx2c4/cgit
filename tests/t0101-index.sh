#!/bin/sh

. ./setup.sh

prepare_tests "Check content on index page"

run_test 'generate index page' 'cgit_url "" >trash/tmp'
run_test 'find foo repo' 'grep -e "foo" trash/tmp'
run_test 'find foo description' 'grep -e "\[no description\]" trash/tmp'
run_test 'find bar repo' 'grep -e "bar" trash/tmp'
run_test 'find bar description' 'grep -e "the bar repo" trash/tmp'
run_test 'find foo+bar repo' 'grep -e ">foo+bar<" trash/tmp'
run_test 'verify foo+bar link' 'grep -e "/foo+bar/" trash/tmp'
run_test 'verify "with%20space" link' 'grep -e "/with%20space/" trash/tmp'
run_test 'no tree-link' '! grep -e "foo/tree" trash/tmp'
run_test 'no log-link' '! grep -e "foo/log" trash/tmp'

tests_done
