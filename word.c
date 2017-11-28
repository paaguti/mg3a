/*
 *		Word mode commands.
 *
 * The routines in this file implement commands that work word at a
 * time. There are all sorts of word mode commands.
 */

#include	"def.h"

INT	forwword(INT f, INT n);
INT	backchar(INT f, INT n);
INT	forwchar(INT f, INT n);

/*
 * Move the cursor backward by "n" words. All of the details of motion
 * are performed by the "backchar" and "forwchar" routines.
 */

INT
backword(INT f, INT n)
{
	if (n < 0) return forwword(f, -n);
	if (backchar(FFRAND, 1) == FALSE)
		return FALSE;
	while (n--) {
		while (inword() == FALSE) {
			if (backchar(FFRAND, 1) == FALSE)
				return TRUE;
		}
		while (inword() != FALSE) {
			if (backchar(FFRAND, 1) == FALSE)
				return TRUE;
		}
	}
	return forwchar(FFRAND, 1);
}


/*
 * Move the cursor forward by the specified number of words. All of
 * the motion is done by "forwchar".
 */

INT
forwword(INT f, INT n)
{
	if (n < 0)
		return backword(f, -n);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FFRAND, 1) == FALSE)
				return TRUE;
		}
		while (inword() != FALSE) {
			if (forwchar(FFRAND, 1) == FALSE)
				return TRUE;
		}
	}
	return TRUE;
}


/*
 * Mg3a: Help function since they are so alike.
 */

static INT
changeword(INT firstcase, INT restcase, INT n)
{
	INT	c;
	INT nextoffset;
	charset_t charset = curbp->charset;
	INT *special;
	INT i, speclen, first;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0) return FALSE;

	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FFRAND, 1) == FALSE)
				return TRUE;
		}

		first = 1;

		while (inword() != FALSE) {
			c = ucs_char(charset, curwp->w_dotp, curwp->w_doto, &nextoffset);

			if (ucs_changecase(first ? firstcase : restcase, c, &special, &speclen)) {
				if (ldeleteraw(nextoffset - curwp->w_doto, KNONE) == FALSE) return FALSE;

				for (i = 0; i < speclen; i++) {
					if (linsert_ucs(charset, 1, special[i]) == FALSE)
						return FALSE;
				}
			} else {
				adjustpos(curwp->w_dotp, nextoffset);
			}

			first = 0;
		}
	}
	return TRUE;
}


/*
 * Move the cursor forward by the specified number of words. As you
 * move, convert any characters to upper case.
 */

INT
upperword(INT f, INT n)
{
	return changeword(CASEUP, CASEUP, n);
}


/*
 * Move the cursor forward by the specified number of words. As you
 * move convert characters to lower case.
 */

INT
lowerword(INT f, INT n)
{
	return changeword(CASEDOWN, CASEDOWN, n);
}


/*
 * Move the cursor forward by the specified number of words. As you
 * move convert the first character of the word to upper case, and
 * subsequent characters to lower case. Error if you try and move past
 * the end of the buffer.
 *
 * Mg3a: Use titlecase instead of uppercase.
 */

INT
capword(INT f, INT n)
{
	return changeword(CASETITLE, CASEDOWN, n);
}


/*
 * Kill forward by "n" words.
 */

INT
delfword(INT f, INT n)
{
	INT	size;
	LINE	*dotp;
	INT	doto;
	size_t	dotpos;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0) return FALSE;

	initkill();

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	dotpos  = curwp->w_dotpos;
	size = 0;

	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FFRAND, 1) == FALSE)
				goto out;	/* Hit end of buffer.	*/
			++size;
		}
		while (inword() != FALSE) {
			if (forwchar(FFRAND, 1) == FALSE)
				goto out;	/* Hit end of buffer.	*/
			++size;
		}
	}
out:
	adjustpos3(dotp, doto, dotpos);
	return ldelete(size, KFORW);
}


/*
 * Kill backwards by "n" words. The rules for success and failure are
 * now different, to prevent strange behavior at the start of the
 * buffer. The command only fails if something goes wrong with the
 * actual delete of the characters. It is successful even if no
 * characters are deleted, or if you say delete 5 words, and there are
 * only 4 words left. I considered making the first call to "backchar"
 * special, but decided that that would just be wierd. Normally this
 * is bound to "M-Rubout" and to "M-Backspace".
 */

INT
delbword(INT f, INT n)
{
	INT	size;
	LINE	*dotp;
	INT	doto;
	size_t	dotpos;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0) return FALSE;
	initkill();
	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	dotpos  = curwp->w_dotpos;
	if (backchar(FFRAND, 1) == FALSE)
		return (TRUE);			/* Hit buffer start.	*/
	size = 1;				/* One deleted.		*/
	while (n--) {
		while (inword() == FALSE) {
			if (backchar(FFRAND, 1) == FALSE)
				goto out;	/* Hit buffer start.	*/
			++size;
		}
		while (inword() != FALSE) {
			if (backchar(FFRAND, 1) == FALSE)
				goto out;	/* Hit buffer start.	*/
			++size;
		}
	}
	if (forwchar(FFRAND, 1) == FALSE)
		return FALSE;
	--size;					/* Undo assumed delete. */
out:
	adjustpos3(dotp, doto, dotpos);
	return ldeleteback(size, KBACK);
}


/*
 * Return TRUE if the character at dot is a character that is
 * considered to be part of a word. The word character list is hard
 * coded. Should be setable.
 */

int
inword()
{
/* can't use lgetc in ISWORD due to bug in OSK cpp */
//	return curwp->w_doto != llength(curwp->w_dotp) &&
//		ISWORD(curwp->w_dotp->l_text[curwp->w_doto]);

	return (curwp->w_doto != llength(curwp->w_dotp) &&
		ucs_isword(ucs_char(curbp->charset, curwp->w_dotp, curwp->w_doto, NULL)))
		? TRUE : FALSE;
}

