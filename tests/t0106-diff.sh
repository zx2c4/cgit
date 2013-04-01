#!/bin/sh

test_description='Check content on diff page'
. ./setup.sh

test_expect_success 'generate foo/diff' 'cgit_url "foo/diff" >tmp'
test_expect_success 'find diff header' 'grep "a/file-5 b/file-5" tmp'
test_expect_success 'find blob link' 'grep "<a href=./foo/tree/file-5?id=" tmp'
test_expect_success 'find added file' 'grep "new file mode 100644" tmp'

test_expect_success 'find hunk header' '
	grep "<div class=.hunk.>@@ -0,0 +1 @@</div>" tmp
'

test_expect_success 'find added line' '
	grep "<div class=.add.>+5</div>" tmp
'

test_done
