VERSION= 250112
PKG = mg-${VERSION}.pkg
PREFIX= /usr/local
INSTALL_DIR=${:! pwd !}/stage

.PHONY: all clean clean-stage

all: ${PKG}

realclean: clean
	rm -f ${PKG}

clean: clean-stage
	rm -f  pkg-plist manifest
	rm -rf build_dir

clean-stage:
	rm -rf stage

src:
	[ -d src ] && cd src; git pull
	[ -d src ] || git clone https://github.com/paaguti/mg3a.git src

SRC_FILES = ${:! find src -type f !}

${PKG}: clean-stage src
	mkdir -p ${INSTALL_DIR}${PREFIX}
	cd src; pwd; bash bootstrap.sh
	cd src; pwd; bash configure
	gmake -C src all
	echo gmake DESTDIR=${INSTALL_DIR} -C src install
	gmake DESTDIR=${INSTALL_DIR} -C src install
	cat manifest.in | sed -e 's|%%VERSION%%|${VERSION}|' | sed -e 's|%%PREFIX%%|${PREFIX}|' > manifest
	echo ${:! find stage -type f !:C/^stage//} | tr ' ' '\n' > pkg-plist
	pkg create -M manifest -r stage -p pkg-plist
