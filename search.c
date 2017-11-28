/*
 *		Search commands.
 *
 * The functions in this file implement the search commands (both
 * plain and incremental searches are supported) and the query-replace
 * command.
 *
 * The plain old search code is part of the original MicroEMACS
 * "distribution". The incremental search code, and the query-replace
 * code, is by Rich Ellison.
 */

#include	"def.h"
#include	"macro.h"

#define SRCH_BEGIN	(0)			/* Search sub-codes.	*/
#define SRCH_FORW	(-1)
#define SRCH_BACK	(-2)
#define SRCH_NOPR	(-3)
#define SRCH_ACCM	(-4)
#define SRCH_MARK	(-5)

typedef struct	{
	INT	s_code;
	LINE	*s_dotp;
	INT	s_doto;
}	SRCHCOM;

static	SRCHCOM cmds[NSRCH];
static	INT	cip;

INT	srch_lastdir = SRCH_FORW;	// Last search flags.
INT	case_fold_search = 1;		// Case-insensitive search
INT	word_search = 0;		// Word search
#ifdef SEARCHSIMPLE
INT	searchissimple = 0;
#endif

static void	is_cpush(INT cmd);
static void	is_lpush(void);
static void	is_pop(void);
static INT	is_peek(void);
static void	is_undo(INT *pptr, INT *dir);
static INT	is_find(INT dir);
static void	is_prompt(INT dir, INT flag, INT success);
static void	is_dspl(char *prompt, INT flag);
static INT	eq(INT bc, INT pc);

INT	forwsrch(INT word_search, int incremental);
INT	backsrch(INT word_search);
INT	readpattern(char *prompt);
INT	isearch(INT dir);
INT	ctrlg(INT f, INT n);
INT	forwchar(INT f, INT n);
INT	backchar(INT f, INT n);
INT	killwholeline(INT f, INT n);


/*
 * Mg3a: word definition for word search
 */

static int
isword(INT c)
{
	return c == '_' || ucs_isalnum(c) || ucs_nonspacing(c);
}


/*
 * Mg3a: Toggle word search mode
 */

INT
word_search_mode(INT f, INT n)
{
	if (f & FFARG) word_search = (n > 0);
	else word_search = !word_search;

	ewprintf("Word search is %s", word_search ? "on" : "off");
	return TRUE;
}


/*
 * Mg3a: Toggle case-folded search mode
 */

INT
case_fold_mode(INT f, INT n)
{
	if (f & FFARG) case_fold_search = (n > 0);
	else case_fold_search = !case_fold_search;

	ewprintf("Case folded search is %s", case_fold_search ? "on" : "off");
	return TRUE;
}


#ifdef SEARCHALL

static BUFFER *firstbuf = NULL;
static INT searchcount = 0;


/*
 * Mg3a: Next non-system buffer (circular)
 */

static BUFFER *
nextbuf(BUFFER *bp)
{
	BUFFER *new = bp;

	while (1) {
		new = new->b_bufp;
		if (new == NULL) new = bheadp;
		if (new->b_bname[0] != '*') return new;
		if (new == bp) return bp;
	}
}


/*
 * Mg3a: Previous non-system buffer (circular)
 */

static BUFFER *
prevbuf(BUFFER *bp)
{
	BUFFER *new = bheadp, *save=bheadp;

	while (1) {
		if (new->b_bname[0] != '*') save = new;
		new = new->b_bufp;
		if (new == NULL) return save;
		if (new == bp) return save;
	}
}


/*
 * Mg3a: Set position in a new buffer appropriate for search
 */

static void
searchallsetpos(BUFFER *bp, INT direction)
{
	curbp = bp;
	showbuffer(curbp, curwp);

	if (direction == SRCH_FORW) {
		adjustpos3(lforw(curbp->b_linep), 0, 0);
	} else {
		adjustpos3(lback(curbp->b_linep), llength(lback(curbp->b_linep)), curbp->b_size);
	}
}


/*
 * Mg3a: Get a pattern from some source for multi-buffer search
 * Return True if got a pattern.
 */


static INT
searchallsetpat(INT f, INT direction)
{
	if (f & FFARG) {
		if (direction == SRCH_FORW) {
			return readpattern("Search all");
		} else {
			return readpattern("Search all backward");
		}
	} else if (pat[0]) {
		if (firstbuf == NULL) {
			firstbuf = curbp;
			searchallsetpos(curbp, direction);
		}
		return TRUE;
	}

	ewprintf("No search string");
	return FALSE;
}


/*
 * Mg3a: Search forward through all non-system buffers
 */

INT
forwsearchall(INT f, INT n)
{
	BUFFER *startbuf = curbp, *bp;
	INT s;

	if ((s = searchallsetpat(f, SRCH_FORW)) != TRUE) return s;

	srch_lastdir = SRCH_FORW;

	while (1) {
		if (forwsrch(word_search, 0) == FALSE) {
			bp = nextbuf(curbp);
			searchallsetpos(bp, srch_lastdir);
			if (bp == startbuf || bp == firstbuf) break;
		} else {
			searchcount++;
			if (!(f & FFRAND)) ewprintf("Match #%d", searchcount);
			return TRUE;
		}
	}

	if (searchcount > 0) {
		if (!(f & FFRAND)) ewprintf("%d matches found", searchcount);
		searchcount = 0;
		return TRUE;
	} else {
		ewprintf("Search all failed: \"%s\"", pat);
		return FALSE;
	}
}


/*
 * Mg3a: Search backward through all non-system buffers
 */

INT
backsearchall(INT f, INT n)
{
	BUFFER *startbuf = curbp, *bp;
	INT s;

	if ((s = searchallsetpat(f, SRCH_BACK)) != TRUE) return s;

	srch_lastdir = SRCH_BACK;

	while (1) {
		if (backsrch(word_search) == FALSE) {
			bp = prevbuf(curbp);
			searchallsetpos(bp, srch_lastdir);
			if (bp == startbuf || bp == firstbuf) break;
		} else {
			searchcount++;
			ewprintf("Match #%d", searchcount);
			return TRUE;
		}
	}

	if (searchcount > 0) {
		ewprintf("%d matches found", searchcount);
		searchcount = 0;
		return TRUE;
	} else {
		ewprintf("Search all failed: \"%s\"", pat);
		return FALSE;
	}
}

#endif


/*
 * Search forward. Get a search string from the user, and search for
 * it, starting at ".". If found, "." gets moved to just after the
 * matched characters, and display does all the hard stuff. If not
 * found, it just prints a message.
 */

INT
forwsearch(INT f, INT n)
{
	INT	s;

	if ((s=readpattern("Search")) != TRUE)
		return s;
	if (forwsrch(word_search, 0) == FALSE) {
		ewprintf("Search failed: \"%s\"", pat);
		return FALSE;
	}
	srch_lastdir = SRCH_FORW;
	return TRUE;
}


/*
 * Reverse search. Get a search string from the user, and search,
 * starting at "." and proceeding toward the front of the buffer. If
 * found "." is left pointing at the first character of the pattern
 * [the last character that was matched].
 */

INT
backsearch(INT f, INT n)
{
	INT	s;

	if ((s=readpattern("Search backward")) != TRUE)
		return (s);
	if (backsrch(word_search) == FALSE) {
		ewprintf("Search failed: \"%s\"", pat);
		return FALSE;
	}
	srch_lastdir = SRCH_BACK;
	return TRUE;
}


/*
 * Search again, using the same search string and direction as the
 * last search command. The direction has been saved in
 * "srch_lastdir", so you know which way to go.
 */

INT
searchagain(INT f, INT n)
{
	INT result;

#ifdef SEARCHSIMPLE
	if (searchissimple) return internal_searchsimple(pat, 1);
#endif
	if (pat[0] == 0 || srch_lastdir == SRCH_NOPR) {
		ewprintf("No last search");
		return FALSE;
	}

	if (srch_lastdir == SRCH_FORW) {
		result = forwsrch(word_search, 0);
	} else {
		result = backsrch(word_search);
	}

	if (result == FALSE) {
		ewprintf("Search failed: \"%s\"", pat);
		return FALSE;
	}

	return TRUE;
}


/*
 * Use incremental searching, initially in the forward direction.
 * isearch ignores any explicit arguments.
 */

INT
forwisearch(INT f, INT n)
{
	return isearch(SRCH_FORW);
}


/*
 * Use incremental searching, initially in the reverse direction.
 * isearch ignores any explicit arguments.
 */

INT
backisearch(INT f, INT n)
{
	return isearch(SRCH_BACK);
}


/*
 * Incremental Search.
 *	dir is used as the initial direction to search.
 *	^S	switch direction to forward
 *	^R	switch direction to reverse
 *	^Q	quote next character (allows searching for ^N etc.)
 *	<ESC>	exit from Isearch
 *	<DEL>	undoes last character typed. (tricky job to do this correctly).
 *	other ^ exit search, don't set mark
 *	else	accumulate into search string
 */

INT
isearch(INT dir)
{
	INT		c;
	LINE		*clp;
	INT		cbo;
	INT		success;
	INT		pptr;
	char		opat[NPAT];
	unsigned char	charbuf[4];
	INT		i, charlen;

#ifndef NO_MACRO
	if (macrodef) {
		return emessage(ABORT, "Can't isearch in macro");
	}
#endif
	for (cip=0; cip<NSRCH; cip++)
		cmds[cip].s_code = SRCH_NOPR;
	stringcopy(opat, pat, NPAT);
	cip = 0;
	pptr = -1;
	clp = curwp->w_dotp;
	cbo = curwp->w_doto;
	is_lpush();
	is_cpush(SRCH_BEGIN);
	success = TRUE;
	is_prompt(dir, TRUE, success);
	for (;;) {
		update();
		switch (c = getkey(FALSE)) {
		case CCHR('['):
			srch_lastdir = dir;
			curwp->w_markp = clp;
			curwp->w_marko = cbo;
			ewprintf("Mark set");
			if (typeahead()) ungetkey(c);	// Escape sequence
			return (TRUE);

		case CCHR('G'):
			if (success != TRUE) {
				while (is_peek() == SRCH_ACCM)
					is_undo(&pptr, &dir);
				success = TRUE;
				is_prompt(dir, pptr < 0, success);
				break;
			}
			adjustpos(clp, cbo);
			srch_lastdir = dir;
			ctrlg(FFRAND, 0);
			stringcopy(pat, opat, NPAT);
			return ABORT;

		case CCHR(']'):
		case CCHR('S'):
			if (dir == SRCH_BACK) {
				dir = SRCH_FORW;
				is_lpush();
				is_cpush(SRCH_FORW);
				success = TRUE;
			}
			if (success==FALSE && dir==SRCH_FORW)
				break;
			is_lpush();
			pptr = strlen(pat);
			forwchar(FFRAND, 1);
			if (is_find(SRCH_FORW) != FALSE) is_cpush(SRCH_MARK);
			else {
				backchar(FFRAND, 1);
				ttbeep();
				success = FALSE;
			}
			is_prompt(dir, pptr < 0, success);
			break;

		case CCHR('R'):
			if (dir == SRCH_FORW) {
				dir = SRCH_BACK;
				is_lpush();
				is_cpush(SRCH_BACK);
				success = TRUE;
			}
			if (success==FALSE && dir==SRCH_BACK)
				break;
			is_lpush();
			pptr = strlen(pat);
			backchar(FFRAND, 1);
			if (is_find(SRCH_BACK) != FALSE) is_cpush(SRCH_MARK);
			else {
				forwchar(FFRAND, 1);
				ttbeep();
				success = FALSE;
			}
			is_prompt(dir, pptr < 0, success);
			break;

		case CCHR('H'):
		case CCHR('?'):
			is_undo(&pptr, &dir);
			if (is_peek() != SRCH_ACCM) success = TRUE;
			is_prompt(dir, pptr < 0, success);
			break;

		case CCHR('\\'):
		case CCHR('Q'):
			c = (char) getkey(FALSE);
			goto  addchar;
		case CCHR('J'):
		case CCHR('M'):
			if (pptr == -1) {
				if (dir == SRCH_FORW) {
					return forwsearch(0, 1);
				} else {
					return backsearch(0, 1);
				}
			}

			c = CCHR('J');
			goto  addchar;

		default:
			if (c < 256 && ISCTRL(c)) {
				ungetkey(c);
				srch_lastdir = dir;
				curwp->w_markp = clp;
				curwp->w_marko = cbo;
				ewprintf("Mark set");
				return	TRUE;
			}	/* and continue */
		case CCHR('I'):
		addchar:
			if (pptr == -1)
				pptr = 0;
			if (pptr == 0)
				success = TRUE;
#ifdef UTF8
			ucs_chartostr(termcharset, c, charbuf, &charlen);

			if (pptr + charlen >= NPAT) {
				ewprintf("Pattern too long");
				return FALSE;
			}

			for (i = 0; i < charlen; i++) {
				pat[pptr++] = charbuf[i];
			}
#else
			pat[pptr++] = c;
			if (pptr == NPAT) {
				ewprintf("Pattern too long");
				return FALSE;
			}
#endif
			pat[pptr] = '\0';
			is_lpush();
			if (success != FALSE) {
				if (is_find(dir) != FALSE)
					is_cpush(c);
				else {
					success = FALSE;
					ttbeep();
					is_cpush(SRCH_ACCM);
				}
			} else
				is_cpush(SRCH_ACCM);
			is_prompt(dir, FALSE, success);
		}
	}
	/*NOTREACHED*/
}


static void
is_cpush(INT cmd) {
	if (++cip >= NSRCH)
		cip = 0;
	cmds[cip].s_code = cmd;
}

static void
is_lpush() {
	INT	ctp;

	ctp = cip+1;
	if (ctp >= NSRCH)
		ctp = 0;
	cmds[ctp].s_code = SRCH_NOPR;
	cmds[ctp].s_doto = curwp->w_doto;
	cmds[ctp].s_dotp = curwp->w_dotp;
}

static void
is_pop() {
	if (cmds[cip].s_code != SRCH_NOPR) {
		adjustpos(cmds[cip].s_dotp, cmds[cip].s_doto);
		cmds[cip].s_code = SRCH_NOPR;
	}
	if (--cip <= 0)
		cip = NSRCH-1;
}

static INT
is_peek() {
	return cmds[cip].s_code;
}


/* this used to always return TRUE (the return value was checked) */

static void
is_undo(INT *pptr, INT *dir)
{
	INT	redo = FALSE ;
	switch (cmds[cip].s_code) {
	case SRCH_BEGIN:
	case SRCH_NOPR:
		*pptr = -1;
	case SRCH_MARK:
		break;

	case SRCH_FORW:
		*dir = SRCH_BACK;
		redo = TRUE;
		break;

	case SRCH_BACK:
		*dir = SRCH_FORW;
		redo = TRUE;
		break;

	case SRCH_ACCM:
	default:
#ifdef UTF8
		if (termisutf8) {
			*pptr -= 1;
			while (*pptr > 0 && (pat[*pptr] & 0xc0) == 0x80) {
				*pptr -= 1;
			}

			if (*pptr < 0)
				*pptr = 0;
			pat[*pptr] = '\0';
			break;
		} else {
			*pptr -= 1;
			if (*pptr < 0)
				*pptr = 0;
			pat[*pptr] = '\0';
			break;
		}
#else
		*pptr -= 1;
		if (*pptr < 0)
			*pptr = 0;
		pat[*pptr] = '\0';
		break;
#endif
	}
	is_pop();
	if (redo) is_undo(pptr, dir);
}


static INT
is_find(INT dir)
{
	INT	plen;
	LINE	*odotp ;
	INT	i, len, odoto;

	odoto = curwp->w_doto;
	odotp = curwp->w_dotp;

	i = 0;
	len = strlen(pat);
	plen = 0;

	while (i < len) {
		if (!ucs_nonspacing(ucs_strtochar(termcharset, pat, len, i, &i))) {
			plen++;
		}
	}

	if (plen != 0) {
		if (dir==SRCH_FORW) {
			backchar(FFARG | FFRAND, plen);
			if (forwsrch(0, 1) == FALSE) {
				adjustpos(odotp, odoto);
				return FALSE;
			}
			return TRUE;
		}
		if (dir==SRCH_BACK) {
			forwchar(FFARG | FFRAND, plen);
			if (backsrch(0) == FALSE) {
				adjustpos(odotp, odoto);
				return FALSE;
			}
			return TRUE;
		}
		ewprintf("bad call to is_find");
		return FALSE;
	}
	return FALSE;
}


/*
 * If called with "dir" not one of SRCH_FORW or SRCH_BACK, this
 * routine used to print an error message. It also used to return TRUE
 * or FALSE, depending on if it liked the "dir". However, none of the
 * callers looked at the status, so I just made the checking vanish.
 */

static void
is_prompt(INT dir, INT flag, INT success)
{
	if (dir == SRCH_FORW) {
		if (success != FALSE)
			is_dspl("I-search", flag);
		else
			is_dspl("Failing I-search", flag);
	} else if (dir == SRCH_BACK) {
		if (success != FALSE)
			is_dspl("I-search backward", flag);
		else
			is_dspl("Failing I-search backward", flag);
	} else ewprintf("Broken call to is_prompt");
}


/*
 * Prompt writing routine for the incremental search. The "prompt" is
 * just a string. The "flag" determines whether pat should be printed.
 */

static void
is_dspl(char *prompt, INT flag)
{

	if (flag != FALSE)
		ewprintf("%s: ", prompt);
	else
		ewprintf("%s: %s", prompt, pat);
}


/*
 * Query Replace. Replace strings selectively. Does a search and
 * replace operation.
 */

INT
queryrepl(INT f, INT n)
{
	INT		s;
	size_t		rcnt = 0;	/* Replacements made so far	*/
	INT		plen;		/* length of found string	*/
	char		news[NPAT];	/* replacement string		*/
	INT		i, len, c, doreplace, case_exact;

	if (curbp->b_flag & BFREADONLY) return readonly();

#ifndef NO_MACRO
	if (macrodef) {
		return emessage(ABORT, "Can't query replace in macro");
	}
#endif
	if ((s = readpattern("Query replace")) != TRUE) return s;
	if (eread("Query replace %s with: ", news, NPAT, EFREPLACE, pat) == ABORT) return ABORT;

	ewprintf("Query replacing %s with %s:", pat, news);

	i = 0;
	len = strlen(pat);
	plen = 0;

	while (i < len) {
		if (!ucs_nonspacing(ucs_strtochar(termcharset, pat, len, i, &i))) {
			plen++;
		}
	}

	/*
	 * Search forward repeatedly, checking each time whether to
	 * insert or not. The "!" case makes the check always true, so
	 * it gets put into a tighter loop for efficiency.
	 */

	case_exact = (f & FFARG) || !case_fold_search;
	doreplace = 1;

	while (forwsrch(word_search, 0) == TRUE) {
	retry:
		update();
		switch ((c = getkey(FALSE))) {
		case ' ':
		case '.':
		case ',':
			if (doreplace) {
				u_boundary = 1;		// UNDO
				if (lreplace(plen, news, case_exact) == FALSE)
					return (FALSE);
				rcnt++;
			}

			if (c == ' ') break;
			if (c == '.') goto stopsearch;
			doreplace = 0;
			goto retry;

		case CCHR('G'): /* ^G */
			ctrlg(FFRAND, 0);
			goto stopsearch;

		case CCHR('['): /* ESC */
			if (typeahead()) ungetkey(c);	// Escape sequence
			goto stopsearch;

		case '!':
			u_boundary = 1;		// UNDO
			do {
				if (doreplace) {
					if (lreplace(plen, news, case_exact) == FALSE)
						return (FALSE);
					rcnt++;
				}
				doreplace = 1;
			} while (forwsrch(word_search, 0) == TRUE);
			goto stopsearch;

		case CCHR('H'):
		case CCHR('?'):		/* To not replace */
			break;

		default:
			if (c < 256 && ISCTRL(c)) {
				ungetkey(c);
				goto stopsearch;
			}
			ewprintf("<SP> replace, [.] rep-end, [,] rep-now, <DEL> don't, [!] repl rest <ESC> quit");
			goto retry;
		}
		doreplace = 1;
	}
stopsearch:
	if (rcnt == 0)
		ewprintf("(No replacements done)");
	else if (rcnt == 1)
		ewprintf("(1 replacement done)");
	else
		ewprintf("(%z replacements done)", rcnt);
	return TRUE;
}


/*
 * This routine does the real work of a forward search. The pattern is
 * sitting in the external variable "pat". If found, dot is updated,
 * the window system is notified of the change, and TRUE is returned.
 * If the string isn't found, FALSE is returned.
 *
 * Mg3a: Optional word search.
 */

INT
forwsrch(INT word_search, int incremental) {
	LINE	*clp;
	INT	cbo;
	LINE	*tlp;
	INT	tbo;
	INT	*pp;
	INT	c;
	INT	localpat[NPAT];
	INT	i, j, len;
	INT 	prevc=' ', tc;
	charset_t charset = curbp->charset;

#ifdef SEARCHSIMPLE
	if (searchissimple && !incremental) return internal_searchsimple(pat, 0);
#endif
	i = 0;
	j = 0;
	len = strlen(pat);

	while (i < len + 1) {
		c = ucs_strtochar(termcharset, pat, len + 1, i, &i);
		localpat[j++] = c;
	}

	clp = curwp->w_dotp;
	cbo = curwp->w_doto;

	if (word_search && cbo != 0) {
		prevc = ucs_char(charset, clp, ucs_prevc(charset, clp, cbo), NULL);
	}

	for(;;) {
		if (cbo == llength(clp)) {
			if((clp = lforw(clp)) == curbp->b_linep) break;
			cbo = 0;
			c = CCHR('J');
		} else {
			c = ucs_char(charset, clp, cbo, &cbo);
		}

		if (eq(c, localpat[0]) != FALSE && (!word_search || !isword(c) || !isword(prevc))) {
			tlp = clp;
			tbo = cbo;
			tc = c;
			pp  = &localpat[1];
			while (*pp != 0) {
				if (tbo == llength(tlp)) {
					tlp = lforw(tlp);
					if (tlp == curbp->b_linep)
						goto fail;
					tbo = 0;
					tc = CCHR('J');
				} else {
					tc = ucs_char(charset, tlp, tbo, &tbo);
				}

				if (eq(tc, *pp++) == FALSE)
					goto fail;
			}

			if (word_search && isword(tc) && tbo != llength(tlp) &&
				isword(ucs_char(charset, tlp, tbo, NULL))) {
				goto fail;
			}

			adjustpos(tlp, tbo);
			return TRUE;
		}
	fail:	;
		prevc = c;
	}
	return FALSE;
}


/*
 * This routine does the real work of a backward search. The pattern
 * is sitting in the external variable "pat". If found, dot is
 * updated, the window system is notified of the change, and TRUE is
 * returned. If the string isn't found, FALSE is returned.
 *
 * Mg3a: Optional word search.
 */

INT
backsrch(INT word_search) {
	LINE	*clp;
	INT	cbo;
	LINE	*tlp;
	INT	tbo;
	INT	c;
	INT	*epp;
	INT	*pp;
	INT	localpat[NPAT];
	INT	i, j, len;
	INT	prevc=' ', tc;
	charset_t charset = curbp->charset;

	i = 0;
	j = 0;
	len = strlen(pat);

	while (i < len + 1) {
		c = ucs_strtochar(termcharset, pat, len + 1, i, &i);
		localpat[j++] = c;
	}

	for (epp = &localpat[0]; epp[1] != 0; ++epp) {}

	clp = curwp->w_dotp;
	cbo = curwp->w_doto;

	if (word_search && cbo != llength(clp)) {
		prevc = ucs_char(charset, clp, cbo, NULL);
	}

	for (;;) {
		if (cbo == 0) {
			clp = lback(clp);
			if (clp == curbp->b_linep) break;
			cbo = llength(clp);
			c = CCHR('J');
		} else {
			cbo = ucs_prevc(charset, clp, cbo);
			c = ucs_char(charset, clp, cbo, NULL);
		}

		if (eq(c, *epp) != FALSE && (!word_search || !isword(c) || !isword(prevc))) {
			tlp = clp;
			tbo = cbo;
			tc = c;
			pp  = epp;
			while (pp != &localpat[0]) {
				if (tbo == 0) {
					tlp = lback(tlp);
					if (tlp == curbp->b_linep)
						goto fail;
					tbo = llength(tlp);
					tc = CCHR('J');
				} else {
					tbo = ucs_prevc(charset, tlp, tbo);
					tc = ucs_char(charset, tlp, tbo, NULL);
				}

				if (eq(tc, *--pp) == FALSE)
					goto fail;
			}

			if (word_search && isword(tc) && tbo != 0 &&
				isword(ucs_char(charset, tlp,
					ucs_prevc(charset, tlp, tbo), NULL))) {
				goto fail;
			}

			adjustpos(tlp, tbo);
			return TRUE;
		}
	fail:	;
		prevc = c;
	}
	return FALSE;
}


/*
 * Compare two characters.
 * The "bc" comes from the buffer.
 * The "pc" is from the pattern.
 */

static INT
eq(INT bc, INT pc)
{
	if (case_fold_search) {
		bc = ucs_case_fold(bc);
		pc = ucs_case_fold(pc);
	}

	return bc == pc ? TRUE : FALSE;
}


/*
 * Read a pattern. Stash it in the external variable "pat". The "pat"
 * is not updated if the user types in an empty line. If the user
 * typed an empty line, and there is no old pattern, it is an error.
 * Display the old pattern, in the style of Jeff Lomicka. There is
 * some do-it-yourself control expansion.
 */

INT
readpattern(char *prompt)
{
	INT	s;
	char	tpat[NPAT];

#ifdef SEARCHALL
	firstbuf = curbp;
	searchcount = 0;
#endif
#ifdef SEARCHSIMPLE
	searchissimple = 0;
#endif

	if (pat[0] == '\0') s = ereply("%s: ", tpat, NPAT, prompt);
	else s = eread("%s: (default %S) ", tpat, NPAT, EFSEARCH, prompt, pat);

	if (s == TRUE)				/* Specified		*/
		stringcopy(pat, tpat, NPAT);
	else if (s==FALSE && pat[0]!=0)		/* CR, but old one	*/
		s = TRUE;
	return s;
}

#ifdef SEARCHSIMPLE


INT
internal_searchsimple(char *str, INT circular)
{
	LINE *startlp, *lp;
	INT len;

	searchissimple = 1;
	srch_lastdir = SRCH_FORW;

	startlp = curwp->w_dotp;

	lp = lforw(startlp);
	len = strlen(str);

	while (lp != startlp) {
		if (!circular && lp == curbp->b_linep) return FALSE;
		if (llength(lp) >= len && strncmp(str, ltext(lp), len) == 0) {
			adjustpos(lp, 0);
			curwp->w_flag |= WFFORCE;
			return TRUE;
		}
		lp = lforw(lp);
	}

	if (circular != 2) ewprintf("Not found");
	return FALSE;
}


INT
searchsimple(INT f, INT n)
{
	INT s;

	if ((s = readpattern("Search simple")) != TRUE) return s;

	return internal_searchsimple(pat, 1);
}

#ifdef SEARCHALL

INT
searchallsimple(INT f, INT n)
{
	INT s;

	if ((s = readpattern("Search all simple")) != TRUE) return s;

	searchissimple = 1;

	return forwsearchall(FFRAND, 1);
}

#endif

#endif
