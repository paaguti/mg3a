# $FreeBSD$

PORTNAME=	mg3a
DISTVERSION=	171201
CATEGORIES=	editors
MASTER_SITES=   http://github.com/paaguti/mg3a/archive/
DISTNAME=	171201

PATCH_SITES=	https://raw.githubusercontent.com/paaguti/mg3a/master/debian/patches/
PATCHFILES=	01-pipein.diff 02-Makefile.bsd
PATCH_DIST_STRIP=	-p1

MAINTAINER=	paaguti@gmail.com
COMMENT=	Yet another public domain MicroEMACS-based editor with UTF8 support

LICENSE=	BSD2CLAUSE
#LICENSE_FILE=	${WRKSRC}/LICENSE

CONFLICTS_INSTALL=	mg

USE_GITHIB=	yes
GH_ACCOUNT=	paaguti
GH_PROJECT=	mg3a
GH_TAGNAME=	171201

WRKSRC=${WRKDIR}/mg3a-${DISTVERSION}
.include <bsd.port.mk>
