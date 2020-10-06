#!/bin/sh

test_description='Ensure that git does not access $HOME'
. ./setup.sh

test -n "$(which strace 2>/dev/null)" || {
	skip_all='Skipping access validation tests: strace not found'
	test_done
	exit
}

strace true 2>/dev/null || {
	skip_all='Skipping access validation tests: strace not functional'
	test_done
	exit
}

test_no_home_access () {
	non_existent_path="/path/to/some/place/that/does/not/possibly/exist"
	while test -d "$non_existent_path"; do
		non_existent_path="$non_existent_path/$(date +%N)"
	done &&
	strace \
		-E HOME="$non_existent_path" \
		-E CGIT_CONFIG="$PWD/cgitrc" \
		-E QUERY_STRING="url=$1" \
		-e access -f -o strace.out cgit &&
	! grep "$non_existent_path" strace.out
}

test_no_home_access_success() {
	test_expect_success "do not access \$HOME: $1" "
		test_no_home_access '$1'
	"
}

test_no_home_access_success
test_no_home_access_success foo
test_no_home_access_success foo/refs
test_no_home_access_success foo/log
test_no_home_access_success foo/tree
test_no_home_access_success foo/tree/file-1
test_no_home_access_success foo/commit
test_no_home_access_success foo/diff
test_no_home_access_success foo/patch
test_no_home_access_success foo/snapshot/master.tar.gz

test_done
