/*
 *		Character class tables.
 *
 * Do it yourself character classification macros, that understand the
 * multinational character set, and let me ask some questions the
 * standard macros (in ctype.h) don't let you ask.
 */

#include	"def.h"

/*
 * This table, indexed by a character drawn from the 256 member
 * character set, is used by my own character type macros to answer
 * questions about the type of a character.
 *
 * Mg3a: Tweaked to be useful for Mg3a when the locale isn't
 * recognized.
 */

const uint_least8_t cinfo[256] = {
	_C,		_C,		_C,		_C,	/* 0x0X */
	_C,		_C,		_C,		_C,
	_C,		_C|_S,		_C|_S,		_C|_S,
	_C|_S,		_C|_S,		_C,		_C,
	_C,		_C,		_C,		_C,	/* 0x1X */
	_C,		_C,		_C,		_C,
	_C,		_C,		_C,		_C,
	_C,		_C,		_C,		_C,
	_S,		0,		0,		0,	/* 0x2X */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	_D|_W,		_D|_W,		_D|_W,		_D|_W,	/* 0x3X */
	_D|_W,		_D|_W,		_D|_W,		_D|_W,
	_D|_W,		_D|_W,		0,		0,
	0,		0,		0,		0,
	0,		_U|_W,		_U|_W,		_U|_W,	/* 0x4X */
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		_U|_W,	/* 0x5X */
	_U|_W,		_U|_W,		_U|_W,		_U|_W,
	_U|_W,		_U|_W,		_U|_W,		0,
	0,		0,		0,		0,
	0,		_L|_W,		_L|_W,		_L|_W,	/* 0x6X */
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		_L|_W,	/* 0x7X */
	_L|_W,		_L|_W,		_L|_W,		_L|_W,
	_L|_W,		_L|_W,		_L|_W,		0,
	0,		0,		0,		_C,
	_W,		_W,		_W,		_W,	/* 0x8X */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,	/* 0x9X */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,	/* 0xAX */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,	/* 0xBX */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,	/* 0xCX */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,	/* 0xDX */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,	/* 0xEX */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,	/* 0xFX */
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
	_W,		_W,		_W,		_W,
};


/*
 * Find the name of a keystroke. Returns a pointer to the terminating
 * '\0'.
 */

char *
keyname(char *cp, INT k)
{
    char *np;

    if(k < 0) k = CHARMASK(k);			/* sign extended char */
    switch(k) {
//	case CCHR('@'): np = "NUL"; break;
	case CCHR('I'): np = "TAB"; break;
//	case CCHR('J'): np = "LFD"; break; /* yuck, but that's what GNU calls it */
	case CCHR('M'): np = "RET"; break;
	case CCHR('['): np = "ESC"; break;
	case ' ':	np = "SPC"; break; /* yuck again */
	case CCHR('?'): np = "DEL"; break;
	default:
	    if (k >= 128) {
	    	if (termcharset) {
		    	sprintf(cp, "U+%04X", (int)k);
		} else {
		    	sprintf(cp, "0%o", (int)k);
		}
		cp += strlen(cp);
		return cp;
	    }
	    if(k < ' ') {
		*cp++ = 'C';
		*cp++ = '-';
		k = CCHR(k);
		if(ISUPPER(k)) k = TOLOWER(k);
	    }
	    *cp++ = k;
	    *cp = '\0';
	    return cp;
    }
    strcpy(cp, np);
    return cp + strlen(cp);
}
