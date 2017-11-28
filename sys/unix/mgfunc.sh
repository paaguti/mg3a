#!/bin/sh
grep="$1"
shift
match="$(LC_ALL=C grep -n "^$grep[^A-Za-z0-9_]" ${*:-*.c} /dev/null | head -1)"
case $match in "") echo Not found; exit 1;; esac
file="${match%%:*}"
num="${match#*:}"
num="${num%%:*}"
mg -p"$grep" "+$num" "$file"
