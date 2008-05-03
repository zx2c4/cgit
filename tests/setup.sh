# This file should be sourced by all test-scripts
#
# Main functions:
#   prepare_tests(description) - setup for testing, i.e. create repos+config
#   run_test(description, script) - run one test, i.e. eval script
#
# Helper functions
#   cgit_query(querystring) - call cgit with the specified querystring
#   cgit_url(url) - call cgit with the specified virtual url
#
# Example script:
#
# . setup.sh
# prepare_tests "html validation"
# run_test 'repo index' 'cgit_url "/" | tidy -e'
# run_test 'repo summary' 'cgit_url "/foo" | tidy -e'


mkrepo() {
	name=$1
	count=$2
	dir=$PWD
	test -d $name && return
	printf "Creating testrepo %s\n" $name
	mkdir -p $name
	cd $name
	git init
	for ((n=1; n<=count; n++))
	do
		echo $n >file-$n
		git add file-$n
		git commit -m "commit $n"
	done
	cd $dir
}

setup_repos()
{
	rm -rf trash/cache
	mkdir -p trash/cache
	mkrepo trash/repos/foo 5 >/dev/null
	mkrepo trash/repos/bar 50 >/dev/null
	cat >trash/cgitrc <<EOF
virtual-root=/
cache-root=$PWD/trash/cache

cache-size=1021
snapshots=tar.gz tar.bz zip
enable-log-filecount=1
enable-log-linecount=1
summary-log=5
summary-branches=5
summary-tags=5

repo.url=foo
repo.path=$PWD/trash/repos/foo/.git
# Do not specify a description for this repo, as it then will be assigned
# the constant value "[no description]" (which actually used to cause a
# segfault).

repo.url=bar
repo.path=$PWD/trash/repos/bar/.git
repo.desc=the bar repo
EOF
}

prepare_tests()
{
	setup_repos
	rm -f test-output.log 2>/dev/null
	test_count=0
	test_failed=0
	echo "[$0]" "$@" >test-output.log
	echo "$@" "($0)"
}

tests_done()
{
	printf "\n"
	if test $test_failed -gt 0
	then
		printf "test: *** %s failure(s), logfile=%s\n" \
			$test_failed "$(pwd)/test-output.log"
		false
	fi
}

run_test()
{
	desc=$1
	script=$2
	((test_count++))
	printf "\ntest %d: name='%s'\n" $test_count "$desc" >>test-output.log
	printf "test %d: eval='%s'\n" $test_count "$2" >>test-output.log
	eval "$2" >>test-output.log 2>>test-output.log
	res=$?
	printf "test %d: exitcode=%d\n" $test_count $res >>test-output.log
	if test $res = 0
	then
		printf " %2d) %-60s [ok]\n" $test_count "$desc"
	else
		((test_failed++))
		printf " %2d) %-60s [failed]\n" $test_count "$desc"
	fi
}

cgit_query()
{
	CGIT_CONFIG="$PWD/trash/cgitrc" QUERY_STRING="$1" "$PWD/../cgit"
}

cgit_url()
{
	CGIT_CONFIG="$PWD/trash/cgitrc" QUERY_STRING="url=$1" "$PWD/../cgit"
}

