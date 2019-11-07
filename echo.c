/*
 *		Echo line reading and writing.
 *
 * Common routines for reading and writing characters in the echo line
 * area of the display screen. Used by the entire known universe.
 */

#include	"def.h"
#include	"key.h"
#include 	"kbd.h"
#include	"macro.h"

#include 	<stdarg.h>
#include	<dirent.h>

static INT	veread(char *fp, char *buf, INT nbuf, INT flag, va_list ap);
static INT	complt(INT flags, INT c, char *buf, INT nbuf, INT cpos, INT startcol);
static INT	complt_msg(char *msg);
static void	eformat(char *fp, va_list ap);
static void	eputi(INT i, INT r);
static void	eputs(char *s);
static void	eputc(INT c);
static void	truncated_puts(char *s, INT maxwidth);

INT	ctrlg(INT f, INT n);

INT	epresf	= FALSE;		/* Stuff in echo line flag.	*/
INT	macro_show_message = 1;		/* Show messages even in macro	*/

/*
 * Erase the echo line.
 */

void
eerase() {
	if (macro_show_message <= 0) return;

	ttcolor(CTEXT);
	ttmove(nrow - 1, 0);
	tteeol();
	ttflush();
	epresf = FALSE;
}


/*
 * Mg3a: Position to an emptied echo line
 */

void
estart()
{
	if (macro_show_message <= 0) return;

	eerase();
	ttmove(nrow - 1, 0);
}


/*
 * Ask "yes" or "no" question. Return ABORT if the user answers the
 * question with the abort ("^G") character. Return FALSE for "no" and
 * TRUE for "yes". No formatting services are available. No newline
 * required.
 */

INT
eyorn(char *sp)
{
	INT	s;

#ifndef NO_MACRO
	if(inmacro) return TRUE;
#endif
	estart();
	truncated_puts(sp, ncol - 14);
	ewprintfc("? (y or n) ");
	ttflush();
	for (;;) {
		s = getkey(FALSE);
		if (s == 'y' || s == 'Y') return TRUE;
		if (s == 'n' || s == 'N') return FALSE;
		if (s == CCHR('G')) return ctrlg(FFRAND, 1);
		estart();
		ewprintfc("Please answer y or n.  ");
		truncated_puts(sp, ncol - 36);
		ewprintfc("? (y or n) ");
		ttflush();
	}
	/*NOTREACHED*/
}


/*
 * Like eyorn, but for more important question. User must type either
 * all of "yes" or "no", and the trainling newline.
 */

INT
eyesno(char *sp)
{
	INT	s;
	char	buf[64];

#ifndef NO_MACRO
	if(inmacro) return TRUE;
#endif
	s = ereply("%s? (yes or no) ", buf, sizeof(buf), sp);
	for (;;) {
		if (s == ABORT) return ABORT;
#ifndef NO_MACRO
		if (macrodef) {
			LINE *lp = maclcur;

			maclcur = lback(lp);
			removefromlist(lp);
			free(lp);
		}
#endif
		if (ascii_strcasecmp(buf, "yes") == 0) return TRUE;
		if (ascii_strcasecmp(buf, "no") == 0) return FALSE;

		s = ereply("Please answer yes or no.  %s? (yes or no) ",
			   buf, sizeof(buf), sp);
	}
	/*NOTREACHED*/
}


/*
 * Write out a prompt, and read back a reply. The prompt is now
 * written out with full "ewprintf" formatting, although the arguments
 * are in a rather strange place. This is always a new message.
 */

INT
ereply(char *prompt, char *buf, INT nbuf, ...)
{
	va_list pvar;
	INT i;

	va_start(pvar, nbuf);
	i = veread(prompt, buf, nbuf, 0, pvar);
	va_end(pvar);
	return i;
}


/*
 * This is the general "read input from the echo line" routine. The
 * basic idea is that the prompt string "prompt" is written to the
 * echo line, and a one line reply is read back into the supplied
 * "buf" (with maximum length "nbuf"). The flag control how things are
 * read.
 */

INT
eread(char *prompt, char *buf, INT nbuf, INT flag, ...)
{
	va_list pvar;
	INT i;

	va_start(pvar, flag);
	i = veread(prompt, buf, nbuf, flag, pvar);
	va_end(pvar);
	return i;
}


/*
 * Mg3a: Return the next character from the string.
 */

static INT
nextch(char *str, INT len, INT offset, INT *outoffset)
{
	return ucs_strtochar(termcharset, str, len, offset, outoffset);
}


/*
 * Mg3a: Return the offset of the previous character.
 */

static INT
oneback(char *buf, INT cpos)
{
	if (cpos > 0) cpos--;

	if (termisutf8) {
		while (cpos > 0 && (buf[cpos] & 0xc0) == 0x80) cpos--;
	}

	return cpos;
}


static INT shiftleft = 0, shiftright = 0;	/* Shifting for the input editor */


/*
 * Mg3a: Redisplay all the entered text and set the shift limits.
 */

static void
redisplayall(char *str, INT len, INT startcol, INT flash)
{
	INT i, nexti, col, width, w, screenw, leftedge, screenw3;

	width = ucs_termwidth_str(termcharset, str, len);

	screenw = ncol - startcol - 1;
//	screenw3 = screenw >> 3;
//	screenw3 = screenw >> 2;
	screenw3 = screenw >> 1;
//	screenw3 = 3*screenw/4;

	shiftright = ncol - 1;

	if (screenw <= 0) return;

	if (width <= screenw) {
		leftedge = 0;
		shiftleft = -1;
	} else {
		leftedge = width - (width - screenw3 - 1) % (screenw - screenw3) - screenw3;
		shiftleft = startcol + screenw3;
	}

	i = 0;
	w = 0;

	while (i < len && w < leftedge) {
		w += ucs_termwidth(ucs_strtochar(termcharset, str, len, i, &i));
	}

	while (i < len && ucs_termwidth(ucs_strtochar(termcharset, str, len, i, &nexti)) == 0) {
		i = nexti;
	}

	ttmove(nrow - 1, startcol);

	if (flash) {
		tteeol();
		ttmove(nrow - 1, startcol);
	}

	if (leftedge > 0) eputc('$');

	while (w-- > leftedge) eputc(' ');

	while (i < len) {
		eputc(ucs_strtochar(termcharset, str, len, i, &i));
	}

	if (!flash) {
		col = ttcol;
		tteeol();
		ttmove(nrow - 1, col);
	}
}


/*
 * Mg3a: Erase one (possibly combined) character backward and return
 * the new offset.
 */

static INT
eraseoneback(char *buf, INT nbuf, INT cpos)
{
	INT w, c;

	while (cpos > 0) {
		cpos = oneback(buf, cpos);
		c = nextch(buf, nbuf, cpos, NULL);
		w = ucs_termwidth(c);

		while (w-- > 0) {
			if (ttcol > shiftleft) {
				ttputc('\b');
				ttputc(' ');
				ttputc('\b');
				ttcol--;
			}
		}

		if (!ucs_nonspacing(c)) break;
	}

	return cpos;
}


/*
 * Mg3a: Redisplay from a safe point, if needed
 */

static void
redisplay(char *str, INT len, INT startcol)
{
	INT col, safeidx, safecol, newcol;
	INT c;
	INT i, nexti;
	extern INT preserve_ligatures;

	if (ttcol <= shiftleft || ttcol > shiftright) {
		redisplayall(str, len, startcol, 0);
		return;
	}

	if (!preserve_ligatures) return;

	col = 0;
	i = 0;
	safeidx = 0;
	safecol = col;

	while (i < len) {
		c = ucs_strtochar(termcharset, str, len, i, &nexti);

		// Do the preserve ligatures thing

		if (c < 0x300 || (c >= 0x4e00 && c < 0xa000)) {
			safeidx = i;
			safecol = col;
		}

		col += ucs_termwidth(c);
		i = nexti;
	}

	newcol = ttcol - (col - safecol);

	if (newcol <= shiftleft || newcol > shiftright) {
		redisplayall(str, len, startcol, 0);
		return;
	}

	ttmove(nrow - 1, newcol);

	i = safeidx;

	while (i < len) {
		eputc(ucs_strtochar(termcharset, str, len, i, &i));
	}
}


/*
 * Mg3a: Find the full filename of
 */

static char*
fullfilenameof(char *buf)
{
	char *p;

	p = strstr(buf, "//");
	if (p == buf || !p) p = strstr(buf, "/~");

	if (p) return p + 1;
	else return buf;
}


/* Whether to auto-insert the directory when asking for a file */

INT insert_default_directory = 1;


/*
 * Mg3a: The general read routine
 */

static INT
veread(char *fp, char *buf, INT nbuf, INT flag, va_list ap)
{
	INT	cpos;
	INT	i, status;
	INT	c;
	unsigned char	charbuf[4];
	INT	charlen;
	INT	startcol;
	INT	meta;

#ifndef NO_MACRO
	if(inmacro) {
	    if (outofarguments(1)) return ABORT;

	    if (llength(maclcur) >= nbuf)
	    	return emessage(ABORT, "Argument too long");

	    memcpy(buf, ltext(maclcur), llength(maclcur));

	    buf[llength(maclcur)] = '\0';
	    maclcur = lforw(maclcur);

	    return buf[0] ? TRUE : FALSE;
	}
#endif
	cpos = 0;
	ttcolor(CTEXT);
	ttmove(nrow - 1, 0);
	epresf = TRUE;
	eformat(fp, ap);
	startcol = ttcol;
	if ((flag & EFFILE) && insert_default_directory) {
		cpos = stringcopy(buf, dirnameofbuffer(curbp), nbuf);
	}
	// redisplayall clears the screen, sets shiftleft and shiftright
	redisplayall(buf, cpos, startcol, 0);
	ttflush();
	for (;;) {
		c = getkey(FALSE);
		if ((meta = (c == METACH))) c = getkey(FALSE);
		if (!meta && c == CCHR('I') && cpos == 0) {
			complt(flag, c, buf, nbuf, cpos, startcol);
			cpos = strlen(buf);
			continue;
		} else if ((flag & EFAUTO) && (c == ' ' || c == CCHR('I')) &&
			!(c == ' ' && !meta && !(flag & EFAUTOSPC)))
		{
			complt(flag, c, buf, nbuf, cpos, startcol);
			cpos = strlen(buf);
			continue;
		} else if (meta) switch (c) {
		    case METACH:
		    	c = CCHR('U');
			break;
		    default:
		    	// All invalid escape sequences end up here
		    	while (type_ahead()) ttgetc();
			complt_msg(" [no binding]");
			continue;
		}
		switch (c) {
		    case CCHR('J'):
			c = CCHR('M');		/* and continue		*/
		    case CCHR('M'):		/* Return, done.	*/
			if (cpos != 0 && ((flag & EFAUTOCR) || flag == 0)) {
				status = complt(flag, c, buf, nbuf, cpos, startcol);
				cpos = strlen(buf);
				if (!(status & MATCHFULL)) continue;
			}

			buf[cpos] = '\0';

			if (flag & EFFILE) {
				// Overlapping copy allowed by stringcopy
				stringcopy(buf, fullfilenameof(buf), nbuf);
				cpos = strlen(buf);
			}

			if (macrodef) {
			    LINE *lp;

			    if ((lp = lalloc(cpos)) == NULL) {
				// FALSE could be interpreted as a default response
				// so a quiet ABORT is needed.
				thisflag |= CFNOQUIT;
				return ABORT;
			    }

			    insertafter(maclcur, lp);
			    maclcur = lp;

			    maclcur->l_flag = macrocount;
			    memcpy(ltext(lp), buf, cpos);
			}

			goto done;

		    case CCHR('G'):		/* Bell, abort.		*/
			eputc(CCHR('G'));
			ctrlg(FFRAND, 0);
			ttflush();
			return ABORT;

		    case CCHR('H'):
		    case CCHR('?'):		/* Rubout, erase.	*/
			if (cpos > 0) {
				cpos = eraseoneback(buf, nbuf, cpos);
				redisplay(buf, cpos, startcol);
				ttflush();
			}
			break;

		    case CCHR('R'):
		    	redisplayall(buf, cpos, startcol, 1);
			ttflush();
			break;

		    case CCHR('X'):		/* C-X			*/
		    case CCHR('U'):		/* C-U, kill line.	*/
		    	cpos = 0;
			redisplayall(buf, cpos, startcol, 1);
			ttflush();
			break;

		    case CCHR('W'):		/* C-W, kill to beginning of */
						/* previous word	*/
			/* back up to first word character or beginning */
			while ((cpos > 0) && !ucs_isword(nextch(buf, nbuf, oneback(buf, cpos), NULL))) {
				cpos = eraseoneback(buf, nbuf, cpos);
			}
			while ((cpos > 0) && ucs_isword(nextch(buf, nbuf, oneback(buf, cpos), NULL))) {
				cpos = eraseoneback(buf, nbuf, cpos);
			}
			redisplay(buf, cpos, startcol);
			ttflush();
			break;

		    case CCHR('\\'):
		    case CCHR('Q'):		/* C-Q, quote next	*/
			c = getkey(FALSE);	/* and continue		*/
		    default:			/* All the rest.	*/
#ifdef UTF8
			status = ucs_chartostr(termcharset, c, charbuf, &charlen);

			if (status == TRUE && cpos + charlen < nbuf &&
				!(cpos == 0 && ucs_nonspacing(c)) && c != 0)
			{
				for (i = 0; i < charlen; i++) {
					buf[cpos++] = charbuf[i];
				}

				eputc(c);

				if (ttcol > shiftright) redisplayall(buf, cpos, startcol, 0);

				ttflush();
			}
#else
			if (cpos < nbuf-1) {
				buf[cpos++] = (char) c;
				eputc((char) c);
				ttflush();
			}
#endif
		}
	}
done:	return buf[0] ? TRUE : FALSE;
}


/*
 * Mg3a: Deliver next buffer name for completion.
 */

static char *
getnext_bufname(INT first)
{
	static BUFFER *bp = NULL;

	if (first) bp = bheadp;
	else if (!bp->b_bufp) return NULL;
	else bp = bp->b_bufp;

	return bp->b_bname;
}


/*
 * Mg3a: Deliver next charset name for completion.
 */

static char *
getnext_charsetname(INT first)
{
	static charset_entry *p = NULL;

	if (first) p = &charsets[0];
	else if (!p->name) return NULL;
	else p++;

	return p->name;
}


/*
 * Mg3a: Deliver next file name in open directory "dir" for
 * completion.
 */

static DIR *dir;

static char *
getnext_filename(INT first)
{
	struct dirent *direntry;

	direntry = readdir(dir);

#ifdef NO_O
	while (direntry != NULL) {
		char *ext;

		ext = strrchr(direntry->d_name, '.');

		if (!ext || strcmp(ext, ".o") != 0) break;

		direntry = readdir(dir);
	}
#endif

	if (direntry == NULL) {
		closedir(dir);
		return NULL;
	}

	return direntry->d_name;
}


/*
 * Mg3a: Deliver next function name for completion.
 */

static char *
getnext_funcname(INT first)
{
	static FUNCTNAMES *p = NULL;

	if (first) p = &functnames[0];
	else if (p >= &functnames[nfunct-1]) return NULL;
	else p++;

	while (p->n_flag & FUNC_HIDDEN) {
		if (p >= &functnames[nfunct-1]) return NULL;
		p++;
	}

	return p->n_name;
}


#ifdef USER_MACROS

/*
 * Mg3a: Deliver next function or macro name for completion.
 */

static char *
getnext_funcmacroname(INT first)
{
	static INT infuncs;
	char *s;

	if (first) infuncs = 1;

	if (infuncs) {
		s = getnext_funcname(first);

		if (s) return s;

		infuncs = 0;
		return getnext_macro(1);
	} else {
		return getnext_macro(0);
	}
}

#endif

/* Case folding for completion of file names and buffer names */

#ifdef __CYGWIN__
INT complete_fold_file = 1;
#else
INT complete_fold_file = 0;
#endif


/*
 * Mg3a: Error display for complt().
 */

static INT
complt_msg(char *msg)
{
	INT msglen, nshown, i;

	/* Set up backspaces, etc., being mindful of echo line limit */
	msglen = strlen(msg);
	nshown = (ttcol + msglen >= ncol) ?
			ncol - ttcol - 1 : msglen;
	for (i = 0; i < nshown; i++)
		ttputc(msg[i]);		/* output msg			*/
	i = nshown;
	while (i-- > 0)			/* move back before msg		*/
		ttputc('\b');
	ttflush();			/* display to user		*/
	ungetkey(getkey(FALSE));	/* stop and restart		*/
	i = nshown;
	while (i-- > 0)			/* blank out			*/
		ttputc(' ');
	i = nshown;
	while (i-- > 0)			/* and back up			*/
		ttputc('\b');
	return 0;
}


/*
 * Mg3a: default a completion
 */

static INT
complt_default(char *buf, INT nbuf, INT startcol, char *defaultstr)
{
	if (defaultstr) stringcopy(buf, defaultstr, nbuf);
	redisplayall(buf, strlen(buf), startcol, 0);
	ttflush();
	return 0;
}


/*
 * Mg3a: Normalize slashes on Cygwin
 */

static void
normalize_slashes(char *buf)
{
#ifdef __CYGWIN__
	while (*buf) {
		if (*buf == '\\') *buf = '/';
		buf++;
	}
#endif
}


/*
 * Do completion on a list of objects.
 */

static INT
complt(INT flags, INT c, char *buf, INT nbuf, INT cpos, INT startcol)
{
	INT		i, like;
	char		completebuf[NFILEN];
	INT		count, bufcount, completecount;
	INT		completelen, c2, status;
	char 		dirname[NFILEN], *p;
	static char	lastvar[VARLEN] = "", lastlocalvar[VARLEN] = "";
	static char	lastcmd[NXNAME] = "", lastkeymap[MODENAMELEN] = "";
	static char	lastreplace[NPAT] = "", lastmisc[NFILEN] = "";
	char		*(*fm)(INT first);
	int		addspace = 0;
#ifdef UCSNAMES
	static char	lastunicode[UNIDATALEN] = "";
#endif

	buf[cpos] = 0;

	bufcount = ucs_strchars(termcharset, buf);
	completecount = 0;
	status = 0;

	if (flags & (EFFUNC|EFMACRO)) {
		if (cpos == 0 && lastcmd[0]) {
			return complt_default(buf, nbuf, startcol, lastcmd);
		}

#ifdef USER_MACROS
		if ((flags & EFMACRO) == 0) fm = getnext_funcname;
		else if ((flags & EFFUNC) == 0) fm = getnext_macro;
		else fm = getnext_funcmacroname;
#else
		fm = getnext_funcname;
#endif
		completecount = ucs_complete(buf, completebuf, sizeof(completebuf),
			fm, 0, &status);
		if (status & MATCHFULL) {
			stringcopy(lastcmd, completebuf, sizeof(lastcmd));
		}
	} else if (flags & EFBUF) {
		if (cpos == 0) {
			return complt_default(buf, nbuf, startcol, "*scratch*");
		}
		completecount = ucs_complete(buf, completebuf, sizeof(completebuf),
			getnext_bufname, complete_fold_file, &status);
	} else if (flags & EFCHARSET) {
		if (cpos == 0 && bufdefaultcharset) {
			return complt_default(buf, nbuf, startcol,
				charsettoname(bufdefaultcharset));
		}
		completecount = ucs_complete(buf, completebuf, sizeof(completebuf),
			getnext_charsetname, 1, &status);
	} else if (flags & EFVAR) {
		if (cpos == 0 && lastvar[0]) {
			return complt_default(buf, nbuf, startcol, lastvar);
		}
		completecount = ucs_complete(buf, completebuf, sizeof(completebuf),
			getnext_varname, 0, &status);
		if (status & MATCHFULL) {
			stringcopy(lastvar, completebuf, sizeof(lastvar));
		}
	} else if (flags & EFLOCALVAR) {
		if (cpos == 0 && lastlocalvar[0]) {
			return complt_default(buf, nbuf, startcol, lastlocalvar);
		}
		completecount = ucs_complete(buf, completebuf, sizeof(completebuf),
			getnext_localvarname, 0, &status);
		if (status & MATCHFULL) {
			stringcopy(lastlocalvar, completebuf, sizeof(lastlocalvar));
		}
	} else if (flags & EFKEYMAP) {
		if (cpos == 0 && lastkeymap[0]) {
			return complt_default(buf, nbuf, startcol, lastkeymap);
		}
		completecount = ucs_complete(buf, completebuf, sizeof(completebuf),
			getnext_mapname, 0, &status);
		if (status & MATCHFULL) {
			stringcopy(lastkeymap, completebuf, sizeof(lastkeymap));
		}
#ifdef UCSNAMES
	} else if (flags & EFUNICODE) {
		if (c == CCHR('M')) {
			stringcopy(lastunicode, buf, sizeof(lastunicode));
			return MATCHFULL;
		}
		if (cpos == 0 && lastunicode[0]) {
			return complt_default(buf, nbuf, startcol, lastunicode);
		}
		if (ascii_strcasecmp(buf, "euro") == 0) {
			return complt_default(buf, nbuf, startcol, "EURO SIGN");
		}
		completecount = ucs_complete(buf, completebuf, sizeof(completebuf),
			getnext_unicode, 1, &status);
#endif
	} else if (flags & EFSEARCH) {
		if (cpos == 0) return complt_default(buf, nbuf, startcol, pat);
		return 0;
	} else if (flags & EFREPLACE) {
		if (c == CCHR('M')) {
			stringcopy(lastreplace, buf, sizeof(lastreplace));
			return MATCHFULL;
		}
		if (cpos == 0) return complt_default(buf, nbuf, startcol, lastreplace);
		return 0;
	} else if (flags & EFFILE) {
		if (cpos == 0) return complt_default(buf, nbuf, startcol, dirnameofbuffer(curbp));

		normalize_slashes(buf);

		p = adjustname(fullfilenameof(buf));

		if (p) {
			stringcopy(dirname, p, sizeof(dirname));
			p = filenameof(dirname);
			*p = 0;

			if ((dir = opendir(dirname)) != NULL) {
				p = filenameof(buf);
				bufcount = ucs_strchars(termcharset, p);
				completecount = ucs_complete(p, completebuf, sizeof(completebuf),
					getnext_filename, complete_fold_file, &status);
			} else {
				complt_msg(" [Error opening directory]");
				return MATCHERR;
			}
		} else {
			return MATCHERR;
		}
	} else {
		if (c == CCHR('M')) {
			stringcopy(lastmisc, buf, sizeof(lastmisc));
			return MATCHFULL;
		}
		if (cpos == 0) return complt_default(buf, nbuf, startcol, lastmisc);
		return 0;
	}

	if (status & MATCHNONE) {
		if ((flags & EFNOSTRICTSPC) && c == ' ') {
			addspace = 1;
		} else if ((flags & (EFNOSTRICTCR|EFCHARSET)) && c == CCHR('M')) {
			return MATCHFULL;
		} else if ((flags & EFFUNC) && name_function(buf)) {
			// Functions that are hidden can still be entered
			return MATCHFULL;
		} else {
			complt_msg(" [No match]");
			return status;
		}
	}

	completelen = strlen(completebuf);
	i = 0;

	for (count = 0; count < bufcount; count++) {
		ucs_strtochar(termcharset, completebuf, completelen, i, &i);
	}

	while (count < completecount) {
		c2 = ucs_strtochar(termcharset, completebuf, completelen, i, &i);
		count++;
		if (c == ' ' && !ucs_isword(c2)) break;
	}

	completebuf[i] = 0;

	if (flags & EFFILE) {
		p = filenameof(buf);

		like = (strncmp(p, completebuf, &buf[cpos] - p) == 0);

		*p = 0;
		stringconcat(buf, completebuf, nbuf);

		p = adjustname(fullfilenameof(buf));

		if (p && isdirectory(p)) {
			stringconcat(buf, dirname_addslash(buf), nbuf);
		}
	} else {
		like = (strncmp(buf, completebuf, cpos) == 0);

		stringcopy(buf, completebuf, nbuf);
	}

	if (addspace) stringconcat(buf, " ", nbuf);

	if (like) eputs(&buf[cpos]);

	if (!like || ttcol > shiftright) {
		redisplayall(buf, strlen(buf), startcol, 0);
	}

	ttflush();

	if ((status & MATCHAMBIGUOUS) && !(c == CCHR('M') && (status & MATCHFULL))) {
		complt_msg(" [Ambiguous]");
	}

	return status;
}


/*
 * Special "printf" for the echo line. Each call to "ewprintf" starts
 * a new line in the echo area, and ends with an erase to end of the
 * echo line. The formatting is done by a call to the standard
 * formatting routine.
 */

void
ewprintf(char *fp, ...)
{
	va_list pvar;

	if (macro_show_message <= 0) return;

	va_start(pvar, fp);
	ttcolor(CTEXT);
	ttmove(nrow - 1, 0);
	eformat(fp, pvar);
	va_end(pvar);
	tteeol();
	ttflush();
	epresf = TRUE;
}


/*
 * Mg3a: Special "printf" for the echo line. Each call to "ewprintfc"
 * continues where it last was in the echo area, and does not erase to
 * the end of line. The formatting is done by a call to the standard
 * formatting routine.
 */

void
ewprintfc(char *fp, ...)
{
	va_list pvar;

	if (macro_show_message <= 0) return;

	va_start(pvar, fp);
	eformat(fp, pvar);
	va_end(pvar);
	epresf = TRUE;
}


/*
 * Printf style formatting. This is called by both "ewprintf" and
 * "ereply" to provide formatting services to their clients. The move
 * to the start of the echo line, and the erase to the end of the echo
 * line, is done by the caller.
 *
 * Mg3a:
 *
 * Parameters are expected to be INT, except for %s, %z, %j, %S.
 *
 * %c is a byte, expressed as ASCII, a name, C-x, or octal
 * %C is a Unicode character, expressed literally
 * %k is a key sequence (without parameters)
 * %K is a key name
 * %U is a Unicode character, expressed as U+xxxx
 *
 * %d is a decimal integer
 * %o is an octal integer
 * %x is a hexadecimal integer (lowercase)
 *
 * %s is a string, in the charset termcharset
 * %z is a size_t, as an unsigned decimal integer
 * %j is an intmax_t, as a decimal integer
 *
 * %S is a string, limited to TRUNCNAME columns
 */

static void
eformat(char *fp, va_list ap)
{
	INT 	c;
	char	kname[80];
	INT 	i, len;


	i = 0;
	len = strlen(fp);

	while (i < len) {
		c = nextch(fp, len, i, &i);

		if (c != '%')
			eputc(c);
		else {
			if (i >= len) return;

			c = nextch(fp, len, i, &i);

			switch (c) {
			case 'c':
				c = va_arg(ap, INT);
				if (c < 128) {
					keyname(kname, c);
				} else {
					sprintf(kname, "0%o", (int)c);
				}
				eputs(kname);
				break;

			case 'C':
				eputc(va_arg(ap, INT));
				break;

			case 'U':
				sprintf(kname, "U+%04X", (int)va_arg(ap, INT));
				eputs(kname);
				break;

			case 'k':
				for (c = 0; c < key.k_count; c++) {
					if (c > 0) eputc(' ');
					keyname(kname, key.k_chars[c]);
					eputs(kname);
				}
				break;

			case 'K':
				keyname(kname, va_arg(ap, INT));
				eputs(kname);
				break;

			case 'd':
				eputi(va_arg(ap, INT), 10);
				break;

			case 'o':
				eputi(va_arg(ap, INT), 8);
				break;

			case 'x':
				eputi(va_arg(ap, INT), 16);
				break;

			case 's':
				eputs(va_arg(ap, char *));
				break;

			case 'z': /* size_t */
				sprintf(kname, "%zu", va_arg(ap, size_t));
				eputs(kname);
				break;

			case 'j': /* intmax_t */
				sprintf(kname, "%jd", va_arg(ap, intmax_t));
				eputs(kname);
				break;

			case 'S':
				truncated_puts(va_arg(ap, char *), TRUNCNAME);
				break;

			default:
				eputc(c);
			}
		}
	}
}

/*
 * Put integer, in radix "r".
 */

static void
eputi(INT i, INT r)
{
	INT q;

	if (i < 0) {
	    if (i + MAXINT < 0) {eputs("MININT"); return;}
	    eputc('-');
	    i = -i;
	}

	if ((q = i/r) != 0) eputi(q, r);

	eputc("0123456789abcdef"[i%r]);
}

static void
eputs(char *s)
{
	INT i, len;

	len = strlen(s);
	i = 0;

	while (i < len) {
		eputc(nextch(s, len, i, &i));
	}
}

static void
eputc(INT c)
{
	if (ttcol < ncol) {
		ttcol += ucs_termwidth(c);

		if (ttcol < ncol) ucs_put(c);
	}
}

/*
 * Mg3a: Truncated string output
 */

static void
truncated_puts(char *s, INT maxwidth)
{
	INT i, c, col, len;

	len = strlen(s);
	col = 0;

	for (i = 0; i < len;) {
		c = ucs_strtochar(termcharset, s, len, i, &i);
		col += ucs_termwidth(c);

		if (col > maxwidth) {
			eputc('$');
			return;
		}

		eputc(c);
	}
}

/*
 * Mg3a: Show a message though in a macro, and return status. If the
 * status is ABORT, do not show the ordinary "Quit" message.
 */

INT
emessage(INT status, char *fp, ...)
{
	va_list pvar;

	va_start(pvar, fp);
	ttcolor(CTEXT);
	ttmove(nrow - 1, 0);
	eformat(fp, pvar);
	va_end(pvar);
	tteeol();
	ttflush();
	epresf = TRUE;

	if (status == ABORT) thisflag |= CFNOQUIT;

	return status;
}
