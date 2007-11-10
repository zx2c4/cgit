#!/bin/sh

. ./setup.sh

prepare_tests "Check content on diff page"

run_test 'generate foo/diff' 'cgit_url "foo/diff" >trash/tmp'
run_test 'find diff header' 'grep -e "a/file-5 b/file-5" trash/tmp'
run_test 'find blob link' 'grep -e "<a href=./foo/tree/file-5?id=" trash/tmp'
run_test 'find added file' 'grep -e "new file mode 100644" trash/tmp'

run_test 'find hunk header' '
	grep -e "<div class=.hunk.>@@ -0,0 +1 @@</div>" trash/tmp
'

run_test 'find added line' '
	grep -e "<div class=.add.>+5</div>" trash/tmp
'

tests_done
