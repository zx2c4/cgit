#!/bin/sh

test_description='Ensure that git does not access $HOME'
. ./setup.sh

test -n "$(which strace 2>/dev/null)" || {
	skip_all='Skipping access validation tests: strace not found'
	test_done
	exit
}

test_expect_success 'no access to $HOME' '
	non_existant_path="/path/to/some/place/that/does/not/possibly/exist"
	while test -d "$non_existant_path"; do
		non_existant_path="$non_existant_path/$(date +%N)"
	done
	strace \
		-E HOME="$non_existant_path" \
		-E CGIT_CONFIG="$PWD/cgitrc" \
		-E QUERY_STRING="url=foo/commit" \
		-e access -f -o strace.out cgit &&
	test_must_fail grep "$non_existant_path" strace.out
'

test_done
