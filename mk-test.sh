#!/bin/bash
make clean

./configure --enable-dired --enable-pipein --enable-prefixregion \
  --enable-pythonmode --enable-clikemode \
  --enable-usermodes  --enable-usermacros \
  --enable-searchall  # --disable-mouse

make
