#!/bin/sh

. ./setup.sh

prepare_tests "Verify snapshot"

run_test 'get foo/snapshot/master.tar.gz' '
	cgit_url "foo/snapshot/master.tar.gz" >trash/tmp
'

run_test 'check html headers' '
	head -n 1 trash/tmp |
	     grep -e "Content-Type: application/x-gzip" &&

	head -n 2 trash/tmp |
	     grep -e "Content-Disposition: inline; filename=.master.tar.gz."
'

run_test 'strip off the header lines' '
	 tail -n +6 trash/tmp > trash/master.tar.gz
'

run_test 'verify gzip format' 'gunzip --test trash/master.tar.gz'
run_test 'untar' '
	rm -rf trash/master &&
	tar -xf trash/master.tar.gz -C trash
'

run_test 'count files' '
	c=$(ls -1 trash/master/ | wc -l) &&
	test $c = 5
'

run_test 'verify untarred file-5' '
	 grep -e "^5$" trash/master/file-5 &&
	 test $(cat trash/master/file-5 | wc -l) = 1
'

tests_done
