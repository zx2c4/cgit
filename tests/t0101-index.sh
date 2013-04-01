#!/bin/sh

test_description='Check content on index page'
. ./setup.sh

test_expect_success 'generate index page' 'cgit_url "" >tmp'
test_expect_success 'find foo repo' 'grep "foo" tmp'
test_expect_success 'find foo description' 'grep "\[no description\]" tmp'
test_expect_success 'find bar repo' 'grep "bar" tmp'
test_expect_success 'find bar description' 'grep "the bar repo" tmp'
test_expect_success 'find foo+bar repo' 'grep ">foo+bar<" tmp'
test_expect_success 'verify foo+bar link' 'grep "/foo+bar/" tmp'
test_expect_success 'verify "with%20space" link' 'grep "/with%20space/" tmp'
test_expect_success 'no tree-link' '! grep "foo/tree" tmp'
test_expect_success 'no log-link' '! grep "foo/log" tmp'

test_done
