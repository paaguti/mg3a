/*
 *		Buffer handling.
 */

#include	"def.h"
#include	"kbd.h"			/* needed for modes */
#include	"macro.h"

static BUFFER 	*makelist(INT f, INT n);

INT	prevbuffer(INT f, INT n);
INT	newline(INT f, INT n);
INT	onlywind(INT f, INT n);
INT	delwind(INT f, INT n);


/*
 * Attach a buffer to a window. The values of dot and mark come from
 * the buffer if the use count is 0. Otherwise, they come from some
 * other window. *scratch* is the default alternate buffer.
 */

INT
usebuffer(INT f, INT n)
{
	BUFFER 	*bp = curbp->b_altb;
	INT	s;
	char	bufn[NFILEN];

	s = eread("Switch to buffer: (default %S) ",
		  bufn, NFILEN, EFBUF, bp ? bp->b_bname : "*scratch*");

	if (s == ABORT) {
		return s;
	} else if (s == FALSE) {
		if (bp == NULL && (bp = bfind("*scratch*", TRUE)) == NULL) {
			return FALSE;
		}
	} else if ((bp = bfind(bufn, TRUE)) == NULL) {
		return FALSE;
	}

	curbp = bp;
	return showbuffer(bp, curwp);
}


/*
 * Pop to buffer asked for by the user.
 */

INT
poptobuffer(INT f, INT n)
{
	BUFFER 	*bp = curbp->b_altb;
	WINDOW 	*wp;
	INT	s;
	char	bufn[NFILEN];

	s = eread("Switch to buffer in other window: (default %S) ",
		  bufn, NFILEN, EFBUF, bp ? bp->b_bname : "*scratch*");

	if (s == ABORT) {
		return s;
	} else if (s == FALSE) {
		if (bp == NULL && (bp = bfind("*scratch*", TRUE)) == NULL) {
			return FALSE;
		}
	} else if ((bp = bfind(bufn, TRUE)) == NULL) {
		return FALSE;
	}

	if ((wp = popbuf(bp)) == NULL) return FALSE;
	curbp = bp;
	curwp = wp;
	return TRUE;
}


/*
 * Dispose of a buffer. Clear the buffer (ask if the buffer has been
 * changed). Then free the header line and the buffer header.
 */

INT
internal_killbuffer(BUFFER *bp)
{
	BUFFER	*bp1;
	BUFFER	*bp2;
	WINDOW	*wp;

	/*
	 * Find some other buffer to display. try the alternate
	 * buffer, then the first different buffer in the buffer list.
	 * if there's only one buffer, create buffer *scratch* and
	 * make it the alternate buffer. return if *scratch* is only
	 * buffer
	 *
	 * Mg3a: if *scratch* is the only buffer, rename the buffer,
	 * then proceed.
	 */

	if ((bp1 = bp->b_altb) == NULL || bp1 == bp) {
		bp1 = (bp == bheadp) ? bp->b_bufp : bheadp;
		if (bp1 == NULL) {
			/* only one buffer. see if it's *scratch* */
			if (bp == bfind("*scratch*", FALSE)) {
				stringcopy(bp->b_bname, "*temp*", 7);
				bp->b_flag &= ~(BFCHG|BFSCRATCH);
				upmodes(bp);
			}
			/* create *scratch* for alternate buffer */
			if ((bp1 = bfind("*scratch*",TRUE)) == NULL)
				return FALSE;
		}
	}
	// Emacs (22+) doesn't ask when no file
	if (bp->b_fname[0] == 0) bp->b_flag &= ~BFCHG;
	if (bclear(bp) != TRUE) return FALSE;
	for (wp = wheadp; wp; wp = wp->w_wndp) {
	    if (wp->w_bufp == bp) {
		showbuffer(bp1, wp);
	    }
	}
	if (bp == curbp) curbp = bp1;
	if (gmarkp == bp->b_linep) {
		gmarkp = NULL;
		gmarko = 0;
	}

	// UNDO
	u_clear(bp);
	//

	free(bp->b_linep);			/* Release header line. */
	bp2 = NULL;				/* Find the header.	*/
	bp1 = bheadp;
	while (bp1 != bp) {
		if (bp1->b_altb == bp)
			bp1->b_altb = (bp->b_altb == bp1) ? NULL : bp->b_altb;
		bp2 = bp1;
		bp1 = bp1->b_bufp;
	}
	bp1 = bp1->b_bufp;			/* Next one in chain.	*/
	if (bp2 == NULL)			/* Unlink it.		*/
		bheadp = bp1;
	else
		bp2->b_bufp = bp1;
	while (bp1 != NULL) {			/* Finish with altb's	*/
		if (bp1->b_altb == bp)
			bp1->b_altb = (bp->b_altb == bp1) ? NULL : bp->b_altb;
		bp1 = bp1->b_bufp;
	}
	free(bp->b_bname);			/* Release name block	*/
	free(bp);				/* Release buffer block */
	return TRUE;
}


/*
 * Dispose of a buffer, by name. Ask for the name. Look it up. Bound
 * to "C-X K".
 */

INT
killbuffer(INT f, INT n)
{
	BUFFER	*bp;
	INT	s;
	char	bufn[NFILEN];

	if ((s=eread("Kill buffer: (default %S) ", bufn, NFILEN, EFBUF,
		    curbp->b_bname)) == ABORT) return (s);
	else if (s == FALSE) bp = curbp;
	else if ((bp=bfind(bufn, FALSE)) == NULL) return buffernotfound(bufn);

	return internal_killbuffer(bp);
}


/*
 * Mg3a: kill current buffer and window like Emacs. Bound to
 * "C-X 4 0".
 */

INT
killbuffer_andwindow(INT f, INT n)
{
	INT s;

 	if ((s = internal_killbuffer(curbp)) != TRUE) return s;

	return delwind(f, n);
}


/*
 * Mg3a: Kill a buffer without asking for a name. Not in Emacs.
 */

INT
killbuffer_quickly(INT f, INT n)
{
	if (f & FFUNIV) curbp->b_flag &= ~BFCHG;

	return internal_killbuffer(curbp);
}


/*
 * Mg3a: Kill this buffer a la Emacs
 */

INT
killthisbuffer(INT f, INT n)
{
	return internal_killbuffer(curbp);
}


/*
 * Switch the current window to the next buffer. System buffers,
 * except *scratch*, are skipped.
 */

INT
nextbuffer(INT f, INT n)
{
	BUFFER *bp, *bp1;

	if (n < 0) return prevbuffer(f, -n);
	else if (n == 0) return TRUE;

	bp = curwp->w_bufp;
	bp1 = bp;

	while (n > 0) {
		bp1 = bp->b_bufp;

		while (bp1 != bp) {
			if (bp1 == NULL) {
				bp1 = bheadp;
			} else if (bp1->b_flag & BFSYS) {
				bp1 = bp1->b_bufp;
			} else
				break;
		}

		if (bp1 == bp) return TRUE;

		bp = bp1;
		n--;
	}

	curbp = bp1;
	showbuffer(curbp, curwp);
	return TRUE;
}


/*
 * Switch the current window to the previous buffer. System buffers,
 * except *scratch*, are skipped.
 */

INT
prevbuffer(INT f, INT n)
{
	BUFFER *bp, *bp1, *outbp = NULL;

	if (n < 0) return nextbuffer(f, -n);
	else if (n == 0) return TRUE;

	bp = curwp->w_bufp;

	while (n > 0) {
		bp1 = bp->b_bufp;

		while (bp1 != bp) {
			if (bp1 == NULL) {
				bp1 = bheadp;
			} else {
				if (!(bp1->b_flag & BFSYS)) {
					outbp = bp1;
				}

				bp1 = bp1->b_bufp;
			}
		}

		if (outbp == NULL) return TRUE;

		bp = outbp;
		n--;
	}

	curbp = outbp;
	showbuffer(curbp, curwp);
	return TRUE;
}


/*
 * Save some buffers - just call anycb with the arg flag.
 */

INT
savebuffers(INT f, INT n)
{
	if (anycb(f & FFARG) == ABORT) return ABORT;
	return TRUE;
}


/*
 * Display the buffer list. This is done in two parts. The "makelist"
 * routine figures out the text, and puts it in a buffer. "popbuf"
 * then pops the data onto the screen. Bound to "C-X C-B".
 */

INT
listbuffers(INT f, INT n)
{
	BUFFER *bp;
	WINDOW *wp;

	if ((bp=makelist(f, n)) == NULL || (wp=popbuf(bp)) == NULL)
		return FALSE;

	return TRUE;
}


/*
 * Mg3a: Buffer menu in current window
 */

INT
bufed(INT f, INT n)
{
	BUFFER *bp;

	if ((bp=makelist(f, n)) == NULL)
		return FALSE;

	curbp = bp;
	return showbuffer(bp, curwp);
}


/*
 * Mg3a: Buffer menu in another window
 */

INT
bufed_otherwindow(INT f, INT n)
{
	BUFFER *bp;
	WINDOW *wp;

	if ((bp=makelist(f, n)) == NULL || (wp=popbuf(bp)) == NULL)
		return FALSE;

	curbp = bp;
	curwp = wp;

	return TRUE;
}

/* Width of buffer name in *Buffer List* */

INT buffer_name_width = 24;


/*
 * This routine rebuilds the text for the list buffers command. Return
 * TRUE if everything works. Return FALSE if there is an error (if
 * there is no memory).
 *
 * Mg3a: added flags, multinational buffer names, and optionally
 * showing lines.
 */

static BUFFER *
makelist(INT f, INT n)
{
	char		*cp1;
	INT		c;
	BUFFER 		*bp;
	LINE		*lp;
	size_t		nbytes, nlines;
	BUFFER		*blp;
	char		line[NFILEN+80+NFILEN];
	INT		nlen;
	INT 		len, i, nexti, w;
	INT		bufwidth;

	bufwidth = buffer_name_width;
	if (bufwidth == 0) bufwidth = ncol / 2 - 10;

	if (bufwidth < 14) bufwidth = 14;
	else if (bufwidth > NFILEN) bufwidth = NFILEN;

	if ((blp = bfind("*Buffer List*", TRUE)) == NULL) return NULL;

	// UNDO
	u_clear(blp);
	//

	blp->b_flag &= ~BFCHG;			/* Blow away old.	*/
	if (bclear(blp) != TRUE) return NULL;

	blp->b_flag |= BFREADONLY|BFSYS;
	blp->charset = termcharset;

	if (setbufmode(blp, "bufmenu") == FALSE) return NULL;

	sprintf(line, " MLCB %-*s", (int)bufwidth, "Buffer");
	cp1 = line + strlen(line);
	if (f & FFARG) {
		sprintf(cp1, "  %11s %9s  %s", "Size", "Lines", "File");
	} else {
		sprintf(cp1, "  %-8s %s", "Size", "File");
	}
	if (addline(blp, line) == FALSE) return NULL;
	make_underline(line);
	if (addline(blp, line) == FALSE) return NULL;
	bp = bheadp;				/* For all buffers	*/
	while (bp != NULL) {
		cp1 = &line[0];			/* Start at left edge	*/

		*cp1++ = (bp == curbp) ? '.' : ' ';
		*cp1++ = (bp->b_flag & BFCHG) ? '*' : ' ';

#ifdef DIRED
		if (bp->b_flag & BFDIRED) {
			strcpy(cp1, "dir");
			cp1 += 3;
		} else
#endif
		{
			*cp1++ = (bp->b_flag & BFUNIXLF) ? 'l' : 'c';
			*cp1++ = (bp->charset == utf_8) ? 'u' :
				(bp->charset == doscharset) ? 'd' :
				(bp->charset == buf8bit) ? '8' : 'l';
			*cp1++ = (bp->b_flag & BFBOM) ? 'b' : 'n';
		}

		*cp1++ = ' ';			/* Gap.			*/

		nlen = 0;

		len = strlen(bp->b_bname);
		i = 0;

		while (i < len) {
			c = ucs_strtochar(termcharset, bp->b_bname, len, i, &nexti);

			if (c == '\t') w = 1;
			else w = ucs_termwidth(c);

			if (n != 0 && nlen + w > bufwidth) break;

			if (c == '\t') {
				*cp1++ = ' ';
			} else {
				while (i < nexti) *cp1++ = bp->b_bname[i++];
			}

			nlen += w;
			i = nexti;
		}

		while (nlen++ < bufwidth) *cp1++ = ' ';

		if (i < len) *cp1++ = '$';
		else *cp1++ = ' ';

		nbytes = 0;			/* Count bytes in buf.	*/
		nlines = 0;
		if (bp != blp) {
			INT nlsize;

			lp = lforw(bp->b_linep);
			nlsize = 1 + ((bp->b_flag & BFUNIXLF) == 0);
			while (lp != bp->b_linep) {
				nbytes += llength(lp)+nlsize;
				nlines++;
				lp = lforw(lp);
			}
			if(nbytes) nbytes-=nlsize;	/* no bonus newline	*/
			if (nlines && llength(lback(bp->b_linep)) == 0) nlines--;
			if (bp->b_flag & BFBOM) nbytes += 3;
		}

		if (f & FFARG) {
			sprintf(cp1, " %11zu %9zu  %s", nbytes, nlines, bp->b_fname);
		} else {
			sprintf(cp1, " %-8zu %s", nbytes, bp->b_fname);
		}

#ifdef DIRED
		cp1 += strlen(cp1);
		if (bp->b_flag & BFDIRED) strcpy(cp1, "/");
#endif

		if (addline_extra(blp, line, LCBUFNAME, bp->b_bname) == FALSE)
			return NULL;
		bp = bp->b_bufp;
	}
	return buftotop(blp);				/* put dot at beginning of buffer */
}


/*
 * Make a line of the string "text" and append this line to the
 * buffer. Handcraft the EOL on the end. Return TRUE if it worked and
 * FALSE if you ran out of room.
 */

INT
addline(BUFFER *bp, char *text)
{
	LINE	*lp;
	INT	ntext;

	ntext = strlen(text);

	if ((lp = lalloc(ntext)) == NULL)
		return FALSE;

	memcpy(ltext(lp), text, ntext);

	addlast(bp, lp);

	return TRUE;
}


/*
 * Mg3a: Like addline with extra hidden info
 */

INT
addline_extra(BUFFER *bp, char *text, int flag, char *extra)
{
	LINE	*lp;
	INT	ntext;
	INT	extralen;

	ntext = strlen(text);
	extralen = strlen(extra) + 1;

	if ((lp = lalloc(ntext + extralen)) == NULL)
		return FALSE;

	memcpy(ltext(lp), text, ntext);
	memcpy(&ltext(lp)[ntext], extra, extralen);

	lsetlength(lp, ntext);
	lp->l_flag = flag;

	addlast(bp, lp);

	return TRUE;
}


static INT
bufed_gotobuf_aux(INT flag)
{
	LINE *lp = curwp->w_dotp;
	char *name;
	BUFFER *bp;
	WINDOW *wp;

	if (lp->l_flag != LCBUFNAME) {
		ewprintf("Not a buffer line");
		return FALSE;
	}

	name = &ltext(lp)[llength(lp)];

	if ((bp = bfind(name, FALSE)) == NULL) {
		return buffernotfound(name);
	}

	if (flag == 0 || flag == 1) {
		if (flag == 1) onlywind(0, 1);
		curbp = bp;
		return showbuffer(bp, curwp);
	} else {
		if (flag == 3) {
			onlywind(0, 1);
			if (curbp->b_altb != NULL) {
				curbp = curbp->b_altb;
				showbuffer(curbp, curwp);
			}
		}
		if ((wp = popbuf(bp)) == NULL) return FALSE;
		if (flag != 4) {
			curbp = bp;
			curwp = wp;
		}
		return TRUE;
	}
}


INT
bufed_gotobuf(INT f, INT n)
{
	return bufed_gotobuf_aux(0);
}

INT
bufed_gotobuf_one(INT f, INT n)
{
	return bufed_gotobuf_aux(1);
}

INT
bufed_gotobuf_other(INT f, INT n)
{
	return bufed_gotobuf_aux(2);
}

INT
bufed_gotobuf_two(INT f, INT n)
{
	return bufed_gotobuf_aux(3);
}

INT
bufed_gotobuf_switch_other(INT f, INT n)
{
	return bufed_gotobuf_aux(4);
}


/*
 * Look through the list of buffers, giving the user a chance to save
 * them. Return TRUE if there are any changed buffers afterwards.
 * Buffers that don't have an associated file don't count. Return
 * FALSE if there are no changed buffers.
 */

INT
anycb(INT saveall)
{
	BUFFER	*bp;
	INT	s = FALSE, save = FALSE;
	int	showsprompt = 0;
	char	prompt[NFILEN + 80];

	for (bp = bheadp; bp; bp = bp->b_bufp) {
		if (bp->b_fname[0] &&  (bp->b_flag & BFCHG)) {
			snprintf(prompt, sizeof(prompt), "Save file %s", bp->b_fname);
			if (saveall || (save = eyorn(prompt)) == TRUE) {
				if (buffsave(bp) == TRUE) {
					bp->b_flag &= ~BFCHG;
					upmodes(bp);
				} else {
					s = TRUE;
				}
				showsprompt = 0;
			} else {
				showsprompt = 1;
				s = TRUE;
			}
			if (save == ABORT) return (save);
			save = TRUE;
		}
	}
	if (save == FALSE) {
		ewprintf("(No files need saving)");
	} else if (showsprompt) {
		eerase();
	}
	return s;
}


/*
 * Search for a buffer, by name. If not found, and the "cflag" is
 * TRUE, create a buffer and put it in the list of all buffers. Return
 * pointer to the BUFFER block for the buffer.
 */

BUFFER *
bfind(char *bname, INT cflag)
{
	BUFFER	*bp;
	LINE	*lp;

	bp = bheadp;
	while (bp != NULL) {
		if (fncmp(bname, bp->b_bname) == 0)
			return bp;
		bp = bp->b_bufp;
	}
	if (cflag!=TRUE) return NULL;
	/*NOSTRICT*/
	if ((bp=(BUFFER *)malloc_msg(sizeof(BUFFER))) == NULL) return NULL;
	if ((bp->b_bname=malloc_msg(strlen(bname)+1)) == NULL) {
		free(bp);
		return NULL;
	}
	if ((lp = lalloc(0)) == NULL) {
		free(bp->b_bname);
		free(bp);
		return NULL;
	}
	bp->b_altb = bp->b_bufp	 = NULL;
	bp->b_dotp  = lp;
	bp->b_doto  = 0;
	bp->b_markp = NULL;
	bp->b_marko = 0;
	bp->b_nwnd  = 0;
	bp->b_linep = lp;
	bp->l_flag = 0;
	bp->b_fname[0] = '\0';
	strcpy(bp->b_bname, bname);
	lp->l_fp = lp;
	lp->l_bp = lp;
	bp->b_bufp = bheadp;
	bheadp = bp;
	bp->undo_list = NULL;
	bp->undo_current = NULL;
	bp->undo_size = 0;
	bp->undo_forward = 0;
	bp->b_size = 0;
	bp->b_dotpos = 0;

	resetbuffer(bp);	// Reset user-changeable settings

	return bp;
}


/*
 * Mg3a: Reset user-changeable settings for a buffer.
 */

void
resetbuffer(BUFFER *bp)
{
	INT i;

	bp->b_flag  = defb_flag;
	bp->b_nmodes = defb_nmodes;
	bp->charset = bufdefaultcharset;

	for (i = 0; i <= defb_nmodes; i++) {
		bp->b_modes[i] = defb_modes[i];
	}

	for (i = 0; i < localvars; i++) {
		bp->localvar.var[i] = MININT;
	}

	bp->localmodename[0] = 0;

	if (strcmp(bp->b_bname, "*scratch*") == 0 && bp->b_fname[0] == 0) {
		bp->b_flag |= BFSCRATCH;
	}

	if (bp->b_nwnd) refreshbuf(bp);
}


/*
 * This routine blows away all of the text in a buffer. If the buffer
 * is marked as changed then we ask if it is ok to blow it away; this
 * is to save the user the grief of losing text. The window chain is
 * nearly always wrong if this gets called; the caller must arrange
 * for the updates that are required. Return TRUE if everything looks
 * good.
 */

INT
bclear(BUFFER *bp)
{
	LINE	*lp;
	INT	s;

	if ((bp->b_flag&BFCHG) != 0		/* Changed.		*/
	&& (s=eyesno("Buffer modified; kill anyway")) != TRUE)
		return (s);
	bp->b_flag  &= ~BFCHG;			/* Not changed		*/
	while ((lp=lforw(bp->b_linep)) != bp->b_linep)
		lfree(bp, lp, 0);
	bp->b_dotp  = bp->b_linep;		/* Fix "."		*/
	bp->b_doto  = 0;
	bp->b_markp = NULL;			/* Invalidate "mark"	*/
	bp->b_marko = 0;
	bp->b_dotpos = 0;
	bp->b_size = 0;
	return TRUE;
}


/*
 * Display the given buffer in the given window.
 *
 * Mg3a: simplified with no flags; we always need full update and
 * reframe.
 */

INT
showbuffer(BUFFER *bp, WINDOW *wp)
{
	BUFFER		*obp;
	WINDOW		*owp;
	const INT	addflags = WFFORCE|WFHARD|WFMODE;

	if (wp->w_bufp == bp) {			/* Easy case!	*/
		wp->w_flag |= addflags;
		return TRUE;
	}

	/* First, dettach the old buffer from the window */
	if ((bp->b_altb = obp = wp->w_bufp) != NULL) {
		if (--obp->b_nwnd == 0) {
			obp->b_dotp   = wp->w_dotp;
			obp->b_doto   = wp->w_doto;
			obp->b_markp  = wp->w_markp;
			obp->b_marko  = wp->w_marko;
			obp->b_dotpos = wp->w_dotpos;
		}
	}

	/* Now, attach the new buffer to the window */
	wp->w_bufp = bp;
	wp->w_linep = lforw(bp->b_linep);	/* Otherwise, weirdly points into wrong buffer */

	wp->w_dotp   = lforw(bp->b_linep);	/* For safety's sake	*/
	wp->w_doto   = 0;
	wp->w_markp  = NULL;
	wp->w_marko  = 0;
	wp->w_dotpos = 0;

	if (bp->b_nwnd++ == 0) {		/* First use.		*/
		wp->w_dotp   = bp->b_dotp;
		wp->w_doto   = bp->b_doto;
		wp->w_markp  = bp->b_markp;
		wp->w_marko  = bp->b_marko;
		wp->w_dotpos = bp->b_dotpos;
	} else {
		/* already on screen, steal values from other window */
		for (owp = wheadp; owp != NULL; owp = owp->w_wndp) {
			if (owp->w_bufp == bp && owp != wp) {
				wp->w_dotp   = owp->w_dotp;
				wp->w_doto   = owp->w_doto;
				wp->w_markp  = owp->w_markp;
				wp->w_marko  = owp->w_marko;
				wp->w_dotpos = owp->w_dotpos;
				break;
			}
		}
	}

	wp->w_flag |= addflags;
	return TRUE;
}


/*
 * Pop the buffer we got passed onto the screen. Returns a status.
 */

WINDOW *
popbuf(BUFFER *bp)
{
	WINDOW *wp;

	if (bp->b_nwnd == 0) {		/* Not on screen yet.	*/
		if ((wp=wpopup()) == NULL) return NULL;
	} else {
		refreshbuf(bp);

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp) return wp;
		}

		wp = curwp;
	}
	showbuffer(bp, wp);
	return wp;
}


/*
 * Insert another buffer at dot.  Very useful.
 */

INT
bufferinsert(INT f, INT n)
{
	BUFFER 		*bp;
	LINE		*clp;
	size_t		nline;
	INT		s;
	char		bufn[NFILEN];

	/* Get buffer to use from user */
	if (curbp->b_altb == NULL) {
		s = eread("Insert buffer: ", bufn, NFILEN, EFBUF);
	} else {
		s = eread("Insert buffer: (default %S) ", bufn, NFILEN,
			 EFBUF, curbp->b_altb->b_bname);
	}

	if (s == ABORT) return (s);

	if (s == FALSE) {
		if (curbp->b_altb != NULL) bp = curbp->b_altb;
		else return FALSE;
	} else if ((bp = bfind(bufn, FALSE)) == NULL) {
		return buffernotfound(bufn);
	}

	if (bp == curbp) {
		ewprintf("Cannot insert buffer into self");
		return FALSE;
	}

	/* insert the buffer */
	nline = 0;
	clp = lforw(bp->b_linep);
	if (lnewline() == FALSE) return FALSE;
	endofpreviousline();
	for(;;) {
		if (linsert_general_string(bp->charset, 1, ltext(clp), llength(clp)) == FALSE)
			return FALSE;
		if((clp = lforw(clp)) == bp->b_linep) break;
		if (lnewline() == FALSE)
			return FALSE;
		nline++;
	}
	if (ldelnewline() == FALSE) return FALSE;
	if (nline == 1) ewprintf("[Inserted 1 line]");
	else	ewprintf("[Inserted %z lines]", nline);
	return (TRUE);
}


/*
 * Turn off the dirty bit on this buffer.
 */

INT
notmodified(INT f, INT n)
{
	curbp->b_flag &= ~BFCHG;
	winflag(curbp, WFMODE);
	ewprintf("Modification-flag cleared");
	return TRUE;
}


/*
 * Popbuf and set all windows to top of buffer.
 */

INT
popbuftop(BUFFER *bp)
{
	return popbuf(buftotop(bp)) != NULL ? TRUE : FALSE;
}
