#!/bin/sh

test_description='Check Git version is correct'
CGIT_TEST_NO_CREATE_REPOS=YesPlease
. ./setup.sh

test_expect_success 'extract Git version from Makefile' '
	sed -n -e "/^GIT_VER[ 	]*=/ {
		s/^GIT_VER[ 	]*=[ 	]*//
		p
	}" ../../Makefile >makefile_version
'

# Note that Git's GIT-VERSION-GEN script applies "s/-/./g" to the version
# string to produce the internal version in the GIT-VERSION-FILE, so we
# must apply the same transformation to the version in the Makefile before
# comparing them.
test_expect_success 'test Git version matches Makefile' '
	( cat ../../git/GIT-VERSION-FILE || echo "No GIT-VERSION-FILE" ) |
	sed -e "s/GIT_VERSION[ 	]*=[ 	]*//" -e "s/\\.dirty$//" >git_version &&
	sed -e "s/-/./g" makefile_version >makefile_git_version &&
	test_cmp git_version makefile_git_version
'

test_expect_success 'test submodule version matches Makefile' '
	if ! test -e ../../git/.git
	then
		echo "git/ is not a Git repository" >&2
	else
		(
			cd ../.. &&
			sm_sha1=$(git ls-files --stage -- git |
				sed -e "s/^[0-9]* \\([0-9a-f]*\\) [0-9]	.*$/\\1/") &&
			cd git &&
			git describe --match "v[0-9]*" $sm_sha1
		) | sed -e "s/^v//" -e "s/-/./" >sm_version &&
		test_cmp sm_version makefile_version
	fi
'

test_done
