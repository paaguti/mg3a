/*
 *		Basic cursor motion commands.
 *
 * The routines in this file are the basic command functions for
 * moving the cursor around on the screen, setting mark, and swapping
 * dot with mark. Only moves between lines, which might make the
 * current buffer framing bad, are hard.
 */

#include	"def.h"

INT	forwchar(INT f, INT n);
INT	setmark(INT f, INT n);
INT	backline(INT f, INT n);
INT	backpage(INT f, INT n);
INT	nextwind(INT f, INT n);
INT	endvisualline(INT f, INT n);


/*
 * Go to beginning of line.
 */
INT
gotobol(INT f, INT n)
{
	adjustpos(curwp->w_dotp, 0);
	return TRUE;
}


/*
 * Move cursor backwards. Do the right thing if the count is less than
 * 0. Error if you try to move back from the beginning of the buffer.
 *
 * Mg3a: Acts according to the current charset
 */

INT
backchar(INT f, INT n)
{
	LINE	*lp;
	INT	doto;

	if (n < 0) return forwchar(f, -n);
	while (n--) {
		doto = curwp->w_doto;

		if (doto == 0) {
			if ((lp=lback(curwp->w_dotp)) == curbp->b_linep) {
				if (!(f & FFRAND))
					ewprintf("Beginning of buffer");
				return FALSE;
			}
			adjustpos(lp, llength(lp));
		} else {
			adjustpos(curwp->w_dotp, ucs_backward(curbp->charset, curwp->w_dotp, doto));
		}
	}
	return TRUE;
}


/*
 * Go to end of line.
 */

INT
gotoeol(INT f, INT n)
{
	adjustpos(curwp->w_dotp, llength(curwp->w_dotp));
	return TRUE;
}


/*
 * Move cursor forwards. Do the right thing if the count is less than
 * 0. Error if you try to move forward from the end of the buffer.
 *
 * Mg3a: Acts according to the current charset
 */

INT
forwchar(INT f, INT n)
{
	LINE *lp;
	INT doto;

	if (n < 0) return backchar(f, -n);
	while (n--) {
		doto = curwp->w_doto;

		if (doto == llength(curwp->w_dotp)) {
			if ((lp = lforw(curwp->w_dotp)) == curbp->b_linep) {
				if (!(f & FFRAND))
					ewprintf("End of buffer");
				return FALSE;
			}
			adjustpos(lp, 0);
		} else {
			adjustpos(curwp->w_dotp, ucs_forward(curbp->charset, curwp->w_dotp, doto));
		}
	}
	return TRUE;
}


/*
 * Mg3a: Go back to first non-blank. From Emacs.
 */

INT
backtoindent(INT f, INT n)
{
	INT doto;

	getindent(curwp->w_dotp, &doto, NULL);
	adjustpos(curwp->w_dotp, doto);

	return TRUE;
}


/*
 * Mg3a: Return the line shift for the offset
 */

static INT
getshift(INT offset)
{
	INT col;

	col = getcolumn(curwp, curwp->w_dotp, offset);

	if (col > ncol - 2) {
		return col - col % (ncol >> 1) - (ncol >> 2);
	} else {
		return 0;
	}
}


/*
 * Mg3a: Goto beginning of visual line. From Emacs.
 */

INT
beginvisualline(INT f, INT n)
{
	INT shift, offset;

	if (n < 0) return endvisualline(f, -n);

	while (n-- > 0 && curwp->w_doto != 0) {
		shift = getshift(curwp->w_doto);
		offset = getoffset(curwp, curwp->w_dotp, shift);

		if (shift) {
			// Skip "$"
			offset = ucs_forward(curbp->charset, curwp->w_dotp, offset);
		}

		adjustpos(curwp->w_dotp, offset);
	}

	return TRUE;
}


/*
 * Mg3a: Goto end of visual line. From Emacs.
 */

INT
endvisualline(INT f, INT n)
{
	INT shift, offset;

	if (n < 0) return beginvisualline(f, -n);

	while (n-- > 0 && curwp->w_doto != llength(curwp->w_dotp)) {
		shift = getshift(curwp->w_doto);
		offset = getoffset(curwp, curwp->w_dotp, shift + ncol - 2);

		if (shift == 0 && offset == curwp->w_doto) {
			offset = getoffset(curwp, curwp->w_dotp, ncol + (ncol>>1) - (ncol>>2) - 2);
		}

		adjustpos(curwp->w_dotp, offset);
	}

	return TRUE;
}


/*
 * Go to the beginning of the buffer.
 */

INT
gotobob(INT f, INT n)
{
	setmark(f, n);
	adjustpos3(lforw(curbp->b_linep), 0, 0);
#ifdef DIRED
	if (curbp->b_flag & BFDIRED) diredsetoffset();
#endif
	return TRUE;
}


/*
 * Go to the end of the buffer.
 */

INT
gotoeob(INT f, INT n)
{
	if (normalizebuffer(curbp) == FALSE) return FALSE;
	setmark(f, n);
	adjustpos3(lback(curbp->b_linep), llength(lback(curbp->b_linep)), curbp->b_size);
	return TRUE;
}


/*
 * Move forward by full lines. If the number of lines to move is less
 * than zero, call the backward line function to actually do it. The
 * last command controls how the goal column is set.
 */

INT
forwline(INT f, INT n)
{
	LINE	*lp, *blp;

	if (n < 0) return backline(f, -n);

	if (!(lastflag & CFCPCN)) setgoal();
	thisflag |= CFCPCN;

	if (n == 0) return TRUE;
	blp = curbp->b_linep;

	for (lp = curwp->w_dotp; lp != blp && n > 0; n--) lp = lforw(lp);

	if (lp == blp) {
		if (normalizebuffer(curbp) == FALSE) return FALSE;
		lp = lback(blp);
	}

	adjustpos(lp, getgoal(lp));
#ifdef DIRED
	if (curbp->b_flag & BFDIRED) diredsetoffset();
#endif
	return TRUE;
}


/*
 * This function is like "forwline", but goes backwards. The scheme is
 * exactly the same. Check for arguments that are less than zero and
 * call your alternate.
 */

INT
backline(INT f, INT n)
{
	LINE	*lp, *blp;

	if (n < 0) return forwline(f, -n);

	if (!(lastflag & CFCPCN)) setgoal();
	thisflag |= CFCPCN;

	if (n == 0) return TRUE;
	blp = curbp->b_linep;

	for (lp = curwp->w_dotp; lp != blp && n > 0; n--) lp = lback(lp);

	if (lp == blp) lp = lforw(blp);

	adjustpos(lp, getgoal(lp));
#ifdef DIRED
	if (curbp->b_flag & BFDIRED) diredsetoffset();
#endif
	return TRUE;
}


/*
 * Set the current goal column, which is saved in the external
 * variable "curgoal", to the current cursor column.
 */

void
setgoal()
{
	curgoal = getcolumn(curwp, curwp->w_dotp, curwp->w_doto);
}


/*
 * This routine looks at a line (pointed to by the LINE pointer "lp")
 * and the current vertical motion goal column (set by the "setgoal"
 * routine above) and returns the best offset to use when a vertical
 * motion is made into the line.
 */

INT
getgoal(LINE *lp)
{
	return getoffset(curwp, lp, curgoal);
}


/*
 * Scroll forward by a specified number of lines, or by a full page if
 * no argument. The "2" is the window overlap (this is the default
 * value from ITS EMACS). Because the top line in the window is
 * zapped, we have to do a hard update and get it back.
 */

INT
forwpage(INT f, INT n)
{
	LINE	*lp;

	if (!(f & FFARG)) {
		n = curwp->w_ntrows - 2;	/* Default scroll.	*/
		if (n <= 0)			/* Forget the overlap	*/
			n = 1;			/* if tiny window.	*/
	} else if (n < 0)
		return backpage(f, -n);
#ifdef	CVMVAS
	else					/* Convert from pages	*/
		n *= curwp->w_ntrows;		/* to lines.		*/
#endif
	reframe_window(curwp);			/* Makes it work in a script */
	lp = curwp->w_linep;
	while (n-- && lforw(lp)!=curbp->b_linep)
		lp = lforw(lp);
	curwp->w_linep = lp;
	curwp->w_flag |= WFHARD;
	/* if in current window, don't move dot */
	for(n = curwp->w_ntrows; n-- && lp!=curbp->b_linep; lp = lforw(lp))
		if(lp==curwp->w_dotp) return TRUE;
	adjustpos(curwp->w_linep, 0);
#ifdef DIRED
	if (curbp->b_flag & BFDIRED) diredsetoffset();
#endif
	return TRUE;
}


/*
 * This command is like "forwpage", but it goes backwards. The "2",
 * like above, is the overlap between the two windows. The value is
 * from the ITS EMACS manual. The hard update is done because the top
 * line in the window is zapped.
 */

INT
backpage(INT f, INT n)
{
	LINE	*lp;

	if (!(f & FFARG)) {
		n = curwp->w_ntrows - 2;	/* Default scroll.	*/
		if (n <= 0)			/* Don't blow up if the */
			n = 1;			/* window is tiny.	*/
	} else if (n < 0)
		return forwpage(f, -n);
#ifdef	CVMVAS
	else					/* Convert from pages	*/
		n *= curwp->w_ntrows;		/* to lines.		*/
#endif
	reframe_window(curwp);			/* Makes it work in a script */
	lp = curwp->w_linep;
	while (n-- && lback(lp)!=curbp->b_linep)
		lp = lback(lp);
	curwp->w_linep = lp;
	curwp->w_flag |= WFHARD;
	/* if in current window, don't move dot */
	for(n = curwp->w_ntrows; n-- && lp!=curbp->b_linep; lp = lforw(lp))
		if(lp==curwp->w_dotp) return TRUE;
	adjustpos(curwp->w_linep, 0);
#ifdef DIRED
	if (curbp->b_flag & BFDIRED) diredsetoffset();
#endif
	return TRUE;
}


/*
 * These functions are provided for compatibility with Gosling's
 * Emacs. They are used to scroll the display up (or down) one line at
 * a time.
 */

INT
forw1page(INT f, INT n)
{
	if (!(f & FFARG))  {
        	n = 1;
		f = FFUNIV;
	}

	return forwpage(f, n);
}

INT
back1page(INT f, INT n)
{
	if (!(f & FFARG)) {
        	n = 1;
		f = FFUNIV;
	}

	return backpage(f, n);
}


/*
 * Page the other window. Check to make sure it exists, then nextwind,
 * forwpage or backpage and restore window pointers.
 *
 * Mg3a: help function
 */

static INT
pagenext_aux(INT f, INT n, INT forw)
{
	WINDOW *wp;

	if (wheadp->w_wndp == NULL) {
		ewprintf("No other window");
		return FALSE;
	}
	wp = curwp;
	nextwind(0, 1);
	if (forw) forwpage(f, n);
	else backpage(f, n);
	curwp = wp;
	curbp = wp->w_bufp;
	return TRUE;
}


/*
 * Mg3a: Page other window forward
 */

INT
pagenext(INT f, INT n)
{
	return pagenext_aux(f, n, 1);
}


/*
 * Mg3a: Page other window backward
 */

INT
pagenext_backward(INT f, INT n)
{
	return pagenext_aux(f, n, 0);
}


/*
 * Internal set mark routine, used by other functions (daveb).
 */

void
isetmark()
{
	if (curwp->w_dotp == curbp->b_linep) normalizebuffer(curbp);
	curwp->w_markp = curwp->w_dotp;
	curwp->w_marko = curwp->w_doto;
}

/*
 * Reset mark
 */

void iresetmark()
{
	curwp->w_markp = NULL;
	curwp->w_marko = -1;
}
/*
 * Set the mark in the current window to the value of dot. A message
 * is written to the echo line. (ewprintf knows about macros)
 * paaguti:
 *   If the current dot is on the mark, reset the mark
 *   reste mark when supplying an argument
 */

INT
setmark(INT f, INT n)
{
	if ((curwp->w_markp == curwp->w_dotp) && (curwp->w_marko == curwp->w_doto))
	{
		iresetmark();
		ewprintf ("Mark reset");
	} else {
		isetmark();
		ewprintf("Mark set");
	}
	return TRUE;
}

/*
 * Mg3a: Mark whole buffer, like in Emacs. The region will have a
 * newline at the end, unless it's a single line or empty.
 */

INT
markwholebuffer(INT f, INT n)
{
	LINE *blp = curbp->b_linep;

	if (lforw(blp) == blp || lforw(blp) != lback(blp)) {
		if (normalizebuffer(curbp) == FALSE) return FALSE;
	}
	adjustpos3(lforw(curbp->b_linep), 0, 0);
	curwp->w_markp = lback(curbp->b_linep);
	curwp->w_marko = llength(curwp->w_markp);
	ewprintf("Mark set");
	return TRUE;
}


/*
 * Swap the values of "dot" and "mark" in the current window. This is
 * pretty easy, because all of the hard work gets done by the standard
 * routine that moves the mark about. The only possible error is "no
 * mark".
 */

INT
swapmark(INT f, INT n)
{
	LINE	*odotp;
	INT	odoto;

	if (curwp->w_markp == NULL) {
		ewprintf("No mark in this window");
		return FALSE;
	}
	odotp = curwp->w_dotp;
	odoto = curwp->w_doto;
	adjustpos(curwp->w_markp, curwp->w_marko);
	curwp->w_markp = odotp;
	curwp->w_marko = odoto;
	return TRUE;
}


/*
 * Go to a specific line, mostly for looking up errors in C programs,
 * which give the error a line number. If an argument is present, then
 * it is the line number, else prompt for a line number to use.
 *
 * Mg3a: number format check.
 */

INT
gotoline(INT f, INT n)
{
	LINE	*clp;
	INT	s;
	char	buf[32];
	size_t	pos;

	if (!(f & FFARG)) {
		if ((s=ereply("Goto line: ", buf, sizeof(buf))) != TRUE)
			return s;
		if (!getINT(buf, &n, 0)) {
			ewprintf("Invalid line number");
			return FALSE;
		}
	}

	if (n > 0) {
		clp = lforw(curbp->b_linep);	/* "clp" is first line	*/
		pos = 0;
		while (--n > 0) {
			if (lforw(clp) == curbp->b_linep) break;
			pos += llength(clp) + 1;
			clp = lforw(clp);
		}
	} else {
		clp = lback(curbp->b_linep);	/* clp is last line */
		pos = curbp->b_size - llength(clp);
		while (n++ < 0) {
			if (lback(clp) == curbp->b_linep) break;
			clp = lback(clp);
			pos -= llength(clp) + 1;
		}
	}
	adjustpos3(clp, 0, pos);
	return TRUE;
}
