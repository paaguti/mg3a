/*
 * This file is in the public domain.
 *
 * Author: Pedro A. Aranda <paaguti@hotmail.com>
 *
 * Modified by: Bengt Larsson.
 *
 */

/*
 * Implement a mode for editing Python code.
 */

#include "def.h"


#ifdef LANGMODE_PYTHON


static int
is_blank(INT c)
{
	return (c == ' ') || (c == '\t');
}


static LINE *
findnonblank(LINE *lp)
{
    while (lback(lp) != curbp->b_linep) {
		INT curchar, offs;
		lp = lback(lp);
		for (offs = 0; offs < llength(lp); offs++) {
			curchar = lgetc(lp,offs);
			if (is_blank(curchar))
				continue;
			if (curchar != '#')
				return lp;
			break;
		}
    }
    return lp;
}


/*
 * Get the level of indention after line lp is processed
 * return value = value affecting subsequent lines.
 */

static INT
_getpyindent(LINE *lp)
{
	INT col, i, c;

	if (getindent(lp, NULL, &col) == FALSE) return -1;

	for (i = llength(lp) - 1; i >= 0; i--) {
		c = lgetc(lp, i);

		if (c == ':') {
			if (col < MAXINT - 100) col += gettabsize(curbp);
			break;
		} else if (!is_blank(c)) {
			break;
		}
	}

	return col;
}


static INT
getpyindent(LINE *lp)
{
	return _getpyindent(findnonblank(lp));
}


/*
 * Enable/toggle python-mode
 */

INT
pymode(INT f, INT n)
{
	return changemode(curbp, f, n, "python");
}


/*
 * Attempt to indent current line
 */

INT
py_indent(INT f, INT n)
{
	return setindent(getpyindent(curwp->w_dotp));
}


/*
 * Python newline and indent
 */

INT
py_lfindent(INT f, INT n)
{
	if (n < 0) return FALSE;

	while (n-- > 0) {
		if (lnewline() == FALSE) return FALSE;
		if (py_indent(FFRAND, 1) == FALSE) return FALSE;
	}

	return TRUE;
}

#endif
