/*
 *		Assorted commands.
 *
 * The file contains the command processors for a large assortment of
 * unrelated commands. The only thing they have in common is that they
 * are all command processors.
 */

#include	"def.h"
#include	"macro.h"

/* char comment_begin[20], comment_end[20]; */

INT	backchar(INT f, INT n);
INT	forwchar(INT f, INT n);
INT	delwhite(INT f, INT n);
INT	backdel(INT f, INT n);
extern INT comment_line(INT f, INT n);
INT gotobol(INT n, INT f);
INT gotoeol(INT n, INT f);
/*
 * Display a bunch of useful information about the current location of
 * dot. The character under the cursor (in octal), the current line,
 * row, and column, and approximate position of the cursor in the file
 * (as a percentage) is displayed. The column position assumes an
 * infinite position display; it does not truncate just because the
 * screen does. This is normally bound to "C-X =".
 *
 * Mg3a: enhanced with UTF-8 and combining characters.
 */

#define STATLEN 512
#define NUMLEN 80

INT
showcpos(INT f, INT n)
{
	LINE		*clp;
	size_t		nchar;
	size_t		cchar = 0;
	size_t		nline;
	size_t		cline = 0;
	INT		row;
	INT		cbyte = 0;	/* Current line/char/byte */
	int		ratio;
	INT 		ch;
	char		statstring[STATLEN], num[NUMLEN];
	INT		statlen, numlen;
	INT 		outoffset, nextoffset;
	INT 		arg;
	INT		unicode;
	INT		maxrow;

	arg = f & FFARG;
	outoffset = curwp->w_doto;

	if (curwp->w_doto == llength(curwp->w_dotp)) ch = 10;
	else ch = ucs_char(curbp->charset, curwp->w_dotp, curwp->w_doto, &outoffset);

#ifdef UCSNAMES
	if (arg && ch >= 0 && termcharset && unicode_database_exists()) {
		return advanced_display(f, n);
	}
#endif
	clp = lforw(curbp->b_linep);		/* Collect the data.	*/
	nchar = 0;
	nline = 0;
	for (;;) {
		++nline;			/* Count this line	*/
		if (clp == curwp->w_dotp) {
			cline = nline;		/* Mark line		*/
			cchar = nchar + curwp->w_doto;
			if (curwp->w_doto == llength(clp))
				cbyte = '\n';
			else
				cbyte = lgetc(clp, curwp->w_doto);
		}
		nchar += llength(clp);		/* Now count the chars	*/
		clp = lforw(clp);
		if (clp == curbp->b_linep) break;
		nchar++;			/* count the newline	*/
		if (!(curbp->b_flag & BFUNIXLF)) nchar++;
	}
	row = curwp->w_toprow + 1;		/* Determine row.	*/
	maxrow = row + curwp->w_ntrows - 1;
	clp = curwp->w_linep;
	while (clp != curwp->w_dotp) {
		row++;
		clp = lforw(clp);
		if (row > maxrow || clp == curbp->b_linep) {
			row = 0;
			break;
		}
	}
	/*NOSTRICT*/
	ratio = nchar ? (100.0*(double)cchar) / nchar : 100;

	snprintf(statstring, STATLEN,
		"  point=%zu(%d%%)  line=%zu  row=" INTFMT "  col=" INTFMT,
			cchar, ratio, cline, row, getcolpos());
	statlen = strlen(statstring);

	if (ch >= 0 && outoffset < llength(curwp->w_dotp) && termcharset &&
		ucs_nonspacing(ucs_char(curbp->charset, curwp->w_dotp, outoffset, NULL)))
	{
		estart();

		if (f & FFNUM) {
			n++;				// n is 1 + number of ^U
		} else if (f & FFUNIV) {
			if (n == 4) n = 2;		// ^U
			else if (n == 16) n = 3;	// ^U^U
			else if (n == 64) n = 4;	// ^U^U^U
			else if (n == 256) n = 5;	// ^U^U^U^U
			else if (n == 1024) n = 6;	// ^U^U^U^U^U
		}

		unicode = (n == 2 || n == 4 || n == 6);

		if (!unicode) {
			if (curbp->charset != utf_8) ch = lgetc(curwp->w_dotp, curwp->w_doto);
			ewprintfc("Combined Char: ");
		} else {
			ewprintfc("Unicode: ");
		}

		if (n == 3 || n == 4) {
			ewprintfc("%d", ch);
		} else if (n == 2) {
			ewprintfc("%U", ch);
		} else if (n == 5 || n == 6) {
			ewprintfc("0%o", ch);
		} else {
			ewprintfc("0x%x", ch);
		}

		ch = ucs_char(curbp->charset, curwp->w_dotp, outoffset, &nextoffset);

		while (ucs_nonspacing(ch)) {
			if (!unicode) {
				if (curbp->charset != utf_8) ch = lgetc(curwp->w_dotp, outoffset);
				ewprintfc("+");
			} else {
				ewprintfc(" ");
			}

			if (n == 3 || n == 4) {
				snprintf(num, NUMLEN, "%d", (int)ch);
			} else if (n == 2) {
				snprintf(num, NUMLEN, "U+%04X", (int)ch);
			} else if (n == 5 || n == 6) {
				snprintf(num, NUMLEN, "0%o", (int)ch);
			} else {
				snprintf(num, NUMLEN, "0x%x", (int)ch);
			}

			numlen = strlen(num);

			if (nextoffset < llength(curwp->w_dotp) &&
				ucs_nonspacing(ucs_char(curbp->charset, curwp->w_dotp, nextoffset, NULL)))
			{
				if (ttcol + numlen + 4 + (unicode ? 0 : statlen) > ncol - 2) {
					ewprintfc("...");
					break;
				}
			} else {
				if (ttcol + numlen + (unicode ? 0 : statlen) > ncol - 2) {
					ewprintfc("...");
					break;
				}
			}
			ewprintfc("%s", num);
			if (nextoffset >= llength(curwp->w_dotp)) break;
			outoffset = nextoffset;
			ch = ucs_char(curbp->charset, curwp->w_dotp, outoffset, &nextoffset);
		}

		if (!unicode) ewprintfc("%s",statstring);
	} else if (ch > 127 && curbp->charset == utf_8) {
		if (arg == 0) {
			ewprintf("UTF-8 Char: 0x%x (%d)%s", ch, ch, statstring);
		} else {
			ewprintf("Unicode: %U (%d, 0%o)", ch, ch, ch);
		}
	} else {
		if (arg == 0 || !termcharset || ch < 0) {
			ewprintf("Char: %c (%d, 0%o, 0x%x)%s", cbyte, cbyte, cbyte, cbyte, statstring);
		} else {
			ewprintf("Unicode: %U (%d, 0%o)", ch, ch, ch);
		}
	}

	return TRUE;
}


/*
 * Return 1-based column position
 */

INT
getcolpos()
{
	return getcolumn(curwp, curwp->w_dotp, curwp->w_doto) + 1;
}


/*
 * Twiddle the two characters on either side of dot. If dot is at the
 * end of the line twiddle the two characters before it. Return with
 * an error if dot is at the beginning of line; it seems to be a bit
 * pointless to make this work. This fixes up a very common typo with
 * a single stroke. Normally bound to "C-T".
 *
 * Mg3a: Slightly trickier with UTF-8.
 */

INT
twiddle(INT f, INT n)
{
	LINE	*dotp;
	INT	doto;
	INT	c1, c2, c3, i;
	charset_t charset;

	if (curbp->b_flag & BFREADONLY) return readonly();

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	charset = curbp->charset;

	if (doto == llength(dotp)) doto = ucs_backward(charset, dotp, doto);
	if (doto == 0) return FALSE;

	c1 = ucs_backward(charset, dotp, doto);
	c2 = doto;
	c3 = ucs_forward(charset, dotp, doto);

	adjustpos(curwp->w_dotp, c3);

	for (i = c1; i < c2; i++) {
		if (linsert(1, lgetc(curwp->w_dotp, i)) == FALSE) return FALSE;
	}

	adjustpos(curwp->w_dotp, c1);

	if (ldeleteraw(c2 - c1, KNONE) == FALSE) return FALSE;

	adjustpos(curwp->w_dotp, c3);

	return TRUE;
}


/*
 * Mg3a: Like inword() but with the search definition of a word
 */

static int
tinword()
{
	INT c;

	if (curwp->w_doto == llength(curwp->w_dotp)) return 0;
	c = ucs_char(curbp->charset, curwp->w_dotp, curwp->w_doto, NULL);
	return c == '_' || ucs_isalnum(c) || ucs_nonspacing(c);
}


/*
 * Mg3a: A simple implementation of "transpose-words" that only does
 * the basic transpose of two adjacent words.
 */

INT
twiddleword(INT f, INT n)
{
	INT word1b, word1e, word2b, word2e;
	INT i;
	LINE *lp1, *lp2;

	if (curbp->b_flag & BFREADONLY) return readonly();

	/* Dot is considered to be before the current character */
	backchar(FFRAND, 1);

	/* Skip forward to end of current word */
	while(tinword()) forwchar(FFRAND, 1);

	/* If no next word found, just quit */
	while(!tinword()) if (forwchar(FFRAND, 1) == FALSE) return FALSE;

	/* Beginning of second word */
	lp2 = curwp->w_dotp;
	word2b = curwp->w_doto;

	while(tinword()) forwchar(FFRAND, 1);

	/* End of second word */
	word2e = curwp->w_doto;

	/* Go back to beginning of second word */
	adjustpos(lp2, word2b);

	backchar(FFRAND, 1);

	/* If no previous word, quit, putting the cursor at a reasonable place */
	while(!tinword()) if (backchar(FFRAND, 1) == FALSE) {
		adjustpos(lp2, word2e);
		return FALSE;
	}

	/* Skip backward over first word */
	/* Don't straddle lines at the beginning of a line */
	while(tinword()) if (curwp->w_doto == 0 || backchar(FFRAND, 1) == FALSE) break;

	/* Undo last backchar if needed */
	if (!tinword()) forwchar(FFRAND, 1);

	/* Beginning of first word */
	lp1 = curwp->w_dotp;
	word1b = curwp->w_doto;

	while(tinword()) forwchar(FFRAND, 1);

	/* End of first word */
	word1e = curwp->w_doto;

	if (lp1 != lp2) {
		/* Different lines */
		for (i = word2b; i < word2e; i++) if (linsert(1, lgetc(lp2, i)) == FALSE) return FALSE;
		lchange(WFHARD);
		lp1 = curwp->w_dotp;
		adjustpos(lp2, word2b);
		if (ldeleteraw(word2e - word2b, KNONE) == FALSE) return FALSE;
		for (i = word1b; i < word1e; i++) if (linsert(1, lgetc(lp1, i)) == FALSE) return FALSE;
		lp2 = curwp->w_dotp;
		word2e = curwp->w_doto;
		adjustpos(lp1, word1b);
		if (ldeleteraw(word1e - word1b, KNONE) == FALSE) return FALSE;
		adjustpos(lp2, word2e);
	} else {
		/* Same line */
		adjustpos(curwp->w_dotp, word2e);
		for (i = word1b; i < word1e; i++)
			if (linsert(1, lgetc(curwp->w_dotp, i)) == FALSE) return FALSE;
		adjustpos(curwp->w_dotp, word1b);
		for (i = word2b; i < word2e + word2e - word2b ; i+=2)
			if (linsert(1, lgetc(curwp->w_dotp, i)) == FALSE) return FALSE;
		if (ldeleteraw(word1e - word1b, KNONE) == FALSE) return FALSE;
		adjustpos(curwp->w_dotp, word2e - word1e + word1b);
		if (ldeleteraw(word2e - word2b, KNONE) == FALSE) return FALSE;
		adjustpos(curwp->w_dotp, word2e);
	}

	return TRUE;
}


/*
 * Open up some blank space. The basic plan is to insert a bunch of
 * newlines, and then back up over them. Everything is done by the
 * subcommand processors. They even handle the looping. Normally this
 * is bound to "C-O".
 */

INT
openline(INT f, INT n)
{
	INT	s;

	s = lnewline_n(n);			/* Insert newlines.	*/
	if (s == TRUE)				/* Then back up overtop */
		s = backchar(FFRAND, n);	/* of them all.		*/
	return s;
}

/*
 * Mg3a: Insert a newline.
 */

INT
newline(INT f, INT n)
{
	return lnewline_n(n);
}


/*
 * Insert a newline. If you are at the end of the line and the next
 * line is a blank line, just move into the blank line. This makes "C-
 * O" and "C-X C-O" work nicely, and reduces the ammount of screen
 * update that has to be done. This would not be as critical if screen
 * update were a lot more efficient.
 *
 * Mg3a: Previously newline. Not the default anymore.
 */

INT
newlineclassic(INT f, INT n)
{
	LINE	*lp;
	INT	s;

	if (n < 0) return FALSE;
	while (n--) {
		lp = curwp->w_dotp;
		if (llength(lp) == curwp->w_doto
		&& lforw(lp) != curbp->b_linep
		&& llength(lforw(lp)) == 0) {
			if ((s=forwchar(FFRAND, 1)) != TRUE)
				return s;
		} else if ((s=lnewline()) != TRUE)
			return s;
	}
	return TRUE;
}


/*
 * Delete blank lines around dot. What this command does depends if
 * dot is sitting on a blank line. If dot is sitting on a blank line,
 * this command deletes all the blank lines above and below the
 * current line. If it is sitting on a non blank line then it deletes
 * all of the blank lines after the line. Normally this command is
 * bound to "C-X C-O". Any argument is ignored.
 */

INT
deblank(INT f, INT n)
{
	LINE	*lp1;
	LINE	*lp2;
	INT	nld;

	if (curbp->b_flag & BFREADONLY) return readonly();

	lp1 = curwp->w_dotp;
	while (llength(lp1)==0 && (lp2=lback(lp1))!=curbp->b_linep)
		lp1 = lp2;
	lp2 = lp1;
	nld = 0;
	while ((lp2=lforw(lp2))!=curbp->b_linep && llength(lp2)==0)
		++nld;
	if (nld == 0)
		return (TRUE);
	adjustpos(lforw(lp1), 0);
	return ldeleteraw(nld, KNONE);
}


/*
 * Delete any whitespace around dot, then insert a space.
 */

INT
justone(INT f, INT n) {
	int s;

	if ((s = delwhite(f, n)) != TRUE) return s;
	return linsert(1, ' ');
}


/*
 * Delete any whitespace around dot.
 */

INT
delwhite(INT f, INT n)
{
	LINE	*dotp;
	INT	doto, dotend, len;
	INT	c;

	if (curbp->b_flag & BFREADONLY) return readonly();

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	len = llength(dotp);

	for (dotend = doto; dotend < len; dotend++) {
		c = lgetc(dotp, dotend);
		if (c != ' ' && c != '\t') break;
	}

	for (; doto > 0; doto--) {
		c = lgetc(dotp, doto - 1);
		if (c != ' ' && c != '\t') break;
	}

	/* Check for iffy whitespace followed by combining character */
	if (dotend > doto && ucs_nonspacing(ucs_char(curbp->charset, dotp, dotend, NULL))) {
		dotend--;
	}

	adjustpos(curwp->w_dotp, doto);
	return ldeleteraw(dotend - doto, KNONE);
}


/*
 * Insert a newline, then enough tabs and spaces to duplicate the
 * indentation of the previous line. Assumes tabs are every eight
 * characters. Quite simple. Figure out the indentation of the current
 * line. Insert a newline by calling the standard routine. Insert the
 * indentation by inserting the right number of tabs and spaces.
 * Return TRUE if all ok. Return FALSE if one of the subcomands
 * failed. Normally bound to "C-J".
 */

INT
indent(INT f, INT n)
{
	INT	nicol = -1;
	LINE	*prevline;

	if (n < 0) return (FALSE);

	while (n--) {
		if (lnewline() == FALSE) return FALSE;

		if (nicol < 0) {
			prevline = lback(curwp->w_dotp);

			if (getindent(prevline, NULL, &nicol) == FALSE) return FALSE;
		}

		if (tabtocolumn(0, nicol) == FALSE) return FALSE;
	}
	return TRUE;
}


/*
 * Mg3a: As indent(), but indent using same combination of tabs and
 * spaces (whitespace) as the previous line.
 */

INT
indentsame(INT f, INT n)
{
	INT	i = 0;
	LINE 	*plp = NULL;

	if (n < 0) return (FALSE);

	while (n--) {
		if (lnewline() == FALSE) return FALSE;

		if (!plp) {
			plp = lback(curwp->w_dotp);
			getindent(plp, &i, NULL);
		}

		if (i && linsert_str(1, ltext(plp), i) == FALSE) return FALSE;
	}

	return TRUE;
}


/*
 * Delete forward. This is real easy, because the basic delete routine
 * does all of the work. Watches for negative arguments, and does the
 * right thing. If any argument is present, it kills rather than
 * deletes, to prevent loss of text if typed with a big argument.
 * Normally bound to "C-D".
 */

INT
forwdel(INT f, INT n)
{
	if (n < 0) return backdel(f, -n);

	inhibit_undo_boundary(f, forwdel);

	if (f & FFARG) {
		initkill();
		return ldelete(n, KFORW);
	} else {
		return ldelete(n, KNONE);
	}
}


/*
 * Mg3a: deletes a character backward, emulating soft tabs.
 */

static INT
backtab(INT tabwidth, INT kflag)
{
	LINE *dotp;
	INT doto, col, tocol, i, c;

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;

	// If not after whitespace, do normal delete

	if (doto == 0 || ((c = lgetc(dotp, doto - 1)) != '\t' && c != ' ')) {
		return ldeleteback(1, kflag);
	}

	col = getcolumn(curwp, dotp, doto);

	// Check if on tab column

	if (col % tabwidth == 0) {
		tocol = col - tabwidth;
	} else {
		tocol = col - 1;
	}

	// Scan back across whitespace

	for (i = doto; i > 0; i--) {
		c = lgetc(dotp, i - 1);
		if (c != '\t' && c != ' ') break;
	}

	// Find out where we are (can't do this going backward)

	col = getcolumn(curwp, dotp, i);

	// Scan forward, preserving appropriately much whitespace

	for (; i < doto; i++) {
		c = lgetc(dotp, i);
		if (c == '\t') {
			if ((col | 7) + 1 > tocol) break;
			col = (col | 7) + 1;
		} else {
			if (col + 1 > tocol) break;
			col++;
		}
	}

	// Adjust

	if (i < doto) {
		if (ldeleterawback(doto - i, kflag) == FALSE) return FALSE;
	}

	// Insert

	if (col < tocol) return linsert(tocol - col, ' ');

	return TRUE;
}

/*
 * Delete backwards. This is quite easy too, because it's all done
 * with other functions. Like delete forward, this actually does a
 * kill if presented with an argument.
 *
 * Mg3a: Special handling of overwrite mode and soft tabs.
 */

INT
backdel(INT f, INT n)
{
	INT w, next, doto;
	charset_t charset = curbp->charset;
	INT kflag = KNONE, c;
	INT tabwidth, taboptions;

	if (n < 0)
		return forwdel(f, -n);
	if (f & FFARG) {			/* Really a kill.	*/
		kflag = KBACK;
		initkill();
	}

	inhibit_undo_boundary(f, backdel);

	if (curbp->b_flag & BFOVERWRITE) {
		while (n--) {
			if (curwp->w_doto == 0) {
				if (ldeleterawback(1, kflag) == FALSE) return FALSE;
			} else {
				next = ucs_backward(charset, curwp->w_dotp, curwp->w_doto);
				c = ucs_char(charset, curwp->w_dotp, next, NULL);

				if (c == '\t') {
					w = 0;
				} else {
					w = ucs_fulltermwidth(c);
				}

				if (ldeleterawback(curwp->w_doto - next, kflag) == FALSE) return FALSE;

				if (w && curwp->w_doto != llength(curwp->w_dotp)) {
					doto = curwp->w_doto;
					if (linsert(w, ' ') == FALSE) return FALSE;
					adjustpos(curwp->w_dotp, doto);
				}
			}
		}

		return TRUE;
	}

	tabwidth = get_variable(curbp->localvar.v.soft_tab_size, soft_tab_size);

	if (tabwidth > 0 && tabwidth <= 100) {
		taboptions = get_variable(curbp->localvar.v.tab_options, tab_options);

		while (n--) {
			if ((taboptions & TAB_ALTERNATE) && !in_leading_whitespace()) {
				if (backtab(8, kflag) != TRUE) return FALSE;
			} else {
				if (backtab(tabwidth, kflag) != TRUE) return FALSE;
			}
		}

		return TRUE;
	}

	return ldeleteback(n, kflag);
}


/*
 * Mg3a: Delete characters backwards and untabify. Kill if presented
 * with an argument. Pretend spaces were killed if they were tabs.
 */

INT
backdeluntab(INT f, INT n)
{
	INT 		blankfill;
	LINE		*dotp;
	INT		doto, prevdoto, c;
	charset_t	charset = curbp->charset;
	INT 		kflag, prevcol, overwrite, eol;

	if (n < 0)
		return forwdel(f, -n);

	kflag = KNONE;

	if (f & FFARG) {			/* Really a kill.	*/
		initkill();
		kflag = KBACK;
	}

	inhibit_undo_boundary(f, backdeluntab);

	overwrite = (curbp->b_flag & BFOVERWRITE);

	while (n-- > 0) {
		dotp = curwp->w_dotp;
		doto = curwp->w_doto;

		prevdoto = ucs_backward(charset, dotp, doto);
		eol = (doto == llength(dotp));

		if (doto == 0 || lgetc(dotp, prevdoto) != '\t') {
			if (doto != 0 && overwrite) {
				c = ucs_char(charset, dotp, prevdoto, NULL);
				blankfill = ucs_fulltermwidth(c);
				if (ldeleterawback(doto - prevdoto, kflag) == FALSE) return FALSE;
				if (blankfill && !eol) {
					doto = curwp->w_doto;
					if (linsert(blankfill, ' ') == FALSE) return FALSE;
					adjustpos(curwp->w_dotp, doto);
				}
			} else {
				if (ldeleteback(1, kflag) == FALSE) return FALSE;
			}

			continue;
		}

		if (overwrite) {
			// Overwrite mode works according to a full terminal
			prevcol = getfullcolumn(curwp, dotp, prevdoto);
		} else {
			prevcol = getcolumn(curwp, dotp, prevdoto);
		}

		blankfill = (prevcol | 7) - prevcol;
		if (overwrite && !eol) blankfill++;

		if (ldeleterawback(doto - prevdoto, KNONE) == FALSE) return FALSE;
		if (blankfill > 0 && linsert(blankfill, ' ') == FALSE) return FALSE;
		if (kflag == KBACK && kinsert(' ', kflag) == FALSE) return FALSE;
		if (overwrite && !eol) adjustpos(curwp->w_dotp, curwp->w_doto - 1);
	}

	return TRUE;
}


/*
 * Mg3a: too many lines killed
 */

static INT
killline_overflow()
{
	ewprintf("Integer overflow in size of region");
	return FALSE;
}


/*
 * Kill line. If called without an argument, it kills from dot to the
 * end of the line, unless it is at the end of the line, when it kills
 * the newline. If called with an argument of 0, it kills from the
 * start of the line to dot. If called with a positive argument, it
 * kills from dot forward over that number of newlines. If called with
 * a negative argument it kills any text before dot on the current
 * line, then it kills back abs(arg) lines.
 */

INT
killline(INT f, INT n) {
	INT	i, c;
	size_t	chunk;
	LINE	*lp = curwp->w_dotp, *blp = curbp->b_linep;
	INT	doto = curwp->w_doto;

	if (curbp->b_flag & BFREADONLY) return readonly();

	initkill();

	if (lp == blp) return FALSE;

	if (!(f & FFARG)) {
		chunk = llength(lp) - doto;
		for (i = doto; i < llength(lp); i++) {
			if ((c = lgetc(lp, i)) != ' ' && c != '\t') break;
		}
		if (i == llength(lp)) chunk++;
	} else if (n > 0) {
		chunk = llength(lp) - doto + 1;
		for (i = 1; i < n; i++) {
			lp = lforw(lp);
			if (lp == blp) break;
			chunk += llength(lp) + 1;
		}
	} else {
		chunk = doto;
		for (i = 0; i > n; i--) {
			lp = lback(lp);
			if (lp == blp) break;
			chunk += llength(lp) + 1;
		}
	}

	if (chunk > MAXINT) return killline_overflow();

	if (n > 0) {
		return ldeleteraw(chunk, KFORW);
	} else {
		return ldeleterawback(chunk, KBACK);
	}
}


/*
 * Mg3a: for special behavior of killwholeline (and yank)
 */

INT kill_whole_lines = 0;


/*
 * Mg3a: Kill whole line. Compatible with Emacs.
 */

INT
killwholeline(INT f, INT n) {
	INT	i, s;
	size_t	chunk = 0;
	LINE	*lp = curwp->w_dotp, *blp = curbp->b_linep;

	if (curbp->b_flag & BFREADONLY) return readonly();

	initkill();

	if (!(lastflag & CFKWL)) setgoal();
	thisflag |= CFKWL;

	if (lp == blp) return FALSE;

	if (n == 0) {
		adjustpos(lp, 0);
		chunk = llength(lp);
	} else if (n > 0) {
		adjustpos(lp, 0);
		for (i = 0; i < n && lp != blp; i++) {
			chunk += llength(lp) + 1;
			if (lforw(lp) == blp) normalizebuffer(curbp);	// Whole line
			lp = lforw(lp);
		}
	} else {
		adjustpos(lp, llength(lp));
		for (i = 0; i > n && lp != blp; i--) {
			chunk += llength(lp) + 1;
			lp = lback(lp);
		}
	}

	if (chunk > MAXINT) return killline_overflow();

	if (n >= 0) {
		s = ldeleteraw(chunk, KFORW);
	} else {
		s = ldeleterawback(chunk, KBACK);
	}

	if (kill_whole_lines) {
		adjustpos(curwp->w_dotp, getgoal(curwp->w_dotp));
		kbuf_wholelines = (n >= 0) ? 1 : -1;
	}

	return s;
}


/*
 * Yank text back from the kill buffer.
 *
 * Mg3a: Uses general string insert. Added kill-whole-lines feature.
 * Now with mark around all copies.
 */

INT
yank(INT f, INT n)
{
	char	*kbuf;
	INT	ksize;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0)  return FALSE;
	if (n == 0) return TRUE;

	if (kbuf_wholelines) {
		setgoal();
		adjustpos(curwp->w_dotp, (kbuf_wholelines > 0) ? 0 : llength(curwp->w_dotp));
	}

	getkillbuffer(&kbuf, &ksize);

	isetmark();

	if (linsert_general_string(kbuf_charset, n, kbuf, ksize) == FALSE) return FALSE;

	if (kbuf_wholelines) adjustpos(curwp->w_dotp, getgoal(curwp->w_dotp));

	return TRUE;
}


/*
 * Mg3a: No-break. Replace space with no-break space, and hyphen with
 * n-dash, otherwise insert a no-break space.
 */

INT
no_break(INT f, INT n)
{
	INT c, end, replace;
	LINE *dotp = curwp->w_dotp;
	INT doto = curwp->w_doto;

	replace = 160;

	if (doto < llength(dotp)) {
		c = ucs_char(curbp->charset, dotp, doto, &end);

		if (c == ' ' || c == '-') {
			if (ldeleteraw(end - doto, KNONE) == FALSE) return FALSE;
		}

		if (c == '-') replace = 0x2013;
	}

	return linsert_ucs(curbp->charset, 1, replace);
}


/*
 * Mg3a: Delete trailing whitespace in a buffer, with a message.
 */

INT
internal_deletetrailing(BUFFER *bp, int msg)
{
	LINE *lp, *blp, *nextp;
	INT i, c, len;
	size_t modified = 0, pos = 0;

	if (bp->b_flag & BFREADONLY) return readonly();

	// UNDO
	if (bp == curbp) u_modify();
	else u_clear(bp);
	//

	blp = bp->b_linep;

	for (lp = lforw(blp); lp != blp;) {
		len = llength(lp);
		nextp = lforw(lp);	// In case we reallocate

		for (i = len; i > 0; i--) {
			c = lgetc(lp, i - 1);
			if (c != ' ' && c != '\t') break;
		}

		if (i != len) {
			// UNDO
			if (bp == curbp &&
				!u_entry(UFL_DELETE, pos + i, &ltext(lp)[i], len - i))
			{
				return FALSE;
			}
			//

			ltrim(bp, lp, i, pos + i);
			modified++;
		}

		pos += i + 1;
		lp = nextp;
	}

	if (msg) {
		if (modified > 0) {
			ewprintf("Deleted trailing whitespace from %z line%s", modified,
				modified != 1 ? "s" : "");
		} else {
			ewprintf("No trailing whitespace to delete");
		}
	}

	return TRUE;
}


/*
 * Mg3a: Delete trailing whitespace, a la Emacs
 */

INT
deletetrailing(INT f, INT n)
{
	return internal_deletetrailing(curbp, 1);
}


/*
 * Mg3a: Join lines. From Emacs.
 */

INT
joinline(INT f, INT n)
{
	LINE *lp, *blp;
	INT s, doto;

	blp = curbp->b_linep;

	if (f & FFARG) {
		lp = curwp->w_dotp;
	} else {
		lp = lback(curwp->w_dotp);
	}

	if (lp == blp || lforw(lp) == blp) return FALSE;

	adjustpos(lp, llength(lp));

	if ((s = ldelnewline()) != TRUE) return s;
	if ((s = delwhite(0, 1)) != TRUE) return s;

	lchange(WFHARD);

	if (curwp->w_doto != 0) {
		doto = curwp->w_doto;
		if ((s = linsert(1, ' ')) != TRUE) return s;
		adjustpos(curwp->w_dotp, doto);
	}

	return TRUE;
}


/*
 * Mg3a extension, as joinline, but default forward
 */

INT
joinline_forward(INT f, INT n)
{
	return joinline((f & FFARG) ? 0 : FFARG, n);
}


/*
 * Mg3a: Given that you are in "fromcol", tab to "tocol" using a mix
 * of hard tabs and spaces while maximizing the number of hard tabs,
 * including converting spaces to hard tabs in whitespace before the
 * cursor.
 */

static INT
mixed_tab_to_column(INT fromcol, INT tocol)
{
	LINE *dotp = curwp->w_dotp;
	INT doto = curwp->w_doto;
	INT i, c;

	if (curbp->b_flag & BFREADONLY) return readonly();

	// Scan back across whitespace

	for (i = doto; i > 0; i--) {
		c = lgetc(dotp, i - 1);
		if (c != ' ' && c != '\t') break;
	}

	// Skip forward over tabs

	for (; i < doto; i++) {
		c = lgetc(dotp, i);
		if (c != '\t') break;
	}

	// Adjust

	if (i < doto) {
		fromcol = getcolumn(curwp, dotp, i);	// dotp is not valid after next
		if (ldeleterawback(doto - i, KNONE) == FALSE) return FALSE;
	}

	// Insert

	if ((fromcol | 7) + 1 <= tocol) {
		fromcol -= (fromcol & 7);

		i = (tocol - fromcol) / 8;
		if (i && linsert(i, '\t') == FALSE) return FALSE;
	}

	i = (tocol - fromcol) % 8;
	if (i && linsert(i, ' ') == FALSE) return FALSE;

	return TRUE;
}


/*
 * Mg3a: Given that you are in "fromcol", tab to "tocol", using either
 * mixed hard tabs and spaces or only spaces, based on an editor
 * variable.
 */

INT
tabtocolumn(INT fromcol, INT tocol)
{
	if (get_variable(curbp->localvar.v.tabs_with_spaces, tabs_with_spaces)) {
		return linsert(tocol - fromcol, ' ');
	} else {
		return mixed_tab_to_column(fromcol, tocol);
	}
}


/*
 * Mg3a: Insert a number of tabs as hard tabs, mixed hard tabs and
 * spaces, or spaces, based on parameters.
 */

static INT
internal_inserttabs(INT n, INT tabwidth, INT tabspaces)
{
	INT col, tocol;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (n < 0) return FALSE;
	if (n == 0) return TRUE;

	if (tabwidth <= 0 || tabwidth > 100) {
		if (tabspaces) {
			tabwidth = 8;
		} else {
			return linsert(n, '\t');
		}
	}

	col = getcolumn(curwp, curwp->w_dotp, curwp->w_doto);

	if (n > MAXINT / tabwidth || col > MAXINT - n * tabwidth) {
		return column_overflow();
	}

	tocol = col - col % tabwidth + n * tabwidth;

	if (tabspaces) {
		return linsert(tocol - col, ' ');
	} else {
		return mixed_tab_to_column(col, tocol);
	}
}


/*
 * Mg3a: Insert a number of tabs as hard tabs, mixed hard tabs and
 * spaces, or spaces, based on editor variables.
 */

INT
inserttabs(INT n)
{
	INT tabwidth, tabspaces;

	tabwidth = get_variable(curbp->localvar.v.soft_tab_size, soft_tab_size);
	tabspaces = get_variable(curbp->localvar.v.tabs_with_spaces, tabs_with_spaces);

	return internal_inserttabs(n, tabwidth, tabspaces);
}


/*
 * Mg3a: Insert tab command.
 */

INT
insert_tab(INT f, INT n)
{
	INT tabwidth, tabspaces, taboptions;

	tabwidth = get_variable(curbp->localvar.v.soft_tab_size, soft_tab_size);
	tabspaces = get_variable(curbp->localvar.v.tabs_with_spaces, tabs_with_spaces);

	if (tabwidth > 0 && tabwidth <= 100) {
		taboptions = get_variable(curbp->localvar.v.tab_options, tab_options);

		if ((taboptions & TAB_ALTERNATE) && !in_leading_whitespace()) {
			tabwidth = 8;
		}
	}

	return internal_inserttabs(n, tabwidth, tabspaces);
}


/*
 * Mg3a: Insert tab width 8 command.
 */

INT
insert_tab_8(INT f, INT n)
{
	INT tabwidth, tabspaces;

	tabwidth = get_variable(curbp->localvar.v.soft_tab_size, soft_tab_size);
	tabspaces = get_variable(curbp->localvar.v.tabs_with_spaces, tabs_with_spaces);

	if (tabwidth > 0 && tabwidth <= 100) tabwidth = 8;

	return internal_inserttabs(n, tabwidth, tabspaces);
}


/* Mg3a: utility function */

INT
gettabsize(BUFFER *bp)
{
	INT tabsize;

	tabsize = get_variable(bp->localvar.v.soft_tab_size, soft_tab_size);

	if (tabsize <= 0 || tabsize > 100) return 8;

	return tabsize;
}

static int
is_space(INT c)
{
	return c == ' ' || c == '\t';
}


/*
 * Mg3a: Delete trailing whitespace in the current line
 */

INT
deltrailwhite()
{
	LINE *dotp;
	INT len, doto;
	INT i;

	if (curbp->b_flag & BFREADONLY) return readonly();

	dotp = curwp->w_dotp;
	len = llength(dotp);

	for (i = len; i > 0; i--) {
		if (!is_space(lgetc(dotp, i - 1))) break;
	}

	doto = curwp->w_doto;
	adjustpos(curwp->w_dotp, i);
	if (ldeleteraw(len - i, KNONE) == FALSE) return FALSE;
	if (doto < curwp->w_doto) adjustpos(curwp->w_dotp, doto);
	return TRUE;
}


/*
 * Mg3a: Delete leading whitespace in the current line
 */

INT
delleadwhite()
{
	LINE *dotp;
	INT len, doto;
	INT i;

	if (curbp->b_flag & BFREADONLY) return readonly();

	dotp = curwp->w_dotp;
	len = llength(dotp);

	for (i = 0; i < len; i++) {
		if (!is_space(lgetc(dotp, i))) break;
	}

	doto = curwp->w_doto;
	adjustpos(curwp->w_dotp, 0);
	if (ldeleteraw(i, KNONE) == FALSE) return FALSE;
	adjustpos(curwp->w_dotp, doto - i < 0 ? 0 : doto - i);
	return TRUE;
}


/*
 * Mg3a: Get the indent offset and/or column. Returns FALSE on column
 * overflow otherwise TRUE.
 */

INT
getindent(LINE *lp, INT *outoffset, INT *outcolumn)
{
	INT i, c, len, ret;
	UINT col;	// No nasty overflow

	col = 0;
	ret = TRUE;

	len = llength(lp);

	for (i = 0; i < len; i++) {
		if (outcolumn && col > MAXINT - 8) {
			ret = column_overflow();
			break;
		}

		c = lgetc(lp, i);

		if (c == '\t') {
			col = (col | 7) + 1;
		} else if (c == ' ') {
			col++;
		} else {
			break;
		}
	}

	if (outoffset) *outoffset = i;
	if (outcolumn) *outcolumn = col;

	return ret;
}


/*
 * Mg3a: Set the indent of the current line using current tabsettings.
 * Preserve the position. Report an error if the column is negative.
 */

INT
setindent(INT col)
{
	INT soff, i;

	if (col < 0) return FALSE;

	getindent(curwp->w_dotp, &i, NULL);

	soff = curwp->w_doto - i;
	if (soff < 0) soff = 0;

	adjustpos(curwp->w_dotp, 0);

	if (ldeleteraw(i, KNONE) == FALSE) return FALSE;
	if (tabtocolumn(0, col) == FALSE) return FALSE;

	adjustpos(curwp->w_dotp, curwp->w_doto + soff);

	return TRUE;
}

INT set_comment(void)
{
	ewprintf("comment delimiter(s) not defined!");
	return FALSE;
}

INT comment_line(INT f, INT n)
/* Version 0.1: if there is a comment_begin, insert it and then try to insert a comment_end
   ^U makes comment from point to end of line
 */
{
	char *comment_begin = curwp->w_bufp->localsvar.v.comment_begin;
	char *comment_end   = curwp->w_bufp->localsvar.v.comment_end;

	if (curwp->w_bufp->b_flag & BFREADONLY) return readonly();
	if ((comment_begin == NULL) || (strlen(comment_begin) == 0)) return set_comment();

	if (curwp->w_markp != NULL) {
		if (curwp->w_dotp != curwp->w_markp)
			return comment_region(f,n);
	}
	/*
	 * paaguti: if mark and dot are on the same line we can do comment-line
	 */
	if (f==0)
		gotobol(TRUE, 1);
	linsert_str(1, comment_begin, strlen(comment_begin));
/*	ewprintf("comment-line: comment-begin = %s",comment_begin); */

	if ((comment_end == NULL) || (strlen (comment_end) == 0))
		return TRUE;

	gotoeol(TRUE,1);
	linsert_str(1, comment_end, strlen(comment_end));
/*	ewprintf("comment-end: comment-end = %s",comment_end); */

	return TRUE;
}
