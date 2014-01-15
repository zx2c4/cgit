#!/bin/sh

[ "$#" -gt 0 ] && printf "%s " "$*"
tr '[:lower:]' '[:upper:]'
