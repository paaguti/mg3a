/*
 * Code for dealing with paragraphs and filling. Adapted from MicroEMACS 3.6
 * and GNU-ified by mwm@ucbvax.	 Several bug fixes by blarson@usc-oberon.
 */
#include "def.h"

INT	fillcol = 70;

static INT	skipblanks(INT offset);
static int	para_sep(LINE *lp);

INT	gotoeop(INT f, INT n);
INT	backchar(INT f, INT n);
INT	forwchar(INT f, INT n);
INT	selfinsert(INT f, INT n);
INT	delwhite(INT f, INT n);


/*
 * Go back to the line before the beginning of a paragraph
 */

INT
gotobop(INT f, INT n)
{
	LINE *lp, *blp = curbp->b_linep;
	INT ret = TRUE;

	if (n < 0) return gotoeop(f, -n);
	if (n == 0) return TRUE;

	lp = curwp->w_dotp;

	for (;n > 0; n--) {
		while (lp != blp && para_sep(lp)) lp = lback(lp);
		while (lp != blp && !para_sep(lp)) lp = lback(lp);

		if (para_sep(lforw(lp))) ret = FALSE;

		if (lp == blp) {
			lp = lforw(blp);
			break;
		}
	}

	adjustpos(lp, 0);

	return ret;
}


/*
 * Go forward to the line after the end of a paragraph
 */

INT
gotoeop(INT f, INT n)
{
	LINE *lp, *blp = curbp->b_linep;
	INT ret = TRUE;

	if (n < 0) return gotobop(f, -n);
	if (n == 0) return TRUE;

	lp = curwp->w_dotp;

	for (; n > 0; n--) {
		while (lp != blp && para_sep(lp)) lp = lforw(lp);
		while (lp != blp && !para_sep(lp)) lp = lforw(lp);

		if (para_sep(lback(lp))) ret = FALSE;

		if (lp == blp) {
			if (normalizebuffer(curbp) == FALSE) return FALSE;
			lp = lback(blp);
			break;
		}
	}

	adjustpos(lp, 0);

	return ret;
}


/* Kill n paragraphs */

INT
killpara(INT f, INT n)
{
	LINE *markp;
	INT marko;
	REGION r;
	INT s;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n == 0) return TRUE;

	if (curwp->w_dotp == curbp->b_linep) {
		if (normalizebuffer(curbp) == FALSE) return FALSE;
	}

	markp = curwp->w_dotp;
	marko = curwp->w_doto;

	gotoeop(FFRAND, n);

	if ((s = getregion_mark(&r, markp, marko)) != TRUE) return s;

	initkill();

	return ldeleteregion(&r, KKILL);
}


/*
 * Mg3a: Mark n paragraphs, as in Emacs. No inverse video for the
 * region, so an informative message.
 */

INT
markpara(INT f, INT n)
{
	char *where = "after";

	if (n == 0) {
		ewprintf("Cannot mark zero paragraphs");
		return FALSE;
	}

	gotoeop(FFRAND, n);

	isetmark();

	gotobop(FFRAND, n);

	if (n < 0) {
		n = -n;
		where = "before";
	}

	ewprintf("Mark set; %d paragraph%s in region %s point", n, n == 1 ? "" : "s", where);
	return TRUE;
}


/*
 * Mg3a: Transpose paragraphs. From Emacs.
 */

INT
twiddlepara(INT f, INT n)
{
	LINE *dotp, *next;
	INT doto;
	LINE *lp1, *lp2, *lp3, *blp = curbp->b_linep, *lp4, *lp;
	size_t undo_len, pos1, pos3, pos4;
	WINDOW *wp;

	if (curbp->b_flag & BFREADONLY) return readonly();

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;

	if (gotobop(FFRAND, 1) == FALSE) goto error;
	lp1 = curwp->w_dotp;
	if (para_sep(lp1)) lp1 = lforw(lp1);
	adjustpos(lp1, 0);
	pos1 = curwp->w_dotpos;			// Start of first paragraph

	if (gotoeop(FFRAND, 1) == FALSE) goto error;
	lp2 = curwp->w_dotp;			// Start of interparagraph space

	for (lp3 = lp2; lp3 != blp && para_sep(lp3); lp3 = lforw(lp3)) {}
	if (lp3 == blp) goto error;
	adjustpos(lp3, 0);
	pos3 = curwp->w_dotpos;			// Start of second paragraph

	if (gotoeop(FFRAND, 1) == FALSE) goto error;
	lp4 = curwp->w_dotp;
	pos4 = curwp->w_dotpos;			// After second paragraph

	// UNDO
	if ((undo_len = pos3 - pos1) > MAXINT) {
		u_clear(curbp);
		undo_len = 0;
	}

	u_modify();
	if (!u_entry_range(UFL_DELETE, lp1, 0, undo_len, pos1)) return FALSE;
	//

	lchange(WFHARD);

	// Move the dots out of the way so that we don't have to
	// readjust them. Make both paragraphs visible if possible.

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			for (lp = lp1; lp != lp4; lp = lforw(lp)) {
				if (wp->w_dotp == lp) {
					wp->w_dotp = curwp->w_dotp;
					wp->w_doto = curwp->w_doto;
					wp->w_dotpos = curwp->w_dotpos;
				}
				if (wp->w_linep == lp) wp->w_linep = lp3;
			}
		}
	}

	for (lp = lp2; lp != lp3;) {
		next = lforw(lp);
		removefromlist(lp);
		insertbefore(lp4, lp);
		lp = next;
	}

	for (lp = lp1; lp != lp3;) {
		next = lforw(lp);
		removefromlist(lp);
		insertbefore(lp4, lp);
		lp = next;
	}

	// UNDO
	if (!u_entry_range(UFL_INSERT, lp2, 0, undo_len, pos1 + pos4 - pos3)) return FALSE;
	//

	return TRUE;

error:
	adjustpos(dotp, doto);
	ewprintf("Cannot transpose paragraphs");
	return FALSE;
}


/*
 * Check to see if we're past fillcol, and if so,
 * justify this line. As a last step, justify the line.
 */

INT
fillword(INT f, INT n)
{
	char	c;
	INT	i, nce;
	INT	local_fillcol;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0) return FALSE;
	if (n == 0) return TRUE;

	local_fillcol = get_variable(curbp->localvar.v.fillcol, fillcol);

	if (getfullcolumn(curwp, curwp->w_dotp, curwp->w_doto) <= local_fillcol) {
		return selfinsert(f|FFRAND, n);
	}

	if (curwp->w_doto != llength(curwp->w_dotp)) {
		selfinsert(f|FFRAND, n);
		nce = llength(curwp->w_dotp) - curwp->w_doto;
	} else nce = 0;

	adjustpos(curwp->w_dotp, getfulloffset(curwp, curwp->w_dotp, local_fillcol));

	if ((c = lgetc(curwp->w_dotp, curwp->w_doto)) != ' ' && c != '\t')
		do {
			backchar(FFRAND, 1);
		} while ((c = lgetc(curwp->w_dotp, curwp->w_doto)) != ' '
		      && c != '\t' && curwp->w_doto > 0);

	if (curwp->w_doto == 0)
		do {
			forwchar(FFRAND, 1);
		} while ((c = lgetc(curwp->w_dotp, curwp->w_doto)) != ' '
		      && c != '\t' && curwp->w_doto < llength(curwp->w_dotp));

	delwhite(FFRAND, 1);
	if (lnewline() == FALSE) return FALSE;
	i = llength(curwp->w_dotp) - nce;
	adjustpos(curwp->w_dotp, i>0 ? i : 0);
	if (nce == 0 && curwp->w_doto != 0) return fillword(f, n);
	return TRUE;
}

/* Set fill column to n. */

INT
setfillcol(INT f, INT n)
{
	curbp->localvar.v.fillcol = ((f & FFARG) ? n : getcolpos());
	ewprintf("Fill column set to %d", curbp->localvar.v.fillcol);
	return TRUE;
}


/*
 * Mg3a: New paragraph filling
 */

static int
is_blank(INT c)
{
	return c == ' ' || c == '\t';
}


/*
 * Mg3a: Skip blanks in current line
 */

static INT
skipblanks(INT offset)
{
	LINE *lp = curwp->w_dotp;
	INT len = llength(lp);

	while (offset < len && is_blank(lgetc(lp, offset))) offset++;

	return offset;
}

/*
 * Mg3a: Change any number of whitespace characters around the cursor
 * to a number of spaces. The cursor can start anywhere in the
 * whitespace and will be left after it.
 */

static int
setblanks(INT blanks)
{
	LINE *lp = curwp->w_dotp;
	INT orig = curwp->w_doto, c, start, end, len;
	int hastab = 0;

	for (start = orig; start > 0; start--) {
		c = lgetc(lp, start - 1);

		if (!is_blank(c)) break;
		if (c != ' ') hastab = 1;
	}

	len = llength(lp);

	for (end = orig; end < len; end++) {
		c = lgetc(lp, end);

		if (!is_blank(c)) break;
		if (c != ' ') hastab = 1;
	}

	adjustpos(curwp->w_dotp, end);

	if (hastab) {
		return ldeleterawback(end - start, KNONE) == TRUE && linsert(blanks, ' ') == TRUE;
	}

	if (end - start == blanks) return 1;
	if (end - start < blanks) return linsert(blanks - (end - start), ' ') == TRUE;
	return ldeleterawback(end - start - blanks, KNONE) == TRUE;
}


/*
 * Mg3a: return true if the line is a paragraph separator.
 */

static int
para_sep_full(LINE *lp, INT fromstartcol, INT startcol)
{
	INT i, len, startidx = 0;

	if (lp == curbp->b_linep) return 1;

	len = llength(lp);

	if (fromstartcol) {
		startidx = getfulloffset(curwp, lp, startcol);
	}

	for (i = startidx; i < len; i++) {
		if (!is_blank(lgetc(lp, i))) return 0;
	}

//	ewprintf("end, startidx = %d", startidx);
	return 1;
}

/*
 * Mg3a: para_sep, simple version
 */

static int
para_sep(LINE *lp)
{
	return para_sep_full(lp, 0, 0);
}


/*
 * Mg3a: test for some sentence-ending delimiters.
 */

static int
endsentence(INT c)
{
	return c == '.' || c == '!' || c == '?';
}


/*
 * Mg3a: Word character for first word of adaptive fill.
 */

static int
adaptive_word_char(INT c, INT numbered)
{
	static const ucs2 adapt_extra[] = {'"', '\'', 0x00ab, 0x00bb, 0x2039, 0x203a, 0};
	const ucs2 *p;

	if (numbered ? ucs_isalpha(c) : ucs_isalnum(c)) return 1;

	if (c >= 0x2018 && c <= 0x201f) return 1;

	for (p = adapt_extra; *p; p++) {
		if (c == *p) return 1;
	}

	return 0;
}


INT fill_options = 0;		/* Options for fill-paragraph */

/*
 * Mg3a: The new paragraph filler.
 */

#define FILL_SINGLESPACE	1	// Use single spaces
#define FILL_ADAPTIVE		2	// Adaptive filling
#define FILL_HERE		4	// Fill to here. Start of first word is at cursor.
#define	FILL_REPLICATE	        8	// Replicate the indent of the first line
#define FILL_HYPHEN	       16	// Break and re-assemble hyphens
#define FILL_REASSEMBLE	       32	// Always reassemble hyphenated words
#define FILL_NUMBERED	       64	// Adaptive filling, numbered list

INT
fillpara(INT f, INT n)
{
	LINE *lp, *blp;
	INT indent, nexti, len, col, c, indentcol, wordlastc;
	INT startw, endw, nextw, wordnextlastc, wordsep, firstwcol;
	INT startwcol, breakidx, breakcol, firstword;
	LINE *replicate_lp = NULL;
	INT replicate_len = 0, eraselen, temp;
	size_t lineno;
	charset_t charset = curbp->charset;
	INT opt, local_fillcol;

	if (curbp->b_flag & BFREADONLY) return readonly();

	opt = get_variable(curbp->localvar.v.fill_options, fill_options);
	local_fillcol = get_variable(curbp->localvar.v.fillcol, fillcol);

	if (f & FFARG) opt ^= n;
	if (opt & FILL_REPLICATE) opt |= FILL_HERE;
	if (opt & FILL_NUMBERED) opt |= FILL_ADAPTIVE;

	lp = curwp->w_dotp;
	blp = curbp->b_linep;

	// Save our position in a global mark

	gmarkp = curwp->w_dotp;
	gmarko = curwp->w_doto;

	// Find the first line of the paragraph

	if (opt & FILL_HERE) {}
	else if (para_sep(lp)) {
		while (para_sep(lp)) {
			if (lp == blp) return FALSE;
			lp = lforw(lp);
		}
		adjustpos(lp, 0);
	} else {
		while (!para_sep(lback(lp))) {
			lp = lback(lp);
		}
		adjustpos(lp, 0);
	}

	len = llength(lp);

	// Determine where on the first line the paragraph starts,
	// and set up the indent column.

	if (opt & FILL_HERE) {
		// Manual fill. The paragraph starts exactly at the current dot.

		indent = curwp->w_doto;
		nextw = skipblanks(indent);
		if (nextw == len) {
			ewprintf("No words after cursor");
			return FALSE;
		}
		if (nextw > indent && ldeleteraw(nextw - indent, KNONE) == FALSE) return FALSE;

		startw = indent;
		indentcol = getfullcolumn(curwp, curwp->w_dotp, indent);	// "lp" may be invalid
		col = indentcol;
	} else if (opt & FILL_ADAPTIVE) {
		// The more sophisticated adaptive fill.

		firstword = 0;

		for (indent = 0; indent < len; ) {
			c = ucs_char(charset, lp, indent, &nexti);
			if (adaptive_word_char(c, opt & FILL_NUMBERED)) break;
			if (is_blank(c)) firstword = nexti;
			indent = nexti;
		}

		if (indent == len) {
			firstword = skipblanks(0);
		}

		startw = firstword;
		indentcol = getfullcolumn(curwp, lp, firstword);
		col = indentcol;
	} else {
		// The Emacs simplest fill, which keeps the indent of the
		// first line but indents otherwise to column 0.

		startw = skipblanks(0);
		indentcol = 0;
		col = getfullcolumn(curwp, lp, startw);
	}

	firstwcol = col;
	lineno = 1;

	winflag(curbp, WFHARD);

	// Ready, at the first word of the first line of the paragraph

	while (1) {
		// startw shall here be the offset of the beginning of the word
		// col shall here be the column of startw
		// firstwcol shall be the column of the first word on the line
		// lineno shall be the line number
		// curwp->w_dotp shall be the current line
		// and curwp->w_doto is irrelevant and set later

		lp = curwp->w_dotp;

		len = llength(lp);
		startwcol = col;
		wordlastc = ' ';
		wordnextlastc = ' ';
		breakidx = startw;
		breakcol = col;

		// Skip the characters of a word.

		for (endw = startw; endw < len; ) {
			c = ucs_char(charset, lp, endw, &nexti);

			if (is_blank(c) && ((opt & FILL_SINGLESPACE) ||
				!(wordlastc == '.' && c == ' ' &&
				nexti < len && !is_blank(lgetc(lp, nexti)))))
			{
				break;
			}

			col += ucs_fulltermwidth(c);

			if ((opt & FILL_HYPHEN) && c == '-' && ucs_isword(wordlastc)) {
				if (col <= local_fillcol || breakcol == firstwcol) {
					breakidx = nexti;
					breakcol = col;
				}
			}

			wordnextlastc = wordlastc;
			wordlastc = c;
			endw = nexti;
		}

		nextw = skipblanks(endw);

		if ((opt & (FILL_HYPHEN|FILL_REASSEMBLE)) && nextw == len && wordlastc == '-' &&
			ucs_isword(wordnextlastc) &&
			!para_sep_full(lforw(lp), opt & FILL_REPLICATE, indentcol))
		{
			// Reassemble hyphens.

			if (opt & FILL_REPLICATE) {
				adjustpos(lforw(lp), 0);
				eraselen = getfulloffset(curwp, curwp->w_dotp, indentcol);
				if (ldeleteraw(eraselen, KNONE) == FALSE) return FALSE;
			}
			adjustpos(lp, len);
			if (ldelnewline() == FALSE) return FALSE;
			if (!setblanks(0)) return FALSE;
			col = startwcol;
			continue;
		}

		if (col > local_fillcol && breakcol > firstwcol) {
			// Break for a new line.

			// Carry over the blanks before the word so that a mark
			// at the beginning of the word is carried with it.
			while (breakidx > 0 && is_blank(lgetc(lp, breakidx - 1))) breakidx--;
			adjustpos(curwp->w_dotp, breakidx);
			if (opt & FILL_REPLICATE) {
				if (lnewline() == FALSE) return FALSE;
				if (lineno++ == 1) {
					replicate_lp = lback(curwp->w_dotp);
					replicate_len = getfulloffset(curwp, replicate_lp, indentcol);
				}
				if (linsert_str(1, ltext(replicate_lp), replicate_len) == FALSE) {
					return FALSE;
				}
			} else {
				if (lnewline() == FALSE) return FALSE;
				if (tabtocolumn(0, indentcol) == FALSE) return FALSE;
			}
			startw = curwp->w_doto;
			// Erase any carried-over blanks, still keeping a mark
			// at the beginning of the word.
			temp = skipblanks(startw);
			if (temp > startw) {
				if (ldeleteraw(temp - startw, KNONE) == FALSE) return FALSE;
			}
			col = indentcol;
			firstwcol = col;
			continue;
		}

		adjustpos(curwp->w_dotp, nextw);

		if (nextw == len) {
			// At end of line. Join with next line, unless at end of paragraph.

			if (para_sep_full(lforw(lp), opt & FILL_REPLICATE, indentcol)) {
				if (!setblanks(0)) return FALSE;
				adjustpos(gmarkp, gmarko);
				winflag(curbp, WFHARD);	// For debugging
				return TRUE;
			} else {
				if (opt & FILL_REPLICATE) {
					adjustpos(lforw(lp), 0);
					eraselen = getfulloffset(curwp, curwp->w_dotp, indentcol);
					if (ldeleteraw(eraselen, KNONE) == FALSE) return FALSE;
				}

				adjustpos(lp, len);

				// Insert a blank, which has to be inserted anyway, to not
				// make a mark from the beginning of the first word on the
				// next line stick.

				if (nextw == endw && !is_blank(lgetc(lforw(lp), 0))) {
					if (linsert(1, ' ') == FALSE) return FALSE;
				}

				if (ldelnewline() == FALSE) return FALSE;
			}
		}

		// Determine word separation

		if (!(opt & FILL_SINGLESPACE) && (nextw - endw > 1 || nextw == len) &&
			endsentence(wordlastc))
		{
			wordsep = 2;
		} else {
			wordsep = 1;
		}

		if (!setblanks(wordsep)) return FALSE;

		// Set up for next word

		startw = curwp->w_doto;
		col += wordsep;
	}
}
