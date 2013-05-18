#!/bin/sh

test_description='Verify snapshot'
. ./setup.sh

test_expect_success 'get foo/snapshot/master.tar.gz' '
	cgit_url "foo/snapshot/master.tar.gz" >tmp
'

test_expect_success 'check html headers' '
	head -n 1 tmp |
	grep "Content-Type: application/x-gzip" &&

	head -n 2 tmp |
	grep "Content-Disposition: inline; filename=.master.tar.gz."
'

test_expect_success 'strip off the header lines' '
	strip_headers <tmp >master.tar.gz
'

test_expect_success 'verify gzip format' '
	gunzip --test master.tar.gz
'

test_expect_success 'untar' '
	rm -rf master &&
	tar -xzf master.tar.gz
'

test_expect_success 'count files' '
	ls master/ >output &&
	test_line_count = 5 output
'

test_expect_success 'verify untarred file-5' '
	grep "^5$" master/file-5 &&
	test_line_count = 1 master/file-5
'

test_expect_success 'get foo/snapshot/master.zip' '
	cgit_url "foo/snapshot/master.zip" >tmp
'

test_expect_success 'check HTML headers (zip)' '
	head -n 1 tmp |
	grep "Content-Type: application/x-zip" &&

	head -n 2 tmp |
	grep "Content-Disposition: inline; filename=.master.zip."
'

test_expect_success 'strip off the header lines (zip)' '
	strip_headers <tmp >master.zip
'

if test -n "$(which unzip 2>/dev/null)"; then
	test_set_prereq UNZIP
else
	say 'Skipping ZIP validation tests: unzip not found'
fi

test_expect_success UNZIP 'verify zip format' '
	unzip -t master.zip
'

test_expect_success UNZIP 'unzip' '
	rm -rf master &&
	unzip master.zip
'

test_expect_success UNZIP 'count files (zip)' '
	ls master/ >output &&
	test_line_count = 5 output
'

test_expect_success UNZIP 'verify unzipped file-5' '
	grep "^5$" master/file-5 &&
	test_line_count = 1 master/file-5
'

test_done
