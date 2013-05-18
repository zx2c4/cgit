#!/bin/sh

test_description='Validate cache'
. ./setup.sh

test_expect_success 'verify cache-size=0' '

	rm -f cache/* &&
	sed -e "s/cache-size=1021$/cache-size=0/" cgitrc >cgitrc.tmp &&
	mv -f cgitrc.tmp cgitrc &&
	cgit_url "" &&
	cgit_url "foo" &&
	cgit_url "foo/refs" &&
	cgit_url "foo/tree" &&
	cgit_url "foo/log" &&
	cgit_url "foo/diff" &&
	cgit_url "foo/patch" &&
	cgit_url "bar" &&
	cgit_url "bar/refs" &&
	cgit_url "bar/tree" &&
	cgit_url "bar/log" &&
	cgit_url "bar/diff" &&
	cgit_url "bar/patch" &&
	ls cache >output &&
	test_line_count = 0 output
'

test_expect_success 'verify cache-size=1' '

	rm -f cache/* &&
	sed -e "s/cache-size=0$/cache-size=1/" cgitrc >cgitrc.tmp &&
	mv -f cgitrc.tmp cgitrc &&
	cgit_url "" &&
	cgit_url "foo" &&
	cgit_url "foo/refs" &&
	cgit_url "foo/tree" &&
	cgit_url "foo/log" &&
	cgit_url "foo/diff" &&
	cgit_url "foo/patch" &&
	cgit_url "bar" &&
	cgit_url "bar/refs" &&
	cgit_url "bar/tree" &&
	cgit_url "bar/log" &&
	cgit_url "bar/diff" &&
	cgit_url "bar/patch" &&
	ls cache >output &&
	test_line_count = 1 output
'

test_expect_success 'verify cache-size=1021' '

	rm -f cache/* &&
	sed -e "s/cache-size=1$/cache-size=1021/" cgitrc >cgitrc.tmp &&
	mv -f cgitrc.tmp cgitrc &&
	cgit_url "" &&
	cgit_url "foo" &&
	cgit_url "foo/refs" &&
	cgit_url "foo/tree" &&
	cgit_url "foo/log" &&
	cgit_url "foo/diff" &&
	cgit_url "foo/patch" &&
	cgit_url "bar" &&
	cgit_url "bar/refs" &&
	cgit_url "bar/tree" &&
	cgit_url "bar/log" &&
	cgit_url "bar/diff" &&
	cgit_url "bar/patch" &&
	ls cache >output &&
	test_line_count = 13 output &&
	cgit_url "foo/ls_cache" >output.full &&
	strip_headers <output.full >output &&
	test_line_count = 13 output &&
	# Check that ls_cache output is cached correctly
	cgit_url "foo/ls_cache" >output.second &&
	test_cmp output.full output.second
'

test_done
