/*
 *		Text line handling.
 *
 * The functions in this file are a general set of line management
 * utilities. They are the only routines that touch the text. They
 * also touch the buffer and window structures, to make sure that the
 * necessary updating gets done. There are routines in this file that
 * handle the kill buffer too. It isn't here for any good reason.
 *
 * Note that this code only updates the dot and mark values in the
 * window list. Since all the code acts on the current window, the
 * buffer that we are editing must be being displayed, which means
 * that "b_nwnd" is non zero, which means that the dot and mark values
 * in the buffer headers are nonsense.
 */

/*

Summary of insert/overwrite into the current buffer functions:

 linsert_str		insert n copies of a raw, untranslated string. The function
			everything else is built on.
 linsert		insert n copies of a byte, or n newlines.
 linsert_ucs		insert n copies of an UCS-4 character, translated to a given charset,
 			or n newlines.
 linsert_overwrite	insert or overwrite n copies of an UCS-4 character, translated to a
			given charset, or n newlines.
 linsert_general_string	insert n copies of a string with a given charset, translating to
			the buffer charset, and splitting lines on newlines. Used by yank().

 linsert_overwrite_general_string	insert or overwrite a string, otherwise as
					linsert_general_string. Used in macro replay.

*/

#include	"def.h"

#define RECOVER 1

#ifndef MALLOCROUND
#define MALLOCROUND 0
#endif

#if RECOVER
static void reduceoverhead(LINE *lp1);
#endif

#if TESTCMD == 2
int count_alloc=0, count_realloc=0;
int count_kalloc=0;
double count_kcopy=0.0;
#endif

INT	forwchar(INT f, INT n);
INT	backchar(INT f, INT n);


#ifndef NBLOCK
#define NBLOCK	16			/* Line block chunk size	*/
#endif

#ifndef KBLOCK
#define KBLOCK	256			/* Kill buffer block size.	*/
#endif

static char	*kbufp	= NULL;		/* Kill buffer data.		*/
static INT	kused	= 0;		/* # of bytes used in KB.	*/
static INT	ksize	= 0;		/* # of bytes allocated in KB.	*/
static INT	kstart	= 0;		/* # of first used byte in KB.	*/

INT		kbuf_flag = 0;
charset_t	kbuf_charset = NULL;
INT		kbuf_wholelines = 0;

/* The global mark gets updated like all the window dot and marks	*/

LINE		*gmarkp = NULL;		/* Global mark pointer		*/
INT		gmarko = 0;		/* Global mark offset		*/

#ifdef UPDATE_DEBUG
INT update_debug = 0;

static void
do_debug()
{
	refreshbuf(NULL);
	update();
	if (getkey(FALSE) == 'q') update_debug = 0;
}

INT
do_update_debug(INT f, INT n)
{
	update_debug = 1;
	return TRUE;
}
#endif

INT
readonly()
{
	ewprintf("Buffer is read only");
	return FALSE;
}

static INT
overflow()
{
	ewprintf("Integer overflow when allocating memory");
	return FALSE;
}


/*
 * This routine allocates a block of memory large enough to hold a
 * LINE containing "used" characters. The block is rounded up to
 * whatever needs to be allocated. (use lallocx for lines likely to
 * grow.) Return a pointer to the new block, or NULL if there isn't
 * any memory left. Print a message in the message line if no space.
 *
 * Mg3a: use realloc.
 */

static LINE *
lrealloc(LINE *lp, INT used)
{
	INT	size, offset;
#if TESTCMD == 2
	LINE *lpold = lp;
#endif

	offset = offsetof(LINE, l_text);

	if (used < 0 || used > MAXINT - offset - MALLOCROUND) {
		overflow();
		return NULL;
	}

	size = used + offset;
	if (size < (INT)sizeof(LINE)) size = sizeof(LINE);	/* use padding  */

#if MALLOCROUND
	size += (MALLOCROUND-1);	/* round up to a size optimal to malloc */
	size &= ~(MALLOCROUND-1);
#endif
	if ((lp = (LINE *)realloc(lp, size)) == NULL) {
		return outofmem(size);
	}

	lp->l_size = size - offset;
	lp->l_used = used;
	lp->l_flag = 0;
#if TESTCMD == 2
	if (lp == lpold) count_realloc++; else count_alloc++;
#endif
	return lp;
}

LINE *
lalloc(INT used)
{
	return lrealloc(NULL, used);
}


/*
 * Like lalloc, only round amount desired up because this line will
 * probably grow. We always make room for at least one more char.
 * (thus making 0 not a special case anymore.)
 *
 * Mg3a: realloc, and scale better if copy
 */

static LINE *
lreallocx(LINE *lp, INT used)
{
	INT size;

	if (used < 0 || used > MAXINT - (used>>4) - NBLOCK) {
		overflow();
		return NULL;
	}

	size = used + (used>>4) + NBLOCK;

	if((lp = lrealloc(lp, size)) != NULL) lp->l_used = used;

	return lp;
}

LINE *
lallocx(INT used)
{
	return lreallocx(NULL, used);
}


/*
 * Delete line "lp". Fix all of the links that might point at it (they
 * are moved to offset 0 of the next line). Unlink the line from
 * whatever buffer it might be in. Release the memory. The buffers are
 * updated too; the magic conditions described in the above comments
 * don't hold here.
 *
 * Mg3a: Add buffer as parameter to scale to many buffers. Deallocate
 * mark if last line in the buffer.
 */

void
lfree(BUFFER *bp, LINE *lp, size_t pos)
{
	WINDOW *wp;
	LINE *blp = bp->b_linep;

	if (gmarkp == lp) {
		gmarkp = lforw(lp);
		if (gmarkp == blp) gmarkp = NULL;
		gmarko = 0;
	}

	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_bufp == bp) {
			if (wp->w_linep == lp)
				wp->w_linep = lp->l_fp;
			if (wp->w_dotp	== lp) {
				wp->w_dotp  = lp->l_fp;
				wp->w_doto  = 0;
			}
			if (wp->w_markp == lp) {
				wp->w_markp = lp->l_fp;
				if (wp->w_markp == blp) wp->w_markp = NULL;
				wp->w_marko = 0;
			}
			if (wp->w_dotpos > pos) {
				if (wp->w_dotpos <= pos + llength(lp)) {
					wp->w_dotpos = pos;
				} else {
					wp->w_dotpos -= llength(lp) + 1;
				}
			}
		}
	}

	if (bp->b_nwnd == 0) {
		if (bp->b_dotp	== lp) {
			bp->b_dotp = lp->l_fp;
			bp->b_doto = 0;
		}
		if (bp->b_markp == lp) {
			bp->b_markp = lp->l_fp;
			if (bp->b_markp == blp) bp->b_markp = NULL;
			bp->b_marko = 0;
		}
		if (bp->b_dotpos > pos) {
			if (bp->b_dotpos <= pos + llength(lp)) {
				bp->b_dotpos = pos;
			} else {
				bp->b_dotpos -= llength(lp) + 1;
			}
		}
	}

	bp->b_size -= llength(lp);
	if (lforw(lp) != blp) bp->b_size--;

	lp->l_bp->l_fp = lp->l_fp;
	lp->l_fp->l_bp = lp->l_bp;

	free(lp);
}


/*
 * This routine gets called when a character is changed in place in
 * the current buffer. It updates all of the required flags in the
 * buffer and window system. The flag used is passed as an argument;
 * if the buffer is being displayed in more than 1 window we change
 * EDIT to HARD. Set MODE if the mode line needs to be updated (the
 * "*" has to be set).
 */

void
lchange(INT flag)
{
	WINDOW *wp;

	if (!(curbp->b_flag & BFCHG)) {		/* First change, so	*/
		flag |= WFMODE;			/* update mode lines.	*/
		curbp->b_flag |= BFCHG;
	}

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			wp->w_flag |= flag;

			if (wp != curwp) wp->w_flag |= WFHARD;
		}
	}
}


/*
 * Insert "n" copies of the string "str" of length "len" at the
 * current location of dot. In the easy case all that happens is the
 * text is stored in the line. In the hard case, the line has to be
 * reallocated. When the window list is updated, take special care; I
 * screwed it up once. You always update dot in the current window.
 * You update mark, and a dot in another window, if it is greater than
 * the place where you did the insert. Return TRUE if all is well, and
 * FALSE on errors.
 *
 * Mg3a: rewritten quite a bit, uses realloc
 */

INT
linsert_str(INT n, char *str, INT len)
{
	char	*cpdot, *startdot;
	LINE	*lp1;
	LINE	*lp2;
	INT	doto;
	WINDOW	*wp;
	INT	oldlen, newlen, nlen;
	INT	i, j;
	size_t	pos;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0 || len < 0) return FALSE;
	if (n == 0 || len == 0) return TRUE;

#ifdef UPDATE_DEBUG
if (update_debug) do_debug();
#endif

	if (curwp->w_dotp == curbp->b_linep) {
		if (normalizebuffer(curbp) == FALSE) return FALSE;
	}

	lp1 = curwp->w_dotp;
	oldlen = llength(lp1);

	if (n == 1) {
		if (len > MAXINT - oldlen) return overflow();
		nlen = len;
	} else {
		if (len > MAXINT / n || n*len > MAXINT - oldlen) return overflow();
		nlen = n*len;
	}

	newlen = oldlen + nlen;
	lp2 = lp1;

	if (newlen > lsize(lp1)) {
		if ((lp2 = lreallocx(lp1, newlen)) == NULL)
			return FALSE;

		if (lp1 != lp2) {
			lp2->l_bp->l_fp = lp2;
			lp2->l_fp->l_bp = lp2;
		}
	}

	// UNDO
	u_modify();
	//

	lchange(WFEDIT);

	lp2->l_flag = 0;
	lsetlength(lp2, newlen);

	doto = curwp->w_doto;
	cpdot = &ltext(lp2)[doto];

	if (oldlen > doto) {
		memmove(cpdot + nlen, cpdot, oldlen - doto);
	}

	startdot = cpdot;

	for (i = 0; i < n; i++) {
		if (len <= 4) {
			for (j = 0; j < len; j++) {
				*cpdot++ = str[j];
			}
		} else {
			memcpy(cpdot, str, len);
			cpdot += len;
		}
	}

	if (gmarkp == lp1) {
		gmarkp = lp2;
		if (gmarko > doto) gmarko += nlen;
	}

	pos = curwp->w_dotpos;

	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			if (wp->w_linep == lp1)
				wp->w_linep = lp2;

			if (wp->w_dotp == lp1) {
				wp->w_dotp = lp2;
				if (wp == curwp || wp->w_doto > doto)
					wp->w_doto += nlen;
			}

			if (wp->w_dotpos > pos)
				wp->w_dotpos += nlen;

			if (wp->w_markp == lp1) {
				wp->w_markp = lp2;
				if (wp->w_marko > doto)
					wp->w_marko += nlen;
			}
		}
	}

	curwp->w_dotpos += nlen;
	curbp->b_size += nlen;

	// UNDO
	if (!u_entry(UFL_INSERT, pos, startdot, nlen)) return FALSE;
	//

	return TRUE;
}


/*
 * Mg3a: as linsert_str, but a single byte. Also handle newline.
 */

INT
linsert(INT n, INT c)
{
	char cc = c;

	if (c == '\n') return lnewline_n(n);

	return linsert_str(n, &cc, 1);
}


/*
 * Mg3a: as linsert, but insert a full UCS-4 character according to
 * the charset.
 */

INT
linsert_ucs(charset_t charset, INT n, INT c)
{
	unsigned char buf[4];
	INT len;

	if (c == '\n') return lnewline_n(n);

	ucs_chartostr(charset, c, buf, &len);
	return linsert_str(n, (char *)&buf[0], len);
}


/*
 * Mg3a: duplicate the current line below it
 *
 * IGNORE parameters
 */
INT
lduplicate(INT f, INT n)
{
	INT s;
	struct LINE *dotp = curwp->w_dotp;
	INT          doto = curwp->w_doto;

	if (curbp->b_flag & BFREADONLY) return readonly();

	/*
	 * Goto the end of line
	 * 	c&p from gotoeol(f,n);
	 */
	adjustpos(curwp->w_dotp, llength(curwp->w_dotp));

	/*
	 * Insert a newline
	 */
	s = lnewline_n(1);
	if (TRUE == s) {
		linsert_str(1, ltext(dotp), llength(dotp)); /* Insert line contents */
	}
	/*
	 * restore the cursor
	 */
	adjustpos(dotp,	doto);
	return s;
}

#if 0							/* Not yet ready */
/*
 * MG3A: swap current line and next line
 */
INT
lmovedown(INT f, INT n)
{
	int i;
	struct LINE *dotp = curwp->w_dotp;

	/* Check you can do it */
	if (curbp->b_flag & BFREADONLY)
		return readonly();

	if (n < 0) {
		return lmoveup(f, -n);
	}

	for (i = 0; i< n; i++) {
		struct LINE *prevp = dotp -> l_bp;
		struct LINE *nextp = dotp -> l_fp;
		struct LINE *followp = nextp -> l_fp;

		nextp->l_bp = prevp;
		nextp->l_fp = dotp;
		dotp->l_bp = nextp;
		dotp->l_fp = followp;
	}
	/*
	 * Mark buffer as dirty
	 */
	curbp->b_flag |= BFCHG;

	/*
	 * refresh the display
	 */
	refreshbuf(curbp);
	return TRUE;
}
/*
 * Mg3a: move current line up
 */
INT
lmoveup(INT f, INT n)
{
	int i;
	struct LINE *dotp = curwp->w_dotp;
	/* Check you can do it */
	if (curbp->b_flag & BFREADONLY)
		return readonly();

	if (n < 0) {
		return lmovedown(f, -n);
	}
	for (i=0; i<n;i++) {
		struct LINE *prevp =  dotp -> l_bp;
		struct LINE *firstp = prevp -> l_fp;
		struct LINE *nextp =  dotp -> l_fp;

		dotp->l_fp = prevp;
		dotp->l_bp = firstp;
		prevp->l_fp = nextp;
		prevp->l_bp = dotp;
	}
	/*
	 * Mark buffer as dirty
	 */
	curbp->b_flag |= BFCHG;

	/*
	 * refresh the display
	 */
	refreshbuf(curbp);
	return TRUE;
}
#endif

/*
 * Mg3a: erase characters to a visual width, preparing for a
 * subsequent insert to implement overwrite mode.
 */

static INT
lerase_for_overwrite(INT width)
{
	LINE *dotp;
	INT doto, enddoto, col, endcol, goalcol, linelen;
	INT c;

	if (curbp->b_flag & BFREADONLY) return readonly();

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;

	if (width < 0) return column_overflow();

	col = -1;		// No tab yet
	goalcol = width;	// No tab yet

	enddoto = doto;
	endcol = 0;		// No tab yet
	linelen = llength(dotp);

	while (enddoto < linelen && endcol < goalcol) {
		c = ucs_char(curbp->charset, dotp, enddoto, NULL);

		if (endcol > MAXINT - 8) return column_overflow();

		if (c == '\t') {
			if (col < 0) {
				col = getfullcolumn(curwp, dotp, doto);
				if (col > MAXINT - endcol - 8 || col > MAXINT - goalcol)
					return column_overflow();
				endcol += col;
				goalcol += col;
			}

			endcol = (endcol | 7) + 1;
		} else {
			endcol += ucs_fulltermwidth(c);
		}

		enddoto = ucs_forward(curbp->charset, dotp, enddoto);
	}

	if (ldeleteraw(enddoto - doto, KNONE) == FALSE) return FALSE;

	if (endcol > goalcol) {
		doto = curwp->w_doto;
		if (linsert(endcol - goalcol, ' ') == FALSE) return FALSE;
		adjustpos(curwp->w_dotp, doto);
	}

	return TRUE;
}


/*
 * Mg3a: as linsert_ucs (which always inserts), but also do overwrite
 * mode.
 */

INT
linsert_overwrite(charset_t charset, INT n, INT c)
{
	INT width;

	if (!(curbp->b_flag & BFOVERWRITE) || c == '\t' || c == '\n' ||
		curwp->w_doto == llength(curwp->w_dotp))
	{
		return linsert_ucs(charset, n, c);
	}

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0) return FALSE;
	if (n == 0) return TRUE;

	width = ucs_fulltermwidth(c);

	if (width == 2 && n > MAXINT / 2) return column_overflow();

	if (lerase_for_overwrite(width * n) == FALSE) return FALSE;

	return linsert_ucs(charset, n, c);
}


/*
 * Mg3a: add an entire line efficiently. The dot should be at the end
 * of the current line. No window pointers need reset since it's an
 * entirely new line. Does not do undo.
 */

INT
laddline(char *txt, INT len)
{
	LINE *lp1, *lp2, *blp = curbp->b_linep;
	size_t	pos = curwp->w_dotpos;
	WINDOW *wp;
	size_t addlen = (lforw(blp) == blp) ? len : len + 1;
	int wflag = (curbp->b_flag & BFCHG) ? WFHARD : (WFHARD|WFMODE);

#ifdef UPDATE_DEBUG
if (update_debug) do_debug();
#endif
	lp1 = curwp->w_dotp;

	if (curwp->w_doto != llength(lp1)) {
		ewprintf("laddline: not at end of line");
		goto error;
	}

	if ((lp2 = lalloc(len)) == NULL) goto error;

	memcpy(ltext(lp2), txt, len);

	lp2->l_fp = lp1->l_fp;
	lp2->l_bp = lp1;
	lp1->l_fp->l_bp = lp2;
	lp1->l_fp = lp2;

	curwp->w_dotp = lp2;
	curwp->w_doto = len;

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			if (wp->w_dotpos > pos) wp->w_dotpos += addlen;

			wp->w_flag |= wflag;
		}
	}

	curwp->w_dotpos += addlen;
	curbp->b_size += addlen;
	curbp->b_flag |= BFCHG;

	return TRUE;

	// UNDO
error:
	u_clear(curbp);
	return FALSE;
	//
}


/*
 * Insert a newline into the buffer at the current location of dot in
 * the current window.
 */

INT
lnewline()
{
	LINE	*lp1;
	LINE	*lp2;
	INT	doto;
	INT	nlen;
	WINDOW	*wp;
	size_t	pos = curwp->w_dotpos;

#ifdef UPDATE_DEBUG
if (update_debug) do_debug();
#endif
	if (curbp->b_flag & BFREADONLY) return readonly();

	if (curwp->w_dotp == curbp->b_linep) {
		if (normalizebuffer(curbp) == FALSE) return FALSE;
	}
	lp1  = curwp->w_dotp;			/* Get the address and	*/
	lp1->l_flag = 0;
	doto = curwp->w_doto;			/* offset of "."	*/

	// UNDO
	u_modify();
	if (!u_entry(UFL_INSERT, pos, "\n", 1)) return FALSE;
	//

	lchange(WFHARD);

	if(doto == 0) {				/* avoid unnecessary copying */
		if((lp2 = lallocx(0)) == NULL)	/* new first part	*/
			goto error;
		lp2->l_bp = lp1->l_bp;
		lp1->l_bp->l_fp = lp2;
		lp2->l_fp = lp1;
		lp1->l_bp = lp2;
		if (gmarkp == lp1 && gmarko == 0) gmarkp = lp2;
		for(wp = wheadp; wp!=NULL; wp = wp->w_wndp) {
			if (wp->w_bufp == curbp) {
				if (wp->w_linep == lp1) wp->w_linep = lp2;
				if (wp->w_dotp == lp1 && wp->w_doto == 0 && wp != curwp) wp->w_dotp = lp2;
				if (wp->w_markp == lp1 && wp->w_marko == 0) wp->w_markp = lp2;
				if (wp->w_dotpos > pos) wp->w_dotpos++;
			}
		}
		curwp->w_dotpos++;
		curbp->b_size++;
		return	TRUE;
	}
	nlen = llength(lp1) - doto;		/* length of new part	*/
	if((lp2=lallocx(nlen)) == NULL)		/* New second half line */
		goto error;
	if(nlen!=0) bcopy(&lp1->l_text[doto], &lp2->l_text[0], nlen);
	lp1->l_used = doto;
	lp2->l_bp = lp1;
	lp2->l_fp = lp1->l_fp;
	lp1->l_fp = lp2;
	lp2->l_fp->l_bp = lp2;
	if (gmarkp == lp1 && gmarko > doto) {
		gmarkp = lp2;
		gmarko -= doto;
	}
	for(wp = wheadp; wp != NULL; wp = wp->w_wndp) { /* Windows	*/
		if (wp->w_bufp == curbp) {
			if (wp->w_dotp == lp1 && (wp->w_doto > doto || wp == curwp)) {
				wp->w_dotp = lp2;
				wp->w_doto -= doto;
			}
			if (wp->w_markp == lp1 && wp->w_marko > doto) {
				wp->w_markp = lp2;
				wp->w_marko -= doto;
			}
			if (wp->w_dotpos > pos) wp->w_dotpos++;
		}
	}
	curwp->w_dotpos++;
	curbp->b_size++;
#if RECOVER
	reduceoverhead(lp1);
#endif
	return TRUE;

	// UNDO
error:
	u_clear(curbp);
	return FALSE;
	//
}


/*
 * Mg3a: insert "n" newlines.
 */

INT
lnewline_n(INT n)
{
	INT s;

	if (n < 0) return FALSE;

	while (n--) {
		if ((s = lnewline()) != TRUE) return s;
	}

	return TRUE;
}


/*
 * This function deletes "n" characters, starting at dot. It
 * understands how do deal with end of lines, etc. It returns TRUE if
 * all of the characters were deleted, and FALSE if they were not
 * (because dot ran into the end of the buffer. The "kflag" indicates
 * either no insertion, or direction of insertion into the kill
 * buffer.
 *
 * Mg3a: If a kill, store the buffer charset and flags with the
 * killbuffer. Direction is also provided independently.
 */

/* Kill direction */

#define KDFORWARD 1
#define KDBACKWARD 0

static INT
ldelete_internal(INT n, INT kflag, INT direction)
{
	char		*cpdot;
	LINE		*dotp;
	INT		doto;
	INT		chunk;
	WINDOW		*wp;
	INT		savedonenewline = 0;	/* save one newline to delete until the end */
	INT		ret = TRUE;
	size_t		dotpos;

#ifdef UPDATE_DEBUG
if (update_debug) do_debug();
#endif
	while (n > 0) {
		dotp = curwp->w_dotp;
		doto = curwp->w_doto;
		dotpos = curwp->w_dotpos;
		dotp->l_flag = 0;

		if (dotp == curbp->b_linep)	/* Hit end of buffer.	*/
			return FALSE;

		if (direction == KDFORWARD) {
			chunk = dotp->l_used - doto;	/* Size of chunk.	*/
			if (chunk > n)
				chunk = n;
		} else {
			chunk = doto;			/* Size of chunk.	*/
			if (chunk > n)
				chunk = n;
			doto -= chunk;
			dotpos -= chunk;
		}

		if (chunk == 0) {		/* End of line, merge.	*/
			if (direction == KDFORWARD) {
				if (dotp == lback(curbp->b_linep)) {
					ret = FALSE;
					goto end;	/* End of buffer.	*/
				}

				if (!savedonenewline) {	/* Save one newline until later */
					adjustpos3(lforw(curwp->w_dotp), 0, dotpos + 1);
				}
			} else {
				if (dotp == lforw(curbp->b_linep)) {
					ret = FALSE;
					goto end;	/* Beginning of buffer.	*/
				}

				adjustpos3(lback(dotp), llength(lback(dotp)), dotpos - 1);
			}

			lchange(WFHARD);

			if (savedonenewline && ldelnewline_noundo() == FALSE)
				return FALSE;

			savedonenewline = 1;

			if (kflag != KNONE) kinsert('\n', kflag);

			n--;
			continue;
		}

#if TESTCMD == 2
		count_kcopy += chunk;
#endif
		lchange(WFEDIT);

		cpdot = &ltext(dotp)[doto];	/* Scrunch text.	*/

		if (kflag == KFORW) {
			memcpy(&kbufp[kused], cpdot, chunk);
			kused += chunk;
		} else if (kflag == KBACK) {
			memcpy(&kbufp[kstart-chunk], cpdot, chunk);
			kstart -= chunk;
		} else if (kflag != KNONE) panic("broken ldelete call", 0);

		if (doto + chunk != llength(dotp)) {
			memmove(cpdot, cpdot + chunk, llength(dotp) - doto - chunk);
		}

		lsetlength(dotp, llength(dotp) - chunk);

		if (gmarkp == dotp && gmarko >= doto) {
			gmarko -= chunk;
			if (gmarko < doto) gmarko = doto;
		}

		for(wp = wheadp; wp != NULL; wp = wp->w_wndp) {
			if (wp->w_bufp == curbp) {
				if (wp->w_dotp == dotp && wp->w_doto >= doto) {
					wp->w_doto -= chunk;

					if (wp->w_doto < doto)
						wp->w_doto = doto;
				}

				if (wp->w_dotpos > dotpos) {
					if (wp->w_dotpos < dotpos + chunk)
						wp->w_dotpos = dotpos;
					else
						wp->w_dotpos -= chunk;
				}

				if (wp->w_markp == dotp && wp->w_marko >= doto) {
					wp->w_marko -= chunk;

					if (wp->w_marko < doto)
						wp->w_marko = doto;
				}
			}
		}

		curbp->b_size -= chunk;

		n -= chunk;
	}

end:
	if (savedonenewline) {
		if (direction == KDFORWARD) endofpreviousline();
		if (ldelnewline_noundo() == FALSE) return FALSE;
	}
#if RECOVER
	reduceoverhead(curwp->w_dotp);
#endif
	return ret;
}


/*
 * Do initial checks, do stuff with undo.
 */

// UNDO
static INT
ldelete_undo(LINE *lp, INT offset, INT n, INT kflag, INT direction, size_t pos)
{
	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0 || lp == curbp->b_linep) return FALSE;

	if (kflag != KNONE) {
		if ((kflag == KFORW) != (direction == KDFORWARD))
			panic("ldelete kflag mismatch", 0);

		/* Grow the killbuffer in advance */
		if (kgrow(kflag == KBACK, n) == FALSE) return FALSE;

		kbuf_flag = curbp->b_flag;
		kbuf_charset = curbp->charset;
		kbuf_wholelines = 0;

	}

	u_modify();
	if (!u_entry_range(UFL_DELETE, lp, offset, n, pos)) return FALSE;

	return ldelete_internal(n, kflag, direction);
}
//


/*
 * Mg3a: Delete functions.
 *
 * NOTE: the new *back functions delete in the backward direction and
 * assume the cursor is at the end of the range to be deleted. KBACK
 * must only be used with these functions.
 *
 * The raw functions delete bytes, the others delete full-blown
 * characters, including possibly combined characters.
 */

INT
ldelete(INT n, INT kflag)
{
	INT count = 0;
	LINE *lp = curwp->w_dotp, *blp = curbp->b_linep;
	INT doto = curwp->w_doto, doto2, len, ret = TRUE, ret2;
	charset_t charset = curbp->charset;

	if (lp == blp) return FALSE;

	while (n > 0) {
		len = llength(lp);

		while (n > 0 && doto < len) {
			doto2 = ucs_forward(charset, lp, doto);
			if (count > MAXINT - (doto2 - doto) - 1) return overflow();
			count += doto2 - doto;
			n--;
			doto = doto2;
		}

		if (n > 0) {
			if (lforw(lp) == blp) {ret = FALSE; break;}
			n--;
			count++;
			lp = lforw(lp);
			doto = 0;
		}
	}

	ret2 = ldelete_undo(curwp->w_dotp, curwp->w_doto, count, kflag, KDFORWARD, curwp->w_dotpos);
	return (ret == FALSE) ? ret : ret2;
}

INT
ldeleteback(INT n, INT kflag)
{
	INT count = 0;
	LINE *lp = curwp->w_dotp, *blp = curbp->b_linep;
	INT doto = curwp->w_doto, doto2, ret = TRUE, ret2;
	charset_t charset = curbp->charset;

	if (lp == blp) return FALSE;

	while (n > 0) {
		while (n > 0 && doto > 0) {
			doto2 = ucs_backward(charset, lp, doto);
			if (count > MAXINT - (doto - doto2) - 1) return overflow();
			count += doto - doto2;
			n--;
			doto = doto2;
		}

		if (n > 0) {
			if (lback(lp) == blp) {ret = FALSE; break;}
			n--;
			count++;
			lp = lback(lp);
			doto = llength(lp);
		}
	}

	ret2 = ldelete_undo(lp, doto, count, kflag, KDBACKWARD, curwp->w_dotpos - count);
	return (ret == FALSE) ? ret : ret2;
}


INT
ldeleteraw(INT n, INT kflag)
{
	if (curwp->w_dotp == curbp->b_linep) return FALSE;

	return ldelete_undo(curwp->w_dotp, curwp->w_doto, n, kflag, KDFORWARD, curwp->w_dotpos);
}


INT
ldeleterawback(INT n, INT kflag)
{
	LINE *lp = curwp->w_dotp, *blp = curbp->b_linep;
	INT doto = curwp->w_doto, ret = TRUE, ret2, saven = n;

	if (lp == blp) return FALSE;

	while (n > 0) {
		if (doto >= n) {
			doto -= n;
			n = 0;
			break;
		}

		n -= doto;
		doto = 0;

		if (lback(lp) == blp) {ret = FALSE; break;}

		n--;
		lp = lback(lp);
		doto = llength(lp);
	}

	ret2 = ldelete_undo(lp, doto, saven-n, kflag, KDBACKWARD, curwp->w_dotpos - (saven - n));
	return (ret == FALSE) ? ret : ret2;
}


/*
 * Delete a region. KKILL or KNONE must be used for kflag. Note that
 * GNU Emacs considers a forward region to have dot at the end.
 */

INT
ldeleteregion(const REGION *rp, INT kflag)
{
	if (kflag == KKILL) kflag = rp->r_forward ? KFORW : KBACK;
	else if (kflag != KNONE) panic("ldeleteregion kflag mismatch", 0);

	if (rp->r_forward) {
		adjustpos3(rp->r_linep, rp->r_offset, rp->r_dotpos);
		return ldelete_undo(rp->r_linep, rp->r_offset, rp->r_size, kflag, KDFORWARD, rp->r_dotpos);
	} else {
		adjustpos3(rp->r_endlinep, rp->r_endoffset, rp->r_enddotpos);
		return ldelete_undo(rp->r_linep, rp->r_offset, rp->r_size, kflag, KDBACKWARD, rp->r_dotpos);
	}
}


/*
 * Delete a newline. Join the current line with the next line. If the
 * next line is the magic header line always return TRUE; merging the
 * last line with the header line can be thought of as always being a
 * successful operation, even if nothing is done, and this makes the
 * kill buffer work "right". Easy cases can be done by shuffling data
 * around. Hard cases require that lines be moved about in memory.
 * Return FALSE on error and TRUE if all looks ok.
 */

INT
ldelnewline()
{
	LINE	*lp1;
	LINE	*lp2;
	WINDOW	*wp;
	LINE	*lp3;
	size_t	pos;

#ifdef UPDATE_DEBUG
if (update_debug) do_debug();
#endif
	if (curbp->b_flag & BFREADONLY) return readonly();

	lp1 = curwp->w_dotp;
	lp2 = lp1->l_fp;
	lp1->l_flag = 0;
	if (lp1 == curbp->b_linep || lp2 == curbp->b_linep)		/* At the buffer end.	*/
		return TRUE;

	// UNDO
	u_modify();
	pos = curwp->w_dotpos - curwp->w_doto + llength(curwp->w_dotp);
	if (!u_entry(UFL_DELETE, pos, "\n", 1)) return FALSE;
	//

	/* First line empty */
	if (llength(lp1) == 0) {
		lp1->l_bp->l_fp = lp2;
		lp2->l_bp = lp1->l_bp;

		if (gmarkp == lp1) gmarkp = lp2;

		for(wp = wheadp; wp != NULL; wp = wp->w_wndp) {
			if (wp->w_bufp == curbp) {
				if (wp->w_linep == lp1) wp->w_linep = lp2;
				if (wp->w_dotp  == lp1) wp->w_dotp  = lp2;
				if (wp->w_markp == lp1) wp->w_markp = lp2;
				if (wp->w_dotpos > pos) wp->w_dotpos--;
			}
		}

		curbp->b_size--;

		free(lp1);
		return TRUE;
	}

	/* First line has room for second line */
	if (lp2->l_used <= lp1->l_size - lp1->l_used) {
		if (lp2->l_used)
			bcopy(&lp2->l_text[0], &lp1->l_text[lp1->l_used], lp2->l_used);

		if (gmarkp == lp2) {
			gmarkp = lp1;
			gmarko += llength(lp1);
		}
		for(wp = wheadp; wp != NULL; wp = wp->w_wndp) {
			if (wp->w_bufp == curbp) {
				if (wp->w_linep == lp2)
					wp->w_linep = lp1;
				if (wp->w_dotp == lp2) {
					wp->w_dotp  = lp1;
					wp->w_doto += lp1->l_used;
				}
				if (wp->w_markp == lp2) {
					wp->w_markp  = lp1;
					wp->w_marko += lp1->l_used;
				}
				if (wp->w_dotpos > pos) wp->w_dotpos--;
			}
		}
		curbp->b_size--;
		lp1->l_used += lp2->l_used;
		lp1->l_fp = lp2->l_fp;
		lp2->l_fp->l_bp = lp1;
		free(lp2);
		return TRUE;
	}

	/* General case */
	if (lp1->l_used > MAXINT - lp2->l_used) {overflow(); goto error;}
	if ((lp3=lalloc(lp1->l_used + lp2->l_used)) == NULL)
		goto error;
	bcopy(&lp1->l_text[0], &lp3->l_text[0], lp1->l_used);
	bcopy(&lp2->l_text[0], &lp3->l_text[lp1->l_used], lp2->l_used);
	lp1->l_bp->l_fp = lp3;
	lp3->l_fp = lp2->l_fp;
	lp2->l_fp->l_bp = lp3;
	lp3->l_bp = lp1->l_bp;
	if (gmarkp == lp1) {
		gmarkp = lp3;
	} else if (gmarkp == lp2) {
		gmarkp = lp3;
		gmarko += llength(lp1);
	}
	for(wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			if (wp->w_linep==lp1 || wp->w_linep==lp2)
				wp->w_linep = lp3;
			if (wp->w_dotp == lp1)
				wp->w_dotp  = lp3;
			else if (wp->w_dotp == lp2) {
				wp->w_dotp  = lp3;
				wp->w_doto += lp1->l_used;
			}
			if (wp->w_markp == lp1)
				wp->w_markp  = lp3;
			else if (wp->w_markp == lp2) {
				wp->w_markp  = lp3;
				wp->w_marko += lp1->l_used;
			}
			if (wp->w_dotpos > pos) wp->w_dotpos--;
		}
	}
	curbp->b_size--;
	free(lp1);
	free(lp2);
	return TRUE;

	// UNDO
error:
	u_clear(curbp);
	return FALSE;
	//
}


/*
 * Replace plen characters before dot with argument string. Control-J
 * characters in st are interpreted as newlines. There is a casehack
 * disable flag (normally it likes to match case of replacement to
 * what was there).
 *
 * Mg3a: The replacement string is in termcharset.
 */

INT
lreplace(INT plen, char *st, INT case_exact)
{
	INT		rlen;		/* replacement length		*/
	INT		c;		/* used for random characters	*/
	INT		i, j, nextoffset;
	charset_t	charset = curbp->charset;
	INT		tocase;
	INT		*special, speclen;

	if (curbp->b_flag & BFREADONLY) return readonly();

	/*
	 * Find the capitalization of the word that was found.
	 * case_exact says use exact case of replacement string (same
	 * thing that happens with lowercase found), so bypass check.
	 */

	backchar(FFARG | FFRAND, plen);

	tocase = CASEDOWN;

	c = ucs_char(charset, curwp->w_dotp, curwp->w_doto, &nextoffset);

	if (!case_exact && ucs_isupper(c)) {
		tocase = CASEUP;

		if (ucs_changecase(CASEUP, c, &special, &speclen)) {
		   	tocase = CASETITLE;
		} else if (ucs_changecase(CASETITLE, c, &special, &speclen)) {
		   	tocase = CASEUP;
		} else if (nextoffset < llength(curwp->w_dotp)) {
			c = ucs_char(charset, curwp->w_dotp, nextoffset, NULL);

			if (ucs_islower(c) && !ucs_isupper(c)) {
				tocase = CASETITLE;
			}
		}
	}

	rlen = strlen(st);
	if (ldelete(plen, KNONE) == FALSE) return FALSE;

	/*
	 * do the replacement:	If was capital, then place first
	 * char as if upper, and subsequent chars as if lower.
	 * If inserting upper, check replacement for case.
	 */
	i = 0;

	while (i < rlen) {
		c = ucs_strtochar(termcharset, st, rlen, i, &i);

		if (tocase != CASEDOWN && ucs_changecase(tocase, c, &special, &speclen)) {
			for (j = 0; j < speclen; j++) {
				if (linsert_ucs(charset, 1, special[j]) == FALSE)
					return FALSE;
			}
		} else {
			if (linsert_ucs(charset, 1, c) == FALSE)
				return FALSE;
		}

		if (tocase == CASETITLE) tocase = CASEDOWN;
	}

	return TRUE;
}


/*
 * Delete all of the text saved in the kill buffer. Called by commands
 * when a new kill context is being created. The kill buffer array is
 * released, just in case the buffer has grown to immense size. No
 * errors.
 */

void
kdelete() {
	if (kbufp != NULL) {
		free(kbufp);
		kbufp = NULL;
		kstart = kused = ksize = 0;
	}
}


/*
 * Insert a character to the kill buffer, enlarging the buffer if
 * there isn't any room. Always grow the buffer in chunks, on the
 * assumption that if you put something in the kill buffer you are
 * going to put more stuff there too later. Return TRUE if all is
 * well, and FALSE on errors. Print a message on errors. Dir says
 * whether to put it at back or front.
 */

INT
kinsert(INT c, INT dir)
{
	if (dir == KFORW) {
		if (kused == ksize  && kgrow(0, 1) == FALSE)
			return FALSE;

		kbufp[kused++] = c;
	} else if (dir == KBACK) {
		if (kstart == 0 && kgrow(1, 1) == FALSE)
			return FALSE;

		kbufp[--kstart] = c;
	} else panic("broken kinsert call", 0);		/* Oh shit! */

	kbuf_wholelines = 0;

#if TESTCMD == 2
	count_kcopy += 1;
#endif
	return TRUE;
}


/*
 * Mg3a: Insert a string into the killbuffer.
 */

INT
kinsert_str(char *str, INT len, INT dir)
{
	if (dir == KFORW) {
		if (ksize - kused < len && kgrow(0, len) == FALSE)
			return FALSE;

		memcpy(&kbufp[kused], str, len);
		kused += len;
	} else if (dir == KBACK) {
		if (kstart < len && kgrow(1, len) == FALSE)
			return FALSE;

		memcpy(&kbufp[kstart-len], str, len);
		kstart -= len;
	} else panic("broken kinsert_str call", 0);

	kbuf_wholelines = 0;

#if TESTCMD == 2
	count_kcopy += len;
#endif
	return TRUE;
}


/*
 * kgrow - get more kill buffer for the callee. back is true if we are
 * trying to get space at the beginning of the kill buffer.
 *
 * Mg3a: Make it more scaleable for really big killbuffers. Note:
 * addsize must now say how much to add. If that much is already
 * available, nothing is done.
 */

INT
kgrow(INT back, INT addsize)
{
	char	*nbufp;
	INT	nstart;

	if (addsize < 0) return overflow();

	if ((back && addsize < kstart) || (!back && addsize < ksize - kused)) return TRUE;

	if (addsize > MAXINT - (ksize>>2) - KBLOCK ||
	    ksize > MAXINT - (ksize>>2) - KBLOCK - addsize) {
		return overflow();
	}

	addsize = addsize + (ksize>>2) + KBLOCK;

	/* Not realloc since growing backward is in fact quite common. */

	if ((nbufp = malloc_msg(ksize + addsize)) == NULL) return FALSE;

	nstart = back ? (kstart + addsize) : kstart;

	if (kbufp != NULL) {
		memcpy(&nbufp[nstart], &kbufp[kstart], kused - kstart);
		free(kbufp);
	}

#if TESTCMD == 2
	count_kalloc++;
	count_kcopy += kused-kstart;
#endif
	kbufp  = nbufp;
	ksize += addsize;
	kused = kused - kstart + nstart;
	kstart = nstart;
//	ewprintf("Killbuffer size = %d", ksize);
	return TRUE;
}


/*
 * This function gets characters from the kill buffer. If the
 * character index "n" is off the end, it returns "-1". This lets the
 * caller just scan along until it gets a "-1" back.
 */

INT
kremove(INT n)
{
	if (n < 0 || n + kstart >= kused)
		return -1;
	return CHARMASK(kbufp[n + kstart]);
}


/*
 * Mg3a: Return the killbuffer in a more flexible way
 */

void
getkillbuffer(char **kbuf, INT *ksize)
{
	*kbuf = kbufp ? kbufp + kstart : "";	// Don't return NULL
	*ksize = kused - kstart;
}


/*
 * Mg3a: Check the buffer for an UTF-8 byte order mark. If found,
 * remove the BOM and return true. No update of window pointers or dot
 * position because it's only called from readin() which resets them.
 */

int
find_and_remove_bom(BUFFER *bp)
{
	LINE *lp = bp->b_linep;

	if (lforw(lp) == lp) return 0;

	lp = lforw(lp);

	if (ucs_char(utf_8, lp, 0, NULL) == 0xfeff) {
		memmove(&ltext(lp)[0], &ltext(lp)[3], llength(lp)-3);
		lsetlength(lp, llength(lp)-3);
		bp->b_size -= 3;
		winflag(bp, WFHARD);
		return 1;
	}

	return 0;
}


/*
 * Mg3a: Make sure there is an empty line at the end of the buffer and
 * normalize line pointers.
 *
 * Due to special conditions, window flags need not be changed.
 *
 * Added: But undo needs to know about it.
 */

INT
normalizebuffer(BUFFER *bp)
{
	LINE *lastlp, *blp;
	WINDOW *wp;

	blp = bp->b_linep;
	lastlp = lback(blp);

	if (lastlp == blp || llength(lastlp) != 0) {
		if (addline(bp, "") == FALSE) return FALSE;

		// UNDO
		if (bp == curbp && lastlp != blp) {
			if (!u_entry(UFL_INSERT, curbp->b_size - 1, "\n", 1)) return FALSE;
		}
		//

		lastlp = lback(blp);

		if (gmarkp == blp) {
			gmarkp = lastlp;
			gmarko = 0;
		}

		if (bp->b_nwnd == 0) {
			if (bp->b_dotp == blp) {
				bp->b_dotp = lastlp;
				bp->b_doto = 0;
			}
			if (bp->b_markp == blp) {
				bp->b_markp = lastlp;
				bp->b_marko = 0;
			}
		}

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp) {
				if (wp->w_linep == blp) wp->w_linep = lastlp;

				if (wp->w_dotp == blp) {
					wp->w_dotp = lastlp;
					wp->w_doto = 0;
				}

				if (wp->w_markp == blp) {
					wp->w_markp = lastlp;
					wp->w_marko = 0;
				}
			}
		}
	}

	return TRUE;
}


#if RECOVER

/*
 * Mg3a: recover line overhead
 */

static void
reduceoverhead(LINE *lp1)
{
	LINE *lp2;
	WINDOW *wp;

	if (llength(lp1) > lsize(lp1) - (lsize(lp1) >> 2) - 2*NBLOCK) return;

	if ((lp2 = lreallocx(lp1, llength(lp1))) == NULL) return;

	lp2->l_flag = 0;

	if (lp1 != lp2) {
		lp2->l_fp->l_bp = lp2;
		lp2->l_bp->l_fp = lp2;

		if (gmarkp == lp1) gmarkp = lp2;

		for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
			if (wp->w_linep == lp1) wp->w_linep = lp2;
			if (wp->w_dotp  == lp1) wp->w_dotp  = lp2;
			if (wp->w_markp == lp1) wp->w_markp = lp2;
		}
	}

//	ewprintf("reduced");
}
#endif


/*
 * Mg3a: Insert a general string.
 *
 * Insert a string str in charset str_charset and of length len n
 * times. '\n' is inserted as a new line. Heavily optimized. Used by
 * yank().
 */

#define MAXBUF 1024

INT
linsert_general_string(charset_t str_charset, INT n, char *str, INT len)
{
	INT		c;
	INT		i, j;
	charset_t	buf_charset = curbp->charset;
	INT		newline;
	char		buf[MAXBUF];
	INT		buflen, charlen;
	size_t		pos, distance;
	LINE		*prevline;
	INT		offset;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0 || len < 0) return FALSE;
	if (n == 0 || len == 0) return TRUE;

	/* Find first newline */
	newline = findnewline(str, len);

	/* No newline, and same charset */
	if (newline == len && str_charset == buf_charset) {
		return linsert_str(n, str, len);
	}

	// UNDO
	u_modify();
	//

	if (newline < len) {
		/* There are multiple lines. */

		/* Bracketing all the yanks with lnewline/ldelnewline avoids	*/
		/* copying data on each line, but sets WFHARD. It also enables 	*/
		/* the use of laddline.						*/

		if (lnewline_noundo() == FALSE) return FALSE;
		endofpreviousline();
	}

	/* General code */
	while (n--) {
		if (str_charset == buf_charset) {	/* Same charset, efficient */
			// UNDO
			pos = curwp->w_dotpos;
			//

			i = newline;

			if (i > 0 && linsert_str_noundo(str, i) == FALSE) return FALSE;

			while (i < len) {
				i++;

				j = findnewline(&str[i], len - i) + i;

				if (laddline(&str[i], j - i) == FALSE) return FALSE;

				i = j;
			}

			// UNDO
			if (!u_entry(UFL_INSERT, pos, str, len)) return FALSE;
			//
		} else {			/* Different charset, translate */
			// UNDO
			pos = curwp->w_dotpos;
			prevline = lback(curwp->w_dotp);
			offset = curwp->w_doto;
			//

			for (i = 0; i < len;) {
				j = i;
				if (str[i] == '\n') j++;

				buflen = 0;

				while (j < len && str[j] != '\n' && buflen < MAXBUF - 4) {
					if (CHARMASK(str[j]) < 128) {
						// Speeding up ASCII
						buf[buflen++] = str[j++];
					} else {
						c = ucs_strtochar(str_charset, str, len, j, &j);
						ucs_chartostr(buf_charset, c,
						    (unsigned char*)&buf[buflen], &charlen);
						buflen += charlen;
					}
				}

				if (str[i] == '\n') {
					if (laddline(buf, buflen) == FALSE) return FALSE;
				} else {
					if (linsert_str_noundo(buf, buflen) == FALSE) return FALSE;
				}

				i = j;
			}

			// UNDO
			if ((distance = curwp->w_dotpos - pos) > MAXINT) {
				u_clear(curbp);
			} else if (!u_entry_range(UFL_INSERT, lforw(prevline), offset, distance, pos)) {
				return FALSE;
			}
			//
		}
	}

	/* Undo the newline when multiple lines */
	if (newline < len) {
		if (ldelnewline_noundo() == FALSE) return FALSE;
	}

	return TRUE;
}


/*
 * Mg3a: As linsert_general_string, but add overwrite mode
 *
 * Not very optimized. Only used when replaying selfinserting
 * characters when replaying a macro in overwrite mode.
 */

INT
linsert_overwrite_general_string(charset_t charset, INT n, char *str, INT len)
{
	INT c, i;

	if (!(curbp->b_flag & BFOVERWRITE)) {
		return linsert_general_string(charset, n, str, len);
	}

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0 || len < 0) return FALSE;

	for (; n > 0; n--) {
		if (curwp->w_doto == llength(curwp->w_dotp)) {
			return linsert_general_string(charset, n, str, len);
		}

		for (i = 0; i < len; ) {
			c = ucs_strtochar(charset, str, len, i, &i);
			if (linsert_overwrite(curbp->charset, 1, c) == FALSE) return FALSE;
		}
	}

	return TRUE;
}


/*
 * Mg3a: Trim a line to a length. We do it manually because the
 * ldelete* functions are geared towards working in the current
 * window. They can't work on a line in an arbitrary buffer.
 * reduceoverhead() also can't work on a line in an arbitrary buffer.
 */

void
ltrim(BUFFER *bp, LINE *lp, INT len, size_t pos)
{
	WINDOW *wp;
	INT dlen = llength(lp) - len;		// Length deleted
	int wflag = (bp->b_flag & BFCHG) ? WFHARD : (WFHARD|WFMODE);

	if (len < 0 || len >= llength(lp)) return;

	if (gmarkp == lp && gmarko > len) gmarko = len;

	if (bp->b_nwnd == 0) {
		if (bp->b_dotp == lp && bp->b_doto > len) bp->b_doto = len;
		if (bp->b_markp == lp && bp->b_marko > len) bp->b_marko = len;
		if (bp->b_dotpos > pos) {
			if (bp->b_dotpos < pos + dlen) bp->b_dotpos = pos;
			else bp->b_dotpos -= dlen;
		}
	} else {
		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp) {
				if (wp->w_dotp == lp && wp->w_doto > len) wp->w_doto = len;
				if (wp->w_markp == lp && wp->w_marko > len) wp->w_marko = len;
				if (wp->w_dotpos > pos) {
					if (wp->w_dotpos < pos + dlen) wp->w_dotpos = pos;
					else wp->w_dotpos -= dlen;
				}

				wp->w_flag |= wflag;
			}
		}
	}

	bp->b_size -= dlen;
	bp->b_flag |= BFCHG;
	lsetlength(lp, len);
	lp->l_flag = 0;
}


/*
 * Mg3a: A number of operations need to be done without undo.
 */

// UNDO
INT
linsert_str_noundo(char *str, INT len)
{
	INT ret, save_undo = undo_enabled;

	undo_enabled = 0;
	if ((ret = linsert_str(1, str, len)) != TRUE) u_clear(curbp);
	undo_enabled = save_undo;

	return ret;
}

INT
lnewline_noundo()
{
	INT save_undo = undo_enabled;
	INT ret;

	undo_enabled = 0;
	ret = lnewline();
	undo_enabled = save_undo;
	return ret;
}


INT
ldelnewline_noundo()
{
	INT save_undo = undo_enabled;
	INT ret;

	undo_enabled = 0;
	ret = ldelnewline();
	undo_enabled = save_undo;
	return ret;
}


INT
normalizebuffer_noundo(BUFFER *bp)
{
	INT s, save_undo = undo_enabled;

	undo_enabled = 0;
	s = normalizebuffer(bp);
	undo_enabled = save_undo;
	return s;
}
//
