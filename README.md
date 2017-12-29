# mg3a
An extended mg clone

This is a fork for the original mg3a code by Bengt Larson

## Status

I was in contact with Bengt Larson until April 2017. Then, suddenly, his email address was cancelled.
His repository has not been updated since the 4th of April, 2017. 
As of the 15th of December 2017, www.bengtl.net became inaccessible and thus the original code repository www.bengtl.net/files/mg3a is lost. 

## Intent of this repository

The intent of this repository is to preserve the original code and establish a home for further developing this very nice piece of software. My focus will be on Debian/ubuntu, OSX and FreeBSD, which are the three operating systems I use. I'll try not to break things too much and keep compatibility wiht MinGW/CygWin (although it's ages I haven't actively developed anything for that environment)

## What is original, what has been added

The closest to the original code is the first tag in this github. My initial additions have initially been included as patches in the Debian building infrastructure (which is also mine) I habe included a FreeBSD ports directory under sys/bsd and the Homebrew .rb file from my OSX Homebrew personal tap. under sys/osx

## Licensing, etc. 

Refer to the different README.* files

## Ways of working

Always preserve the original code. Use quilt to add features correct bugs. The home for the patches is in debian/patches

