/* dired module for mg 2a	*/
/* by Robert A. Larson		*/

#include "def.h"
#include "kbd.h"

#ifdef DIRED		/* Note after def.h, may be defined there */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

INT	onlywind(INT f, INT n);
INT	forwline(INT f, INT n);
INT	backline(INT f, INT n);
INT	d_undelbak(INT f, INT n);


/*
 * Mg3a: Get only the filename without directory from the line
 */

static char *
d_shortname(LINE *lp)
{
	if (lp->l_flag == LCDIRED) {
		return ltext(lp) + llength(lp);
	} else {
		return "";	// At least not a "bomb" error
	}
}


/*
 * Mg3a: sort function for dired buffer
 */

static int
diredcompare(const void *lpp1, const void *lpp2)
{
	LINE *lp1, *lp2;

	lp1 = *(LINE **)lpp1;
	lp2 = *(LINE **)lpp2;

	return strcmp(d_shortname(lp1), d_shortname(lp2));
}


/*
 * Mg3a: return dired name offset for a line
 */

static INT
dirednameoffset(LINE *lp)
{
	INT i, j, len;

	j = 0;
	len = llength(lp);
	if (len) j = 1;

	for (i = 0; i < 8; i++) {
		while (j < len && lgetc(lp, j) == ' ') j++;
		while (j < len && lgetc(lp, j) != ' ') j++;
	}

	if (j < len) j++;
	return j;
}

/*
 * Mg3a: Re-adjust offset of current line if a Dired line.
 */

void
diredsetoffset(void)
{
	INT offset;

	if (curwp->w_dotp->l_flag == LCDIRED &&
		(offset = dirednameoffset(curwp->w_dotp)) != curwp->w_doto)
	{
		adjustpos(curwp->w_dotp, offset);
		setgoal();
	}
}

/*
 * Mg3a: Sort the dired buffer on the filename. Use byte-by-byte sort.
 */

static BUFFER *
dired_sortbuffer(BUFFER *bp)
{
	LINE *lp;
	WINDOW *wp;
	char *name;
	size_t pos = 0;

	if (!sortlist(bp->b_linep, diredcompare)) {
		ewprintf("Could not sort dired buffer");
		return NULL;
	}

	lp = lforw(bp->b_linep);

	/* Put dot at the first file that is not "." or ".." */

	while (lp != bp->b_linep && lp->l_flag == LCDIRED) {
		name = d_shortname(lp);

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			pos += llength(lp) + 1;
			lp = lforw(lp);
		} else {
			break;
		}
	}

	bp->b_dotp = lp;
	bp->b_doto = dirednameoffset(lp);
	bp->b_dotpos = pos + bp->b_doto;

	/* Reset all the windows with this buffer */

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == bp) {
			wp->w_dotp = bp->b_dotp;
			wp->w_doto = bp->b_doto;
			wp->w_dotpos = bp->b_dotpos;
			wp->w_flag |= (WFFORCE|WFHARD|WFMODE);
		}
	}

	/* Finally normalize */

	if (normalizebuffer_noundo(bp) == FALSE) return NULL;

	return bp;
}

/*
 * Mg3a: "ls"-like time string for Dired.
 */

#define CTIMELEN 32

static char *
timestr(time_t t, time_t now)
{
	char s[CTIMELEN], *ct;
	static char result[CTIMELEN];

	if ((ct = ctime(&t)) == NULL) return "ctime returns NULL";
	if (stringcopy(s, ct, CTIMELEN) >= CTIMELEN) return "ctime overlong string";

	s[strlen(s)-1] = 0;	// chop off '\n'

	if (t < now - 6*30*86400 || t > now) {
		snprintf(result, CTIMELEN, "%.6s  %4s", s + 4, s + 20);
	} else {
		snprintf(result, CTIMELEN, "%.12s", s + 4);
	}

	return result;
}


/*
 * Mg3a: Return the name for the uid. Return as number if the name
 * contains non-ASCII, space, or control.
 */

#define NAMELEN 32

static char *
uidstr(uid_t uid)
{
	static char result[NAMELEN] = "";
	static uid_t prevuid = 0;
	struct passwd *pwent;
	char *cp;
	int c, err = 0;

	if (result[0] && uid == prevuid) return result;

	if ((pwent = getpwuid(uid)) == NULL) err = 1;

	if (!err) {
		for (cp = pwent->pw_name; *cp; cp++) {
			c = CHARMASK(*cp);
			if (c <= 32 || c >= 127) {
				err = 1;
				break;
			}
		}
	}

	if (err) {
		snprintf(result, NAMELEN, "%ld", (long)uid);
	} else {
		stringcopy(result, pwent->pw_name, NAMELEN);
	}

	prevuid = uid;
	return result;
}


/*
 * Mg3a: Return the name for the gid. Return as number if the name
 * contains non-ASCII, space, or control.
 */

static char *
gidstr(gid_t gid)
{
	static char result[NAMELEN] = "";
	static gid_t prevgid = 0;
	struct group *grent;
	char *cp;
	int c, err = 0;

	if (result[0] && gid == prevgid) return result;

	if ((grent = getgrgid(gid)) == NULL) err = 1;

	if (!err) {
		for (cp = grent->gr_name; *cp; cp++) {
			c = CHARMASK(*cp);
			if (c <= 32 || c >= 127) {
				err = 1;
				break;
			}
		}
	}

	if (err) {
		snprintf(result, NAMELEN, "%ld", (long)gid);
	} else {
		stringcopy(result, grent->gr_name, NAMELEN);
	}

	prevgid = gid;
	return result;
}


/*
 * Mg3a: Read in a named directory into an existing buffer, wiping its
 * contents. Set modes and charset appropriate for a dired buffer.
 */

#define DIREDLINELEN (2*NFILEN+1024)

BUFFER *
readin_dired(BUFFER *bp, char *dirname)
{
    DIR	*dir;
    struct dirent *direntry;
    struct stat statbuf;
    char line[DIREDLINELEN];
    mode_t mode;
    char filename[NFILEN];
    char linkname[NFILEN];
    ssize_t linklen;
    nlink_t maxlinks = 0;
    off_t maxsize = 0;
    int linkwidth = 0, sizewidth = 0, userwidth = 0, groupwidth = 0, temp;
    time_t now = time(NULL);

    // UNDO
    u_clear(bp);
    //

    winflag(bp, WFHARD|WFMODE);
    bp->b_flag &= ~BFCHG;
    if(bclear(bp) != TRUE) return NULL;

    bp->charset = termcharset;
    bp->b_flag |= (BFREADONLY|BFDIRED);

    stringcopy(bp->b_fname, dirname, NFILEN);

    if (setbufmode(bp, "dired") == FALSE) {
    	return NULL;
    }

    if ((dir = opendir(dirname)) == NULL) {
	ewprintf("Problem opening directory: %s", strerror(errno));
	return NULL;
    }

    // Pre-scan for the adjustable widths

    // Strictly speaking the entries should be stored in a linked list
    // and then parsed but that is too much work. Let's call the
    // result approximate.

    while ((direntry = readdir(dir)) != NULL) {
        snprintf(filename, NFILEN, "%s%s%s",
		dirname, dirname_addslash(dirname), direntry->d_name);

	// If there is an error with lstat, something is wrong, like a
	// file having been removed or a directory protected after the
	// readdir. We can't quit because of that.

    	if (lstat(filename, &statbuf) < 0) continue;

	if (statbuf.st_nlink > maxlinks) maxlinks = statbuf.st_nlink;
	if (statbuf.st_size > maxsize) maxsize = statbuf.st_size;
	temp = strlen(uidstr(statbuf.st_uid));
	if (temp > userwidth) userwidth = temp;
	temp = strlen(gidstr(statbuf.st_gid));
	if (temp > groupwidth) groupwidth = temp;
    }

    while (maxlinks > 0) {maxlinks /= 10; linkwidth++;}
    while (maxsize > 0) {maxsize /= 10; sizewidth++;}

    rewinddir(dir);

    while ((direntry = readdir(dir)) != NULL) {
        snprintf(filename, NFILEN, "%s%s%s",
		dirname, dirname_addslash(dirname), direntry->d_name);

    	if (lstat(filename, &statbuf) < 0) continue;

	mode = statbuf.st_mode;

	snprintf(line, DIREDLINELEN, "  %c%c%c%c%c%c%c%c%c%c %*ju %-*s %-*s %*jd %s %s",
		S_ISDIR(mode) ? 'd' :
		S_ISREG(mode) ? '-' :
		S_ISLNK(mode) ? 'l' :
		S_ISBLK(mode) ? 'b' :
		S_ISCHR(mode) ? 'c' :
		S_ISFIFO(mode) ? 'p' :
		S_ISSOCK(mode) ? 's' : '?',

		mode & S_IRUSR ? 'r' : '-',
		mode & S_IWUSR ? 'w' : '-',
		mode & S_IXUSR ? (mode & S_ISUID ? 's' : 'x') : (mode & S_ISUID ? 'S' : '-'),
		mode & S_IRGRP ? 'r' : '-',
		mode & S_IWGRP ? 'w' : '-',
		mode & S_IXGRP ? (mode & S_ISGID ? 's' : 'x') : (mode & S_ISGID ? 'S' : '-'),
		mode & S_IROTH ? 'r' : '-',
		mode & S_IWOTH ? 'w' : '-',
		mode & S_IXOTH ? (mode & S_ISVTX ? 't' : 'x') : (mode & S_ISVTX ? 'T' : '-'),
		linkwidth,
		(uintmax_t) statbuf.st_nlink,
		userwidth,
		uidstr(statbuf.st_uid),
		groupwidth,
		gidstr(statbuf.st_gid),
		sizewidth,
		(intmax_t) statbuf.st_size,	// Signed, according to standards
		timestr(statbuf.st_mtime, now),
		direntry->d_name);

	if (S_ISLNK(mode)) {
		linklen = readlink(filename, linkname, NFILEN);

		if (linklen == NFILEN) linklen--;

		if (linklen > 0) {
			linkname[linklen] = 0;
			stringconcat2(line, " -> ", linkname, DIREDLINELEN);
		}
	}

	if (addline_extra(bp, line, LCDIRED, direntry->d_name) == FALSE) {
		closedir(dir);
		return NULL;
	}
    }

    closedir(dir);

    return dired_sortbuffer(bp);
}


/*
 * Create or find the dired buffer, wipe it, and read in the
 * directory.
 */

static BUFFER*
dired_(char *dirname)
{
    BUFFER *bp;
    DIR *dir;

    if ((dir = opendir(dirname)) == NULL) {
    	ewprintf("Problem opening directory: %s", strerror(errno));
	return NULL;
    } else {
    	closedir(dir);
    }

    if((bp = findbuffer(dirname)) == NULL) {
	ewprintf("Could not create buffer");
	return NULL;
    }

    return readin_dired(bp, dirname);
}


/*
 * Get a filename from a line, check if directory
 */

static INT
d_makename(LINE *lp, char *fn, INT size)
{
    char *dirname;

    if (lp->l_flag != LCDIRED) return ABORT;

    dirname = curbp->b_fname;

    snprintf(fn, size, "%s%s%s", dirname, dirname_addslash(dirname), d_shortname(lp));

    return isdirectory(fn) ? TRUE : FALSE;
}


/*
 * Mg3a: Check if current line is a dired line
 */

static int
isdiredline(void)
{
	return curwp->w_dotp->l_flag == LCDIRED;
}


/*
 * Mg3a: Check if current line is a dired line, with message
 */

static int
isdired(void)
{
	if (isdiredline()) return 1;

	ewprintf("Not a dired line");
	return 0;
}


/*
 * Mg3a: Check if the current buffer is a Dired buffer, with message
 */

static int
isdiredbuf(void)
{
	if (curbp->b_flag & BFDIRED) return 1;

	ewprintf("Not a dired buffer");
	return 0;
}


/*
 * Mg3a: Do fancy redisplay, saving the position
 */

static INT
redisplay()
{
	BUFFER *bp;
	winpos_t w;

	getwinpos(&w);

	if ((bp = dired_(curbp->b_fname)) == NULL) return FALSE;

	if (bp != curbp) {
		ewprintf("Error in dired redisplay, unexpected new buffer");
		return FALSE;
	}

	setwinpos(w);

	refreshbuf(bp);
	return TRUE;
}


/*
 * Mg3a: show directory, optionally in other window. Includes
 * adjusting the name. Sets cursor to a filename in the listing if
 * findfile is non-NULL.
 */

static INT
dired_showdir(char *name, INT otherwindow, char *findfile)
{
	char *adjf;
	BUFFER *bp;
	WINDOW *wp;
	LINE *lp, *blp;
	size_t pos = 0;

	if ((adjf = adjustname(name)) == NULL) return FALSE;

	if ((bp = dired_(adjf)) == NULL) return FALSE;

	if (otherwindow) {
		if ((wp = popbuf(bp)) == NULL) return FALSE;
		curbp = bp;
		curwp = wp;
	} else {
		curbp = bp;
		showbuffer(curbp, curwp);
	}

	if (findfile) {
		blp = curbp->b_linep;

		for (lp = lforw(blp); lp != blp; lp = lforw(lp)) {
			if (lp->l_flag == LCDIRED
				&& fncmp(findfile, d_shortname(lp)) == 0)
			{
				curwp->w_dotp = lp;
				curwp->w_doto = dirednameoffset(lp);
				curwp->w_dotpos = pos + curwp->w_doto;
				break;
			}
			pos += llength(lp) + 1;
		}
	}

	return TRUE;
}


INT
dired(INT f, INT n)
{
    char dirname[NFILEN];

    if (eread("Dired: ", dirname, NFILEN, EFFILE) == ABORT) return ABORT;
    return dired_showdir(dirname, 0, NULL);
}


INT
d_otherwindow(INT f, INT n)
{
    char dirname[NFILEN];

    if (eread("Dired other window: ", dirname, NFILEN, EFFILE) == ABORT) return ABORT;
    return dired_showdir(dirname, 1, NULL);
}


INT
d_del(INT f, INT n)
{
    if (n < 0) return FALSE;
    while (n--) {
	if (isdiredline()) lputc(curwp->w_dotp, 0, 'D');
	forwline(0, 1);
    }
    winflag(curbp, WFHARD);
    return TRUE;
}


INT
d_undel(INT f, INT n)
{
    if (n < 0) return d_undelbak(f, -n);
    while (n--) {
	if (isdiredline()) lputc(curwp->w_dotp, 0, ' ');
	forwline(0, 1);
    }
    winflag(curbp, WFHARD);
    return TRUE;
}


INT
d_undelbak(INT f, INT n)
{
    if (n < 0) return d_undel(f, -n);
    while (n--) {
	backline(0, 1);
	if (isdiredline()) lputc(curwp->w_dotp, 0, ' ');
    }
    winflag(curbp, WFHARD);
    return TRUE;
}


static INT
d_findfile_aux(INT flag)
{
    char fname[NFILEN], *adjf;
    BUFFER *bp;
    WINDOW *wp;
    INT s;

    if (!isdired()) return FALSE;

    if ((s = d_makename(curwp->w_dotp, fname, NFILEN)) == ABORT) return FALSE;

    if ((adjf = adjustname(fname)) == NULL) return FALSE;

    if (!isregularfile(adjf) && !isdirectory(adjf) &&
	!(!fileexists(fname) && issymlink(fname)))
    {
    	ewprintf("Not a directory or file");
	return FALSE;
    }

    if (s == TRUE) {
	if ((bp = dired_(adjf)) == NULL) return FALSE;
    } else {
    	if (existsandnotreadable(adjf)) {
		ewprintf("File is not readable: %s", adjf);
		return FALSE;
	}

	if ((bp = findbuffer(adjf)) == NULL) return FALSE;
    }

    if (flag == 0 || flag == 1) {
    	if (flag == 1) onlywind(0, 1);

	curbp = bp;
	showbuffer(bp, curwp);
    } else {
	if ((wp = popbuf(bp)) == NULL) return FALSE;

	curbp = bp;
	curwp = wp;
    }

    if (bp->b_fname[0] != 0) return TRUE;
    return readin(adjf);
}


INT
d_findfile(INT f, INT n)
{
	return d_findfile_aux(0);
}


INT
d_ffonewindow(INT f, INT n)
{
	return d_findfile_aux(1);
}


INT
d_ffotherwindow(INT f, INT n)
{
	return d_findfile_aux(2);
}


/*
 * Mg3a: Do dired on the file's directory, or one level up if Dired.
 */

INT
d_jump(INT f, INT n)
{
	char name[NFILEN], *lastslash;

	if (curbp->b_fname[0] == 0) {
		return dired_showdir(wdir, f & FFARG, NULL);
	} else {
		stringcopy(name, curbp->b_fname, NFILEN);
		lastslash = strrchr(name, '/');

		if (lastslash) lastslash[1] = 0;
		else stringcopy(name, wdir, NFILEN);

		return dired_showdir(name, f & FFARG, filenameof(curbp->b_fname));
	}

}


/*
 * Mg3a: Dired jump in other window
 */

INT
d_jump_otherwindow(INT f, INT n)
{
	return d_jump(FFARG, n);
}


/*
 * Mg3a: up directory
 */

INT
d_updir(INT f, INT n)
{
	return d_jump(f, n);
}


/*
 * Mg3a: Move to subdirectory
 */

static INT
dired_movetodir(INT forward, INT n)
{
	LINE *lp, *blp = curbp->b_linep;

	if (!isdiredbuf()) return FALSE;

	if (n < 0) {
		n = -n;
		forward = !forward;
	}

	lp = curwp->w_dotp;

	for (; n > 0; n--) {
		if (lp != blp) lp = forward ? lforw(lp) : lback(lp);

		while (1) {
			if (lp == blp) {
				ewprintf("No more subdirectories");
				return FALSE;
			}

			if (lp->l_flag == LCDIRED && lgetc(lp, 2) == 'd') break;

			lp = forward ? lforw(lp) : lback(lp);
		}
	}

	adjustpos(lp, dirednameoffset(lp));

	return TRUE;
}


/*
 * Mg3a: next subdirectory
 */

INT
d_nextdir(INT f, INT n)
{
	return dired_movetodir(1, n);
}


/*
 * Mg3a: Previous subdirectory
 */

INT
d_prevdir(INT f, INT n)
{
	return dired_movetodir(0, n);
}


INT
d_expunge(INT f, INT n)
{
    LINE *lp, *nlp;
    char fname[NFILEN];
    size_t pos = 0;

    winflag(curbp, WFHARD);

    for (lp = lforw(curbp->b_linep); lp != curbp->b_linep; lp = nlp) {
	nlp = lforw(lp);

	if (llength(lp) && lgetc(lp, 0) == 'D') {
	    switch (d_makename(lp, fname, NFILEN)) {
		case ABORT:
		    ewprintf("Bad line in dired buffer");
		    return FALSE;
		case FALSE:
		    if (unlink(fname) < 0) {
			ewprintf("Could not delete '%s'", fname);
			return FALSE;
		    }
		    break;
		case TRUE:
		    if (rmdir(fname) < 0) {
			ewprintf("Could not delete directory '%s'", fname);
			return FALSE;
		    }
		    break;
	    }

	    lfree(curbp, lp, pos);
	} else {
	    pos += llength(lp) + 1;
	}
    }

    return TRUE;
}


/*
 * Mg3a: Ask for a file in the Dired directory (not the current
 * directory) using a default. Return a fully qualified filename.
 */

static INT
d_ereadfile(char *prompt, char *buf, INT nbuf, char *srcname, char *defaultname)
{
	char *savedir, *adjf = NULL;
	INT status = TRUE;

	savedir = wdir;
	wdir = curbp->b_fname;

	status = eread(prompt, buf, nbuf, EFFILE, srcname);

	if (status == TRUE) {
		status = (adjf = adjustname(buf)) ? TRUE : FALSE;
	}

	if (status == TRUE) {
		stringcopy(buf, adjf, nbuf);

		if (defaultname && isdirectory(buf)) {
			if ((INT)stringconcat2(buf,
				dirname_addslash(buf),
				filenameof(defaultname),
				nbuf) >= nbuf)
			{
				overlong();
				status = FALSE;
			}
		}
	}

	wdir = savedir;
	return status;
}

INT
d_copy(INT f, INT n)
{
	char frname[NFILEN], toname[NFILEN];
	INT stat;
	off_t count;

	if (!isdired()) return FALSE;

	if (d_makename(curwp->w_dotp, frname, NFILEN) != FALSE || !isregularfile(frname)) {
		ewprintf("Not a file");
		return FALSE;
	}

	stat = d_ereadfile("Copy \"%S\" to: ", toname, NFILEN, d_shortname(curwp->w_dotp), frname);

	if (stat != TRUE) return stat;

	if (copyfile(frname, toname, COPYALLMSG, &count) != TRUE) return FALSE;

	ewprintf("%j bytes copied to \"%s\"", (intmax_t)count, toname);

	return redisplay();
}


INT
d_rename(INT f, INT n)
{
	char frname[NFILEN], toname[NFILEN];
	INT stat;

	if (!isdired()) return FALSE;

	if (d_makename(curwp->w_dotp, frname, NFILEN) == ABORT ||
		(!fileexists(frname) && !issymlink(frname)))
	{
		ewprintf("Not a file");
		return FALSE;
	}

	stat = d_ereadfile("Rename \"%S\" to: ", toname, NFILEN, d_shortname(curwp->w_dotp), frname);

	if (stat != TRUE) return stat;

	stat = rename(frname, toname);

	if (stat < 0) {
		ewprintf("Error during rename: %s", strerror(errno));
		return FALSE;
	}

	ewprintf("Renamed to \"%s\"", toname);
	return redisplay();
}


INT
d_createdir(INT f, INT n)
{
	char dirname[NFILEN];
	INT stat;

	if (!isdiredbuf()) return FALSE;

	stat = d_ereadfile("Create directory: ", dirname, NFILEN, NULL, NULL);

	if (stat != TRUE) return stat;

	stat = mkdir(dirname, 0777);

	if (stat < 0) {
    		ewprintf("Error creating directory: %s", strerror(errno));
		return FALSE;
	}

	ewprintf("Created directory \"%s\"", dirname);

	return redisplay();
}
#endif
