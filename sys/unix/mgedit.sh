#!/bin/sh
case $1 in
	-*) opts="$1"; grep="$2"; shift 2;;
	*) opts=""; grep="$1"; shift;;
esac
mg -p"$grep" $(LC_ALL=C grep -l $opts -e "$grep" ${*:-*.c})
