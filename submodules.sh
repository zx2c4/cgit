#!/bin/sh
#
# submodules.sh: init, update or list git submodules
#
# Copyright (C) 2006 Lars Hjemli
#
# Licensed under GNU General Public License v2
#   (see COPYING for full license text)
#


usage="submodules.sh [-i | -u] [-q] [--cached] [path...]"
init=
update=
quiet=
cached=


say()
{
	if test -z "$quiet"
	then
		echo -e "$@"
	fi
}


die()
{
	echo >&2 -e "$@"
	exit 1
}



#
# Silently checkout specified submodule revision, return exit status of git-checkout
#
# $1 = local path
# $2 = requested sha1
#
module_checkout()
{
	$(cd "$1" && git checkout "$2" 1>/dev/null 2>/dev/null)
}


#
# Find all (requested) submodules, run clone + checkout on missing paths
#
# $@ = requested paths (default to all)
#
modules_init()
{
	git ls-files --stage -- $@ | grep -e '^160000 ' |
	while read mode sha1 stage path
	do
		test -d "$path/.git" && continue

		if test -d "$path"
		then
			rmdir "$path" 2>/dev/null ||
			die "Directory '$path' exist, but not as a submodule"
		fi

		test -e "$path" && die "A file already exist at path '$path'"

		url=$(sed -nre "s/^$path[ \t]+//p" .gitmodules)
		test -z "$url" && die "No url found for $path in .gitmodules"

		git clone "$url" "$path" || die "Clone of submodule '$path' failed"
		module_checkout "$path" "$sha1" || die "Checkout of submodule '$path' failed"
		say "Submodule '$path' initialized"
	done
}

#
# Checkout correct revision of each initialized submodule
#
# $@ = requested paths (default to all)
#
modules_update()
{
	git ls-files --stage -- $@ | grep -e '^160000 ' |
	while read mode sha1 stage path
	do
		if ! test -d "$path/.git"
		then
			say "Submodule '$path' not initialized"
			continue;
		fi
		subsha1=$(cd "$path" && git rev-parse --verify HEAD) ||
		die "Unable to find current revision of submodule '$path'"
		if test "$subsha1" != "$sha1"
		then
			module_checkout "$path" "$sha1" ||
			die "Unable to checkout revision $sha1 of submodule '$path'"
			say "Submodule '$path' reset to revision $sha1"
		fi
	done
}

#
# List all registered submodules, prefixed with:
#  - submodule not initialized
#  + different version checked out
#
# If --cached was specified the revision in the index will be printed
# instead of the currently checked out revision.
#
# $@ = requested paths (default to all)
#
modules_list()
{
	git ls-files --stage -- $@ | grep -e '^160000 ' |
	while read mode sha1 stage path
	do
		if ! test -d "$path/.git"
		then
			say "-$sha1 $path"
			continue;
		fi
		revname=$(cd "$path" && git describe $sha1)
		if git diff-files --quiet -- "$path"
		then
			say " $sha1 $path\t($revname)"
		else
			if test -z "$cached"
			then
				sha1=$(cd "$path" && git rev-parse HEAD)
				revname=$(cd "$path" && git describe HEAD)
			fi
			say "+$sha1 $path\t($revname)"
		fi
	done
}


while case "$#" in 0) break ;; esac
do
	case "$1" in
	-i)
		init=1
		;;
	-u)
		update=1
		;;
	-q)
		quiet=1
		;;
	--cached)
		cached=1
		;;
	--)
		break
		;;
	-*)
		echo "Usage: $usage"
		exit 1
		;;
	--*)
		echo "Usage: $usage"
		exit 1
		;;
	*)
		break
		;;
	esac
	shift
done


if test "$init" = "1"
then
	modules_init $@
elif test "$update" = "1"
then
	modules_update $@
else
	modules_list $@
fi
