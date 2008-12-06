#!/bin/sh

. ./setup.sh


test_url()
{
	tidy_opt="-eq"
	test -z "$NO_TIDY_WARNINGS" || tidy_opt+=" --show-warnings no"
	cgit_url "$1" >trash/tidy-$test_count || return
	sed -ie "1,4d" trash/tidy-$test_count || return
	"$tidy" $tidy_opt trash/tidy-$test_count
	rc=$?

	# tidy returns with exitcode 1 on warnings, 2 on error
	if test $rc = 2
	then
		false
	else
		:
	fi
}

prepare_tests 'Validate html with tidy'

tidy=`which tidy`
test -n "$tidy" || {
	echo "Skipping tests: tidy not found"
	tests_done
	exit
}

run_test 'index page' 'test_url ""'
run_test 'foo' 'test_url "foo"'
run_test 'foo/log' 'test_url "foo/log"'
run_test 'foo/tree' 'test_url "foo/tree"'
run_test 'foo/tree/file-1' 'test_url "foo/tree/file-1"'
run_test 'foo/commit' 'test_url "foo/commit"'
run_test 'foo/diff' 'test_url "foo/diff"'

tests_done
