/*
 * sys/default/chardef.h: character set specific #defines for mg 2a
 * Warning: System specific ones exist
 */

#ifndef	CHARMASK
/*
 * Casting should be at least as efficent as anding with 0xff,
 * and won't have the size problems.  Override in sysdef.h if no
 * unsigned char type.
 */
#define	CHARMASK(c)	((unsigned char) (c))
#endif


/*
 * These flags, and the macros below them, make up a do-it-yourself
 * set of "ctype" macros that understand the DEC multinational set,
 * and let me ask a slightly different set of questions.
 */

#define _W	0x01			/* Word.			*/
#define _U	0x02			/* Upper case letter.		*/
#define _L	0x04			/* Lower case letter.		*/
#define _C	0x08			/* Control.			*/
#define	_D	0x10			/* is decimal digit		*/
#define _S	0x20			/* General whitespace: HT,LF,VT,FF,CR,Space */

#define ISWORD(c)	((cinfo[CHARMASK(c)]&_W)!=0)
#define ISCTRL(c)	((cinfo[CHARMASK(c)]&_C)!=0)
#define ISUPPER(c)	((cinfo[CHARMASK(c)]&_U)!=0)
#define ISLOWER(c)	((cinfo[CHARMASK(c)]&_L)!=0)
#define	ISDIGIT(c)	((cinfo[CHARMASK(c)]&_D)!=0)
#define	ISALPHA(c)	((cinfo[CHARMASK(c)]&(_U|_L))!=0)
#define	ISALNUM(c)	((cinfo[CHARMASK(c)]&(_U|_L|_D))!=0)
#define	ISSPACE(c)	((cinfo[CHARMASK(c)]&_S)!=0)
#define ISDELIM(c)	((cinfo[CHARMASK(c)]&(_W|_C|_S))==0)
#define ISASCII(c)	((c) >= 0 && (c) <= 127)
#define TOUPPER(c)	((c)-0x20)
#define TOLOWER(c)	((c)+0x20)


/*
 * generally useful thing for chars
 */

#define CCHR(x)		((x) ^ 0x40)	/* CCHR('?') == DEL */

#ifndef	METACH
#define	METACH	CCHR('[')
#endif
