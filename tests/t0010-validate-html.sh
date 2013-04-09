#!/bin/sh

test_description='Validate html with tidy'
. ./setup.sh


test_url()
{
	tidy_opt="-eq"
	test -z "$NO_TIDY_WARNINGS" || tidy_opt+=" --show-warnings no"
	cgit_url "$1" >tidy-$test_count.tmp || return
	sed -e "1,4d" tidy-$test_count.tmp >tidy-$test_count || return
	"$tidy" $tidy_opt tidy-$test_count
	rc=$?

	# tidy returns with exitcode 1 on warnings, 2 on error
	if test $rc = 2
	then
		false
	else
		:
	fi
}

tidy=`which tidy 2>/dev/null`
test -n "$tidy" || {
	skip_all='Skipping html validation tests: tidy not found'
	test_done
	exit
}

test_expect_success 'index page' 'test_url ""'
test_expect_success 'foo' 'test_url "foo"'
test_expect_success 'foo/log' 'test_url "foo/log"'
test_expect_success 'foo/tree' 'test_url "foo/tree"'
test_expect_success 'foo/tree/file-1' 'test_url "foo/tree/file-1"'
test_expect_success 'foo/commit' 'test_url "foo/commit"'
test_expect_success 'foo/diff' 'test_url "foo/diff"'

test_done
