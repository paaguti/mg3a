#!/bin/bash
 aclocal
 autoreconf
 automake --add-missing
 autoconf
 ./configure
