#!/bin/sh

. ./setup.sh

prepare_tests "Verify snapshot"

run_test 'get foo/snapshot/test.tar.gz' '
	cgit_url "foo/snapshot/test.tar.gz" >trash/tmp
'

run_test 'check html headers' '
	head -n 1 trash/tmp |
	     grep -e "Content-Type: application/x-tar" &&

	head -n 2 trash/tmp |
	     grep -e "Content-Disposition: inline; filename=.test.tar.gz."
'

run_test 'strip off the header lines' '
	 tail -n +6 trash/tmp > trash/test.tar.gz
'

run_test 'verify gzip format' 'gunzip --test trash/test.tar.gz'
run_test 'untar' 'tar -xf trash/test.tar.gz -C trash'

run_test 'count files' '
	c=$(ls -1 trash/foo/ | wc -l) &&
	test $c = 5
'

run_test 'verify untarred file-5' '
	 grep -e "^5$" trash/foo/file-5 &&
	 test $(cat trash/foo/file-5 | wc -l) = 1
'

tests_done
