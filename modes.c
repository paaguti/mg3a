/*
 * Commands to toggle modes. Without an argument, toggle mode.
 * Negitive or zero argument, mode off.	 Positive argument, mode on.
 */

#include "def.h"
#include "kbd.h"
#include "macro.h"


INT defb_nmodes = 0;
MODE defb_modes[PBMODES] = {0};
INT defb_flag = 0;

/*
 * Mg3a: added execution of init commands when the buffer is the
 * current one.
 */

INT
changemode(BUFFER *bp, INT f, INT n, char *mode)
{
    INT i, s;
    MODE m;

    m = name_mode(mode);

    if(invalid_mode(m)) {
		ewprintf("Can't find mode %s", mode);
		return FALSE;
    }
    if(!(f & FFARG)) {
		for(i=0; i <= bp->b_nmodes; i++)
			if(bp->b_modes[i] == m) {
				n = 0;			/* mode already set */
				break;
			}
    }
    if(n > 0) {
		for(i=0; i <= bp->b_nmodes; i++)
			if(bp->b_modes[i] == m) return TRUE;	/* mode already set */
		if(bp->b_nmodes >= PBMODES-1) {
			ewprintf("Too many modes");
			return FALSE;
		}
		upmodes(bp);
		/* TODO: this is a terrible hack!*/
		bp->localsvar.v.comment_begin = NULL;
		bp->localsvar.v.comment_end = NULL;
		if (name_mode("python") == m || name_mode("make") == m) {
			bp->localsvar.v.comment_begin = strdup("# ");
		}
		/*END HACK*/
		if (bp == curbp && (s = exec_mode_init_commands(m)) != TRUE) return s;
		bp->b_modes[++(bp->b_nmodes)] = m;
    } else {
	/* fundamental is b_modes[0] and can't be unset */
		for(i=1; i <= bp->b_nmodes && m != bp->b_modes[i]; i++) {}
		if(i > bp->b_nmodes) return TRUE;		/* mode wasn't set */
		for(; i < bp->b_nmodes; i++)
			bp->b_modes[i] = bp->b_modes[i+1];
		bp->b_nmodes--;
		upmodes(bp);
    }
    return TRUE;
}

/*
 * Mg3a: Set a mode in a buffer unconditionally.
 * Return TRUE if succeeded.
 */

INT
setbufmode(BUFFER *bp, char *mode)
{
	return changemode(bp, FFARG, 1, mode);
}

/*
 * Mg3a: Clear a mode in a buffer unconditionally.
 * Return TRUE if succeeded.
 */

INT
clearbufmode(BUFFER *bp, char *mode)
{
	return changemode(bp, FFARG, 0, mode);
}


/*
 * Mg3a: test if a mode is set
 */

int
isbufmode(BUFFER *bp, char *mode)
{
	INT i;
	MODE m;

	m = name_mode(mode);

	if (invalid_mode(m)) return 0;

	for (i = 0; i <= bp->b_nmodes; i++) {
		if(bp->b_modes[i] == m) return 1;
	}

	return 0;
}

INT
indentmode(INT f, INT n)
{
    return changemode(curbp, f, n, "indent");
}

INT
fillmode(INT f, INT n)
{
    return changemode(curbp, f, n, "fill");
}

INT
notabmode(INT f, INT n)
{
	INT notabon;

	if (f & FFARG) {
		if (n < 0) {
			curbp->localvar.v.tabs_with_spaces = MININT;
			ewprintf("Local no-tab mode is unset; default is %s",
				tabs_with_spaces ? "on" : "off");

		} else {
			curbp->localvar.v.tabs_with_spaces = (n > 0);
			ewprintf("No-tab mode is %s", (n > 0) ? "on" : "off");
		}
	} else {
		notabon = get_variable(curbp->localvar.v.tabs_with_spaces, tabs_with_spaces);

		curbp->localvar.v.tabs_with_spaces = !notabon;
		ewprintf("No-tab mode is %s", !notabon ? "on" : "off");
	}

	return TRUE;
}

INT
overwrite(INT f, INT n)
{
    if(f & FFARG) {
	if(n <= 0) curbp->b_flag &= ~BFOVERWRITE;
	else curbp->b_flag |= BFOVERWRITE;
    } else curbp->b_flag ^= BFOVERWRITE;
    upmodes(curbp);
    return TRUE;
}


INT
internal_unixlf(INT n)	/* internal function for readin() */
{
    if (n) {
	curbp->b_flag |= BFUNIXLF;
    } else {
	curbp->b_flag &= ~BFUNIXLF;
    }

    upmodes(curbp);

    return TRUE;
}


INT
unixlf(INT f, INT n)		/* called from interface */
{
    INT oldflag = curbp->b_flag;

    if (f & FFARG) {
	internal_unixlf(n > 0);
    } else {
    	internal_unixlf((curbp->b_flag & BFUNIXLF) == 0);
    }

    if (oldflag != curbp->b_flag) {
    	curbp->b_flag |= BFCHG;	/* Mark as changed so as to be written back */
    }
    return TRUE;
}


INT
crlf(INT f, INT n)		/* called from interface */
{
    INT oldflag = curbp->b_flag;

    internal_unixlf(0);

    if (oldflag != curbp->b_flag) {
    	curbp->b_flag |= BFCHG;	/* Mark as changed so as to be written back */
    }

    return TRUE;
}


INT
nocharsets()
{
	if (inmacro) return TRUE;

	ewprintf("Terminal charset not recognized, only 8-bit editing is possible.");
	return FALSE;
}

INT
internal_utf8(INT n)	/* internal function for readin() */
{
    if (n) {
	curbp->charset = utf_8;
    } else {
	curbp->charset = buf8bit;
    }

    upmodes(curbp);

    return TRUE;
}

INT
cmdutf8(INT f, INT n)		/* called from interface */
{
    if (!termcharset) return nocharsets();

    if (f & FFARG) {
	internal_utf8(n > 0);
    } else {
    	internal_utf8(curbp->charset != utf_8);
    }

    refreshbuf(curbp);

    return TRUE;
}


INT
internal_dos(INT n)	/* internal function for readin() */
{
    if (n) {
	curbp->charset = doscharset;
    } else {
	curbp->charset = buf8bit;
    }

    upmodes(curbp);

    return TRUE;
}

INT
cmddos(INT f, INT n)		/* called from interface */
{
    if (!termcharset) return nocharsets();

    if (f & FFARG) {
	internal_dos(n > 0);
    } else {
    	internal_dos(curbp->charset != doscharset);
    }

    refreshbuf(curbp);

    return TRUE;
}


INT
internal_bom(INT n)	/* internal function for readin() */
{
    if (n) {
	curbp->b_flag |= BFBOM;
    } else {
	curbp->b_flag &= ~BFBOM;
    }

    upmodes(curbp);

    return TRUE;
}

INT
internal_ubom(INT n)	/* internal function for readin() */
{
    if (n) {
	curbp->b_flag |= BFBOM;
	curbp->charset = utf_8;
    } else {
	curbp->b_flag &= ~BFBOM;
	curbp->charset = buf8bit;
    }

    upmodes(curbp);

    return TRUE;
}

INT
bom(INT f, INT n)		/* called from interface */
{
    INT oldflag = curbp->b_flag;

    if (!termcharset) return nocharsets();

    if (f & FFARG) {
    	internal_bom(n > 0);
    } else {
    	internal_bom((curbp->b_flag & BFBOM) == 0);
    }

    if (oldflag != curbp->b_flag) {
    	curbp->b_flag |= BFCHG;	/* Mark as changed so as to be written back */
    }

    return TRUE;
}

INT
ubom(INT f, INT n)		/* called from interface */
{
    charset_t charset = curbp->charset;
    INT oldflag = curbp->b_flag;

    if (!termcharset) return nocharsets();

    if (f & FFARG) {
    	internal_ubom(n > 0);
    } else {
    	internal_ubom((curbp->b_flag & BFBOM) == 0);
    }

    if (charset != curbp->charset) refreshbuf(curbp);

    if (oldflag != curbp->b_flag) {
    	curbp->b_flag |= BFCHG;	/* Mark as changed so as to be written back */
    }

    return TRUE;
}

INT
nobom(INT f, INT n)		/* called from interface */
{
    INT oldflag = curbp->b_flag;

    if (!termcharset) return nocharsets();

    curbp->b_flag &= ~BFBOM;

    if (oldflag != curbp->b_flag) {
    	curbp->b_flag |= BFCHG;	/* Mark as changed so as to be written back */
        upmodes(curbp);
    }

    return TRUE;
}


INT
internal_iso(INT n)	/* internal function for readin() */
{
    charset_t oldcharset = curbp->charset;

    curbp->charset = buf8bit;

    if (oldcharset != curbp->charset) {
    	refreshbuf(curbp);
    }

    return TRUE;
}

INT
cmdiso(INT f, INT n)		/* called from interface */
{
    if (!termcharset) return nocharsets();

    internal_iso(1);
    return TRUE;
}


INT
toggle_readonly(INT f, INT n)
{
    if (f & FFARG) {
	if(n <= 0) curbp->b_flag &= ~BFREADONLY;
	else curbp->b_flag |= BFREADONLY;
    } else curbp->b_flag ^= BFREADONLY;
    upmodes(curbp);
    return TRUE;
}


INT
set_default_mode(INT f, INT n)
{
    INT i;
    MODE m;
    char mode[MODENAMELEN];

    if(ereply("Set Default Mode: ", mode, sizeof(mode)) != TRUE)
    	return ABORT;
    if(strcmp(mode, "lf")==0) {
    	if(n<=0) defb_flag &= ~BFUNIXLF;
	else defb_flag |= BFUNIXLF;
	return TRUE;
    }
    if(strcmp(mode, "crlf")==0) {
    	if(n<=0) defb_flag |= BFUNIXLF;
	else defb_flag &= ~BFUNIXLF;
	return TRUE;
    }
    if(strcmp(mode, "bom")==0) {
	if (!termcharset) return nocharsets();

    	if(n<=0) defb_flag &= ~BFBOM;
	else defb_flag |= BFBOM;

	return TRUE;
    }
    if(strcmp(mode, "nobom")==0) {
	if (!termcharset) return nocharsets();

    	if(n>0) defb_flag &= ~BFBOM;
	else defb_flag |= BFBOM;

	return TRUE;
    }
    if(strcmp(mode, "notab")==0) {
        tabs_with_spaces = (n > 0);
	return TRUE;
    }
    if(strcmp(mode, "overwrite") == 0) {
    	if(n<=0) defb_flag &= ~BFOVERWRITE;
	else defb_flag |= BFOVERWRITE;
	return TRUE;
    }
    m = name_mode(mode);
    if(invalid_mode(m)) {
    	ewprintf("can't find mode %s", mode);
	return FALSE;
    }
    if(!(f & FFARG)) {
	for(i=0; i <= defb_nmodes; i++)
	    if(defb_modes[i] == m) {
		n = 0;			/* mode already set */
		break;
	    }
    }
    if(n > 0) {
	for(i=0; i <= defb_nmodes; i++)
	    if(defb_modes[i] == m) return TRUE;	/* mode already set */
	if(defb_nmodes >= PBMODES-1) {
	    ewprintf("Too many modes");
	    return FALSE;
	}
	defb_modes[++defb_nmodes] = m;
    } else {
	/* fundamental is defb_modes[0] and can't be unset */
	for(i=1; i <= defb_nmodes && m != defb_modes[i]; i++) {}
	if(i > defb_nmodes) return TRUE;		/* mode wasn't set */
	for(; i < defb_nmodes; i++)
	    defb_modes[i] = defb_modes[i+1];
	defb_nmodes--;
    }
    return TRUE;
}

int
charsetequal(char *str1, char *str2)
{
	INT c1, c2;

	while (1) {
		while (*str1 == '-' || *str1 == '_') str1++;
		while (*str2 == '-' || *str2 == '_') str2++;

		c1 = *str1;
		c2 = *str2;

		if (c1 >= 'A' && c1 <= 'Z') c1 += 'a' - 'A';
		if (c2 >= 'A' && c2 <= 'Z') c2 += 'a' - 'A';

		if (c1 == c2) {
			if (c1 == 0) return 1;
		} else {
			return 0;
		}

		str1++;
		str2++;
	}
}

charset_t
nametocharset(char *str, INT flags)
{
	charset_entry *p;

	if (charsetequal("TIS-620", str)) str = "CP874";	/* ??? Cygwin	*/

	for (p = charsets; p->name; p++) {
		if ((p->flags & flags) && charsetequal(p->name, str)) return p->set;
	}

	return NULL;
}

char *
charsettoname(charset_t charset)
{
	charset_entry *p;

	for (p = charsets; p->name; p++) {
		if (p->set == charset) return p->name;
	}

	return "UNKNOWN_CHARSET";
}

INT
set_default_charset(INT f, INT n)
{
	INT s;
	char buf[CHARSETLEN];
	charset_t charset;

	if((s = eread("Set Default Charset: ", buf, sizeof(buf), EFCHARSET)) != TRUE)
    		return s;

	if (!termcharset) return nocharsets();

	charset = nametocharset(buf, CHARSETALL);

	if (charset == NULL) {
		ewprintf("Unrecognized charset: %s", buf);
		return FALSE;
	}

	bufdefaultcharset = charset;

	if (charset != utf_8) buf8bit = charset;

	upmodes(NULL);

	ewprintf("Default charset set to %s", charsettoname(charset));
	return TRUE;
}

INT
set_default_8bit_charset(INT f, INT n)
{
	INT s;
	char buf[CHARSETLEN];
	charset_t charset;

	if((s = eread("Set Default 8bit Charset: ", buf, sizeof(buf), EFCHARSET)) != TRUE)
    		return s;

	if (!termcharset) return nocharsets();

	charset = nametocharset(buf, CHARSET8BIT);

	if (charset == NULL) {
		ewprintf("Unrecognized charset: %s", buf);
		return FALSE;
	}

	buf8bit = charset;

	upmodes(NULL);

	ewprintf("Default 8bit charset set to %s", charsettoname(charset));
	return TRUE;
}

INT
set_dos_charset(INT f, INT n)
{
	INT s;
	char buf[CHARSETLEN];
	charset_t charset;

	if((s = eread("Set Alternate Charset: ", buf, sizeof(buf), EFCHARSET)) != TRUE)
    		return s;

	if (!termcharset) return nocharsets();

	charset = nametocharset(buf, CHARSET8BIT);

	if (charset == NULL) {
		ewprintf("Unrecognized charset: %s", buf);
		return FALSE;
	}

	doscharset = charset;

	upmodes(NULL);

	ewprintf("Alternate charset set to %s", charsettoname(charset));
	return TRUE;
}

INT
local_set_charset(INT f, INT n)
{
	INT s;
	char buf[CHARSETLEN];
	charset_t charset;

	if((s = eread("Local Set Charset: ", buf, sizeof(buf), EFCHARSET)) != TRUE)
    		return s;

	if (!termcharset) return nocharsets();

	charset = nametocharset(buf, CHARSETALL);

	if (charset == NULL) {
		ewprintf("Unrecognized charset: %s", buf);
		return FALSE;
	}

	curbp->charset = charset;
	curbp->b_flag |= BFMODELINECHARSET;

	refreshbuf(curbp);

	ewprintf("Local charset set to %s", charsettoname(charset));
	return TRUE;
}

static INT
listcharsetssimple(BUFFER *bp)
{
	charset_entry *p;

	for (p = charsets; p->name; p++) {
		if (addline(bp, p->name) == FALSE) return FALSE;
	}

	return popbuftop(bp);
}


INT
listcharsets(INT f, INT n)
{
	BUFFER *bp;
	charset_entry *p;
	char buf[CHARSETLEN + 80];

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	if (f & FFARG) return listcharsetssimple(bp);

	if (addline(bp, "List of charsets:") == FALSE) return FALSE;
	if (addline(bp, "") == FALSE) return FALSE;

	for (p = charsets; p->name; p++) {
		sprintf(buf, "%c%c%c%c %s",
			p->set == curbp->charset ? '.' : ' ',
			p->set == bufdefaultcharset ? 'd' : ' ',
			p->set == buf8bit ? '8' : ' ',
			p->set == doscharset ? 'a' : ' ',
			p->name);

		if (addline(bp, buf) == FALSE) return FALSE;
	}

	return popbuftop(bp);
}


/*
 * Mg3a: Set a mode name for the local buffer.
 */

INT
localmodename(INT f, INT n)
{
	char name[LOCALMODELEN];

	if (ereply("Local Mode Name: ", name, sizeof(name)) == ABORT) return ABORT;

	stringcopy(curbp->localmodename, name, sizeof(curbp->localmodename));
	upmodes(curbp);
	if (name[0] == 0) eerase();
	return TRUE;
}
