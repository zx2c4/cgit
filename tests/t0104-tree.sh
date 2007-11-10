#!/bin/sh

. ./setup.sh

prepare_tests "Check content on tree page"

run_test 'generate bar/tree' 'cgit_url "bar/tree" >trash/tmp'
run_test 'find file-1' 'grep -e "file-1" trash/tmp'
run_test 'find file-50' 'grep -e "file-50" trash/tmp'

run_test 'generate bar/tree/file-50' 'cgit_url "bar/tree/file-50" >trash/tmp'
run_test 'find line 1' 'grep -e "<a name=.1.>1</a>" trash/tmp'
run_test 'no line 2' 'grep -ve "<a name=.2.>2</a>" trash/tmp'

tests_done
