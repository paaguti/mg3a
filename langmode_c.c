/* $OpenBSD: cmode.c,v 1.8 2012/05/18 02:13:44 lum Exp $ */
/*
 * This file is in the public domain.
 *
 * Author: Kjell Wooding <kjell@openbsd.org>
 *
 * Modified for Mg3a by Bengt Larsson and Pedro Andres Aranda
 * Gutierrez. Still public domain.
 */

/*
 * Implement an non-irritating KNF-compliant mode for editing
 * C code.
 *
 * paaguti: TODO: integrate comment_begin and comment_end variables
 */

#include "def.h"


#ifdef LANGMODE_C

INT cc_strip_trailp = TRUE;		/* Delete Trailing space? */
INT cc_basic_indent = 8;		/* Basic Indent multiple */
INT cc_cont_indent = 4;			/* Continued line indent */
INT cc_colon_indent = -8;		/* Label / case indent */

static INT getmatch(INT, INT);
static INT cc_getindent(LINE *, INT *);
static INT in_whitespace(LINE *, INT);
static INT findcolpos(BUFFER *, LINE *, INT);
static LINE *findnonblank(LINE *);
static INT isnonblank(LINE *, INT);

INT cc_comment(INT, INT);
INT cc_indent(INT, INT);

INT selfinsert(INT, INT);
INT showmatch(INT, INT);


/* Mg3a: begin compatibility functions */

static int
is_space(INT c)
{
	return c == ' ' || c == '\t';
}

/* Mg3a: end compatibility functions */


/*
 * Enable/toggle c-mode
 */

INT cmode(INT f, INT n)
{
	return changemode(curbp, f, n, "c");
}

/*
 * Handle preprocessor char '#'
 * if the line is empty (no chars or all blanks),
 * insert at the beginning of the line
 */
INT
cc_preproc(INT f, INT n)
{
	if (n < 0) return FALSE;

	if (isblankline(curwp->w_dotp)) {
		if (delleadwhite() == FALSE) return FALSE;
	}

	return linsert(n, '#');
}

/*
 * Handle special C character - selfinsert then indent.
 */
INT
cc_char(INT f, INT n)
{
	if (selfinsert(f|FFRAND, n) == FALSE) return (FALSE);
	return (cc_indent(FFRAND, n));
}

/*
 * Handle special C character - selfinsert, show matching char, then indent.
 */
INT
cc_brace(INT f, INT n)
{
	if (n < 0)
		return (FALSE);
	if (showmatch(FFRAND, 1) == FALSE)
		return (FALSE);
	return (cc_indent(FFRAND, n));
}


/*
 * If we are in the whitespace at the beginning of the line,
 * simply act as a regular tab. If we are not, indent
 * current line according to whitespace rules.
 */
INT
cc_tab(INT f, INT n)
{
	INT inwhitep = FALSE;	/* In leading whitespace? */

	inwhitep = in_whitespace(curwp->w_dotp, llength(curwp->w_dotp));

	/* If empty line, or in whitespace */
	if (llength(curwp->w_dotp) == 0 || inwhitep)
		return (selfinsert(f|FFRAND, n));

	return (cc_indent(FFRAND, 1));
}

/*
 * Attempt to indent current line according to KNF rules.
 */
INT
cc_indent(INT f, INT n)
{
	INT pi, mi;			/* Previous indents */
	INT ci;
//	INT dci;			/* current indent, don't care */	// Not used
	LINE *lp;
	INT ret;

	if (n < 0)
		return (FALSE);

//	undo_boundary_enable(FFRAND, 0);
	if (cc_strip_trailp)
		deltrailwhite();

	/*
	 * Search backwards for a non-blank, non-preprocessor,
	 * non-comment line
	 */

	lp = findnonblank(curwp->w_dotp);

	pi = cc_getindent(lp, &mi);

	/* Strip leading space on current line */
	if (delleadwhite() == FALSE) return FALSE;
	/* current indent is computed only to current position */
//	dci = cc_getindent(curwp->w_dotp, &ci);					// Not used
	cc_getindent(curwp->w_dotp, &ci);

	if (pi + ci < 0)
		ret = setindent(0);
	else
		ret = setindent(pi + ci);

//	undo_boundary_enable(FFRAND, 1);

	return (ret);
}

/*
 * Indent-and-newline (technically, newline then indent)
 */
INT
cc_lfindent(INT f, INT n)
{
	if (n < 0)
		return (FALSE);
	if (lnewline() == FALSE)
		return (FALSE);
	return (cc_indent(FFRAND, n));
}

/*
 * Get the level of indention after line lp is processed
 * Note cc_getindent has two returns:
 * curi = value if indenting current line.
 * return value = value affecting subsequent lines.
 */
static INT
cc_getindent(LINE *lp, INT *curi)
{
	INT lo, co;		/* leading space,  current offset*/
	INT nicol = 0;		/* position count */
//	INT ccol = 0;		/* current column */				// Not used
	INT c = '\0';		/* current char */
	INT newind = 0;		/* new index value */
	INT stringp = FALSE;	/* in string? */
	INT escp = FALSE;	/* Escape char? */
	INT lastc = '\0';	/* Last matched string delimeter */
	INT nparen = 0;		/* paren count */
	INT obrace = 0;		/* open brace count */
	INT cbrace = 0;		/* close brace count */
//	INT contp = FALSE;	/* Continue? */					// Not used
	INT firstnwsp = FALSE;	/* First nonspace encountered? */
	INT colonp = FALSE;	/* Did we see a colon? */
	INT questionp = FALSE;	/* Did we see a question mark? */
	INT slashp = FALSE;	/* Slash? */
	INT astp = FALSE;	/* Asterisk? */
	INT cpos = -1;		/* comment position */
	INT cppp  = FALSE;	/* Preprocessor command? */

	*curi = 0;

	/* Compute leading space */
	for (lo = 0; lo < llength(lp); lo++) {
		if (!is_space((c = lgetc(lp, lo))))
			break;
		if (c == '\t') {
			nicol |= 0x07;
		}
		nicol++;
	}

	/* If last line was blank, choose 0 */
	if (lo == llength(lp))
		nicol = 0;

	newind = 0;
//	ccol = nicol;			/* current column */			// Not used
	/* Compute modifiers */
	for (co = lo; co < llength(lp); co++) {
		c = lgetc(lp, co);
		/* We have a non-whitespace char */
		if (!firstnwsp && !is_space(c)) {
//			contp = TRUE;						// Not used
			if (c == '#')
				cppp = TRUE;
			firstnwsp = TRUE;
		}
		if (c == '\\')
			escp = !escp;
		else if (stringp) {
			if (!escp && (c == '"' || c == '\'')) {
				/* unescaped string char */
				if (getmatch(c, lastc))
					stringp = FALSE;
			}
		} else if (c == '"' || c == '\'') {
			stringp = TRUE;
			lastc = c;
		} else if (c == '(') {
			nparen++;
		} else if (c == ')') {
			nparen--;
		} else if (c == '{') {
			obrace++;
			firstnwsp = FALSE;
//			contp = FALSE;						// Not used
		} else if (c == '}') {
			cbrace++;
		} else if (c == '?') {
			questionp = TRUE;
		} else if (c == ':') {
			/* ignore (foo ? bar : baz) construct */
			if (!questionp)
				colonp = TRUE;
		} else if (c == ';') {
//			if (nparen > 0)
//				contp = FALSE;					// Not used
		} else if (c == '/') {
			/* first nonwhitespace? -> indent */
			if (firstnwsp) {
				/* If previous char asterisk -> close */
				if (astp)
					cpos = -1;
				else
					slashp = TRUE;
			}
		} else if (c == '*') {
			/* If previous char slash -> open */
			if (slashp)
				cpos = co;
			else
				astp = TRUE;
		} else if (firstnwsp) {
			firstnwsp = FALSE;
		}

		/* Reset matches that apply to next character only */
		if (c != '\\')
			escp = FALSE;
		if (c != '*')
			astp = FALSE;
		if (c != '/')
			slashp = FALSE;
	}
	/*
	 * If not terminated with a semicolon, and brace or paren open.
	 * we continue
	 */
	if (colonp) {
		*curi += cc_colon_indent;
		newind -= cc_colon_indent;
	}

	*curi -= (cbrace) * cc_basic_indent;
	newind += obrace * cc_basic_indent;

	if (nparen < 0)
		newind -= cc_cont_indent;
	else if (nparen > 0)
		newind += cc_cont_indent;

	*curi += nicol;

	/* Ignore preprocessor. Otherwise, add current column */
	if (cppp) {
		newind = nicol;
		*curi = 0;
	} else {
		newind += nicol;
	}

	if (cpos != -1)
		newind = findcolpos(curbp, lp, cpos);

	return (newind);
}

/*
 * Given a delimeter and its purported mate, tell us if they
 * match.
 */
static INT
getmatch(INT c, INT mc)
{
	INT match = FALSE;

	switch (c) {
	case '"':
		match = (mc == '"');
		break;
	case '\'':
		match = (mc == '\'');
		break;
	case '(':
		match = (mc == ')');
		break;
	case '[':
		match = (mc == ']');
		break;
	case '{':
		match = (mc == '}');
		break;
	}

	return (match);
}

static INT
in_whitespace(LINE *lp, INT len)
{
	INT lo;
	INT inwhitep = FALSE;

	for (lo = 0; lo < len; lo++) {
		if (!is_space(lgetc(lp, lo)))
			break;
		if (lo == len - 1)
			inwhitep = TRUE;
	}

	return (inwhitep);
}


/* convert a line/offset pair to a column position (for indenting) */
static INT
findcolpos(BUFFER *bp, LINE *lp, INT lo)
{
	return getcolumn(curwp, lp, lo);	// Only called with curbp
}

/*
 * Find a non-blank line, searching backwards from the supplied line pointer.
 * For C, nonblank is non-preprocessor, non C++, and accounts
 * for complete C-style comments.
 */
static LINE *
findnonblank(LINE *lp)
{
	INT lo;
	INT nonblankp = FALSE;
	INT commentp = FALSE;
	INT slashp;
	INT astp;
	INT c;

	while (lback(lp) != curbp->b_linep && (commentp || !nonblankp)) {
		lp = lback(lp);
		slashp = FALSE;
		astp = FALSE;

		/* Potential nonblank? */
		nonblankp = isnonblank(lp, llength(lp));

		/*
		 * Search from end, removing complete C-style
		 * comments. If one is found, ignore it and
		 * test for nonblankness from where it starts.
		 */
		slashp = FALSE;
		/* Scan backwards from end to find C-style comment */
		for (lo = llength(lp) - 1; lo >= 0; lo--) {
			if (!is_space((c = lgetc(lp, lo)))) {
				if (commentp) { /* find comment "open" */
					if (c == '*')
						astp = TRUE;
					else if (astp && c == '/') {
						commentp = FALSE;
						/* whitespace to here? */
						nonblankp = isnonblank(lp, lo);
					}
				} else { /* find comment "close" */
					if (c == '/')
						slashp = TRUE;
					else if (slashp && c == '*')
						/* found a comment */
						commentp = TRUE;
				}
			}
		}
	}

	/* Rewound to start of file? */
	if (lback(lp) == curbp->b_linep && !nonblankp)
		return (curbp->b_linep);

	return (lp);
}

/*
 * Given a line, scan forward to 'omax' and determine if we
 * are all C whitespace.
 * Note that preprocessor directives and C++-style comments
 * count as whitespace. C-style comments do not, and must
 * be handled elsewhere.
 */
static INT
isnonblank(LINE *lp, INT omax)
{
	INT nonblankp = FALSE;		/* Return value */
	INT slashp = FALSE;		/* Encountered slash */
	INT lo;				/* Loop index */
	INT c;				/* char being read */

	/* Scan from front for preprocessor, C++ comments */
	for (lo = 0; lo < omax; lo++) {
		if (!is_space((c = lgetc(lp, lo)))) {
			/* Possible nonblank line */
			nonblankp = TRUE;
			/* skip // and # starts */
			if (c == '#' || (slashp && c == '/')) {
				nonblankp = FALSE;
				break;
			} else if (!slashp && c == '/') {
				slashp = TRUE;
				continue;
			}
		}
		slashp = FALSE;
	}
	return (nonblankp);
}


#endif /* LANGMODE_C */
