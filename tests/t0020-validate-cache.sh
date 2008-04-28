#!/bin/sh

. ./setup.sh

prepare_tests 'Validate cache'

run_test 'verify cache-size=0' '

	rm -f trash/cache/* &&
	sed -i -e "s/cache-size=1021$/cache-size=0/" trash/cgitrc &&
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
	test 0 -eq $(ls trash/cache | wc -l)
'

run_test 'verify cache-size=1' '

	rm -f trash/cache/* &&
	sed -i -e "s/cache-size=0$/cache-size=1/" trash/cgitrc &&
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
	test 1 -eq $(ls trash/cache | wc -l)
'

run_test 'verify cache-size=1021' '

	rm -f trash/cache/* &&
	sed -i -e "s/cache-size=1$/cache-size=1021/" trash/cgitrc &&
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
	test 13 -eq $(ls trash/cache | wc -l)
'

tests_done
