#!/bin/sh

test_description='Check filtered content'
. ./setup.sh

test_expect_success 'generate filter/tree/a%2bb' '
	cgit_url "filter/tree/a%2bb" >tmp
'

test_expect_success 'check whether the source filter works' '
	grep "<code>HELLO$" tmp
'

test_expect_success 'generate filter/about/' '
	cgit_url "filter/about/" >tmp
'

test_expect_success 'check whether the about filter works' '
	grep "<div id='"'"'summary'"'"'>HELLO$" tmp
'

test_expect_success 'generate filter/commit/' '
	cgit_url "filter/commit/" >tmp
'

test_expect_success 'check whether the commit filter works' '
	grep "<div class='"'"'commit-subject'"'"'>ADD A+B" tmp
'

test_expect_success 'check whether the email filter works for authors' '
	grep "<AUTHOR@EXAMPLE.COM>" tmp
'

test_expect_success 'check whether the email filter works for committers' '
	grep "<COMMITTER@EXAMPLE.COM>" tmp
'

test_done
