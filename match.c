/*
 * Name:	MicroEMACS
 *		Limited parenthesis matching routines
 *
 * The hacks in this file implement automatic matching of (), [], {},
 * and other characters. It would be better to have a full-blown
 * syntax table, but there's enough overhead in the editor as it is.
 *
 * Since I often edit Scribe code, I've made it possible to blink
 * arbitrary characters -- just bind delimiter characters to
 * "blink-and-insert"
 */

#include	"def.h"
#include	"key.h"

static INT	balance(void);

INT	selfinsert(INT f, INT n);


/*
 * Balance table. When balance() encounters a character that is to be
 * matched, it first searches this table for a balancing left-side
 * character. If the character is not in the table, the character is
 * balanced by itself. This is to allow delimiters in Scribe documents
 * to be matched.
 *
 * Mg3a: Added Unicode angled brackets.
 */

static const struct balance {
	ucs2 left, right;
} bal[] = {
	{ '(', ')' },
	{ '[', ']' },
	{ '{', '}' },
	{ '<', '>' },
	{ 0xab, 0xbb },
	{ 0x2039, 0x203a },
	{ '\0','\0'}
};


/*
 * Mg3a: Return the left paren of, or zero
 */

INT
leftparenof(INT c)
{
	INT i;

	for (i = 0;; i++) {
		if (bal[i].right == c) return bal[i].left;
		if (bal[i].right == 0) return 0;
	}
}


/*
 * Mg3a: Return the right paren of, or 0
 */

INT
rightparenof(INT c)
{
	INT i;

	for (i = 0;; i++) {
		if (bal[i].left == c) return bal[i].right;
		if (bal[i].left == 0) return 0;
	}
}


/*
 * Self-insert character, then show matching character, if any. Bound
 * to "blink-and-insert".
 */

INT
showmatch(INT f, INT n)
{
	INT  s;

	if ((s = selfinsert(f|FFRAND, n)) != TRUE) return s;
	if (balance() != TRUE) ttbeep();	/* unbalanced -- warn user */

	return TRUE;
}


/*
 * Search for and display a matching character.
 *
 * This routine does the real work of searching backward for a
 * balancing character. If such a balancing character is found, it
 * uses displaymatch() to display the match.
 *
 * Mg3a: Do Unicode chars.
 */

static INT
balance()
{
	LINE	*clp;
	INT	cbo, cbsave, size = 0;
	INT	c;
	INT	rbal, lbal;
	ssize_t	depth;
	charset_t charset = curbp->charset;

	rbal = key.k_chars[key.k_count-1];

	if (ucs_width(rbal) == 0) return FALSE;		/* Not useful */

	/* See if there is a matching character -- default to the same */

	lbal = leftparenof(rbal);
	if (lbal == 0) lbal = rbal;

	/* Move behind the inserted character.	We are always guaranteed    */
	/* that there is at least one character on the line, since one was  */
	/* just self-inserted by blinkparen.				    */

	clp = curwp->w_dotp;
	cbo = ucs_prevc(charset, clp, curwp->w_doto);

	depth = 0;			/* init nesting depth		*/

	for (;;) {
		if (cbo == 0) {			/* beginning of line	*/
			clp = lback(clp);
			if (clp == curbp->b_linep)
				return (FALSE);
			cbo = llength(clp)+1;
		}

		cbsave = cbo;

		if (cbo == llength(clp) + 1) {	/* end of line		*/
			cbo--;
			c = '\n';
		} else {			/* somewhere in middle	*/
			c = lgetc(clp, cbo - 1);

			if (c < 128) {		/* speed up ASCII	*/
				cbo--;
			} else {
				cbo = ucs_prevc(charset, clp, cbo);
				c = ucs_char(charset, clp, cbo, NULL);
			}
		}

		/* Check for a matching character.  If still in a nested */
		/* level, pop out of it and continue search.  This check */
		/* is done before the nesting check so single-character	 */
		/* matches will work too.				 */
		if (c == lbal) {
			if (depth == 0) {
				displaymatch(clp, cbo);
				return (TRUE);
			}
			else
				depth--;
		}
		/* Check for another level of nesting.	*/
		if (c == rbal)
			depth++;

		/* Limit max size like Emacs */
		size += (cbsave - cbo);
		if (size > 102400) return FALSE;
	}
	/*NOTREACHED*/
}


/*
 * Mg3a: How long to blink the matching paren, in milliseconds
 */

INT blink_wait = 1000;


/*
 * Display matching character. Matching characters that are not in the
 * current window are displayed in the echo line. If in the current
 * window, move dot to the matching character, sit there a while, then
 * move back.
 */

void
displaymatch(LINE *clp, INT cbo)
{
	LINE	*tlp;
	INT	tbo;
	INT	i;
	INT	bufo;
	INT	c;
	int	inwindow;

	/* Figure out if matching char is in current window by	*/
	/* searching from the top of the window to dot.		*/

	inwindow = 0;
	for (tlp = curwp->w_linep; tlp != lforw(curwp->w_dotp); tlp = lforw(tlp))
		if (tlp == clp)
			inwindow = 1;

	if (inwindow && blink_wait > 0 && blink_wait <= 10000) {
		tlp = curwp->w_dotp;		/* save current position */
		tbo = curwp->w_doto;

		adjustpos(clp, cbo);		/* move to new position */

		update();			/* show match */
		waitforinput(blink_wait);	/* wait for a while, or for input */

		adjustpos(tlp, tbo);		/* return to old position */
	} else {	/* match not in this window so display line in echo area */
		bufo = 0;
		estart();
		ewprintfc("Matches ");
		for (i = 0; i < llength(clp);) { /* expand tabs	*/
			c = ucs_char(curbp->charset, clp, i, &i);

			if (c == '\t') {
				do {
					ewprintfc(" ");
					bufo++;
				} while (bufo & 7);
			} else {
				ewprintfc("%C", c);
				bufo += ucs_termwidth(c);
			}

			if (i > cbo || bufo > ncol) break;
		}
	}
}
