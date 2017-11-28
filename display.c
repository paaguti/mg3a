/*
 * The functions in this file handle redisplay.
 *
 * Mg3a: There is no double buffering anymore. There is a line cache.
 */

#include	"def.h"
#include	"kbd.h"

#ifdef UTF8
#include "cpall.h"
#endif

INT	sgarbf	= TRUE;			/* TRUE if screen is garbage.	*/
INT	tthue	= CNONE;		/* Current color.		*/
INT	ttrow	= HUGE;			/* Physical cursor row.		*/
INT	ttcol	= HUGE;			/* Physical cursor column.	*/


/*
 * Some predeclarations to make ANSI compilers happy
 */

static void 	modeline(WINDOW *wp);
static void 	ttputs(char *str);


/*
 * Mg3a: Set up the terminal's I/O-channel and init the terminal
 */

void
vtinit()
{
	ttopen();
	ttinit();
	ttnowindow();				/* No scroll window.	*/
	/* Screen is indicated as garbage to begin with so update()	*/
	/* will clear.							*/
}


/*
 * Mg3a: Prepare to leave and reset the terminal's I/O channel.
 */

void
vttidy()
{
	ttnowindow();				/* No scroll window.	*/
	ttmove(nrow - 1, 0);			/* Echo line.		*/
	ttcolor(CTEXT);
	tteeol();
	ttmove(nrow - 1, 0);			/* Echo line again, in case dumb terminal */
	tttidy();
	ttflush();
	ttclose();
}


/*
 * Mg3a: Compute the column from the offset (both zero-based)
 */

INT
getcolumn(WINDOW *wp, LINE *lp, INT offset)
{
	INT i, col, len;
	INT c;
	charset_t charset;

	len = llength(lp);
	charset = wp->w_bufp->charset;

	col = 0;
	i = 0;

	if (offset > len) offset = len;

	while (i < offset) {
		if (col > MAXINT - 8) {
			column_overflow();
			return 0;
		}

		c = lgetc(lp, i);

		if (c >= 32 && c < 127) {
			i++;
			col++;
		} else if (c == '\t') {
			i++;
			col = (col | 7) + 1;
		} else {
			c = ucs_char(charset, lp, i, &i);
			col += ucs_termwidth(c);
		}
	}

	return col;
}


/*
 * Mg3a: Compute the offset from the column (both zero-based).
 * Overshoot to the left.
 */

INT
getoffset(WINDOW *wp, LINE *lp, INT column)
{
	INT i, col, len;
	INT c;
	charset_t charset;

	len = llength(lp);
	charset = wp->w_bufp->charset;

	col = 0;
	i = 0;

	while (i < len) {
		c = lgetc(lp, i);

		if (c >= 32 && c < 127) {
			i++;
			col++;
		} else if (c == '\t') {
			i++;
			col = (col | 7) + 1;
		} else {
			c = ucs_char(charset, lp, i, &i);
			col += ucs_termwidth(c);
		}

		if (col > column) return ucs_backward(charset, lp, i);
	}

	return i;
}


/*
 * Mg3a: Lo-tech equivalents for a full terminal.
 * Not called often.
 */

INT
getfullcolumn(WINDOW *wp, LINE *lp, INT offset)
{
	INT savefull = fullterm;
	INT result;

	fullterm = 1;

	result = getcolumn(wp, lp, offset);

	fullterm = savefull;
	return result;
}

INT
getfulloffset(WINDOW *wp, LINE *lp, INT column)
{
	INT savefull = fullterm;
	INT result;

	fullterm = 1;

	result = getoffset(wp, lp, column);

	fullterm = savefull;
	return result;
}


/*
 * Mg3a: Compute the start offset and column from the start column
 * (all zero-based) for a shifted line. May return a position in the
 * middle of a combined character if the terminal does not combine
 * characters. Overshoots to the right out of necessity.
 */

static void
getshifted(charset_t charset, LINE *lp, INT incolumn, INT *outoffset, INT *outcolumn)
{
	INT i, col, nexti, len;
	INT c;

	len = llength(lp);
	col = 0;
	i = 0;

	while (i < len && col < incolumn) {
		c = lgetc(lp, i);

		if (c >= 32 && c < 127) {
			i++;
			col++;
		} else if (c == '\t') {
			i++;
			col = (col | 7) + 1;
		} else {
			c = ucs_char(charset, lp, i, &i);
			col += ucs_termwidth(c);
		}
	}

	/* Consume remaining zero-width on screen */
	while (i < len && ucs_termwidth(ucs_char(charset, lp, i, &nexti)) == 0) {
		i = nexti;
	}

	*outoffset = i;
	*outcolumn = col;
}


/*
 * The line cache. Tag:cache
 */

typedef struct {
	char *line;
	INT size;
	INT used;
	INT shift;
	charset_t charset;
} cacheline;

static cacheline *cache = NULL;
static INT cachesize=0;

#if TESTCMD == 3
INT
getcachesize()
{
	LINE *lp;
	INT i;

	lp = curwp->w_linep;

	for (i = curwp->w_toprow; i < curwp->w_toprow + curwp->w_ntrows; i++) {
		if (lp == curwp->w_dotp && cachesize) {
			return cache[i].size;
		}

		lp = lforw(lp);
	}

	return -1;
}
#endif


/*
 * Mg3a: Check that the line cache is allocated and is the right size
 */

static void
checkcache()
{
	INT i;

	if (cachesize == nrow) return;

	if (cache) {
		for (i = 0; i < cachesize; i++) {
			if (cache[i].line) free(cache[i].line);
		}

		free(cache);
	}

	if ((cache = malloc(nrow * sizeof(cacheline))) == NULL) {
		ewprintf("Couldn't allocate screen cache");
		cachesize = 0;
		return;
	}

	cachesize = nrow;

	for (i = 0; i < cachesize; i++) {
		cache[i].line = NULL;
		cache[i].size = 0;
		cache[i].used = -1;
		cache[i].shift = 0;
	}
}


/*
 * Mg3a: Invalidate one cache line
 */

static void
cacheinvalidateline(INT row)
{
	if (cachesize) cache[row].used = -1;
}


/*
 * Mg3a: Invalidate all cache lines
 */

void
invalidatecache()
{
	INT i;

	for (i = 0; i < cachesize; i++) cache[i].used = -1;
}


/*
 * Mg3a: Set all cache lines to empty
 */

static void
emptyallcachelines()
{
	INT i;

	for (i = 0; i < cachesize; i++) {
		if (cache[i].line) free(cache[i].line);
		cache[i].line = NULL;
		cache[i].size = 0;
		cache[i].used = 0;
		cache[i].shift = 0;
	}
}


/*
 * Mg3a: set line cache for a row, saving the shift
 */

#define LCBLOCK 128

static void
setcache(charset_t charset, LINE *lp, INT len, INT row, INT shift)
{
	if (!cachesize) return;

	if (len > 10000) {
		cache[row].used = -1;
		return;
	}

	// Store a bit extra so we can recognize truncated lines

	if (len < llength(lp)) len = ucs_forward(charset, lp, len);

	if (len > cache[row].size || len < (cache[row].size >> 1) - LCBLOCK) {
		if (cache[row].line) free(cache[row].line);

		cache[row].size = (len + LCBLOCK) & ~(LCBLOCK-1);

		if ((cache[row].line = malloc(cache[row].size)) == NULL) {
			cache[row].size = 0;
			cache[row].used = -1;
			ewprintf("Error allocating line cache");
			return;
		}
	}

	if (len) memcpy(cache[row].line, ltext(lp), len);
	cache[row].used = len;
	cache[row].shift = shift;
	cache[row].charset = charset;
}


/*
 * Whether to try to preserve ligatures. Default off because of
 * cursor flashing in Windows 7 console.
 */

INT preserve_ligatures = 0;


/*
 * Mg3a: Find where to start updating a line by comparing with the
 * cache. Care is taken to not split ligatures and base+combining
 * characters. Return 1 if the line needs updating, 0 otherwise.
 */

static int
matchcache(WINDOW *wp, LINE *lp, INT row, INT shift, INT *outoffset, INT *outcolumn)
{
	INT	i, nexti, j, nextj;
	INT	len1, len2;
	INT	c, c1, c2;
	INT	blank1 = 0, blank2 = 0, col = 0;
	charset_t charset = wp->w_bufp->charset;

	if (cachesize == 0 || cache[row].used < 0 || shift != cache[row].shift ||
		(cache[row].used > 0 && charset != cache[row].charset))
	{
		/* Full update needed */
		*outoffset = 0;
		*outcolumn = 0;
		return 1;
	}

	i = 0;
	j = 0;
	len1 = llength(lp);
	len2 = cache[row].used;
	c1 = 0;
	c2 = 0;

	while (i < len1 || j < len2) {
		// Deal with comparing blanks and tabs.

		// Getting an error character when out-of-bounds is relied on.

		if (blank1) {
			c1 = ' ';
			blank1--;
		} else {
			c1 = ucs_char(charset, lp, i, &nexti);

			if (c1 == '\t') {
				blank1 = (col | 7) - col;
				c1 = ' ';
			}
		}

		if (blank2) {
			c2 = ' ';
			blank2--;
		} else {
			c2 = ucs_strtochar(charset, cache[row].line, len2, j, &nextj);

			if (c2 == '\t') {
				blank2 = (col | 7) - col;
				c2 = ' ';
			}
		}

		if (c1 != c2) break;

		if (col > MAXINT - 2) {
			column_overflow();
			return 0;
		}

		col += ucs_termwidth(c1);

		if (!blank1) i = nexti;
		if (!blank2) j = nextj;
	}

#if 1
	// Anti-cursor flashing and more efficient update

	if (j == len2) {
		while (i < len1) {
			INT newcol;

			if (col > MAXINT - 8) break;

			c1 = ucs_char(charset, lp, i, &nexti);

			if (c1 == '\t') {
				newcol = (col | 7) + 1;
			} else if (c1 == ' ') {
				newcol = col + 1;
			} else {
				break;
			}

			if (newcol - shift > ncol) break;

			col = newcol;
			i = nexti;
		}
	}
#endif

	if (i == len1 && j == len2) {
		/* Line is identical, no update needed */
		return 0;
	} else if (!preserve_ligatures) {
		/* No checking for ligatures */
		if (ucs_nonspacing(c1) || ucs_nonspacing(c2)) {
			/* Non-spacing, backup */
			i = ucs_backward(charset, lp, i);
			col = getcolumn(wp, lp, i);
		}
	} else {
		while (i > 0) {
			i = ucs_backward(charset, lp, i);
			c = ucs_char(charset, lp, i, NULL);

			if (c < 0x300 || (c >= 0x4e00 && c < 0xa000)) break;
		}

		col = getcolumn(wp, lp, i);
	}

	*outoffset = i;
	*outcolumn = col;

	return 1;
}


/*
 * Mg3a: Update a line to a row at the screen while using a line cache.
 * The line may be shifted.
 */

static void
updateline(INT row, LINE *lp, WINDOW *wp, INT shift)
{
	INT i, col, newcol, len;
	INT c;
	charset_t charset;
	INT tempcol, nexti;

	charset = wp->w_bufp->charset;

	if (matchcache(wp, lp, row, shift, &i, &col) == 0) return;
	if (col - shift > ncol) return;

	ttcolor(CTEXT);
	len = llength(lp);

	if (shift && col - shift < 1) {
		// A shifted line, and we need to start from the beginning

		getshifted(charset, lp, shift + 1, &i, &col);

		ttmove(row, 0);
		ttputc('$');

		for (tempcol = 1; tempcol < col - shift; tempcol++) {
			ttputc(' ');
		}
	} else if (col - shift < ncol) {
		// A normal line update

		ttmove(row, col - shift);
	} else if (i == len) {
		// A line that extends to the right edge of the screen,
		// matches the cache line, but the cache line was
		// truncated and this line is not.

		i = ucs_backward(charset, lp, i);
		col = getcolumn(wp, lp, i);
		ttmove(row, col - shift);
	}

	while (i < len) {
		if (col > MAXINT - 8) {
			column_overflow();
			return;
		}

		c = ucs_char(charset, lp, i, &nexti);

		if (c == '\t') {
			newcol = (col | 7) + 1;

			while (col < newcol) {
				col++;
				if (col - shift <= ncol) ttputc(' ');
			}
		} else {
			col += ucs_termwidth(c);
			if (col - shift <= ncol) ucs_put(c);
		}

		if (col - shift > ncol) break;
		i = nexti;
	}

	ttcol = col - shift;

	if (col - shift < ncol) {
		tteeol();
	} else if (col - shift > ncol) {
		ttmove(row, ncol - 1);
		ttputc('$');
		ttcol++;
	}

	setcache(charset, lp, i, row, shift);
}


/*
 * Mg3a: Clear a line, in cache and on screen
 */

static void
emptyline(INT row, WINDOW *wp)
{
	static LINE *empty = NULL;

	if (!empty && (empty = lalloc(0)) == NULL) return;
	updateline(row, empty, wp, 0);
}


#ifdef CHARSDEBUG
extern INT charsoutput;
static INT charsdebug = 0;
static INT totalchars = 0;
extern INT totalkeys;


/*
 * Mg3a: Debug functions. "charsoutput" is the number of bytes output
 * since the last update. "chars" are bytes output and "keys" are
 * bytes input.
 */

INT
charsdebug_set(INT f, INT n) {
	if (!charsdebug) charsdebug = 1;
	else {charsdebug = 0; eerase();}
	return TRUE;
}

static void
charsdebug_display(INT row, INT col)
{
	totalchars += charsoutput;

	ewprintf("charsoutput = %d, column = %d, totalchars = %d, totalkeys = %d, chars/keys = %d",
		charsoutput, getcolpos(), totalchars, totalkeys,
		totalkeys ? totalchars/totalkeys : (INT)0);

	ttmove(row, col);
	charsoutput = 0;
}

INT
charsdebug_zero(INT f, INT n)
{
	if (!charsdebug) ewprintf("charsdebug zeroed");
	totalchars = 0;
	totalkeys = 0;
	charsoutput = 0;
	return TRUE;
}
#endif

INT scrollbyone = 0;		/* Scrolling one line at a time */


/*
 * Mg3a: reframing a window is sometimes needed separately
 */

void
reframe_window(WINDOW *wp)
{
	INT i;
	LINE *lp;

	if (wp->w_flag == 0) return;

	if ((wp->w_flag & WFFORCE) == 0) {
		lp = wp->w_linep;
		for (i = 0; i < wp->w_ntrows; i++) {
			if (lp == wp->w_dotp)
				goto out;
			if (lp == wp->w_bufp->b_linep)
				break;
			lp = lforw(lp);
		}

		if (scrollbyone) {
			if (lp == wp->w_dotp) {
				wp->w_linep = lforw(wp->w_linep);
				wp->w_flag |= WFHARD;
				goto out;
			} else if (lback(wp->w_linep) == wp->w_dotp) {
				wp->w_linep = lback(wp->w_linep);
				wp->w_flag |= WFHARD;
				goto out;
			}
		}
	}
	i = wp->w_force;	/* Reframe this one.	*/
	if (i > 0) {
		--i;
		if (i >= wp->w_ntrows)
			i = wp->w_ntrows-1;
	} else if (i < 0) {
		i += wp->w_ntrows;
		if (i < 0)
			i = 0;
	} else
		i = wp->w_ntrows/2;
	lp = wp->w_dotp;
	while (i != 0 && lback(lp) != wp->w_bufp->b_linep) {
		--i;
		lp = lback(lp);
	}
	wp->w_linep = lp;
	wp->w_flag |= WFHARD;	/* Force full.		*/
out:
	wp->w_force = 0;
}


/*
 * Make sure that the display is right. This is a three part process.
 * First, scan through all of the windows looking for dirty ones.
 * Check the framing, and refresh the screen. Second, make sure that
 * "currow" and "curcol" are correct for the current window.
 *
 * Mg3a: Does the same thing as in Mg2a, but is rewritten a bit.
 */

#define SHIFTED 1
void
update()
{
	LINE		*lp;
	WINDOW		*wp;
	INT		i;
	INT		currow;
	INT		curcol;
#if SHIFTED
	INT		shift = 0;
	INT		curwp_flag;
	static INT	shift_row = -1;
	static INT	prev_shift = 0;
#endif
	if (typeahead()) return;

	checkcache();

	if (sgarbf) {				/* must update everything */
		ttmove(0, 0);
		ttcolor(CTEXT);
		tteeop();
		emptyallcachelines();
		refreshbuf(NULL);
	}
#if SHIFTED
	curwp_flag = curwp->w_flag;
#endif
	wp = wheadp;
	while (wp != NULL) {
		if (wp->w_flag != 0) {		/* Need update.		*/
			reframe_window(wp);

			lp = wp->w_linep;	/* Try reduced update.	*/
			i  = wp->w_toprow;
			if ((wp->w_flag & ~WFMODE) == WFEDIT) {
				while (i < wp->w_toprow + wp->w_ntrows) {
#if SHIFTED
					if (lp != wp->w_bufp->b_linep) {
						if (lp == curwp->w_dotp && i != shift_row) {
							updateline(i, lp, wp, 0);
						}

						lp = lforw(lp);
					}
#else
					if (lp != wp->w_bufp->b_linep) {
						if (lp == curwp->w_dotp) {
							updateline(i, lp, wp, 0);
						}

						lp = lforw(lp);
					}
#endif
					++i;
				}
			} else if (wp->w_flag & (WFEDIT|WFHARD)) {
				while (i < wp->w_toprow + wp->w_ntrows) {
					if (lp != wp->w_bufp->b_linep) {
						updateline(i, lp, wp, 0);
						lp = lforw(lp);
					} else if (!sgarbf) {
						emptyline(i, wp);
					}
					++i;
				}
			}
			if (wp->w_flag & WFMODE)
				modeline(wp);
			wp->w_flag  = 0;
		}
		wp = wp->w_wndp;
	}

#if SHIFTED
	wp = wheadp;
	while (wp != NULL && shift_row >= 0) {	/* Clear the shifted line if not the current one */
		if (wp->w_toprow <= shift_row && shift_row < wp->w_toprow + wp->w_ntrows) {
			lp = wp->w_linep;
			i = wp->w_toprow;
			while (i <= shift_row && lp != wp->w_bufp->b_linep) {
				if (i == shift_row) {
					if (wp != curwp || lp != curwp->w_dotp) {
						/* Back to unshifted */
						updateline(i, lp, wp, 0);
						shift_row = -1;
					}
					break;
				}
				lp = lforw(lp);
				i++;
			}
			break;
		}
		wp = wp->w_wndp;
	}

#endif
	lp = curwp->w_linep;			/* Cursor location.	*/
	currow = curwp->w_toprow;
	while (lp != curwp->w_dotp) {
		++currow;
		lp = lforw(lp);
	}
#if SHIFTED
	/* Tag:shifting */

	curcol = getcolumn(curwp, curwp->w_dotp, curwp->w_doto);
	shift = 0;

	if (curcol > ncol - 2) {
		shift = curcol - curcol % (ncol >> 1) - (ncol >> 2);
		curcol = curcol - shift;
		if (curwp_flag || shift != prev_shift || shift_row != currow) {
			updateline(currow, curwp->w_dotp, curwp, shift);
		}
		shift_row = currow;
	} else if (shift_row >= 0) {
		/* The current line went to unshifted */
		updateline(currow, curwp->w_dotp, curwp, 0);
		shift_row = -1;
	}

	prev_shift = shift;
#else
	curcol = getcolumn(curwp, curwp->w_dotp, curwp->w_doto);
	if (curcol > ncol - 1) curcol = ncol - 1;
#endif

	if (sgarbf != FALSE) {			/* Screen is garbage.	*/
		sgarbf = FALSE;			/* Erase-page clears	*/
		epresf = FALSE;			/* the message area.	*/
	}

	ttmove(currow, curcol);

#ifdef CHARSDEBUG
	if (charsdebug) charsdebug_display(currow, curcol);
#endif
	ttflush();
}


/*
 * Mg3a: Return short form of character set name for modeline
 */

static char *
charsetshortname(char *str)
{
	INT c;
	char *p;
	static char buf[80];

	p = buf;

	while ((c = *str++)) {
		if (c == '-') continue;
		else if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
		*p++ = c;
	}

	*p = 0;

	return buf;
}


INT modeline_show = 0;		/* Modeline control */


/*
 * Mg3a: Mode string for the modeline
 */

#define MODESLEN (256+LOCALMODELEN)

static char *
modestring(BUFFER *bp)
{
	INT mode;
	static char result[MODESLEN];

	stringcopy(result, mode_name_check(bp->b_modes[0]), MODESLEN);

	if (bp->b_flag & BFUNIXLF) {
		if (!(modeline_show & MLNOLF)) stringconcat(result, "-lf", MODESLEN);
	} else {
		if (modeline_show & MLCRLF) stringconcat(result, "-crlf", MODESLEN);
	}

	if (bp->charset && (
			(bp->b_flag & BFMODELINECHARSET) ||
			(bp->charset != buf8bit) ||
			(modeline_show & MLCHARSET))) {

		if ((modeline_show & MLNODEF) == 0 || bp->charset != bufdefaultcharset) {
			stringconcat2(result, "-",
				charsetshortname(charsettoname(bp->charset)), MODESLEN);
		}
	}

	if (bp->b_flag & BFOVERWRITE) stringconcat(result, "-overwrite", MODESLEN);

	if (bp->b_flag & BFBOM) stringconcat(result, "-bom", MODESLEN);

#ifdef SLOW
	{
		extern INT slowspeed;

		if (slowspeed > 0) stringconcat(result, "-slow", MODESLEN);
	}
#endif
	for (mode = 1; mode <= bp->b_nmodes; mode++) {
		stringconcat2(result, "-", mode_name_check(bp->b_modes[mode]), MODESLEN);
	}

	if (bp->localmodename[0]) stringconcat2(result, "-", bp->localmodename, MODESLEN);

	return result;
}


/*
 * Redisplay the mode line for the window pointed to by the "wp". This
 * is the only routine that has any idea of how the modeline is
 * formatted. You can change the modeline format by hacking at this
 * routine. Called by "update" any time there is a dirty window.
 *
 * Mg3a: Updated with new features.
 */

static void
modeline(WINDOW *wp)
{
	BUFFER *bp;
	INT row;
	INT maxcol;
	char *bufmodes;
	INT c, i, len, w, bufmodeslen;

	row = wp->w_toprow + wp->w_ntrows;

	ttmove(row, 0);
	cacheinvalidateline(row);

	bp = wp->w_bufp;

	ttcolor(CMODE);

	ttputs("--");

	if (bp->b_flag & BFREADONLY) {
		if (bp->b_flag & BFCHG) {
			ttputs("%*");
		} else {
			ttputs("%%");
		}
	} else if (bp->b_flag & BFCHG) {	/* "*" if changed.	*/
		ttputs("**");
	} else {
		ttputs("--");
	}

	ttputs("-Mg: ");

	bufmodes = modestring(bp);
	bufmodeslen = ucs_termwidth_str(termcharset, bufmodes, strlen(bufmodes));

	maxcol = ncol - bufmodeslen - 4;
	if (maxcol < ttcol + 14) maxcol = ttcol + 14;
	if (maxcol > ncol) maxcol = ncol;

	len = strlen(bp->b_bname);

	for (i = 0; i < len;) {
		c = ucs_strtochar(termcharset, bp->b_bname, len, i, &i);
		w = ucs_termwidth(c);

		if (ttcol + w > maxcol) {
			ttputs("$");
			if (ttcol == maxcol) ttputs(" ");	// When double-wide
			break;
		}

		ttcol += w;
		ucs_put(c);
	}

	ttputs(" ");

	while (ttcol < (ncol <= 80 ? 35 : 42) && ttcol < ncol - bufmodeslen - 2) {
		ttputc(' ');			/* Pad out with blanks */
		ttcol++;
	}

	ttputs("(");

	ttputs(bufmodes);

	ttputs(")");

	while (ttcol < ncol) {			/* Pad out.		*/
		ttputc('-');
		ttcol++;
	}

	ttcolor(CTEXT);
}


/*
 * Mg3a: Output a string of characters for the modeline in the charset
 * termcharset while limiting output to the screen width.
 */

static void
ttputs(char *s)
{
	INT i, len;
	INT c, w;

	i = 0;
	len = strlen(s);

	while (i < len) {
		c = ucs_strtochar(termcharset, s, len, i, &i);
		w = ucs_termwidth(c);

		if (ttcol + w <= ncol) {
			ucs_put(c);
		} else if (ttcol + 1 == ncol) {
			ttputc(' ');		// Double-wide does not fit, fill with space
		} else {
			break;
		}

		ttcol += w;
	}
}
