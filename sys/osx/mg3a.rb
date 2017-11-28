require "formula"

class Mg3a < Formula
homepage "http://www.bengtl.net/files/mg3a/"
url "http://www.bengtl.net/files/mg3a/mg3a.140514.tar.gz"
sha1 "d69ef01fb7f47d8d568825d7023ab1a54decf392"

option "with-c-mode", "Include the original C mode"
option "with-clike-mode", "Include the C mode that also handles Perl and Java"
option "with-python-mode", "Include the Python mode"
option "without-dired", "Exclude dired functions"
option "without-prefix-region", "Exclude the prefix region mode"
option "with-user-modes", "Include the support for user defined modes"
option "with-user-macros", "Include the support for user defined macros"
option "with-most","Include c-like and python modes, user modes and user macros"
option "with-all","Include all fancy stuff"

# currently, Bengt's makefile doesn't allow to customize
# the supported modes while making mg. We need a small patch for that
# should be removed when that is fixed
patch :p1, :DATA

def install
mg3aopts=""
mg3aopts << "-DDIRED " if build.with? 'dired'
mg3aopts << "-DPREFIXREGION " if build.with? 'prefix-region'
mg3aopts << " -DLANGMODE_C" if build.with? 'c-mode'
mg3aopts << " -DLANGMODE_PYTHON" if build.with? 'python-mode'
mg3aopts << " -DLANGMODE_CLIKE" if build.with? 'clike-mode'
mg3aopts << " -DUSER_MODES" if build.with? 'user-modes'
mg3aopts << " -DUSER_MACROS" if build.with? 'user-macros'
mg3aopts = "-DDIRED -DPREFIXREGION -DLANGMODE_PYTHON -DLANGMODE_CLIKE -DUSER_MODES -DUSER_MACROS" if build.with? 'most'
mg3aopts = "-DALL" if build.with? 'all'

system "make CDEFS=\"#{mg3aopts}\" clean doit"
bin.install "mg"
doc.install Dir['bl/dot.*']
doc.install Dir['README*']
end

test do
system "false"
end
end
__END__
--- a/Makefile	2014-05-12 15:20:44.000000000 +0200
+++ b/Makefile	2014-05-12 15:20:07.000000000 +0200
@@ -30,7 +30,7 @@
 #CDEFS	= -DBSMAP=1 -DMAKEBACKUP=1

 # Everything GNU Emacs and mg2a-like by default
-CDEFS = -DDIRED -DPREFIXREGION
+CDEFS ?= -DDIRED -DPREFIXREGION

 # Everything.
 #CDEFS	= -DALL

