#include	"def.h"

#include 	<pwd.h>
#include	<sys/stat.h>
#include	<fcntl.h>

static	FILE	*ffp;


#define BUFLEN 4096				// Size of local IO buffer

static INT io_buflen = 0, io_bufptr = 0;	// Local IO buffer
static unsigned char iobuf[BUFLEN];


/*
 * Mg3a: Locally buffered getc(), because of stdio locking overhead
 */

#define ffgetc() (io_bufptr < io_buflen ? iobuf[io_bufptr++] : ffgetc_more())

static INT
ffgetc_more()
{
	io_buflen = fread(iobuf, 1, BUFLEN, ffp);
	io_bufptr = 0;
	if (io_buflen == 0) return EOF;
	return iobuf[io_bufptr++];
}


/*
 * Mg3a: Locally buffered write, because of stdio locking overhead
 */

static void
writebuf(char *buf, INT len)
{
	while (len > BUFLEN - io_bufptr) {
		if (ferror(ffp)) return;
		memcpy(&iobuf[io_bufptr], buf, BUFLEN - io_bufptr);
		fwrite(iobuf, 1, BUFLEN, ffp);
		buf += BUFLEN - io_bufptr;
		len -= BUFLEN - io_bufptr;
		io_bufptr = 0;
	}

	memcpy(&iobuf[io_bufptr], buf, len);
	io_bufptr += len;
}


/*
 * Open a file for reading.
 */

INT
ffropen(char *fn)
{
	if ((ffp=fopen(fn, "r")) == NULL) {
		return FIOFNF;
	}

	io_buflen = 0;
	io_bufptr = 0;
	return FIOSUC;
}


/*
 * Open a file for writing.
 */

INT
ffwopen(char *fn)
{
	if ((ffp=fopen(fn, "w")) == NULL) {
		ewprintf("Cannot open file for writing");
		return FIOERR;
	}

	io_bufptr = 0;
	return FIOSUC;
}


/*
 * Close a file.
 */

INT
ffclose()
{
	if (fclose(ffp) == 0) {
		return FIOSUC;
	} else {
		return FIOERR;
	}
}


/*
 * Write a buffer to the already opened file. bp points to the buffer.
 * Return the status. Check only at the newline and end of buffer.
 */

INT
ffputbuf(BUFFER *bp)
{
	LINE *lp;
	LINE *lpend;
	INT	 unixlf = (bp->b_flag & BFUNIXLF);

	lpend = bp->b_linep;

	if (bp->b_flag & BFBOM) writebuf("\xef\xbb\xbf", 3);

	for (lp = lforw(lpend); !ferror(ffp) && lp != lpend; lp = lforw(lp)) {
		writebuf(ltext(lp), llength(lp));

		if (lforw(lp) == lpend && llength(lp) == 0) break;

		if (unixlf) writebuf("\n", 1); else writebuf("\r\n", 2);
	}

	if (!ferror(ffp) && io_bufptr) fwrite(iobuf, 1, io_bufptr, ffp);

	if (ferror(ffp)) {
		ewprintf("Write I/O error");
		return FIOERR;
	}

	return FIOSUC;
}


/*
 * Mg3a: Scan more intelligently for the newline
 */

static INT
ffgetdata(char *buf, INT nbuf)
{
	INT newline;

	if (nbuf > io_buflen - io_bufptr) nbuf = io_buflen - io_bufptr;
	if (nbuf == 0 || iobuf[io_bufptr] == '\n' || iobuf[io_bufptr] == '\r') return 0;
	newline = findnewline((char *)&iobuf[io_bufptr], nbuf);
	if (newline && iobuf[io_bufptr + newline - 1] == '\r') newline--;
	memcpy(buf, &iobuf[io_bufptr], newline);
	io_bufptr += newline;
	return newline;
}


/*
 * Read a line from a file, and store the bytes in the supplied
 * buffer. Stop on end of file or end of line. Don't get upset by
 * files that don't have an end of line on the last line; this seem to
 * be common on CP/M-86 and MS-DOS. Delete any CR followed by a NL:
 * This is the normal format for MS_DOS files, but also occurs when
 * files are transferred from VMS or MS-DOS to Unix.
 */

INT
ffgetline(char *buf, INT nbuf, INT *nbytes)
{
	INT	c;
	INT	i;
	INT crlf = 0;

	i = 0;
	for (;;) {
		i += ffgetdata(&buf[i], nbuf - i);
		if (i >= nbuf) return FIOLONG;

		c = ffgetc();
 rescan:
		if (c == '\r') {		/* Delete any non-stray	*/
			c = ffgetc();		/* carriage returns.	*/
			if (c != '\n') {
				buf[i++] = '\r';
				if (i >= nbuf) return FIOLONG;
				goto rescan;
			}
			crlf = 1;
		}
		if (c==EOF || c=='\n')		/* End of line.		*/
			break;
		buf[i++] = c;
		if (i >= nbuf) return FIOLONG;
	}

	*nbytes = i;

	if (c == EOF) {
		if (ferror(ffp)) {
			ewprintf("File read error");
			return FIOERR;
		}

		return FIOEOF;
	} else {
		if (crlf) {
			count_crlf++;
		} else {
			count_lf++;
		}

		return FIOSUC;
	}
}


/*
 * Rename the file "fname" into a backup copy. On Unix the backup has
 * the same name as the original file, with a "~" on the end; this
 * seems to be newest of the new-speak. The error handling is all in
 * "file.c".
 *
 * Mg3a: Do a file copy instead.
 */

INT
fbackupfile(char *fn)
{
	char	nname[NFILEN];

	if (stringcopy(nname, fn, NFILEN) >= NFILEN ||
		stringconcat(nname, "~", NFILEN) >= NFILEN)
	{
		overlong();
		return ABORT;
	}

	return copyfile(fn, nname, COPYABORTMSG, NULL);
}


/*
 * The string "fn" is a file name.
 * Perform any required appending of directory name or case adjustments.
 */

static char *
checkroot(char *fn)
{
	static char resultname[NFILEN];

#if NFILEN != PATH_MAX
#error NFILEN not equal to PATH_MAX
#endif

	if (fn[0] == 0) return "/";

	if (realpath(fn, resultname) == NULL)
		stringcopy(resultname, fn, NFILEN);

	return resultname;
}


/*
 * Normalize a filename to a full path.
 *
 * Mg3a: always return a pointer to static storage different from
 * "fn", or return NULL on error.
 */

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

char *
adjustname(char *fn)
{
    char *cp, *home;
    char fnb[NFILEN];
    char fnc[NFILEN];
#ifdef __CYGWIN__
    wchar_t widestring[NFILEN], *wstr = widestring;
    size_t wlen;
#endif
    struct passwd *pwent;

/* Always copy/convert fn to local fnc to avoid alias problems */

#ifdef __CYGWIN__
    wlen = mbstowcs(wstr, fn, NFILEN);

    if (wlen == (size_t)(-1)) {
    	ewprintf("Problem with file name: %s", strerror(errno));
	return NULL;
    } else if (wlen == NFILEN) {
    	return overlong();
    }

    if (wlen && ((wstr[0] == '"'  && wstr[wlen-1] == '"') ||
    	(wstr[0] == '\''  && wstr[wlen-1] == '\''))) {
    	/* Compensate for a bug in some versions of Cygwin which leaves the quote marks in. */
    	wstr[wlen-1] = 0;
	wstr++;
    }

    if (cygwin_conv_path(CCP_WIN_W_TO_POSIX | CCP_RELATIVE, wstr, fnc, NFILEN) < 0) {
	return overlong();
    }
#else
    if (stringcopy(fnc, fn, NFILEN) >= NFILEN) return overlong();
#endif

    fn = &fnc[0];

    switch(*fn) {
    	case '/':
	    cp = fnb;
	    *cp++ = *fn++;
	    break;
	case '~':
	    fn++;
	    if(*fn == '/' || *fn == '\0') {
	    	home = getenv("HOME");
		if (!home && (pwent = getpwnam(getlogin())) != NULL) {
			home = pwent->pw_dir;
		}
		if (home) {
		    if (stringcopy(fnb, home, NFILEN) >= NFILEN) return overlong();
		    cp = fnb + strlen(fnb);
	    	    if(*fn) fn++;
		    break;
	    	} else {
		    /* can't find home, continue to default case */
		    fn--;
		}
	    } else {
		cp = fnb;
		while(*fn && *fn != '/') *cp++ = *fn++;
		*cp = '\0';
		if((pwent = getpwnam(fnb)) != NULL) {
		    if (stringcopy(fnb, pwent->pw_dir, NFILEN) >= NFILEN) return overlong();
		    cp = fnb + strlen(fnb);
		    break;
		} else {
		    fn -= strlen(fnb) + 1;
		    /* can't find ~user, continue to default case */
		}
	    }
	default:
	    if (stringcopy(fnb, wdir, NFILEN) >= NFILEN) return overlong();
	    cp = fnb + strlen(fnb);
	    break;
    }
    if (cp - fnb + strlen(fn) + 2 > NFILEN) return overlong();
    if(cp != fnb && cp[-1] != '/') *cp++ = '/';
    while(*fn) {
    	switch(*fn) {
	    case '.':
		switch(fn[1]) {
	            case '\0':
		    	*--cp = '\0';
		    	return checkroot(fnb);
	    	    case '/':
	    	    	fn += 2;
		    	continue;
		    case '.':
		    	if(fn[2]=='/' || fn[2] == '\0') {
			    --cp;
			    while(cp > fnb && *--cp != '/') {}
			    ++cp;
			    if(fn[2]=='\0') {
			        *--cp = '\0';
			        return checkroot(fnb);
			    }
		            fn += 3;
		            continue;
		        }
		        break;
		    default:
		    	break;
	        }
		break;
	    case '/':
	    	fn++;
	    	continue;
	    default:
	    	break;
	}
	while(*fn && (*cp++ = *fn++) != '/') {}
    }
    if (cp[-1]=='/') --cp;
    *cp = '\0';
    return checkroot(fnb);
}


/*
 * Find a startup file for the user and return its name. As a service
 * to other pieces of code that may want to find a startup file (like
 * the terminal driver in particular), accepts a suffix to be appended
 * to the startup file name.
 *
 * Mg3a: Rewritten.
 */

char *
startupfile(char *suffix)
{
	char	*home, *common = "/etc/mg3", *try[] = {".mg3", ".mg", NULL};
	static char	file[NFILEN];
	int i;

	if ((home = getenv("HOME"))) {
		for (i = 0; try[i]; i++) {
			snprintf(file, sizeof(file), "%s%s%s", home, dirname_addslash(home), try[i]);

			if (access(file, F_OK) == 0) {
				if (suffix) {
					stringconcat2(file, "-", suffix, sizeof(file));
					return (access(file, F_OK) == 0) ? file : NULL;
				} else {
					return file;
				}
			}
		}
	}

	return (!suffix && access(common, F_OK) == 0) ? common : NULL;
}


/*
 * Mg3a: Whether filenames compare case-sensitively
 */

#ifdef __CYGWIN__
INT compare_fold_file = 1;
#else
INT compare_fold_file = 0;
#endif


/*
 * Mg3a: Compare filenames
 */

int
fncmp(char *str1, char *str2)
{
	if (compare_fold_file) {
		return ucs_strcasecmp(termcharset, str1, str2);
	} else {
		return strcmp(str1, str2);
	}
}


/*
 * Mg3a: Copy a file. Copy the modes for a new file like /bin/cp.
 */

INT
copyfile(char *infilename, char *outfilename, INT flag, off_t *outsize)
{
	FILE *infile, *outfile;
	size_t len;
	off_t size;
	INT ret;
	struct stat instat;
	int fd;
	mode_t mode;
#ifdef O_BINARY
	int oflag = O_CREAT|O_WRONLY|O_TRUNC|O_BINARY;
#else
	int oflag = O_CREAT|O_WRONLY|O_TRUNC;
#endif

	ret = TRUE;
	size = 0;
	if (outsize) *outsize = size;

	if (stat(infilename, &instat) < 0 ||
		(infile = fopen(infilename, "rb")) == NULL)
	{
		if (flag & COPYERRORMSG) ewprintf("Error opening infile: %s", strerror(errno));
		return FALSE;
	}

	mode = instat.st_mode & 0777;	// Remove the dangerous stuff

	if ((fd = open(outfilename, oflag, mode)) < 0 ||
		(outfile = fdopen(fd, "wb")) == NULL)
	{
		if (flag & COPYERRORMSG) ewprintf("Error opening outfile: %s", strerror(errno));
		if (fd >= 0) close(fd);
		fclose(infile);
		return FALSE;
	}

	while (!feof(infile)) {
		len = fread(iobuf, 1, sizeof(iobuf), infile);
		if (ferror(infile)) {
			if (flag & COPYABORTMSG)
				ewprintf("Error reading infile: %s", strerror(errno));
			ret = ABORT;
			goto out;
		}
		size += fwrite(iobuf, 1, len, outfile);
		if (ferror(outfile)) {
			if (flag & COPYABORTMSG)
				ewprintf("Error writing outfile: %s", strerror(errno));
			ret = ABORT;
			goto out;
		}
	}

out:
	// Well, if you want it water tight...

	if (fclose(outfile) && ret == TRUE) {
		if (flag & COPYABORTMSG)
			ewprintf("Error closing outfile: %s", strerror(errno));
		ret = ABORT;
		fclose(infile);
	} else if (fclose(infile) && ret == TRUE) {
		if (flag & COPYABORTMSG)
			ewprintf("Error closing infile: %s", strerror(errno));
		ret = ABORT;
	}

	if (outsize) *outsize = size;

	return ret;
}


/*
 * Mg3a: Next character in pattern or filename, optionally
 * case-folded, with substitution if invalid.
 */

static INT
nextch(char *str, INT len, INT index, INT *outindex)
{
	INT c;

	c = ucs_strtochar(termcharset, str, len, index, outindex);

	if (termcharset != utf_8 && c == 0xfffd) {
		// Unmapped 8-bit characters are mapped to something unique and negative.
		// Invalid UTF-8 is already >= -255 and <= -128
		c = -(256 + CHARMASK(str[index]));
	} else if (c >= 0 && compare_fold_file) {
		c = ucs_case_fold(c);
	}

	return c;
}


/*
 * Mg3a: the simple version of nextch, just a shorthand.
 */

static INT
nextch2(char *str, INT len, INT index, INT *outindex)
{
	return ucs_strtochar(termcharset, str, len, index, outindex);
}


/*
 * Mg3a: Checks for invalid characters
 */

#define INVALID_CHAR(c)		((c) < 0)
#define INVALID_MULTIBYTE(c)	((c) < 0 && (c) >= -255)


/*
 * Mg3a: Filename pattern matching.
 *
 * Deals with patterns "*", "?", "[abc]", "[a-c]", "[!abc]", "[^abc]",
 * "[-abc]", "[abc-]", "[]abc]".
 *
 * "\" quotes the next character in the pattern everywhere, as seems
 * to be the de facto standard. It also solves a problem with
 * recognizing both "^" and "!" at the beginning of a range.
 *
 * Returns TRUE on match, FALSE or ABORT on not match.
 *
 * The ordering within "[a-b]" ranges is Unicode codepoint order. This
 * is not standard, but easy and consistent.
 */

/*
 * Pedantic: Invalid UTF-8 characters match only themselves and "*",
 * and cannot be quoted with "\".
 *
 * Unmapped 8-bit characters match only themselves, "*" and "?", and
 * can be quoted with "\".
 */

static INT
filematch_pattern_aux(char *pattern, INT len1, char *filename, INT len2)
{
	INT i, nexti, j, nextj, match, negate;
	INT c1, c2, cend, first, s;
	int wild, simplebyte;

#if TESTCMD == 4
	{
		extern INT filematch_count;
		filematch_count++;
	}
#endif

	i = 0;
	j = 0;

	while (i < len1 && j < len2) {
		c1 = nextch(pattern, len1, i, &nexti);
		c2 = nextch(filename, len2, j, &nextj);

		if (c1 == '\\') {
			if (nexti == len1) return ABORT;
			c1 = nextch(pattern, len1, nexti, &nexti);
			if (INVALID_MULTIBYTE(c1)) return ABORT;
			if (c1 != c2) return FALSE;
			i = nexti;
			j = nextj;
		} else if (c1 == '*') {
			while (c1 == '*') {	// do...while violates my aesthetic senses
				i = nexti;
				if (i == len1) return TRUE;
				c1 = nextch(pattern, len1, i, &nexti);
			}

			wild = (c1 == '?' || c1 == '[' || c1 == '\\');
			simplebyte = (!wild && ISASCII(c1) && (!compare_fold_file || !ISALPHA(c1)));

			while (1) {
				s = filematch_pattern_aux(pattern + i, len1 - i, filename + j, len2 - j);
				if (s != FALSE) return s;

				if (wild) {
					// Wildcard. Recurse the hard way.
					j = nextj;
					if (j == len2) return ABORT;		// Faster than FALSE
					nextch2(filename, len2, j, &nextj);
				} else if (!simplebyte) {
					// Not wildcard, but not simple byte. Scan without recursing.
					do {
						j = nextj;
						if (j == len2) return ABORT;
					} while (c1 != nextch(filename, len2, j, &nextj));
				} else {
					// Scan for the simple byte
					char *cp = memchr(filename + nextj, c1,  len2 - nextj);

					if (!cp) return ABORT;
					j = cp - filename;
					nextj = j + 1;
				}
			}
		} else if (c1 == '[') {
			match = 0;
			negate = 0;
			first = 1;

			i = nexti;
			if (i == len1) return ABORT;

			c1 = nextch(pattern, len1, i, &nexti);

			if (c1 == '^' || c1 == '!') {
				negate = 1;
				i = nexti;
				if (i == len1) return ABORT;
			}

			while (1) {
				c1 = nextch(pattern, len1, i, &i);
				if (!first && c1 == ']') {
					break;
				}
				if (c1 == '\\') {
					if (i == len1) return ABORT;
					c1 = nextch(pattern, len1, i, &i);
				}
				if (i == len1 || INVALID_CHAR(c1)) return ABORT;
				if (nextch(pattern, len1, i, &nexti) == '-' &&		// Tag:match
					nexti < len1 &&
					(cend = nextch(pattern, len1, nexti, &nexti)) != ']')
				{
					i = nexti;
					if (cend == '\\') {
						if (i == len1) return ABORT;
						cend = nextch(pattern, len1, i, &i);
					}
					if (i == len1 || INVALID_CHAR(cend)) return ABORT;
					if (cend < c1) {
						return ABORT;
					} else if (c1 <= c2 && c2 <= cend) {
						match = 1;
					}
				} else if (c1 == c2) {
					match = 1;
				}
				first = 0;
			}

			if (negate == match) return FALSE;
			j = nextj;
		} else if ((c1 == '?' && !INVALID_MULTIBYTE(c2)) || c1 == c2) {
			i = nexti;
			j = nextj;
		} else {
			return FALSE;
		}
	}

	// If we get here, the filename is at the end, or the pattern is
	// already at the end. Skip remaining stars in pattern.

	while (i < len1 && nextch(pattern, len1, i, &nexti) == '*') {
		i = nexti;
	}

	return (i == len1 && j == len2) ? TRUE : FALSE;
}


/*
 * Mg3a: whether extra operations for files are done
 */

static int matching_files = 1;


/*
 * Mg3a: Match a pattern, and check for '~' at the start of the
 * pattern.
 */

static int
filematch_pattern(char *pattern, INT len1, char *filename, INT len2)
{
	if (matching_files && nextch2(pattern, len1, 0, NULL) == '~') {
		pattern[len1] = 0;
		if ((pattern = adjustname(pattern)) == NULL) return 0;
		len1 = strlen(pattern);
	}

	return (filematch_pattern_aux(pattern, len1, filename, len2) == TRUE);
}


static int filematch_alt(char *pattern, INT len1, char *filename, INT len2);


/*
 * Mg3a: Assemble a pattern from alternatives and match or recurse.
 */

static int
filematch_assemble(char *pattern, INT endprefix, INT startalt, INT endalt,
	INT startsuffix, INT len1, char *filename, INT len2, int recurse)
{
	char pat[NFILEN];
	INT i = 0, len;

	if (len1 >= NFILEN) return 0;

	memcpy(pat, pattern, endprefix);
	i += endprefix;

	memcpy(pat + i, &pattern[startalt], endalt - startalt);
	i += endalt - startalt;

	memcpy(pat + i, &pattern[startsuffix], len1 - startsuffix);
	len = i + len1 - startsuffix;

	if (recurse) {
		return filematch_alt(pat, len, filename, len2);
	} else {
		return filematch_pattern(pat, len, filename, len2);
	}
}


/*
 * Mg3a: Find alternatives "{pat1,pat2,...}" in the pattern if there are
 * any. If no alternatives or any error, treat directly as a pattern.
 */

static int
filematch_alt(char *pattern, INT len1, char *filename, INT len2)
{
	INT endprefix = -1, startalt = -1, startsuffix = -1, level = 0, c, i, nexti, count = 0;
	int recurse = 0;

	for (i = 0; i < len1;) {
		c = nextch2(pattern, len1, i, &nexti);

		if (c == '\\') {
			nextch2(pattern, len1, nexti, &nexti);
		} else if (c == '{') {
			level++;
			if (++count > 100) {
				// Error, too many alternatives
				emessage(FALSE, "Too many '{' in pattern.");
				return 0;
			}
			if (level == 1 && endprefix < 0) {
				endprefix = i;
				startalt = nexti;
			} else {
				recurse = 1;
			}
		} else if (c == '}') {
			level--;
			if (level == 0) {
				if (startsuffix < 0) startsuffix = nexti;
			} else if (level < 0) {
				// Error. Use directly as pattern.
				return filematch_pattern(pattern, len1, filename, len2);
			}
		}

		i = nexti;
	}

	if (level != 0 || endprefix < 0) {
		// Either error or no alternatives; use as pattern
		return filematch_pattern(pattern, len1, filename, len2);
	}

	while (1) {
		for (i = startalt; i < len1;) {
			c = nextch2(pattern, len1, i, &nexti);

			if (c == '\\') {
				nextch2(pattern, len1, nexti, &nexti);
			} else if (c == '{') {
				level++;
			} else if (c == ',') {
				if (level == 0) {
					if (filematch_assemble(pattern, endprefix, startalt, i,
						startsuffix, len1, filename, len2, recurse)) return 1;
					startalt = nexti;
					break;
				}
			} else if (c == '}') {
				if (level == 0) {
					return filematch_assemble(pattern, endprefix, startalt, i,
						startsuffix, len1, filename, len2, recurse);
				} else {
					level--;
				}
			}

			i = nexti;
		}
	}
}


/*
 * Mg3a: Pathname pattern matching. If the pattern doesn't contain
 * '/', match only the filename part. The filename passed in is always
 * a fully qualified path.
 */

int
filematch(char *pattern, char *filename)
{
	INT len1, len2;

	if (matching_files && strchr(pattern, '/') == NULL) {
		filename = filenameof(filename);
	}

	len1 = strlen(pattern);
	len2 = strlen(filename);

	if (memchr(pattern, '{', len1)) {
		return filematch_alt(pattern, len1, filename, len2);
	} else {
		return filematch_pattern(pattern, len1, filename, len2);
	}
}

#if 0

/*
 * Mg3a: A match that doesn't treat '/' and '~' specially
 */

int
string_match(char *pattern, char *str)
{
	int ret;

	matching_files = 0;
	ret = filematch(pattern, str);
	matching_files = 1;

	return ret;
}


/*
 * Mg3a: A simple match without alternatives
 */

int
simple_match(char *pattern, char *str)
{
	return filematch_pattern_aux(pattern, strlen(pattern), str, strlen(str)) == TRUE;
}

#endif
