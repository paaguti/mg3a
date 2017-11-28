/* Help functions for MicroGnuEmacs 2 */

#include "def.h"
#include "kbd.h"
#include "key.h"
#include "macro.h"

#if APROPOS_LIST
static void	findbind(PF funct, char *ind, KEYMAP *map);
static void	bindfound(void);
#endif

static	INT 	showall(char *ind, KEYMAP *map, int all);

extern INT	rescan(INT f, INT n);
extern INT	ctrlg(INT f, INT n);

/*
 * Read a key from the keyboard, and look it up in the keymap. Display
 * the name of the function currently bound to the key.
 */

INT
desckey(INT f, INT n)
{
    KEYMAP *curmap;
    PF funct;
    char *pep;
    char prompt[(MAXSTRKEY+1)*(MAXKEY)+80];
    INT c;
    INT m;
    INT i;

#ifndef NO_MACRO
    if(inmacro) return TRUE;		/* ignore inside keyboard macro */
#endif
    strcpy(prompt, "Describe key briefly: ");
    pep = prompt + strlen(prompt);
    key.k_count = 0;
    m = curbp->b_nmodes;
    curmap = mode_map(curbp->b_modes[m]);
    for(;;) {
	for(;;) {
	    ewprintf("%s", prompt);
	    pep[-1] = ' ';
	    if (key.k_count >= MAXKEY - 1 || pep >= &prompt[(MAXSTRKEY+1)*(MAXKEY-1)+79]) {
	    	ewprintf("Max Key count reached");
		return FALSE;
	    }
	    pep = keyname(pep, key.k_chars[key.k_count++] = c = getkey(FALSE));
	    if((funct = doscan(curmap, c)) != prefix) break;
	    *pep++ = '-';
	    *pep = '\0';
	    curmap = ele->k_prefmap;
	}
	if(funct != rescan) break;
	if(ucs_isupper(key.k_chars[key.k_count-1])) {
	    funct = doscan(curmap, ucs_tolower(key.k_chars[key.k_count-1]));
	    if(funct == prefix) {
		*pep++ = '-';
		*pep = '\0';
		curmap = ele->k_prefmap;
		continue;
	    }
	    if(funct != rescan) break;
	}
nextmode:
	if(--m < 0) break;
	curmap = mode_map(curbp->b_modes[m]);
	for(i=0; i < key.k_count; i++) {
	    funct = doscan(curmap, key.k_chars[i]);
	    if(funct != prefix) {
		if(i == key.k_count - 1 && funct != rescan) goto found;
		funct = rescan;
		goto nextmode;
	    }
	    curmap = ele->k_prefmap;
	}
	*pep++ = '-';
	*pep = '\0';
    }
found:
    if(funct == rescan) {
    	estart();
    	ewprintfc("%k");
	while (typeahead()) {
		ewprintfc(" %K", getkey(FALSE));
	}
	ewprintfc(" is not bound to any function");
#ifdef USER_MACROS
    } else if ((pep = key_function_to_name_or_macro(funct)) != NULL) {
	ewprintf("%k runs the macro %s", pep);
#endif
    } else if((pep = function_name(funct)) != NULL) {
	ewprintf("%k runs the command %s", pep);
    } else {
	ewprintf("%k is bound to an unnamed function");
    }
    return TRUE;
}


/*
 * This function creates a table, listing all of the command keys and
 * their current bindings, and stores the table in the *help* pop-up
 * buffer. This lets MicroGnuEMACS produce it's own wall chart.
 */

static BUFFER	*bp;
static char buf[MAXKEY*(MAXSTRKEY+1)+EXCLEN+2];	/* used by showall and findbind */

INT
wallchart(INT f, INT n)
{
	INT m;
	static char locbind[80 + MODENAMELEN];
#ifdef USER_MACROS
	int all = ((f & FFARG) == 0);
#else
	const int all = 1;
#endif

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;
	for(m=curbp->b_nmodes; m > 0; m--) {
	    snprintf(locbind, sizeof(locbind),
		"Local keybindings for mode %s:", mode_name_check(curbp->b_modes[m]));
	    if((addline(bp, locbind) == FALSE) ||
		(showall(buf, mode_map(curbp->b_modes[m]), all) == FALSE) ||
		(addline(bp, "") == FALSE)) return FALSE;
	}
	if((addline(bp, "Global bindings:") == FALSE) ||
	    (showall(buf, map_table[0].p_map, all) == FALSE)) return FALSE;
	return popbuftop(bp);
}

static	INT
showall(char *ind, KEYMAP *map, int all)
{
	MAP_ELEMENT *ele;
	INT	i;
	PF	functp;
	char	*cp;
	char	*cp2;
	INT	last;
	int	blank = 0, displaykey = all;

	if (all) {
		if(addline(bp, "") == FALSE) return FALSE;
		blank = 1;
	}

	last = -1;
	for(ele = &map->map_element[0]; ele < &map->map_element[map->map_num] ; ele++) {
	    if(map->map_default != rescan && ++last < ele->k_base && all) {
		cp = keyname(ind, last);
		if(last < ele->k_base - 1) {
		    strcpy(cp, " .. ");
		    cp = keyname(cp + 4, ele->k_base - 1);
		}
		do { *cp++ = ' '; } while(cp < &buf[16]);
		strcpy(cp, function_name(map->map_default));	// Cannot be macro
		if(addline(bp, buf) == FALSE) return FALSE;
	    }
	    last = ele->k_num;
	    for(i=ele->k_base; i <= last; i++) {
		functp = ele->k_funcp[i - ele->k_base];
		if(functp != rescan) {
		    if(functp != prefix) cp2 = function_name(functp);
		    else cp2 = map_name(ele->k_prefmap);
#ifdef USER_MACROS
		    displaykey = all || (key_function_to_name_or_macro(functp) != NULL);
#endif
		    if (cp2 && displaykey) {
			cp = keyname(ind, i);
			do { *cp++ = ' '; } while(cp < &buf[16]);
			strcpy(cp, cp2);
			if (!blank && addline(bp, "") == FALSE) return FALSE;
			blank = 1;
			if (addline(bp, buf) == FALSE) return FALSE;
		    }
		}
	    }
	}
	for(ele = &map->map_element[0]; ele < &map->map_element[map->map_num]; ele++) {
	    if(ele->k_prefmap != NULL) {
		for(i = ele->k_base; ele->k_funcp[i - ele->k_base] != prefix; i++) {
		    if(i >= ele->k_num)  /* damaged map */
			return FALSE;
		}
		cp = keyname(ind, i);
		*cp++ = ' ';
		if(showall(cp, ele->k_prefmap, all) == FALSE) return FALSE;
	    }
	}
	return TRUE;
}


INT
help_help(INT f, INT n)
{
    KEYMAP *kp;
    PF	funct;
    INT c;

    if((kp = name_map("help")) == NULL) return FALSE;
    ewprintf("a b c: ");
    do {
	c = getkey(FALSE);
	if (c == CCHR('G')) return ctrlg(FFRAND, 1);
	funct = doscan(kp, c);
    } while (funct == prefix || funct == help_help || funct == rescan);
#ifndef NO_MACRO
    if(macrodef && macrocount < MAXMACRO) macro[macrocount-1].m_funct = funct;
#endif
    return execute(funct, f, n);
}


static char buf2[128];
#ifdef APROPOS_LIST
static char *buf2p;
#endif


INT
apropos_command(INT f, INT n)
{
    char *cp1, *cp2;
    char string[32];
    FUNCTNAMES *fnp;
    BUFFER *bp;
    char *fn;

    if(ereply("apropos: ", string, sizeof(string)) == ABORT) return ABORT;
	/* FALSE means we got a 0 character string, which is fine */
    bp = emptyhelpbuffer();
    if(bp == NULL) return FALSE;
    for(fnp = &functnames[0]; fnp < &functnames[nfunct]; fnp++) {
	if (fnp->n_flag & FUNC_HIDDEN) continue;
	for(cp1 = fnp->n_name; *cp1; cp1++) {
	    cp2 = string;
	    while(*cp2 && *cp1 == *cp2)
		cp1++, cp2++;
	    if(!*cp2) {
		strcpy(buf2, fnp->n_name);
		if (fnp->n_flag & FUNC_ALIAS) {
			fn = function_name(fnp->n_funct);
			snprintf(buf2, sizeof(buf2), "%-32s (alias of \"%s\")",
				fnp->n_name, fn ? fn : "Unknown");
		}
#ifdef APROPOS_LIST
		buf2p = &buf2[strlen(buf2)];
		findbind(fnp->n_funct, buf, map_table[0].p_map);
#endif
		if(addline(bp, buf2) == FALSE) return FALSE;
		break;
	    } else cp1 -= cp2 - string;
	}
    }
    return popbuftop(bp);
}

#ifdef APROPOS_LIST

static void
findbind(PF funct, char *ind, KEYMAP *map)
{
    MAP_ELEMENT *ele;
    INT i;
    char	*cp;
    INT		last;

    last = -1;
    for(ele = &map->map_element[0]; ele < &map->map_element[map->map_num]; ele++) {
	if(map->map_default == funct && ++last < ele->k_base) {
	    cp = keyname(ind, last);
	    if(last < ele->k_base - 1) {
		strcpy(cp, " .. ");
		keyname(cp + 4, ele->k_base - 1);
	    }
	    bindfound();
	}
	last = ele->k_num;
	for(i=ele->k_base; i <= last; i++) {
	    if(funct == ele->k_funcp[i - ele->k_base]) {
		if(funct == prefix) {
		    cp = map_name(ele->k_prefmap);
		    if(strncmp(cp, buf2, strlen(cp)) != 0) continue;
		}
		keyname(ind, i);
		bindfound();
	    }
	}
    }
    for(ele = &map->map_element[0]; ele < &map->map_element[map->map_num]; ele++) {
	if(ele->k_prefmap != NULL) {
	    for(i = ele->k_base; ele->k_funcp[i - ele->k_base] != prefix; i++) {
		if(i >= ele->k_num) return; /* damaged */
	    }
	    cp = keyname(ind, i);
	    *cp++ = ' ';
	    findbind(funct, cp, ele->k_prefmap);
	}
    }
}
#endif

#ifdef APROPOS_LIST

static void
bindfound()
{
    if(buf2p < &buf2[32]) {
	do { *buf2p++ = ' '; } while(buf2p < &buf2[32]);
    } else {
	*buf2p++ = ',';
	*buf2p++ = ' ';
    }
    strcpy(buf2p, buf);
    buf2p += strlen(buf);
}
#endif


/*
 * Mg3a: Create an empty help buffer
 */

BUFFER *
emptyhelpbuffer()
{
	BUFFER *bp;

	bp = bfind("*help*", TRUE);

	if (bp == NULL) return NULL;

	// UNDO
	u_clear(bp);
	//

	bp->b_flag &= ~BFCHG;
	bp->b_flag |= BFREADONLY|BFSYS;
	bp->charset = termcharset;

	if (bclear(bp) != TRUE) return NULL;

	if (setbufmode(bp, "helpbuf") == FALSE) return NULL;

	return bp;
}


#ifdef HELP_RET

/*
 * Mg3a: RET in a help buffer can have a special function.
 */

INT
help_ret(INT f, INT n)
{
	LINE *dotp = curwp->w_dotp;
	unsigned char buf[4];
	INT len;
	int c;

	if (dotp->l_flag == LCUNICODE &&
		sscanf(&ltext(dotp)[llength(dotp)], "%x", &c) == 1)
	{
		initkill();

		ucs_chartostr(utf_8, c, buf, &len);

		if (kinsert_str((char *)buf, len, KFORW) == FALSE) return FALSE;

		kbuf_charset = utf_8;

		ewprintf("%U %s to killbuffer", (INT)c, (lastflag&CFKILL) ? "appended" : "copied");
		return TRUE;
	} else {
		return lnewline_n(n);
	}
}

#endif
