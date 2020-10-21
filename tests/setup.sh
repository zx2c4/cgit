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

# We don't want to run Git commands through Valgrind, so we filter out the
# --valgrind option here and handle it ourselves.  We copy the arguments
# assuming that none contain a newline, although other whitespace is
# preserved.
LF='
'
test_argv=

while test $# != 0
do
	case "$1" in
	--va|--val|--valg|--valgr|--valgri|--valgrin|--valgrind)
		cgit_valgrind=t
		test_argv="$test_argv${LF}--verbose"
		;;
	*)
		test_argv="$test_argv$LF$1"
		;;
	esac
	shift
done

OLDIFS=$IFS
IFS=$LF
set -- $test_argv
IFS=$OLDIFS

: ${TEST_DIRECTORY=$(pwd)/../git/t}
: ${TEST_OUTPUT_DIRECTORY=$(pwd)}
TEST_NO_CREATE_REPO=YesPlease
. "$TEST_DIRECTORY"/test-lib.sh

# Prepend the directory containing cgit to PATH.
if test -n "$cgit_valgrind"
then
	GIT_VALGRIND="$TEST_DIRECTORY/valgrind"
	CGIT_VALGRIND=$(cd ../valgrind && pwd)
	PATH="$CGIT_VALGRIND/bin:$PATH"
	export GIT_VALGRIND CGIT_VALGRIND
else
	PATH="$(pwd)/../..:$PATH"
fi

FILTER_DIRECTORY=$(cd ../filters && pwd)

if cgit --version | grep -F -q "[+] Lua scripting"; then
	export CGIT_HAS_LUA=1
else
	export CGIT_HAS_LUA=0
fi

mkrepo() {
	name=$1
	count=$2
	test_create_repo "$name"
	(
		cd "$name"
		n=1
		while test $n -le $count
		do
			echo $n >file-$n
			git add file-$n
			git commit -m "commit $n"
			n=$(expr $n + 1)
		done
		case "$3" in
		testplus)
			echo "hello" >a+b
			git add a+b
			git commit -m "add a+b"
			git branch "1+2"
			;;
		commit-graph)
			git commit-graph write
			;;
		esac
	)
}

setup_repos()
{
	rm -rf cache
	mkdir -p cache
	mkrepo repos/foo 5 >/dev/null
	mkrepo repos/bar 50 commit-graph >/dev/null
	mkrepo repos/foo+bar 10 testplus >/dev/null
	mkrepo "repos/with space" 2 >/dev/null
	mkrepo repos/filter 5 testplus >/dev/null
	cat >cgitrc <<EOF
virtual-root=/
cache-root=$PWD/cache

cache-size=1021
snapshots=tar.gz tar.bz tar.lz tar.xz tar.zst zip
enable-log-filecount=1
enable-log-linecount=1
summary-log=5
summary-branches=5
summary-tags=5
clone-url=git://example.org/\$CGIT_REPO_URL.git
enable-filter-overrides=1

repo.url=foo
repo.path=$PWD/repos/foo/.git
# Do not specify a description for this repo, as it then will be assigned
# the constant value "[no description]" (which actually used to cause a
# segfault).

repo.url=bar
repo.path=$PWD/repos/bar/.git
repo.desc=the bar repo

repo.url=foo+bar
repo.path=$PWD/repos/foo+bar/.git
repo.desc=the foo+bar repo

repo.url=with space
repo.path=$PWD/repos/with space/.git
repo.desc=spaced repo

repo.url=filter-exec
repo.path=$PWD/repos/filter/.git
repo.desc=filtered repo
repo.about-filter=exec:$FILTER_DIRECTORY/dump.sh
repo.commit-filter=exec:$FILTER_DIRECTORY/dump.sh
repo.email-filter=exec:$FILTER_DIRECTORY/dump.sh
repo.source-filter=exec:$FILTER_DIRECTORY/dump.sh
repo.readme=master:a+b
EOF

	if [ $CGIT_HAS_LUA -eq 1 ]; then
		cat >>cgitrc <<EOF
repo.url=filter-lua
repo.path=$PWD/repos/filter/.git
repo.desc=filtered repo
repo.about-filter=lua:$FILTER_DIRECTORY/dump.lua
repo.commit-filter=lua:$FILTER_DIRECTORY/dump.lua
repo.email-filter=lua:$FILTER_DIRECTORY/dump.lua
repo.source-filter=lua:$FILTER_DIRECTORY/dump.lua
repo.readme=master:a+b
EOF
	fi
}

cgit_query()
{
	CGIT_CONFIG="$PWD/cgitrc" QUERY_STRING="$1" cgit
}

cgit_url()
{
	CGIT_CONFIG="$PWD/cgitrc" QUERY_STRING="url=$1" cgit
}

strip_headers() {
	while read -r line
	do
		test -z "$line" && break
	done
	cat
}

test -z "$CGIT_TEST_NO_CREATE_REPOS" && setup_repos
