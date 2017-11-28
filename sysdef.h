/*
 *		System V system header file
 */

/* Needed on Linux and Cygwin for getting the prototypes for wcwidth()	*/
/* and wcswidth().							*/

#if defined(__linux__) || defined(__CYGWIN__)
#define _XOPEN_SOURCE 500
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#define	KBLOCK	8192			/* Kill grow.			*/
#define	GOOD	0			/* Good exit status.		*/

typedef uint_least16_t ucs2;		/* BMP Unicode			*/
typedef int_least32_t ucs4;		/* General Unicode		*/
typedef ucs4	KCHAR;			/* Type for internal keystrokes	*/

#define MALLOCROUND 8			/* round up to 8 byte boundary	*/

#define bcopy(s,d,n)	memmove((d),(s),(n))	/* memory-to-memory copy	*/
#define	gettermtype()	getenv("TERM")	/* determine terminal type	*/

