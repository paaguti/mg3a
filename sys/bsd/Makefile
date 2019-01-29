# $FreeBSD$

PORTNAME=	mg3a
DISTVERSION=	190113
CATEGORIES=	editors
MASTER_SITES=   http://github.com/paaguti/mg3a/archive/
DISTNAME=	${DISVERSION}

# No patches in this release
#PATCH_SITES=	https://raw.githubusercontent.com/paaguti/mg3a/master/debian/patches/
#PATCHFILES=	01-pipein.diff 02-Makefile.bsd
#PATCH_DIST_STRIP=	-p1

MAINTAINER=	paaguti@gmail.com
COMMENT=	Yet another public domain MicroEMACS-based editor with UTF8 support

LICENSE=	BSD2CLAUSE
#LICENSE_FILE=	${WRKSRC}/LICENSE

CONFLICTS_INSTALL=	mg

USE_GITHIB=	yes
GH_ACCOUNT=	paaguti
GH_PROJECT=	mg3a
GH_TAGNAME=	${DISVERSION}

WRKSRC=${WRKDIR}/mg3a-${DISTVERSION}
.include <bsd.port.mk>
