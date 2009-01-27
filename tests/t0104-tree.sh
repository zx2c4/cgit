#!/bin/sh

. ./setup.sh

prepare_tests "Check content on tree page"

run_test 'generate bar/tree' 'cgit_url "bar/tree" >trash/tmp'
run_test 'find file-1' 'grep -e "file-1" trash/tmp'
run_test 'find file-50' 'grep -e "file-50" trash/tmp'

run_test 'generate bar/tree/file-50' 'cgit_url "bar/tree/file-50" >trash/tmp'

run_test 'find line 1' '
	grep -e "<a class=.no. id=.n1. name=.n1. href=.#n1.>1</a>" trash/tmp
'

run_test 'no line 2' '
	! grep -e "<a class=.no. id=.n2. name=.n2. href=.#n2.>2</a>" trash/tmp
'

run_test 'generate foo+bar/tree' 'cgit_url "foo%2bbar/tree" >trash/tmp'

run_test 'verify a+b link' '
	grep -e "/foo+bar/tree/a+b" trash/tmp
'

run_test 'generate foo+bar/tree?h=1+2' 'cgit_url "foo%2bbar/tree&h=1%2b2" >trash/tmp'

run_test 'verify a+b?h=1+2 link' '
	grep -e "/foo+bar/tree/a+b?h=1%2b2" trash/tmp
'

tests_done
