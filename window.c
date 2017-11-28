/*
 *		Window handling.
 */

#include	"def.h"

INT	prevwind(INT f, INT n);
INT	shrinkwind(INT f, INT n);


/*
 * Mg3a: Whether to refresh the screen when recentering.
 */

INT	recenter_redisplay = 0;


/*
 * Recenter like in GNU Emacs. If GOSREC is defined, with no argument
 * it defaults to 1 and works like in Gosling.
 */

INT
recenter(INT f, INT n)
{
#ifndef GOSREC
	if (f & FFARG) {
		if (f & FFNUM) {
			curwp->w_force = (n >= 0) ? n + 1 : n;
		} else {
			curwp->w_force = 0;
		}
	} else {
		curwp->w_force = 0;
		if (recenter_redisplay) sgarbf = TRUE;
	}
#else
	curwp->w_force = n;
	sgarbf = TRUE;
#endif
	curwp->w_flag |= WFFORCE;
	return TRUE;
}


/*
 * Mg3a: recenter-top-bottom like in GNU Emacs.
 */

INT
recentertopbottom(INT f, INT n)
{
	if (f & FFARG) {
		if (f & FFNUM) {
			curwp->w_force = (n >= 0) ? n + 1 : n;
			thisflag |= CFCL3;	/* Try to avoid clearing screen */
		} else {
			curwp->w_force = 0;
			thisflag |= CFCL1;	/* Try to avoid clearing screen */
		}
	} else if (lastflag & CFCL1) {
		curwp->w_force = 1;
		thisflag |= CFCL2;
	} else if (lastflag & CFCL2) {
		curwp->w_force = -1;
		thisflag |= CFCL3;
	} else {
		curwp->w_force = 0;
		if (recenter_redisplay && !(lastflag & CFCL3)) sgarbf = TRUE;
		thisflag |= CFCL1;
	}

	curwp->w_flag |= WFFORCE;
	return TRUE;
}


/*
 * Mg3a: move-to-window-line logic.
 */

static INT
internal_movetoline(INT line)
{
	LINE *lp, *blp = curbp->b_linep;

	reframe_window(curwp);	// We depend on the current framing. Makes it work in a script.

	lp = curwp->w_linep;

	if (line < 0) line = curwp->w_ntrows + line;

	if (line < 0) {
		for (; line < 0 && lback(lp) != blp; line++) lp = lback(lp);
	} else {
		for (; line > 0 && lforw(lp) != blp; line--) lp = lforw(lp);
	}

	adjustpos(lp, 0);
	return TRUE;
}


/*
 * Mg3a: "move-to-window-line" a la Emacs.
 */

INT
movetoline(INT f, INT n)
{
	if (!(f & FFARG)) n = curwp->w_ntrows / 2;
	return internal_movetoline(n);
}


/*
 * Mg3a: "move-to-window-line-top-bottom" a la Emacs.
 */

INT
movetoline_topbottom(INT f, INT n)
{
	if (f & FFARG) {
		return internal_movetoline(n);
	} else if (lastflag & CFMR1) {
		thisflag |= CFMR2;
		return internal_movetoline(0);
	} else if (lastflag & CFMR2) {
		return internal_movetoline(-1);
	} else {
		thisflag |= CFMR1;
		return internal_movetoline(curwp->w_ntrows / 2);
	}
}

/*
 * Refresh the display. A call is made to the "ttresize" entry in the
 * terminal handler, which tries to reset "nrow" and "ncol". In the
 * normal case the call to "update" in "main.c" refreshes the screen,
 * and all of the windows need not be recomputed. Note that when you
 * get to the "display unusable" message, the screen will be messed
 * up. If you make the window bigger again, and send another command,
 * everything will get fixed!
 */

INT
refresh(INT f, INT n)
{
	WINDOW *wp;
	INT	oldnrow;
	INT	oldncol;

	if (f & FFARG) {
		ewprintf("Display size is %d by %d", nrow, ncol);
		return TRUE;
	}
	oldnrow = nrow;
	oldncol = ncol;
	ttresize();
	if (nrow!=oldnrow || ncol!=oldncol) {
		wp = wheadp;			/* Find last.		*/
		while (wp->w_wndp != NULL)
			wp = wp->w_wndp;
		if (nrow < wp->w_toprow+3) {	/* Check if too small.	*/
			ewprintf("Display unusable");
			return (FALSE);
		}
		wp->w_ntrows = nrow-wp->w_toprow-2;
	}
	sgarbf = TRUE;
	return TRUE;
}


/*
 * The command to make the next window (next => down the screen) the
 * current window. There are no real errors, although the command does
 * nothing if there is only 1 window on the screen.
 */

INT
nextwind(INT f, INT n)
{
	WINDOW *wp;

	if (n < 0) return prevwind(f, -n);

	if ((wp=curwp->w_wndp) == NULL)
		wp = wheadp;
	curwp = wp;
	curbp = wp->w_bufp;
	return TRUE;
}


/*
 * This command makes the previous window (previous => up the screen)
 * the current window. There arn't any errors, although the command
 * does not do a lot if there is 1 window.
 */

INT
prevwind(INT f, INT n)
{
	WINDOW *wp1;
	WINDOW *wp2;

	if (n < 0) return nextwind(f, -n);

	wp1 = wheadp;
	wp2 = curwp;
	if (wp1 == wp2)
		wp2 = NULL;
	while (wp1->w_wndp != wp2)
		wp1 = wp1->w_wndp;
	curwp = wp1;
	curbp = wp1->w_bufp;
	return TRUE;
}


/*
 * This command makes the current window the only window on the
 * screen. Try to set the framing so that "." does not have to move on
 * the display. Some care has to be taken to keep the values of dot
 * and mark in the buffer structures right if the distruction of a
 * window makes a buffer become undisplayed.
 */

INT
onlywind(INT f, INT n)
{
	WINDOW	*wp;
	LINE	*lp;
	INT	i;

	while (wheadp != curwp) {
		wp = wheadp;
		wheadp = wp->w_wndp;
		if (--wp->w_bufp->b_nwnd == 0) {
			wp->w_bufp->b_dotp  = wp->w_dotp;
			wp->w_bufp->b_doto  = wp->w_doto;
			wp->w_bufp->b_markp = wp->w_markp;
			wp->w_bufp->b_marko = wp->w_marko;
			wp->w_bufp->b_dotpos = wp->w_dotpos;
		}
		free(wp);
	}
	while (curwp->w_wndp != NULL) {
		wp = curwp->w_wndp;
		curwp->w_wndp = wp->w_wndp;
		if (--wp->w_bufp->b_nwnd == 0) {
			wp->w_bufp->b_dotp  = wp->w_dotp;
			wp->w_bufp->b_doto  = wp->w_doto;
			wp->w_bufp->b_markp = wp->w_markp;
			wp->w_bufp->b_marko = wp->w_marko;
			wp->w_bufp->b_dotpos = wp->w_dotpos;
		}
		free(wp);
	}
	lp = curwp->w_linep;
	i  = curwp->w_toprow;
	while (i!=0 && lback(lp)!=curbp->b_linep) {
		--i;
		lp = lback(lp);
	}
	curwp->w_toprow = 0;
	curwp->w_ntrows = nrow-2;		/* 2 = mode, echo.	*/
	curwp->w_linep	= lp;
	curwp->w_flag  |= WFMODE|WFHARD;
	return TRUE;
}


/*
 * Split the current window.
 *
 * Mg3a: Use arguments like in Emacs.
 */

INT
splitwind(INT f, INT n)
{
	WINDOW	*wp;
	LINE	*lp;
	INT	ntru;
	INT	ntrd;
	INT	ntrl;
	WINDOW	*wp1, *wp2;
	INT	rows;

	rows = curwp->w_ntrows;

	if (rows < 3) {
		ewprintf("Cannot split a %d line window", rows);
		return FALSE;
	}

	if (f & FFARG) {
		if (n >= 3 && n <= rows - 2) {
			ntru = n - 1;
			ntrl = rows - ntru - 1;
		} else if (-n >= 3 && -n <= rows - 2) {
			ntrl = -n - 1;
			ntru = rows - ntrl - 1;
		} else {
			ewprintf("Cannot split window as specified");
			return FALSE;
		}
	} else {
		ntru = (rows - 1) / 2;		/* Upper size		*/
		ntrl = rows - ntru - 1;		/* Lower size		*/
	}

	if ((wp = (WINDOW *)malloc_msg(sizeof(WINDOW))) == NULL) return FALSE;

	++curbp->b_nwnd;			/* Displayed twice.	*/
	wp->w_bufp  = curbp;
	wp->w_dotp  = curwp->w_dotp;
	wp->w_doto  = curwp->w_doto;
	wp->w_markp = curwp->w_markp;
	wp->w_marko = curwp->w_marko;
	wp->w_dotpos = curwp->w_dotpos;
	wp->w_flag  = 0;
	wp->w_force = 0;

	lp = curwp->w_linep;
	ntrd = 0;
	while (lp != curwp->w_dotp) {
		++ntrd;
		lp = lforw(lp);
	}
	lp = curwp->w_linep;
	if (ntrd <= ntru || (backward_compat & BACKW_DYNWIND) == 0) {	/* Old is upper window. */
		if (ntrd == ntru)		/* Hit mode line.	*/
			lp = lforw(lp);
		curwp->w_ntrows = ntru;
		wp->w_wndp = curwp->w_wndp;
		curwp->w_wndp = wp;
		wp->w_toprow = curwp->w_toprow+ntru+1;
		wp->w_ntrows = ntrl;
	} else {				/* Old is lower window	*/
		wp1 = NULL;
		wp2 = wheadp;
		while (wp2 != curwp) {
			wp1 = wp2;
			wp2 = wp2->w_wndp;
		}
		if (wp1 == NULL)
			wheadp = wp;
		else
			wp1->w_wndp = wp;
		wp->w_wndp   = curwp;
		wp->w_toprow = curwp->w_toprow;
		wp->w_ntrows = ntru;
		++ntru;				/* Mode line.		*/
		curwp->w_toprow += ntru;
		curwp->w_ntrows	 = ntrl;
		while (ntru--)
			lp = lforw(lp);
	}
	curwp->w_linep = lp;			/* Adjust the top lines */
	wp->w_linep = lp;			/* if necessary.	*/
	curwp->w_flag |= WFMODE|WFHARD;
	wp->w_flag |= WFMODE|WFHARD;
	return TRUE;
}


/*
 * Enlarge the current window. Find the window that loses space. Make
 * sure it is big enough. If so, hack the window descriptions, and ask
 * redisplay to do all the hard work. You don't just set "force
 * reframe" because dot would move.
 */

INT
enlargewind(INT f, INT n)
{
	WINDOW *adjwp;
	LINE	*lp;
	INT	i;

	if (n < 0)
		return shrinkwind(f, -n);
	if (wheadp->w_wndp == NULL) {
		ewprintf("Only one window");
		return FALSE;
	}
	if ((adjwp=curwp->w_wndp) == NULL) {
		adjwp = wheadp;
		while (adjwp->w_wndp != curwp)
			adjwp = adjwp->w_wndp;
	}
	if (adjwp->w_ntrows <= n) {
		ewprintf("Impossible change");
		return FALSE;
	}
	if (curwp->w_wndp == adjwp) {		/* Shrink below.	*/
		lp = adjwp->w_linep;
		for (i=0; i<n && lp!=adjwp->w_bufp->b_linep; ++i)
			lp = lforw(lp);
		adjwp->w_linep	= lp;
		adjwp->w_toprow += n;
	} else {				/* Shrink above.	*/
		lp = curwp->w_linep;
		for (i=0; i<n && lback(lp)!=curbp->b_linep; ++i)
			lp = lback(lp);
		curwp->w_linep	= lp;
		curwp->w_toprow -= n;
	}
	curwp->w_ntrows += n;
	adjwp->w_ntrows -= n;
	curwp->w_flag |= WFMODE|WFHARD;
	adjwp->w_flag |= WFMODE|WFHARD;
	return TRUE;
}


/*
 * Shrink the current window. Find the window that gains space. Hack
 * at the window descriptions. Ask the redisplay to do all the hard
 * work.
 */

INT
shrinkwind(INT f, INT n)
{
	WINDOW *adjwp;
	LINE	*lp;
	INT	i;

	if (n < 0)
		return enlargewind(f, -n);
	if (wheadp->w_wndp == NULL) {
		ewprintf("Only one window");
		return FALSE;
	}
	/*
	 * Bit of flakiness - FFRAND means it was an internal call, and
	 * to be trusted implicitly about sizes.
	 */
	if (!(f & FFRAND) && curwp->w_ntrows <= n) {
		ewprintf("Impossible change");
		return (FALSE);
	}
	if ((adjwp=curwp->w_wndp) == NULL) {
		adjwp = wheadp;
		while (adjwp->w_wndp != curwp)
			adjwp = adjwp->w_wndp;
	}
	if (curwp->w_wndp == adjwp) {		/* Grow below.		*/
		lp = adjwp->w_linep;
		for (i=0; i<n && lback(lp)!=adjwp->w_bufp->b_linep; ++i)
			lp = lback(lp);
		adjwp->w_linep	= lp;
		adjwp->w_toprow -= n;
	} else {				/* Grow above.		*/
		lp = curwp->w_linep;
		for (i=0; i<n && lp!=curbp->b_linep; ++i)
			lp = lforw(lp);
		curwp->w_linep	= lp;
		curwp->w_toprow += n;
	}
	curwp->w_ntrows -= n;
	adjwp->w_ntrows += n;
	curwp->w_flag |= WFMODE|WFHARD;
	adjwp->w_flag |= WFMODE|WFHARD;
	return (TRUE);
}


/*
 * Delete current window. Call shrink-window to do the screen
 * updating, then throw away the window.
 */

INT
delwind(INT f, INT n)
{
	WINDOW *wp, *nwp;

	wp = curwp;			/* Cheap...		*/
	/* shrinkwind returning false means only one window... */
	if (shrinkwind(FFRAND, wp->w_ntrows + 1) == FALSE)
		return FALSE;
	if (--wp->w_bufp->b_nwnd == 0) {
		wp->w_bufp->b_dotp  = wp->w_dotp;
		wp->w_bufp->b_doto  = wp->w_doto;
		wp->w_bufp->b_markp = wp->w_markp;
		wp->w_bufp->b_marko = wp->w_marko;
		wp->w_bufp->b_dotpos = wp->w_dotpos;
	}
	/* since shrinkwind did't crap out, we know we have a second window */
	if (wp == wheadp) wheadp = curwp = wp->w_wndp;
	else if ((curwp = wp->w_wndp) == NULL) curwp = wheadp;
	curbp = curwp->w_bufp;
	for (nwp = wheadp; nwp != NULL; nwp = nwp->w_wndp)
		if (nwp->w_wndp == wp) {
			nwp->w_wndp = wp->w_wndp;
			break ;
		}
	free(wp);
	return TRUE;
}


/*
 * Pick a window for a pop-up. Split the screen if there is only one
 * window. Return a pointer, or NULL on error.
 *
 * Mg3a: Pick the next-below window or the top one, like in GNU Emacs.
 */

WINDOW *
wpopup()
{
	if (wheadp->w_wndp == NULL) {
		if (splitwind(0, 1) == FALSE) return NULL;
	}

	return curwp->w_wndp ? curwp->w_wndp : wheadp;
}


/*
 * Mg3a: Quit a window. From GNU Emacs. Prefer non-system, non-dired,
 * buffers. Resort to scratch if there is nothing else.
 */

INT
quitwind(INT f, INT n)
{
	BUFFER *bp = NULL;

	if (wheadp->w_wndp) return delwind(f, n);

	if (curbp->b_altb && !(curbp->b_altb->b_flag & (BFSYS|BFDIRED))) {
		bp = curbp->b_altb;
	} else {
		for (bp = bheadp; bp; bp = bp->b_bufp) {
			if (bp != curbp && !(bp->b_flag & (BFSYS|BFDIRED))) break;
		}
	}

	if (bp == NULL && (bp = bfind("*scratch*", TRUE)) == NULL) return FALSE;

	curbp = bp;
	return showbuffer(bp, curwp);
}


/*
 * Mg3a: balance window areas. Put larger windows at bottom.
 */

INT
balancewind(INT f, INT n)
{
	INT wcount, wlen, total, longlen, line, len;
	WINDOW *wp;

	wcount = 0;
	for (wp = wheadp; wp; wp = wp->w_wndp) wcount++;

	total = nrow - 1;
	wlen = total/wcount;
	longlen = total - wlen*wcount;			// Excess window length, to be distributed

	line = 0;
	for (wp = wheadp; wp; wp = wp->w_wndp) {
		wp->w_toprow = line;
		len = wlen;
		if (longlen++ >= wcount) len++;		// Balanced bottom windows get it
		wp->w_ntrows = len - 1;
		line += len;
	}

	refreshbuf(NULL);
	return TRUE;
}


/*
 * Mg3a: Shrink window if it's larger than the buffer and the buffer's
 * first line is displayed. From GNU Emacs.
 */

INT
shrinkwind_iflarge(INT f, INT n)
{
	INT count, rows;
	LINE *lp, *blp, *dotp;

	reframe_window(curwp);
	blp = curwp->w_bufp->b_linep;

	/* If first line is not shown, or only one window */
	if (curwp->w_linep != lforw(blp) || wheadp->w_wndp == NULL) return TRUE;

	rows = curwp->w_ntrows;
	lp = lback(blp);

	/* Ignore last zero-length line */
	if (lp != blp && llength(lp) == 0) lp = lback(lp);

	count = 0;

	while (lp != blp) {
		count++;
		lp = lback(lp);
		if (count > rows) return TRUE;
	}

	dotp = curwp->w_dotp;

	if (count < 2) {
		count = 2;
	} else if (dotp == lback(blp) && llength(dotp) == 0) {
		/* Move cursor if window otherwise would scroll */
		endofpreviousline();
	}

	/* We always set it to minimum 2, so no FFRAND	*/
	shrinkwind(0, rows - count);

	/* Must reset for bottom window			*/
	curwp->w_linep = lforw(curbp->b_linep);

	return TRUE;
}
