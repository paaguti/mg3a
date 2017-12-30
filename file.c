/*
 *		File commands.
 */

#include	"def.h"

static INT	insertfile(char *fname, char *newname);
static INT	finsertfile(FILE *f);
static INT	doinsertfile(char *newname);
static void	makename(char *bname, char *fname, INT buflen);
static INT	writeout(BUFFER *bp, char *fn);

size_t		count_lf, count_crlf;


/*
 * Mg3a: check if it's a directory, show a message
 */

static int
isdir(char *filename)
{
	if (isdirectory(filename)) {
		ewprintf("(File is a directory)");
		return TRUE;
	} else {
		return FALSE;
	}
}


/*
 * Mg3a: Check if existing and not readable, show a message
 */

static int
notreadable(char *filename)
{
	if (existsandnotreadable(filename)) {
		ewprintf("File is not readable: %s", filename);
		return TRUE;
	} else {
		return FALSE;
	}
}


/*
 * Mg3a: Automode for Cygwin/Windows
 */

#ifdef AUTOMODE
static int
setmodefromfileextension(int change)
{
	char *ext;
	INT i;
	INT oldflag;
	static const char *dosname[] = {".cmd", ".bat", NULL};
	static const char *PSname[] = {".ps1", ".psd1", ".psm1", NULL};

	oldflag = curbp->b_flag;
	ext = strrchr(curbp->b_fname, '.');

	if (ext) {
		for (i = 0; dosname[i]; i++) {
			if (ascii_strcasecmp(ext, dosname[i]) == 0) {
				if (!change) return 1;

				internal_unixlf(0);
				internal_dos(1);
				internal_bom(0);

				// Report if a change is made to an existing non-empty file.
				// CMD.EXE is not tolerant.

				if (oldflag != curbp->b_flag && curbp->b_linep != lforw(curbp->b_linep)) {
					ewprintf("On demand from CMD.EXE, "
					"the mode has been set to CRLF, Dos charset, and no BOM");
				}

				break;
			}
		}

		for (i = 0; PSname[i]; i++) {
			if (ascii_strcasecmp(ext, PSname[i]) == 0) {
				if (!change) return 1;

				// Powershell can tolerate LF or CRLF but we prefer CRLF
				// for new files due to the default Notepad assignment.

				if (curbp->b_linep == lforw(curbp->b_linep)) internal_unixlf(0);

				// Powershell can do BOM, but otherwise does 8-bit.

				if (curbp->b_flag & BFBOM) internal_utf8(1);
				else internal_iso(1);

				break;
			}
		}
	}

	return autoexec_execute(change);
}
#else
static int
setmodefromfileextension(int change)
{
	return autoexec_execute(change);
}
#endif


/*
 * Insert a file into the current buffer. Real easy - just call the
 * insertfile routine with the file name.
 */

INT
fileinsert(INT f, INT n)
{
	INT	s;
	char	fname[NFILEN], *adjf;
	REGION	r;

	if ((s=eread("Insert file: ", fname, NFILEN, EFFILE)) != TRUE)
		return (s);
	if ((adjf = adjustname(fname)) == NULL) return FALSE;

	// UNDO
	u_modify();
	//

	s = insertfile(adjf, NULL);			/* don't set buffer name */

	// UNDO
	if (s == TRUE && undo_enabled) {
		if (getregion(&r) == TRUE) {
			if (!u_entry_range(UFL_INSERT, r.r_linep, r.r_offset, r.r_size, r.r_dotpos)) {
				return FALSE;
			}
		} else {
			u_clear(curbp);
		}
	}
	//

	return s;
}


/*
 * Select a file for editing. Look around to see if you can find the
 * file in another buffer; if you can find it just switch to the
 * buffer. If you cannot find the file, create a new buffer, read in
 * the text, and switch to the new buffer.
 */

INT
filevisit(INT f, INT n)
{
	BUFFER *bp;
	INT	s;
	char	fname[NFILEN];
	char	*adjf;

	if ((s=eread("Find file: ", fname, NFILEN, EFFILE)) != TRUE)
		return s;
	if ((adjf = adjustname(fname)) == NULL) return FALSE;
#ifndef DIRED
	if (isdir(adjf)) return FALSE;
#endif
	if (notreadable(adjf)) return FALSE;
	if ((bp = findbuffer(adjf)) == NULL) return FALSE;
	curbp = bp;
	showbuffer(bp, curwp);
	if (bp->b_fname[0] == 0)
		return readin(adjf);		/* Read it in.		*/
	return TRUE;
}


/*
 * Mg3a: Visit a file read only
 */

INT
filevisit_readonly(INT f, INT n)
{
	INT s;

	if ((s = filevisit(f, n)) != TRUE) return s;
	curbp->b_flag |= BFREADONLY;
	return TRUE;
}


/*
 * Pop to a file in the other window. Same as last function, just
 * popbuf instead of showbuffer.
 */

INT
poptofile(INT f, INT n)
{
	BUFFER *bp;
	WINDOW *wp;
	INT	s;
	char	fname[NFILEN];
	char	*adjf;

	if ((s=eread("Find file in other window: ", fname, NFILEN, EFFILE)) != TRUE)
		return s;
	if ((adjf = adjustname(fname)) == NULL) return FALSE;
#ifndef DIRED
	if (isdir(adjf)) return FALSE;
#endif
	if (notreadable(adjf)) return FALSE;
	if ((bp = findbuffer(adjf)) == NULL) return FALSE;
	if ((wp = popbuf(bp)) == NULL) return FALSE;
	curbp = bp;
	curwp = wp;
	if (bp->b_fname[0] == 0)
		return readin(adjf);		/* Read it in.		*/
	return TRUE;
}


/*
 * Mg3a: and readonly.
 */

INT
poptofile_readonly(INT f, INT n)
{
	INT s;

	if ((s = poptofile(f, n)) != TRUE) return s;
	curbp->b_flag |= BFREADONLY;
	return TRUE;
}


/*
 * Mg3a: Given a file name, find the buffer it uses.
 */

static BUFFER *
findexistingbuffer(char *fname)
{
	BUFFER *bp;

	for (bp = bheadp; bp; bp = bp->b_bufp) {
		if (fncmp(bp->b_fname, fname) == 0) return bp;
	}

	return NULL;
}


/*
 * Mg3a: Make a unique buffer name for a file.
 */

static void
makeuniquename(char *bname, char *fname, INT buflen)
{
	char	num[80];
	unsigned long count = 1;
	INT 	numlen;

	makename(bname, fname, buflen);			/* New buffer name.	*/

	while (bfind(bname, FALSE) != NULL) {
		sprintf(num,"<%lu>", ++count);
		numlen = strlen(num);
		makename(bname, fname, buflen-numlen);
		strcat(bname, num);
		if (count >= 1000000) {
			panic("A million buffers!? Are you kidding?", 0);
		}
	}
}


/*
 * Given a file name, either find the buffer it uses, or create a new
 * empty buffer to put it in.
 */

BUFFER *
findbuffer(char *fname)
{
	BUFFER *bp;
	char	bname[NFILEN];

	if ((bp = findexistingbuffer(fname))) return bp;

	makeuniquename(bname, fname, NFILEN);			/* New buffer name.	*/

	return bfind(bname, TRUE);
}


/*
 * Mg3a: set charset for a buffer from the content. Not static to
 * allow testing.
 */

void
setcharsetfromcontent(BUFFER *bp)
{
	INT i, len;
	INT c;
	LINE *lp, *blp;
	size_t count_utf8, count_iso;
	charset_t charset = bp->charset;

	if (!termcharset) return;

	count_utf8 = 0;
	count_iso = 0;

	blp = bp->b_linep;
	lp = lforw(blp);

	while (lp != blp) {
		len = llength(lp);

		i = 0;
#ifdef INOUTOPT
		while (i <= len - (INT)sizeof(unsigned)) {
			unsigned *ip = (unsigned*)&ltext(lp)[i];		// Tag:scan
			if (*ip & (~0U/255*128)) break;
			i += sizeof(unsigned);
		}
#endif
		while (i < len) {
			if (lgetc(lp, i) < 128) {
				i++;
			} else {
				c = ucs_char(utf_8, lp, i, &i);

				if (c >= 0) {
					count_utf8++;
				} else {
					count_iso++;
				}
			}
		}

		lp = lforw(lp);
	}

	if (count_utf8 > count_iso) {
		charset = utf_8;
	} else if (count_iso > count_utf8) {
		charset = buf8bit;
	}

	if (bp->charset != charset) {
		bp->charset = charset;
		refreshbuf(bp);
	}
}


/*
 * Read the file "fname" into the current buffer. Make all of the text
 * in the buffer go away, after checking for unsaved changes. This is
 * called by the "read" command, the "visit" command, and the mainline
 * (for "mg file").
 */

INT
readin(char *fname)
{
	INT		status;
	WINDOW		*wp;

	if (bclear(curbp) != TRUE)		/* Might be old.	*/
		return TRUE;

#ifdef DIRED
	if (isdirectory(fname)) return readin_dired(curbp, fname) ? TRUE : FALSE;
#endif

	count_crlf = 0;
	count_lf = 0;

	status = insertfile(fname, fname) ;

	if (count_lf > count_crlf) {
		internal_unixlf(1);		/* Set Unix line endings */
	} else if (count_crlf > count_lf) {
		internal_unixlf(0);		/* Set MS-DOS line endings */
	}

	if (!termcharset) {
		internal_utf8(0);
		internal_bom(0);
	} else if (find_and_remove_bom(curbp)) {
		internal_utf8(1);
		internal_bom(1);
	} else {
		setcharsetfromcontent(curbp);
	}

	setmodefromfileextension(1);
	curbp->b_flag &= ~BFCHG;		/* No change.		*/

	// Reset all windows to beginning

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			wp->w_dotp   = wp->w_linep = lforw(curbp->b_linep);
			wp->w_doto   = 0;
			wp->w_markp  = NULL;
			wp->w_marko  = 0;
			wp->w_dotpos = 0;
		}
	}

	return status;
}

/*
 * Read the file "fname" into the current buffer. Make all of the text
 * in the buffer go away, after checking for unsaved changes. This is
 * called by the "read" command, the "visit" command, and the mainline
 * (for "mg file").
 */

INT
freadin(FILE *f)
{
	INT		status;
	WINDOW		*wp;

	if (bclear(curbp) != TRUE)		/* Might be old.	*/
		return TRUE;

	count_crlf = 0;
	count_lf = 0;

	status = finsertfile(f) ;

	if (count_lf > count_crlf) {
		internal_unixlf(1);		/* Set Unix line endings */
	} else if (count_crlf > count_lf) {
		internal_unixlf(0);		/* Set MS-DOS line endings */
	}

	if (!termcharset) {
		internal_utf8(0);
		internal_bom(0);
	} else if (find_and_remove_bom(curbp)) {
		internal_utf8(1);
		internal_bom(1);
	} else {
		setcharsetfromcontent(curbp);
	}

	setmodefromfileextension(1);
	curbp->b_flag &= ~BFCHG;		/* No change.		*/

	// Reset all windows to beginning

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			wp->w_dotp   = wp->w_linep = lforw(curbp->b_linep);
			wp->w_doto   = 0;
			wp->w_markp  = NULL;
			wp->w_marko  = 0;
			wp->w_dotpos = 0;
		}
	}

	return status;
}


/*
 * Insert a file in the current buffer, after dot. Set mark at the end
 * of the text inserted, point at the beginning. Return a standard
 * status. Print a summary (lines read, error message) out as well. If
 * the BACKUP conditional is set, then this routine also does the read
 * end of backup processing. The BFBAK flag, if set in a buffer, says
 * that a backup should be taken. It is set when a file is read in,
 * but not on a new file (you don't need to make a backup copy of
 * nothing).
 *
 * Mg3a: Scalable reading of a long line. Do not generate undo
 * records, but take care of undo errors.
 */

static INT
insertfile(char *fname, char *newname)
{
	INT		s;

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (isdir(fname)) return FALSE;
	if (notreadable(fname)) return FALSE;

	if (newname != NULL && newname != curbp->b_fname) {
		stringcopy(curbp->b_fname, newname, NFILEN);
	}

	s = ffropen(fname);

	if (s == FIOERR) {				/* Hard file open.	*/
		return FALSE;
	} else if (s == FIOFNF) {			/* File not found.	*/
		if (newname != NULL) {
			ewprintf("(New file)");
			return TRUE;
		} else {
			ewprintf("(File not found)");
			return FALSE;
		}
	}
	return doinsertfile(newname);
}

static INT	finsertfile(FILE *f)
{
	ffrset(f);
	return doinsertfile(NULL);
}

static INT doinsertfile(char *newname)
{
	INT		s, nbytes, maxbytes = 2*NLINE;
	size_t 		nline;
	LINE		*prevline;
	INT		savedoffset;
	size_t	savedpos;
	int		first = 1;
	char	*cp, *cp2;

	prevline = lback(curwp->w_dotp);
	savedoffset = curwp->w_doto;
	savedpos = curwp->w_dotpos;

	if (lnewline_noundo() == FALSE) return FALSE;

	adjustpos3(lback(curwp->w_dotp), savedoffset, savedpos);

	if ((cp = malloc_msg(maxbytes)) == NULL) return FALSE;

	nline = 0;			/* Don't count fake line at end */

	while ((s = ffgetline(cp, maxbytes, &nbytes)) != FIOERR) {
	    switch(s) {
	    case FIOSUC:
		++nline;
		/* and continue */
	    case FIOEOF:	/* the last line of the file		*/
lineread:
		if (first ?
			linsert_str_noundo(cp, nbytes) == FALSE :
			laddline(cp, nbytes) == FALSE)
		{
			s = FIOERR;
			goto endoffile;
		}

		first = 0;
		if (s == FIOEOF) goto endoffile;
		break;
	    case FIOLONG: {	/* a line to long to fit in our buffer	*/
		    INT i, nbytesmore;

		    nbytes = maxbytes;
		    for(;;) {
			if (nbytes == MAXINT) {
				ewprintf("Integer overflow reading a long line");
				s = FIOERR;
				goto endoffile;
			}

		    	nbytesmore = (nbytes>>1);

			if (nbytesmore > MAXINT - nbytes) {
				nbytesmore = MAXINT - nbytes;
			}

			maxbytes = nbytes + nbytesmore;

			if ((cp2 = realloc(cp, maxbytes)) == NULL) {
			    outofmem(maxbytes);
			    s = FIOERR;
			    goto endoffile;
			}
#if TESTCMD == 2
			{
				extern int count_realloc, count_alloc;
				if (cp2 == cp) count_realloc++; else count_alloc++;
			}
#endif
			cp = cp2;
			switch (s = ffgetline(cp + nbytes, nbytesmore, &i)) {
			    case FIOERR:
				goto endoffile;
			    case FIOLONG:
			    	nbytes += nbytesmore;
				break;
			    case FIOSUC:
				nline++;
			    case FIOEOF:
				nbytes += i;
				goto lineread;
			}
		    }
		}
	    default:
		ewprintf("Unknown code %d reading file", s);
		s = FIOERR;
		goto endoffile;
	    }
	}
endoffile:
	free(cp);

	ffclose();			/* Ignore errors.	*/

	if (s == FIOEOF) {		/* Don't zap an error.	*/
		if (nline == 1) ewprintf("(Read 1 line)");
		else		ewprintf("(Read %z lines)", nline);
	}

	if (newname != NULL) {
		curbp->b_flag |= BFCHG | BFBAK;	/* Need a backup.	*/
	} else {
		curbp->b_flag |= BFCHG;
	}

	if (ldelnewline_noundo() == FALSE) return FALSE;

	curwp->w_markp = curwp->w_dotp;
	curwp->w_marko = curwp->w_doto;

	adjustpos3(lforw(prevline), savedoffset, savedpos);

	if (s == FIOEOF) {
		return TRUE;
	} else {
		u_clear(curbp);
		return FALSE;
	}
}


/*
 * Take a file name, and from it fabricate a buffer name.
 *
 * Mg3a: only for Unix-style names
 */

static void
makename(char *bname, char *fname, INT buflen)
{
	char	*cp1;
	char	*cp2;
	INT 	c;
	INT 	i, nexti, len;

	cp1 = strrchr(fname, '/');
	cp2 = bname;

	if (cp1) {
		if (cp1[1]) cp1++;
	} else {
		cp1 = fname;
	}

	i = 0;
	len = strlen(cp1);

	while (i < len) {
		c = ucs_strtochar(termcharset, cp1, len, i, &nexti);

		if (nexti >= buflen) break;

		if (c == '\t') {
			*cp2++ = ' ';
		} else {
			while (i < nexti) {
				*cp2++ = cp1[i++];
			}
		}

		i = nexti;
	}

	*cp2 = 0;
}


/*
 * Ask for a file name, and write the contents of the current buffer
 * to that file. Update the remembered file name and clear the buffer
 * changed flag. This handling of file names is different from the
 * earlier versions, and is more compatable with Gosling EMACS than
 * with ITS EMACS.
 *
 * Mg3a: Change the buffer name. Don't change file or buffer name for
 * Dired. Change mode from file extension if applicable.
 */

INT
filewrite(INT f, INT n)
{
	INT	s;
	char	fname[NFILEN];
	char	*adjfname, bname[NFILEN], *cp;
	const	INT flagstosave = (BFBOM|BFUNIXLF);
	INT	savedflags;
	charset_t savedcharset;

	if ((s=eread("Write file: ", fname, NFILEN, EFFILE)) != TRUE)
		return (s);

	if ((adjfname = adjustname(fname)) == NULL) return FALSE;

	if ((s=writeout(curbp, adjfname)) == TRUE && !(curbp->b_flag & BFDIRED)) {
		cp = curbp->b_bname;	// Save away name in case name is the same
		curbp->b_bname = "";
		makeuniquename(bname, adjfname, NFILEN);
		curbp->b_bname = cp;

		if ((cp = string_dup(bname)) == NULL) return FALSE;

		free(curbp->b_bname);
		curbp->b_bname = cp;

		stringcopy(curbp->b_fname, adjfname, NFILEN);

		// UNDO
		if (curbp->b_flag & BFSYS) u_clear(curbp);
		//

		curbp->b_flag &= ~(BFCHG|BFBAK|BFSYS|BFSCRATCH|BFREADONLY);

		if (setmodefromfileextension(0)) {
			savedflags = curbp->b_flag;
			savedcharset = curbp->charset;
			resetbuffer(curbp);
			curbp->b_flag = (curbp->b_flag & ~flagstosave) | (savedflags & flagstosave);
			curbp->charset = savedcharset;
			setmodefromfileextension(1);
		} else {
			upmodes(curbp);
		}
	}

	return s;
}


/*
 * Save the contents of the current buffer back into its associated
 * file.
 */

#ifndef	MAKEBACKUP
#define	MAKEBACKUP TRUE
#endif

INT	makebackup = MAKEBACKUP;


INT
filesave(INT f, INT n)
{
	return buffsave(curbp);
}


/*
 * Save the contents of the buffer argument into its associated file.
 * Do nothing if there have been no changes (is this a bug, or a
 * feature). Error if there is no remembered file name. If this is the
 * first write since the read or visit, then a backup copy of the file
 * is made. Allow user to select whether or not to make backup files
 * by looking at the value of makebackup.
 */

INT
buffsave(BUFFER *bp)
{
	INT	s;

	if ((emacs_compat & 2) && (bp->b_flag&BFCHG) == 0)	{	/* Return, no changes.	*/
		ewprintf("(No changes need to be saved)");
		return TRUE;
	}
	if (bp->b_fname[0] == '\0') {		/* Must have a name.	*/
		ewprintf("No file name");
		return (FALSE);
	}
	if (get_variable(bp->localvar.v.make_backup, makebackup) && (bp->b_flag & BFBAK)) {
		s = fbackupfile(bp->b_fname);
		if (s == ABORT)			/* Hard error.		*/
			return FALSE;
		if (s == FALSE			/* Softer error.	*/
		&& (s=eyesno("Backup error, save anyway")) != TRUE)
			return s;
	}
	if ((s=writeout(bp, bp->b_fname)) == TRUE) {
		bp->b_flag &= ~(BFCHG | BFBAK);
		upmodes(bp);
	}
	return s;
}

/*
 * Mg3a: Compatibility. There is a variable "make-backup".
 */

INT
makebkfile(INT f, INT n)
{
	if(f & FFARG) makebackup = n > 0;
	else makebackup = !makebackup;
	ewprintf("Backup files %sabled", makebackup ? "en" : "dis");
	return TRUE;
}


INT trim_whitespace = 0;	/* Trim trailing whitespace */


/*
 * This function performs the details of file writing; writing the
 * file in buffer bp to file fn. Uses the file management routines in
 * the "fileio.c" package. Most of the grief is checking of some sort.
 */

static INT
writeout(BUFFER *bp, char *fn)
{
	INT	s;

	if (get_variable(bp->localvar.v.trim_whitespace, trim_whitespace) &&
		!(bp->b_flag & BFREADONLY))
	{
		internal_deletetrailing(bp, 0);
	}

	if ((s=ffwopen(fn)) != FIOSUC)		/* Open writes message. */
		return (FALSE);
	s = ffputbuf(bp);
	if (s == FIOSUC) {			/* No write error.	*/
		s = ffclose();
		if (s==FIOSUC)
			ewprintf("Wrote %s", fn);
		else
			ewprintf("Error closing file: %s", strerror(errno));
	} else					/* Ignore close error	*/
		ffclose();		/* if a write error.	*/
	return (s == FIOSUC) ? TRUE : FALSE;
}


/*
 * Tag all windows for bp (all windows if bp NULL) as needing their
 * mode line updated.
 */

void
upmodes(BUFFER *bp)
{
	WINDOW *wp;

	for (wp = wheadp; wp != NULL; wp = wp->w_wndp)
		if (bp == NULL || wp->w_bufp == bp) wp->w_flag |= WFMODE;
}


/*
 * Mg3a: Tag all windows for bp (all windows if bp NULL) as needing
 * their mode line and content updated.
 */

void
refreshbuf(BUFFER *bp)
{
	WINDOW *wp;

	for (wp = wheadp; wp != NULL; wp = wp->w_wndp)
		if (bp == NULL || wp->w_bufp == bp) wp->w_flag |= WFMODE|WFHARD;
}


/*
 * Mg3a: general update of window flag for all windows associated with bp
 */

void
winflag(BUFFER *bp, INT flag)
{
	WINDOW *wp;

	for (wp = wheadp; wp != NULL; wp = wp->w_wndp)
		if (bp == NULL || wp->w_bufp == bp) wp->w_flag |= flag;
}


/*
 * Mg3a: Move all dots in this buffer to the top.
 */

BUFFER *
buftotop(BUFFER *bp)
{
    WINDOW *wp;

    bp->b_dotp = lforw(bp->b_linep);
    bp->b_doto = 0;
    bp->b_dotpos = 0;

    if(bp->b_nwnd != 0) {
	for(wp = wheadp; wp!=NULL; wp = wp->w_wndp)
	    if(wp->w_bufp == bp) {
		wp->w_dotp = bp->b_dotp;
		wp->w_doto = 0;
		wp->w_dotpos = 0;
		wp->w_flag |= WFMOVE;
	    }
    }

    return bp;
}


/*
 * Mg3a: Undo on the whole buffer. Return success status.
 */

// UNDO
static int
undo_whole_buffer(int flag)
{
	if (!undo_enabled) {
		return 1;
	} else if (curbp->b_size > MAXINT) {
		u_clear(curbp);
		return 1;
	}

	return u_entry_range(flag, lforw(curbp->b_linep), 0, curbp->b_size, 0);
}
//

/*
 * Mg3a: Revert a buffer to the content of the file on disk.
 */

INT
filerevert(INT f, INT n)
{
	INT s;
	winpos_t w;
	int emacs_compatible = !(f & FFRAND);

#ifdef DIRED
	if (curbp->b_flag & BFDIRED) {
		getwinpos(&w);

		resetbuffer(curbp);

		s = readin_dired(curbp, curbp->b_fname) ? TRUE : FALSE;

		if (s == TRUE) {
			setwinpos(w);
			ewprintf("Re-read directory %s", curbp->b_fname);
		}

		return s;
	}
#endif

	if (curbp->b_flag & BFREADONLY) return readonly();

	if (curbp->b_fname[0] == 0) {
		ewprintf("Buffer has no associated file");
		return FALSE;
	}

	if (!(f & FFUNIV)) {
		if ((s = eyesno("Revert buffer from file")) != TRUE) return s;
	}

	getwinpos(&w);

	// UNDO
	if (emacs_compatible) {
		u_modify();
		if (!undo_whole_buffer(UFL_DELETE)) return FALSE;
	} else {
		u_clear(curbp);
	}
	//

	curbp->b_flag &= ~BFCHG;
	bclear(curbp);

	resetbuffer(curbp);

	s = readin(curbp->b_fname);

	// UNDO
	if (emacs_compatible && !undo_whole_buffer(UFL_INSERT)) u_clear(curbp);
	//

	if (s == TRUE) {
		setwinpos(w);
	}

	refreshbuf(curbp);

	return s;
}


/*
 * Mg3a: A better revert
 */

INT
filerevert_forget(INT f, INT n)
{
	return filerevert(f | FFRAND, n);
}
