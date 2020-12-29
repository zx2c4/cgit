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
	gzip -dc master.tar.gz | tar -xf -
'

test_expect_success 'count files' '
	ls master/ >output &&
	test_line_count = 5 output
'

test_expect_success 'verify untarred file-5' '
	grep "^5$" master/file-5 &&
	test_line_count = 1 master/file-5
'

if test -n "$(which lzip 2>/dev/null)"; then
	test_set_prereq LZIP
else
	say 'Skipping LZIP validation tests: lzip not found'
fi

test_expect_success LZIP 'get foo/snapshot/master.tar.lz' '
	cgit_url "foo/snapshot/master.tar.lz" >tmp
'

test_expect_success LZIP 'check html headers' '
	head -n 1 tmp |
	grep "Content-Type: application/x-lzip" &&

	head -n 2 tmp |
	grep "Content-Disposition: inline; filename=.master.tar.lz."
'

test_expect_success LZIP 'strip off the header lines' '
	strip_headers <tmp >master.tar.lz
'

test_expect_success LZIP 'verify lzip format' '
	lzip --test master.tar.lz
'

test_expect_success LZIP 'untar' '
	rm -rf master &&
	lzip -dc master.tar.lz | tar -xf -
'

test_expect_success LZIP 'count files' '
	ls master/ >output &&
	test_line_count = 5 output
'

test_expect_success LZIP 'verify untarred file-5' '
	grep "^5$" master/file-5 &&
	test_line_count = 1 master/file-5
'

if test -n "$(which xz 2>/dev/null)"; then
	test_set_prereq XZ
else
	say 'Skipping XZ validation tests: xz not found'
fi

test_expect_success XZ 'get foo/snapshot/master.tar.xz' '
	cgit_url "foo/snapshot/master.tar.xz" >tmp
'

test_expect_success XZ 'check html headers' '
	head -n 1 tmp |
	grep "Content-Type: application/x-xz" &&

	head -n 2 tmp |
	grep "Content-Disposition: inline; filename=.master.tar.xz."
'

test_expect_success XZ 'strip off the header lines' '
	strip_headers <tmp >master.tar.xz
'

test_expect_success XZ 'verify xz format' '
	xz --test master.tar.xz
'

test_expect_success XZ 'untar' '
	rm -rf master &&
	xz -dc master.tar.xz | tar -xf -
'

test_expect_success XZ 'count files' '
	ls master/ >output &&
	test_line_count = 5 output
'

test_expect_success XZ 'verify untarred file-5' '
	grep "^5$" master/file-5 &&
	test_line_count = 1 master/file-5
'

if test -n "$(which zstd 2>/dev/null)"; then
	test_set_prereq ZSTD
else
	say 'Skipping ZSTD validation tests: zstd not found'
fi

test_expect_success ZSTD 'get foo/snapshot/master.tar.zst' '
	cgit_url "foo/snapshot/master.tar.zst" >tmp
'

test_expect_success ZSTD 'check html headers' '
	head -n 1 tmp |
	grep "Content-Type: application/x-zstd" &&

	head -n 2 tmp |
	grep "Content-Disposition: inline; filename=.master.tar.zst."
'

test_expect_success ZSTD 'strip off the header lines' '
	strip_headers <tmp >master.tar.zst
'

test_expect_success ZSTD 'verify zstd format' '
	zstd --test master.tar.zst
'

test_expect_success ZSTD 'untar' '
	rm -rf master &&
	zstd -dc master.tar.zst | tar -xf -
'

test_expect_success ZSTD 'count files' '
	ls master/ >output &&
	test_line_count = 5 output
'

test_expect_success ZSTD 'verify untarred file-5' '
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
