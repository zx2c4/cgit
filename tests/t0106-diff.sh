#!/bin/sh

. ./setup.sh

prepare_tests "Check content on diff page"

run_test 'generate foo/diff' 'cgit_url "foo/diff" >trash/tmp'
run_test 'find diff header' 'grep "a/file-5 b/file-5" trash/tmp'
run_test 'find blob link' 'grep "<a href=./foo/tree/file-5?id=" trash/tmp'
run_test 'find added file' 'grep "new file mode 100644" trash/tmp'

run_test 'find hunk header' '
	grep "<div class=.hunk.>@@ -0,0 +1 @@</div>" trash/tmp
'

run_test 'find added line' '
	grep "<div class=.add.>+5</div>" trash/tmp
'

tests_done
