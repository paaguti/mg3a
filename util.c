/*
 * Some utility routines
 */

#include "def.h"
#include "macro.h"

#include <sys/stat.h>


/*
 * Local utility function
 */

static int
is_blank(INT c)
{
	return c == ' ' || c == '\t';
}


/*
 * strlen with max
 */

static size_t
safe_strlen(char *s, size_t size)
{
	char *p;

	p = memchr(s, 0, size);
	if (p) return p - s;
	return size;
}


/*
 * Safer stringcopy a la strlcpy
 */

size_t
stringcopy(char *dest, char *src, size_t size)
{
	size_t len;

	len = strlen(src);

	if (len < size) {
		memmove(dest, src, len + 1);
	} else if (0 < size) {
		memmove(dest, src, size);
		dest[size - 1] = 0;
	}

	return len;
}


/*
 * Safer stringcat ala strlcat
 */

size_t
stringconcat(char *dest, char *src, size_t size)
{
	size_t destlen, srclen;

	destlen = safe_strlen(dest, size);
	srclen = strlen(src);

	if (srclen < size && destlen < size - srclen) {
		memmove(dest + destlen, src, srclen + 1);
	} else if (destlen < size) {
		memmove(dest + destlen, src, size - destlen);
		dest[size - 1] = 0;
	} else if (0 < size) {
		dest[size - 1] = 0;
	}

	if (destlen < SIZE_MAX - srclen) return destlen + srclen;

	return SIZE_MAX;
}


/*
 * As stringconcat but append two strings.
 */

size_t
stringconcat2(char *dest, char *src1, char *src2, size_t size)
{
	size_t len1, len2;

	len2 = strlen(src2);
	len1 = stringconcat(dest, src1, size);

	if (len1 < size) {
		if (len2 < size - len1) {
			memmove(dest + len1, src2, len2 + 1);
		} else {
			memmove(dest + len1, src2, size - len1);
			dest[size - 1] = 0;
		}
	}

	if (len1 < SIZE_MAX - len2) return len1 + len2;

	return SIZE_MAX;
}


/*
 * Like strdup with a message.
 */

char *
string_dup(char *s)
{
	size_t size;
	char *ret;

	size = strlen(s) + 1;
	if ((ret = malloc(size)) == NULL) return outofmem(size);
	memcpy(ret, s, size);

	return ret;
}


/*
 * Find newline and return index. Return length if not found.
 */

size_t
findnewline(char *str, size_t len)
{
	char *cp = memchr(str, '\n', len);

	if (cp) return cp - str;
	return len;
}


/*
 * Save the current window position and dot as line numbers and
 * column.
 */

void
getwinpos(winpos_t *w)
{
	LINE *blp = curbp->b_linep, *lp, *linep = curwp->w_linep, *dotp = curwp->w_dotp;
	int count = 0;
	size_t line = 0;

	w->line = 1;
	w->dot = 1;

	for (lp = lforw(blp); lp != blp && count < 2; lp = lforw(lp)) {
		line++;

		if (lp == linep) {w->line = line; count++;}
		if (lp == dotp)  {w->dot = line; count++;}
	}

	w->column = getcolumn(curwp, curwp->w_dotp, curwp->w_doto);
}


/*
 * Restore the current window position and dot from line numbers and
 * column.
 */

void
setwinpos(winpos_t w)
{
	LINE *blp = curbp->b_linep, *lp;
	int count = 0;
	size_t pos = 0, line = 0;

	curwp->w_linep = lforw(blp);
	curwp->w_dotp = lback(blp);
	curwp->w_doto = llength(curwp->w_dotp);
	curwp->w_dotpos = curbp->b_size;

	for (lp = lforw(blp); lp != blp && count < 2; lp = lforw(lp)) {
		line++;

		if (line == w.line) {
			curwp->w_linep = lp;
			count++;
		}

		if (line == w.dot)  {
			curwp->w_dotp = lp;
			curwp->w_doto = getoffset(curwp, lp, w.column);
			curwp->w_dotpos = pos + curwp->w_doto;
			count++;
		}

		pos += llength(lp) + 1;
	}

	curwp->w_flag |= WFHARD;
	curwp->w_flag &= ~WFFORCE;
}


/*
 * Test if directory
 */

int
isdirectory(char *filename)
{
	struct stat statbuf;

    	if (stat(filename, &statbuf) < 0) return 0;

	return S_ISDIR(statbuf.st_mode);
}


/*
 * Test if regular file
 */

int
isregularfile(char *filename)
{
	struct stat statbuf;

    	if (stat(filename, &statbuf) < 0) return 0;

	return S_ISREG(statbuf.st_mode);
}


/*
 * Test if symlink
 */

int
issymlink(char *filename)
{
	struct stat statbuf;

    	if (lstat(filename, &statbuf) < 0) return 0;

	return S_ISLNK(statbuf.st_mode);
}


/*
 * Test if file exists
 */

int
fileexists(char *filename)
{
	return !access(filename, F_OK);
}


/*
 * Test if file exists and is not readable
 */

int
existsandnotreadable(char *filename)
{
	return !access(filename, F_OK) && access(filename, R_OK);
}


/*
 * Test if file exists and is readonly
 */

int
fileisreadonly(char *filename)
{
	return !access(filename, F_OK) && access(filename, W_OK);
}


/*
 * Filename of a pathname
 */

char *
filenameof(char *filename)
{
	char *lastslash;

	lastslash = strrchr(filename, '/');
	if (lastslash) return lastslash + 1;
	return filename;
}


/*
 * Dirname of a buffer
 */

char *
dirnameofbuffer(BUFFER *bp)
{
	static char dir[NFILEN];
	char *p;

	if (bp->b_fname[0]) {
		stringcopy(dir, bp->b_fname, sizeof(dir));

		if (!(bp->b_flag & BFDIRED)) {
			p = filenameof(dir);
			*p = 0;
		}
	} else {
		stringcopy(dir, wdir, sizeof(dir));
	}

	stringconcat(dir, dirname_addslash(dir), sizeof(dir));

	return dir;
}


/*
 * Column too big
 */

INT
column_overflow()
{
	ewprintf("Column overflow");
	return FALSE;
}


/*
 * Out of memory
 */

void *
outofmem(size_t size)
{
	ewprintf("Can't get %z bytes", size);
	return NULL;
}


/*
 * Overlong filename
 */

char *
overlong()
{
	ewprintf("Overlong filename");
	return NULL;
}


/*
 * Invalid codepoint.
 */

INT
invalid_codepoint()
{
	ewprintf("Invalid code point");
	return FALSE;
}


/*
 * Can't find buffer
 */

INT
buffernotfound(char *name)
{
	ewprintf("Could not find buffer \"%s\"", name);
	return FALSE;
}


/*
 * Add a line last in a buffer.
 */

void
addlast(BUFFER *bp, LINE *lp)
{
	LINE *blp = bp->b_linep;
	size_t len = llength(lp);

	if (lforw(blp) != blp) len++;

	insertafter(lback(bp->b_linep), lp);
	bp->b_size += len;
}


/*
 * Insert lp2 after lp1 in a list.
 */

void
insertafter(LINE *lp1, LINE *lp2)
{
	lp2->l_fp = lp1->l_fp;
	lp2->l_bp = lp1;
	lp1->l_fp->l_bp = lp2;
	lp1->l_fp = lp2;
}


/*
 * Insert lp2 before lp1 in a list.
 */

void
insertbefore(LINE *lp1, LINE *lp2)
{
	insertafter(lback(lp1), lp2);
}


/*
 * Insert lp last in list
 */

void
insertlast(LINE *listhead, LINE *lp)
{
	insertafter(lback(listhead), lp);
}


/*
 * Insert lp first in list
 */

void
insertfirst(LINE *listhead, LINE *lp)
{
	insertafter(listhead, lp);
}


/*
 * Remove from list
 */

void
removefromlist(LINE *lp)
{
	lp->l_bp->l_fp = lp->l_fp;
	lp->l_fp->l_bp = lp->l_bp;
	lp->l_fp = lp;
	lp->l_bp = lp;
}


/*
 * Make empty list
 */

LINE *
make_empty_list(LINE *listhead)
{
	LINE *lp;

	if (listhead) {
		while (listhead->l_fp != listhead) {
			lp = listhead->l_fp;
			removefromlist(lp);
			free(lp);
		}

		lp = listhead;
	} else {
		lp = lalloc(0);
		if (lp == NULL) return lp;
		lp->l_fp = lp;
		lp->l_bp = lp;
	}

	lp->l_flag = LCLISTHEAD;
	return lp;
}


/*
 * Returns the digit in a base. Returns negative on error.
 */

INT
basedigit(INT c, INT base)
{
	/* base is supposed to be checked and valid */

	if (base <= 10) {
		if (c >= '0' && c < '0' + base) return c - '0';
	} else {
		if (c >= 'A' && c <= 'Z') c += 'a' - 'A';

		if (c >= '0' && c < '0' + 10) return c - '0';
		if (c >= 'a' && c < 'a' + base - 10) return c - 'a' + 10;
	}

	return -1;
}


/*
 * Check for out of arguments
 */

int
outofarguments(int msg)
{
	if (maclcur == maclhead || maclcur == cmd_maclhead) {
		if (msg) emessage(ABORT, "Too few arguments in macro");
		return 1;
	}

	return 0;
}


/*
 * Return a slash if needed to concatenate a filename, otherwise an
 * empty string
 */

char *
dirname_addslash(char *dirname)
{
	size_t len;

	len = strlen(dirname);

	if (len == 0 || dirname[len - 1] != '/') return "/";
	else return "";
}


/*
 * Return true if line is blank (only whitespace)
 */

int
isblankline(LINE *lp)
{
	INT i, len, c;

	len = llength(lp);

	for (i = 0; i < len; i++) {
		c = lgetc(lp, i);
		if (c != ' ' && c != '\t') return 0;
	}

	return 1;
}


/*
 * Upcase an ASCII string.
 */

void
upcase_ascii(char *s)
{
	while (*s) {
		if (*s >= 'a' && *s <= 'z') *s += 'A' - 'a';
		s++;
	}
}


/*
 * Make an underline of the non-blanks in a string.
 */

void
make_underline(char *s)
{
	while (*s) {
		if (*s != ' ') *s = '-';
		s++;
	}
}


/*
 * Quick case insensitive compare for ASCII strings.
 */

int
ascii_strcasecmp(const char *s1, const char *s2)
{
	int c1, c2;

	while (1) {
		c1 = CHARMASK(*s1);
		c2 = CHARMASK(*s2);

		if (c1 >= 'a' && c1 <= 'z') c1 += 'A' - 'a';
		if (c2 >= 'a' && c2 <= 'z') c2 += 'A' - 'a';

		if (c1 != c2) return c1 - c2;
		if (c1 == 0) return 0;

		s1++;
		s2++;
	}
}


/*
 * Chomp line end characters off a string.
 */

void
chomp(char *s)
{
	size_t len = strlen(s);

	if (len && s[len - 1] == '\n') {
		s[--len] = 0;
		if (len && s[len - 1] == '\r') s[--len] = 0;
	}
}


/*
 * Common init of killbuffer
 */

void
initkill()
{
	if (!(lastflag & CFKILL)) kdelete();
	thisflag |= CFKILL;
}


/*
 * Buffer has shebang line
 */

int
isshebang(BUFFER *bp, char *str)
{
	char buf[NFILEN];
	INT len;
	LINE *lp;

	lp = lforw(bp->b_linep);
	len = llength(lp);

	if (len >= 2 && lgetc(lp, 0) == '#' && lgetc(lp, 1) == '!') {
		if (len >= NFILEN) len = NFILEN - 1;

		memcpy(buf, ltext(lp), len);
		buf[len] = 0;

		return (strstr(buf, str) != NULL);
	}

	return 0;
}


/*
 * Concat a multinational string to a limited fixed width and limited
 * size. Operate in termcharset. Return the total length. The last
 * column within "maxwidth" is reserved to be ' ' or '$'.
 */

INT
concat_limited(char *str, char *add, INT maxwidth, INT size)
{
	INT width = 0, w, c, i, nexti, addlen, len;
	char *cp;

	len = strlen(str);
	addlen = strlen(add);
	cp = str + len;

	for (i = 0; i < addlen; ) {
		c = ucs_strtochar(termcharset, add, addlen, i, &nexti);

		if (c == '\t') w = 1;
		else w = ucs_termwidth(c);

		if (width + w >= maxwidth || len + nexti >= size - 1) break;

		if (c == '\t') {
			*cp++ = ' ';
		} else {
			while (i < nexti) *cp++ = add[i++];
		}

		width += w;
		i = nexti;
	}

	while (width < maxwidth - 1 && cp - str < size - 2) {
		*cp++ = ' ';
		width++;
	}

	*cp++ = (i < addlen) ? '$' : ' ';
	*cp = 0;
	return cp - str;
}


/*
 * Malloc with a message.
 */

void *
malloc_msg(size_t size)
{
	void *p;

	/* use calloc to set points to NULL by default */
	/* if ((p = malloc(size)) == NULL) return outofmem(size); */
	if ((p = calloc(1,size)) == NULL) return outofmem(size);

	return p;
}


/*
 * Get an INT from a string checking for overflow. Disallow MININT as
 * out of range.
 */

int
getINT(char *str, INT *outint, int msg)
{
	long result;
	char *endstr;

	errno = 0;

	result = strtol(str, &endstr, 0);

	if (endstr == str) {
		if (msg) ewprintf("Invalid number");
		return 0;
	}

	if ((INT)result != result || result == MININT) errno = ERANGE;

	if (errno) {
		if (msg) ewprintf("Invalid number: %s", strerror(errno));
		return 0;
	}

	*outint = result;
	return 1;
}


/*
 * Test if in leading whitespace
 */

int
in_leading_whitespace()
{
	INT i, doto = curwp->w_doto;
	LINE *dotp = curwp->w_dotp;

	for (i = doto - 1; i >= 0; i--) {
		if (!is_blank(lgetc(dotp, i))) return 0;
	}

	return 1;
}


/*
 * Store an integer for a macro
 */

INT
macro_store_INT(INT i)
{
	LINE *lp;

	if (macrocount >= MAXMACRO) return FALSE;

	if ((lp = lalloc(30)) == NULL) return FALSE;

	snprintf(ltext(lp), llength(lp), INTFMT, i);
	lsetlength(lp, strlen(ltext(lp)));

	insertafter(maclcur, lp);
	maclcur = lp;
	maclcur->l_flag = macrocount;
	return TRUE;
}


/*
 * Get an integer from a macro argument
 */

INT
macro_get_INT(INT *outint)
{
	INT len;
	char buf[80];

    	if (outofarguments(1)) return ABORT;

	len = llength(maclcur);
	if (len >= (INT)sizeof(buf)) return FALSE;

	memcpy(buf, ltext(maclcur), len);
	buf[len] = 0;

	maclcur = maclcur->l_fp;
	return getINT(buf, outint, 1) ? TRUE : FALSE;
}


/*
 * Compare function for sorting a list on content
 */

static int
compare_content(const void *lpp1, const void *lpp2)
{
	LINE *lp1, *lp2;
	INT len1, len2, len;
	int s;

	lp1 = *(LINE **)lpp1;
	lp2 = *(LINE **)lpp2;

	len1 = llength(lp1);
	len2 = llength(lp2);

	len = len1;
	if (len2 < len) len = len2;

	if ((s = memcmp(ltext(lp1), ltext(lp2), len))) return s;
	else return (len1 == len2) ? 0 : (len1 < len2) ? -1 : 1;
}


/*
 * Sort a list. Return true if succeeded.
 */

int
sortlist(LINE *listhead, int (*sortfunc)(const void *, const void *))
{
	size_t i, count;
	LINE *lp;
	LINE **sortarray;

	lp = lforw(listhead);
	count = 0;

	while (lp != listhead) {
		count++;
		lp = lforw(lp);
	}

	if ((sortarray = (LINE **)malloc(count * sizeof(LINE *))) == NULL) {
		return 0;
	}

	lp = lforw(listhead);

	for (i = 0; i < count; i++) {
		sortarray[i] = lp;
		lp = lforw(lp);
	}

	qsort(sortarray, count, sizeof (LINE*), sortfunc);

	lp = listhead;

	for (i = 0; i < count; i++) {
		removefromlist(sortarray[i]);
		insertafter(lp, sortarray[i]);
		lp = lforw(lp);
	}

	free(sortarray);
	return 1;
}


/*
 * Sort a list on content
 */

int
sortlist_content(LINE *listhead)
{
	return sortlist(listhead, compare_content);
}


/*
 * There was an error in the dot byte position. Compute it the hard
 * way.
 */

static size_t
computepos()
{
	LINE *lp, *dotp = curwp->w_dotp, *blp = curbp->b_linep;
	size_t pos = 0;

	for (lp = lforw(blp); lp != dotp; lp = lforw(lp)) {
		if (lp == blp) panic("adjustpos: could not find dot", 0);
		pos += llength(lp) + 1;
	}

	u_clear(curbp);
	ewprintf("adjustpos error!"); ttbeep();

	return pos + curwp->w_doto;
}


/*
 * Change curwp->w_dotp and curwp->w_doto and recompute dot position.
 * Be paranoid. Recompute dot position if something is wrong and only
 * panic on not finding the line.
 *
 * Note: you can not use adjustpos() to set the position one line
 * beyond the end of a non-empty buffer. You have to avoid that.
 */

void
adjustpos(LINE *to_line, INT to_offset)
{
	LINE *dotp = curwp->w_dotp, *buflp = curbp->b_linep, *flp, *blp;
	INT doto = curwp->w_doto;
	size_t fpos, bpos;
	int err = 0;

	if (to_line == dotp) {
		curwp->w_doto = to_offset;
		curwp->w_dotpos += to_offset - doto;
		return;
	} else if (to_line == buflp) {
		return;
	}

	curwp->w_flag |= WFMOVE;

	flp = blp = dotp;
	fpos = bpos = curwp->w_dotpos - curwp->w_doto;

	while (lforw(flp) != buflp || lback(blp) != buflp) {
		if (lforw(flp) != buflp) {
			if (fpos >= SIZE_MAX - llength(flp)) err = 1;
			fpos += llength(flp) + 1;
			flp = lforw(flp);

			if (flp == dotp) {
				break;
			} else if (flp == to_line) {
				curwp->w_dotp = to_line;
				curwp->w_doto = to_offset;
				curwp->w_dotpos = fpos + to_offset;
				if (err) curwp->w_dotpos = computepos();
				return;
			}
		}

		if (lback(blp) != buflp) {
			blp = lback(blp);
			if (bpos <= (size_t)llength(blp)) err = 1;
			bpos -= llength(blp) + 1;

			if (blp == dotp) {
				break;
			} else if (blp == to_line) {
				curwp->w_dotp = to_line;
				curwp->w_doto = to_offset;
				curwp->w_dotpos = bpos + to_offset;
				if (err) curwp->w_dotpos = computepos();
				return;
			}
		}
	}

	panic("adjustpos: could not find target line", 0);
}


/*
 * Like adjustpos, but the position is given
 */

void
adjustpos3(LINE *to_line, INT to_offset, size_t to_pos)
{
	if (to_line != curwp->w_dotp) curwp->w_flag |= WFMOVE;

	curwp->w_dotp = to_line;
	curwp->w_doto = to_offset;
	curwp->w_dotpos = to_pos;
}


/*
 * Set position from byte position. Usually do it relatively from the
 * current position. Be paranoid. Return success status.
 */

int
position(size_t to_pos)
{
	LINE *blp = curbp->b_linep;
	LINE *lp;
	size_t pos, linelen;
	LINE *dotp = curwp->w_dotp;

	if (curwp->w_dotpos < (size_t)curwp->w_doto) goto overflow;

	pos = curwp->w_dotpos - curwp->w_doto;

	if (to_pos < (pos >> 1) || to_pos >= pos) {
		if (to_pos >= pos) {
			lp = curwp->w_dotp;
		} else {
			lp = lforw(blp);
			pos = 0;
		}

		while (1) {
			linelen = llength(lp) + 1;

			if (pos > SIZE_MAX - linelen) goto overflow;
			if (pos + linelen > to_pos) break;
			if (lp == blp) goto bufenderror;

			pos += linelen;
			lp = lforw(lp);
		}

		curwp->w_dotp = lp;
		curwp->w_doto = to_pos - pos;
		curwp->w_dotpos = to_pos;

		if (lp == blp) {
			if (normalizebuffer_noundo(curbp) == FALSE) return 0;
		}
	} else {
		lp = lback(curwp->w_dotp);

		while (1) {
			linelen = llength(lp) + 1;

			if (lp == blp) goto bufenderror;	// Not allowed when going backward
			if (linelen > pos) goto overflow;
			if (pos - linelen <= to_pos) break;

			pos -= linelen;
			lp = lback(lp);
		}

		curwp->w_dotp = lp;
		curwp->w_doto = to_pos - (pos - linelen);
		curwp->w_dotpos = to_pos;
	}

	if (dotp != curwp->w_dotp) curwp->w_flag |= WFMOVE;
	return 1;

bufenderror:
	ewprintf("(Position error hitting end of buffer)");
	return 0;

overflow:
	ewprintf("(Position overflow)");
	return 0;
}


/*
 * End of previous line. Often done, but awkward with adjustpos()
 */

void
endofpreviousline()
{
	LINE *lp = lback(curwp->w_dotp);

	if (lp != curbp->b_linep) {
		curwp->w_dotpos -= curwp->w_doto + 1;
		curwp->w_dotp = lp;
		curwp->w_doto = llength(lp);
		curwp->w_flag |= WFMOVE;
	}
}


#if TESTCMD
#include "testcmd.inc"
#endif
