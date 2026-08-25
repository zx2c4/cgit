#!/bin/sh

test_description='Check the Expires header on error pages'
. ./setup.sh

# cgit_vprint_error_page() derives page.expires from cache-dynamic-ttl, which
# is a count of minutes, while page.expires is an absolute time. Checking the
# result needs no date arithmetic: a zero TTL has to reproduce Last-Modified
# exactly, and a non-zero one has to land somewhere else without falling back
# to the epoch.

header_value() {
	sed -n "s/^$1: //p" "$2" | tr -d '\r'
}

test_expect_success 'a non-zero dynamic TTL expires the error page later' '
	cgit_url "bar/commit/&id=deadbeefdeadbeefdeadbeefdeadbeefdeadbeef" >output &&
	grep -q "Status: 404 Not found" output &&
	modified=$(header_value Last-Modified output) &&
	expires=$(header_value Expires output) &&
	test -n "$modified" &&
	test -n "$expires" &&
	test "$modified" != "$expires" &&
	case "$expires" in *1970*) return 1 ;; esac
'

test_expect_success 'a zero dynamic TTL expires the error page immediately' '
	rm -rf cache && mkdir -p cache &&
	printf "cache-dynamic-ttl=0\n" >>cgitrc &&
	cgit_url "bar/commit/&id=deadbeefdeadbeefdeadbeefdeadbeefdeadbeef" >output &&
	modified=$(header_value Last-Modified output) &&
	expires=$(header_value Expires output) &&
	test -n "$modified" &&
	test "$modified" = "$expires"
'

test_done
