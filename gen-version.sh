v=$(git-describe --abbrev=4 HEAD | sed -e 's/-/./g')
test -z "$v" && exit 1
echo "CGIT_VERSION = $v"
echo "CGIT_VERSION = $v" > VERSION
