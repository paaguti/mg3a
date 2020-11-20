/*
 *		Region based commands.
 *
 * The routines in this file deal with the region, that magic space
 * between "." and mark. Some functions are commands. Some functions
 * are just for internal use.
 */

#include	"def.h"
#include	"macro.h"

INT	forwchar(INT f, INT n);
INT	backchar(INT f, INT n);
INT	setprefix(INT f, INT n);


/*
 * Kill the region. Ask "getregion" to figure out the bounds of the
 * region.
 */

INT
killregion(INT f, INT n)
{
	INT	s;
	REGION	region;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if ((s=getregion(&region)) != TRUE) return s;

	initkill();

	return ldeleteregion(&region, KKILL);
}


/*
 * Mg3a: Delete the region.
 */

INT
deleteregion(INT f, INT n)
{
	INT	s;
	REGION	region;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if ((s=getregion(&region)) != TRUE) return s;

	return ldeleteregion(&region, KNONE);
}


/*
 * Copy all of the characters in the region to the kill buffer. Don't
 * move dot at all. This is a bit like a kill region followed by a
 * yank.
 */

INT
copyregion(INT f, INT n)
{
	LINE	*linep;
	INT	loffs;
	INT	s, len, rsize;
	REGION	region;

	if ((s=getregion(&region)) != TRUE)
		return s;

	initkill();

	kbuf_flag = curbp->b_flag;
	kbuf_charset = curbp->charset;

	rsize = region.r_size;			/* Region size.		*/

	if (kgrow(!region.r_forward, rsize) == FALSE) return FALSE;

	if (region.r_forward) {
		linep = region.r_linep;
		loffs = region.r_offset;

		while (rsize > 0) {
			if (loffs == llength(linep)) {	/* End of line.		*/
				if ((s=kinsert('\n', KFORW)) != TRUE)
					return (s);
				linep = lforw(linep);
				loffs = 0;
				rsize--;
			} else {			/* Middle of line.	*/
				len = llength(linep) - loffs;
				if (len > rsize) len = rsize;
				if ((s=kinsert_str(&ltext(linep)[loffs], len, KFORW)) != TRUE)
					return s;
				loffs += len;
				rsize -= len;
			}
		}
	} else {
		linep = region.r_endlinep;
		loffs = region.r_endoffset;

		while (rsize > 0) {
			if (loffs == 0) {		/* Beginning of line.	*/
				if ((s=kinsert('\n', KBACK)) != TRUE)
					return (s);
				linep = lback(linep);
				loffs = llength(linep);
				rsize--;
			} else {			/* Middle of line.	*/
				len = loffs;
				if (len > rsize) len = rsize;
				if ((s=kinsert_str(&ltext(linep)[loffs-len], len, KBACK)) != TRUE)
					return s;
				loffs -= len;
				rsize -= len;
			}
		}
	}
	ewprintf("Region copied as kill");
	return TRUE;
}


/*
 * Mg3a: append-next-kill from GNU Emacs
 */

INT
appendnextkill(INT f, INT n)
{
	thisflag |= CFKILL;
	ewprintf("If the next command is a kill, it will append");
	return TRUE;
}


/*
 * Mg3a: Change case of all characters in the region to upper or lower.
 */

static INT
upperlowerregion(INT tocase)
{
	INT		c;
	INT		s;
	REGION		region;
	INT		nextoffset;
	charset_t	charset = curbp->charset;
	LINE		*prevline;
	INT		*special;
	INT		i, speclen, save_undo = undo_enabled;
	size_t		rsize;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if ((s=getregion(&region)) != TRUE) return s;

	// UNDO
	u_modify();
	if (!u_entry_range(UFL_DELETE, region.r_linep, region.r_offset, region.r_size, region.r_dotpos)) {
		return FALSE;
	}
	undo_enabled = 0;
	//

	adjustpos3(region.r_linep, region.r_offset, region.r_dotpos);
	prevline = lback(region.r_linep);

	while (region.r_size > 0) {
		if (curwp->w_doto == llength(curwp->w_dotp)) {
			adjustpos(lforw(curwp->w_dotp), 0);
			region.r_size--;
		} else {
			c = ucs_char(charset, curwp->w_dotp, curwp->w_doto, &nextoffset);

			region.r_size -= nextoffset - curwp->w_doto;

			if (ucs_changecase(tocase, c, &special, &speclen)) {
				if (ldeleteraw(nextoffset - curwp->w_doto, KNONE) == FALSE) goto error;

				for (i = 0; i < speclen; i++) {
					if (linsert_ucs(charset, 1, special[i]) == FALSE) goto error;
				}
			} else {
				adjustpos(curwp->w_dotp, nextoffset);
			}
		}
	}

	// UNDO
	if ((rsize = curwp->w_dotpos - region.r_dotpos) > MAXINT) {
		u_clear(curbp);
		rsize = 0;
	}
	//

	if (!region.r_forward) {
		curwp->w_markp = curwp->w_dotp;
		curwp->w_marko = curwp->w_doto;		// Mark may have been overrun
		adjustpos3(lforw(prevline), region.r_offset, region.r_dotpos);
	}

	// UNDO
	undo_enabled = save_undo;

	if (!u_entry_range(UFL_INSERT, lforw(prevline), region.r_offset, rsize, region.r_dotpos)) {
		return FALSE;
	}
	//

	return TRUE;
error:
	undo_enabled = save_undo;
	u_clear(curbp);
	return FALSE;
}

/*
 * Lower case region.
 */

INT
lowerregion(INT f, INT n)
{
	return upperlowerregion(CASEDOWN);
}


/*
 * Upper case region.
 */

INT
upperregion(INT f, INT n)
{
	return upperlowerregion(CASEUP);
}


static INT
overflow()
{
	ewprintf("Integer overflow calculating region size");
	return FALSE;
}


/*
 * This routine figures out the bound of the region in the current
 * window, and stores the results into the fields of the REGION
 * structure. Dot and mark are usually close together, but I don't
 * know the order, so I scan outward from dot, in both directions,
 * looking for mark. All of the callers of this routine should be
 * ready to get an ABORT status, because I might add a "if regions is
 * big, ask before clobberring" flag.
 *
 * Mg3a: extended with new fields, uses a general mark
 */

INT
getregion_mark(REGION *rp, LINE *markp, INT marko)
{
	LINE	*flp;
	LINE	*blp;
	INT	fsize;
	INT	bsize;
	INT	lines;
	LINE	*dotp;
	INT	doto;

	if (markp == NULL) {
		ewprintf("No mark in getregion_mark");
		return FALSE;
	} else if (markp == curbp->b_linep) {
		return FALSE;
	}

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;

	if (dotp == markp) {
		rp->r_linep = rp->r_endlinep = dotp;
		rp->r_lines = 1;

		if (doto < marko) {
			rp->r_offset = doto;
			rp->r_endoffset = marko;
			rp->r_dotpos = curwp->w_dotpos;
			rp->r_enddotpos = curwp->w_dotpos - doto + marko;
			rp->r_size = marko - doto;
			rp->r_forward = 0;
		} else {
			rp->r_offset = marko;
			rp->r_endoffset = doto;
			rp->r_dotpos = curwp->w_dotpos - doto + marko;
			rp->r_enddotpos = curwp->w_dotpos;
			rp->r_size = doto - marko;
			rp->r_forward = 1;
		}

		return TRUE;
	}

	flp = blp = dotp;
	bsize = doto;
	fsize = llength(flp) - doto + 1;
	lines = 1;

	while (lforw(flp) != curbp->b_linep || lback(blp) != curbp->b_linep) {
		if (lines > MAXINT - 1) return overflow();

		lines++;

		if (lforw(flp) != curbp->b_linep) {
			flp = lforw(flp);

			if (fsize > MAXINT - llength(flp) - 1) return overflow();

			if (flp == markp) {
				rp->r_linep = dotp;
				rp->r_offset = doto;
				rp->r_endlinep = markp;
				rp->r_endoffset = marko;
				rp->r_size = fsize + marko;
				rp->r_lines = lines;
				rp->r_forward = 0;
				rp->r_dotpos = curwp->w_dotpos;
				rp->r_enddotpos = curwp->w_dotpos + rp->r_size;
				return TRUE;
			}

			fsize += llength(flp) + 1;
		}

		if (lback(blp) != curbp->b_linep) {
			blp = lback(blp);

			if (bsize > MAXINT - llength(blp) - 1) return overflow();

			bsize += llength(blp) + 1;

			if (blp == markp) {
				rp->r_linep = markp;
				rp->r_offset = marko;
				rp->r_endlinep = dotp;
				rp->r_endoffset = doto;
				rp->r_size = bsize - marko;
				rp->r_lines = lines;
				rp->r_forward = 1;
				rp->r_dotpos = curwp->w_dotpos - rp->r_size;
				rp->r_enddotpos = curwp->w_dotpos;
				return TRUE;
			}
		}
	}

	ewprintf("Bug: lost mark");		/* Gak!			*/
	return FALSE;
}


/*
 * Mg3a: the normal getregion
 */

INT
getregion(REGION *rp)
{
	if (curwp->w_markp == NULL) {
		ewprintf("No mark set in this window");
		return FALSE;
	}

	return getregion_mark(rp, curwp->w_markp, curwp->w_marko);
}


#ifdef PREFIXREGION

/*
 * Implements one of my favorite keyboard macros; put a string at the
 * beginning of a number of lines in a buffer. The quote string is
 * settable by using set-prefix-string. Great for quoting mail, which
 * is the real reason I wrote it, but also has uses for creating bar
 * comments (like the one you're reading) in C code.
 */

#define PREFIXLENGTH 40
static char prefix_string[PREFIXLENGTH] = { '>', '\0' };

/*
 * Prefix the region with whatever is in prefix_string. Leaves dot at
 * the beginning of the line after the end of the region. If an
 * argument is given, prompts for the line prefix string.
 */

INT
prefixregion(INT f, INT n)
{
	INT	s;
	REGION	region;
	INT	len;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if ((f & FFARG) && ((s = setprefix(FFRAND, 1)) != TRUE))
		return s;

	if ((s = getregion(&region)) != TRUE)
		return (s);

	adjustpos(region.r_linep, 0);

	len = strlen(prefix_string);

	while (region.r_lines-- > 0) {
		if (linsert_general_string(termcharset, 1, prefix_string, len) == FALSE) return FALSE;
		if (lforw(curwp->w_dotp) != curbp->b_linep) adjustpos(lforw(curwp->w_dotp), 0);
	}

	lchange(WFHARD);	// Make sure buffer is changed as before

	return TRUE;
}


/*
 * Set prefix string.
 */

INT
setprefix(INT f, INT n)
{
	char	buf[PREFIXLENGTH];
	INT	s;

	if (prefix_string[0] == '\0')
		s = ereply("Prefix string: ",buf,sizeof buf);
	else
		s = ereply("Prefix string (default %s): ",
				buf,sizeof buf,prefix_string);
	if (s == TRUE)
		stringcopy(prefix_string, buf, PREFIXLENGTH);
	if ((s == FALSE) && (prefix_string[0] != '\0')) /* CR -- use old one */
		s = TRUE;
	return s;
}

#endif


/*
 * Mg3a: Move the region right or left by "n" columns
 */

static INT
shiftregion(INT n)
{
	REGION r;
	LINE *prevline;
	INT s, i, col, origcol;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if ((s = getregion(&r)) != TRUE) return s;

	prevline = lback(r.r_linep);
	origcol = getcolumn(curwp, curwp->w_dotp, curwp->w_doto);

	adjustpos3(r.r_linep, 0, r.r_dotpos - r.r_offset);

	while (r.r_lines-- > 0) {
		if (getindent(curwp->w_dotp, &i, &col) == FALSE) return FALSE;
		if (n > MAXINT - col) return column_overflow();
		col += n;
		if (col < 0) col = 0;
		if (ldeleteraw(i, KNONE) == FALSE) return FALSE;
		if (tabtocolumn(0, col) == FALSE) return FALSE;
		if (r.r_lines > 0) adjustpos(lforw(curwp->w_dotp), 0);
	}

	if (r.r_forward) {
		adjustpos(curwp->w_dotp, getoffset(curwp, curwp->w_dotp, origcol));
	} else {
		adjustpos(lforw(prevline), getoffset(curwp, lforw(prevline), origcol));
	}

	return TRUE;
}


/*
 * Mg3a: Move the region right or left by "n" tabstops
 */

static INT
tabregion(INT n)
{
	INT tabwidth;

	tabwidth = get_variable(curbp->localvar.v.soft_tab_size, soft_tab_size);

	if (tabwidth <= 0 || tabwidth > 100) tabwidth = 8;

	if ((n > 0 && n > MAXINT / tabwidth) ||
		(n < 0 && n < MININT / tabwidth)) return column_overflow();

	return shiftregion(n * tabwidth);
}


/*
 * Mg3a: One of many attempts at a reasonable shift region command.
 * Move the region n tabs to the right.
 */

INT
tabregionright(INT f, INT n)
{
	return tabregion(n);
}


/*
 * Mg3a: Move the region n tabs to the left.
 */

INT
tabregionleft(INT f, INT n)
{
	return tabregion(-n);
}


#ifdef INDENT_RIGIDLY
/*
 * Mg3a: An Emacs-compatible interface to shiftregion().
 */

INT
indent_rigidly(INT f, INT n)
{
	return shiftregion(n);
}
#endif

#if 0
/*
 * paaguti, comment all lines between the line with the mark and the line with the dot
 *
 * TODO
 */
INT comment_region(INT f, INT n)
{
	ewprintf("TODO: comment-region");
	return FALSE;
}
#endif
