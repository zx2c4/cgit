#!/bin/sh

. ./setup.sh

prepare_tests 'Check Git version is correct'

run_test 'extract Git version from Makefile' '
	sed -n -e "/^GIT_VER[ 	]*=/ {
		s/^GIT_VER[ 	]*=[ 	]*//
		p
	}" ../Makefile >trash/makefile_version
'

run_test 'test Git version matches Makefile' '
	( cat ../git/GIT-VERSION-FILE || echo "No GIT-VERSION-FILE" ) |
	sed -e "s/GIT_VERSION[ 	]*=[ 	]*//" >trash/git_version &&
	diff -u trash/git_version trash/makefile_version
'

run_test 'test submodule version matches Makefile' '
	if ! test -e ../git/.git
	then
		echo "git/ is not a Git repository" >&2
	else
		(
			cd .. &&
			sm_sha1=$(git ls-files --stage -- git |
				sed -e "s/^[0-9]* \\([0-9a-f]*\\) [0-9]	.*$/\\1/") &&
			cd git &&
			git describe --match "v[0-9]*" $sm_sha1
		) | sed -e "s/^v//" >trash/sm_version &&
		diff -u trash/sm_version trash/makefile_version
	fi
'

tests_done
