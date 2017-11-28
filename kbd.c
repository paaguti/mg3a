/*
 *		Terminal independent keyboard handling.
 */

#include	"def.h"
#include	"kbd.h"
#include	"key.h"
#include 	"macro.h"

key_struct key;		// The actual definition

INT	negative_argument(INT f, INT n);
INT	digit_argument(INT f, INT n);
INT	insert(INT f, INT n);
INT	extend(INT f, INT n);


static int
checkkeycount()
{
	if (key.k_count >= MAXKEY - 1) {
		ewprintf("Max Key count reached");
		return 1;
	}
	return 0;
}

#ifdef	BSMAP
INT bs_map = BSMAP;

/*
 * Toggle backspace mapping
 */

INT
bsmap(INT f, INT n)
{
	if(f & FFARG)	bs_map = n > 0;
	else		bs_map = ! bs_map;
	ewprintf("Backspace mapping %sabled", bs_map ? "en" : "dis");
	return TRUE;
}
#endif

#ifndef NO_DPROMPT
#define PROMPTL ((MAXSTRKEY+1)*MAXKEY+1)
  char	prompt[PROMPTL], *promptp;
#endif

static	INT	pushed = FALSE;
static	INT	pushedc;

void
ungetkey(INT c)
{
	pushedc = c;
	pushed = TRUE;
}


/*
 * Mg3a: Translate an 8-bit character to UCS-4.
 * Leave alone if charset is NULL.
 */

INT
translate(charset_t charset, INT c)
{
	if (c < 0 || c >= 256) {
		return 0xfffd;
	} else if (c < 128 || !charset) {
		return c;
	} else {
		return charset[c - 128];
	}
}

#if TESTCMD == 3
int revhit=0, revtotal=0, revmiss=0;
#endif


/*
 * Mg3a: Reverse-translate from UCS-4 to an 8-bit charset. Use '?' as
 * substitution character if no match. Leave alone if charset is NULL.
 */

INT
reverse_translate(charset_t charset, INT c)
{
	INT i, ri;
	static uint_least8_t reverse_cache[256];

	if (c < 0 || c > 0x10ffff || c == 0xfffd) {
		return '?';
	} else if (c < 128) {
		return c;
	} else if (charset) {
		ri = reverse_cache[c & 255] & 127;
#if TESTCMD == 3
		revtotal++;
		if (charset[ri] == c) revhit++;
#endif
		if (charset[ri] == c) return ri + 128;

		for (i = 0; i < 128; i++) {
			if (c == charset[i]) {
				reverse_cache[c & 255] = i;
				return i + 128;
			}
		}
#if TESTCMD == 3
		revmiss++;
#endif
	} else if (c < 256) {
		return c;
	}

	return '?';
}


/*
 * Mg3a: Read an UTF-8 character from the keyboard.
 * Return '?' on any error.
 */

static INT
getutf8()
{
	INT c1, c2, c3, c4;

	c1 = getkbd();

	if ((c1 & 0x80) == 0) {
		return c1;
	} else if ((c1 & 0xe0) == 0xc0) {
		c2 = getkbd();
		if ((c2 & 0xc0) != 0x80) return '?';

		return ((c1 & 0x1f) << 6) | (c2 & 0x3f);
	} else if ((c1 & 0xf0) == 0xe0) {
		c2 = getkbd();
		if ((c2 & 0xc0) != 0x80) return '?';
		c3 = getkbd();
		if ((c3 & 0xc0) != 0x80) return '?';

		return ((c1 & 0x0f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f);
	} else if ((c1 & 0xf8) == 0xf0) {
		c2 = getkbd();
		if ((c2 & 0xc0) != 0x80) return '?';
		c3 = getkbd();
		if ((c3 & 0xc0) != 0x80) return '?';
		c4 = getkbd();
		if ((c4 & 0xc0) != 0x80) return '?';

		return ((c1 & 0x07) << 18) | ((c2 & 0x3f) << 12) |
		       ((c3 & 0x3f) << 6) | (c4 & 0x3f);
	} else {
		return '?';
	}
}


/*
 * Mg3a: getkey has been modified to return UCS-4
 */

INT
getkey(INT flag)
{
	INT	c;

	if (pushed) {
		pushed = FALSE;
		return pushedc;
	}
#ifndef NO_DPROMPT
	if (flag) {
		if(prompt[0]!='\0' && !waitforinput(2000)) {
			ewprintf("%s", prompt);	/* avoid problems with % */
			update();		/* put the cursor back	 */
			epresf = KPROMPT;
		}
		if(promptp > prompt) *(promptp-1) = ' ';
	}
#endif
	if (termisutf8) {
		c = getutf8();
	} else {
		c = translate(termcharset, getkbd());
	}
#ifdef 	BSMAP
	if(bs_map) {
		if(c==CCHR('H')) c=CCHR('?');
		else if(c==CCHR('?')) c=CCHR('H');
	}
#endif
#ifndef NO_DPROMPT
	if (flag && promptp < &prompt[PROMPTL - MAXSTRKEY - 2]) {
	    promptp = keyname(promptp, c);
	    *promptp++ = '-';
	    *promptp = '\0';
	}
#endif
	return c;
}


/*
 * doscan scans a keymap for a keyboard character and returns a
 * pointer to the function associated with that character. Sets ele to
 * the keymap element the keyboard was found in as a side effect.
 */

MAP_ELEMENT *ele;

PF
doscan(KEYMAP *map, INT c)
{
    MAP_ELEMENT *elec = &map->map_element[0];
    MAP_ELEMENT *last = &map->map_element[map->map_num];

    while(elec < last && c > elec->k_num) elec++;
    ele = elec;			/* used by prefix and binding code	*/
    if(elec >= last || c < elec->k_base)
	return map->map_default;
    return elec->k_funcp[c - elec->k_base];
}


INT
doin()
{
	KEYMAP	*curmap;
	PF	funct;

#ifndef NO_DPROMPT
    *(promptp = prompt) = '\0';
#endif
    curmap = mode_map(curbp->b_modes[curbp->b_nmodes]);
    key.k_count = 0;
    while((funct=doscan(curmap,(key.k_chars[key.k_count++]=getkey(TRUE))))
		== prefix) {
	if (checkkeycount()) return FALSE;
	curmap = ele->k_prefmap;
    }
#ifndef NO_MACRO
    if(macrodef && macrocount < MAXMACRO) {
	macro[macrocount++].m_funct = funct;
    }
#endif
    return execute(funct, 0, 1);
}


INT
rescan(INT f, INT n)
{
    INT c;
    KEYMAP *curmap;
    INT i;
    PF	fp = NULL;
    INT mode = curbp->b_nmodes;

    for(;;) {
	if(ucs_isupper(key.k_chars[key.k_count-1])) {
	    c = ucs_tolower(key.k_chars[key.k_count-1]);
	    curmap = mode_map(curbp->b_modes[mode]);
	    for(i=0; i < key.k_count-1; i++) {
		if((fp=doscan(curmap,(key.k_chars[i]))) != prefix) break;
		curmap = ele->k_prefmap;
	    }
	    if(fp==prefix) {
		if((fp = doscan(curmap, c)) == prefix)
		    while((fp=doscan(curmap,key.k_chars[key.k_count++] =
			    getkey(TRUE))) == prefix) {
			if (checkkeycount()) return FALSE;
			curmap = ele->k_prefmap;
		    }
		if(fp!=rescan) {
#ifndef NO_MACRO
		    if(macrodef && macrocount <= MAXMACRO)
			macro[macrocount-1].m_funct = fp;
#endif
		    return execute(fp, f, n);
		}
	    }
	}
	/* try previous mode */
	if(--mode < 0) {
		ewprintf("That key is not bound");
		thisflag |= CFNOQUIT;
		return ABORT;
	}
	curmap = mode_map(curbp->b_modes[mode]);
	for(i=0; i < key.k_count; i++) {
	    if((fp=doscan(curmap,(key.k_chars[i]))) != prefix) break;
	    curmap = ele->k_prefmap;
	}
	if(fp==prefix) {
	    while((fp=doscan(curmap,key.k_chars[i++]=getkey(TRUE)))
		    == prefix) {
		if (checkkeycount()) return FALSE;
		curmap = ele->k_prefmap;
	    }
	    key.k_count = i;
	}
	if(fp!=rescan && i>=key.k_count-1) {
#ifndef NO_MACRO
	    if(macrodef && macrocount <= MAXMACRO)
		macro[macrocount-1].m_funct = fp;
#endif
	    return execute(fp, f, n);
	}
    }
}


static INT
overflow()
{
	ewprintf("Integer overflow in argument");
	return FALSE;
}


static INT
repeatedrepeat()
{
	ewprintf("Repeated repeat count");
	return FALSE;
}


INT
universal_argument(INT f, INT n)
{
    INT c, nn;
    KEYMAP *curmap;
    PF	funct;

    if (f & FFUNIV2) return repeatedrepeat();	// To guard against infinite recursion

    if (f & FFNUM) {				// ^U after a repeat count
    	f |= FFUNIV2;
    	nn = n;
    } else if (f & FFUNIV) {			// Repeated ^U
    	if (n >= MAXINT / 4 || n <= MININT / 4) return overflow();
	nn = n * 4;
    } else {					// Default to 4
    	nn = 4;
    }

    key.k_chars[0] = c = getkey(TRUE);
    key.k_count = 1;

    if (!(f & FFUNIV2)) {
	if (c == '-') return negative_argument(f, 1);
	if (c >= '0' && c <= '9') return digit_argument(f, 1);
    }

    curmap = mode_map(curbp->b_modes[curbp->b_nmodes]);

    while ((funct = doscan(curmap, c)) == prefix) {
	curmap = ele->k_prefmap;
	key.k_chars[key.k_count++] = c = getkey(TRUE);
	if (checkkeycount()) return FALSE;
    }

#ifndef NO_MACRO
    if (macrodef && macrocount < MAXMACRO-2) {
	if (f & FFARG) macrocount -= 3;
	macro[macrocount++].m_flag  = f ? f : FFUNIV;	/* Mg3a: preserve flag */
	macro[macrocount++].m_count = nn;
	macro[macrocount++].m_funct = funct;
    }
#endif

    return execute(funct, f ? f : FFUNIV, nn);		/* Mg3a: Pass along same flag, or FFUNIV */
}


INT
digit_argument(INT f, INT n)
{
    INT nn, c;
    KEYMAP *curmap;
    PF	funct;

    if (f & (FFUNIV2|FFNUM)) return repeatedrepeat();

    nn = key.k_chars[key.k_count-1] - '0';
    if (nn < 0 || nn > 9) nn = 0;

    for (;;) {
	c = getkey(TRUE);
	if(c < '0' || c > '9') break;
	if (nn > (MAXINT - 9) / 10) return overflow();
	nn *= 10;
	nn += c - '0';
    }

    if (MININT + nn >= 0) return overflow();	// Truly paranoid

    key.k_chars[0] = c;
    key.k_count = 1;

    curmap = mode_map(curbp->b_modes[curbp->b_nmodes]);

    while ((funct = doscan(curmap, c)) == prefix) {
	curmap = ele->k_prefmap;
	key.k_chars[key.k_count++] = c = getkey(TRUE);
	if (checkkeycount()) return FALSE;
    }

#ifndef NO_MACRO
    if (macrodef && macrocount < MAXMACRO-2) {
	if (f & FFARG) macrocount -= 3;
	else macro[macrocount-1].m_funct = universal_argument;
	macro[macrocount++].m_flag  = FFNUM;		/* preserve flag */
	macro[macrocount++].m_count = nn;
	macro[macrocount++].m_funct = funct;
    }
#endif

    return execute(funct, FFNUM, nn);
}


INT
negative_argument(INT f, INT n)
{
    INT nn = 0, c;
    KEYMAP *curmap;
    PF	funct;

    if (f & (FFUNIV2|FFNUM)) return repeatedrepeat();

    for (;;) {
	c = getkey(TRUE);
	if(c < '0' || c > '9') break;
	if (nn > (MAXINT - 9) / 10) return overflow();
	nn *= 10;
	nn += c - '0';
    }

    if (nn == 0) nn = 1;
    else if (MININT + nn >= 0) return overflow();

    nn = -nn;
    key.k_chars[0] = c;
    key.k_count = 1;

    curmap = mode_map(curbp->b_modes[curbp->b_nmodes]);

    while ((funct = doscan(curmap, c)) == prefix) {
	curmap = ele->k_prefmap;
	key.k_chars[key.k_count++] = c = getkey(TRUE);
	if (checkkeycount()) return FALSE;
    }

#ifndef NO_MACRO
    if (macrodef && macrocount < MAXMACRO-2) {
	if (f & FFARG) macrocount -= 3;
	else macro[macrocount-1].m_funct = universal_argument;
	macro[macrocount++].m_flag  = FFNUM;		/* preserve flag */
	macro[macrocount++].m_count = nn;
	macro[macrocount++].m_funct = funct;
    }
#endif

    return execute(funct, FFNUM, nn);
}


/*
 * Insert a character. While defining a macro, create a "LINE"
 * containing all inserted characters.
 */

INT
selfinsert(INT f, INT n)
{
    INT c;
#ifndef NO_MACRO
    LINE *lp;
    INT len, linelen;
    unsigned char buf[4];
#endif

    inhibit_undo_boundary(f, selfinsert);

    if (curbp->b_flag & BFREADONLY) return readonly();

    if (n < 0)	return FALSE;
    if (n == 0) return TRUE;

    c = key.k_chars[key.k_count-1];

#ifndef NO_MACRO
    // Much simplified macro-add code. Append to fixed-size chunks,
    // create new chunk if anything unusual.

    if (macrodef && macrocount < MAXMACRO) {
	ucs_chartostr(termcharset, c, buf, &len);

	if ((f & FFARG) || !(lastflag & CFINS) || llength(maclcur) > lsize(maclcur) - len) {
		if ((lp = lalloc(n == 1 ? 80 : len)) == NULL) return FALSE;

	        macro[macrocount-1].m_funct = insert;

		insertafter(maclcur, lp);
		maclcur = lp;

		memcpy(ltext(maclcur), buf, len);
		lsetlength(maclcur, len);
		maclcur->l_flag = macrocount;

		if (n == 1) thisflag |= CFINS;
//		ewprintf("new n=%d, len=%d", n, len);
	} else {
		macrocount--;

		linelen = llength(maclcur);

		memcpy(&ltext(maclcur)[linelen], buf, len);
		lsetlength(maclcur, linelen + len);

		thisflag |= CFINS;
//		ewprintf("append n=%d, len=%d", n, linelen + len);
	}
    }
#endif

    return linsert_overwrite(curbp->charset, n, c);
}


INT quoted_char_radix = 8;	/* Radix of "quoted-insert" */


/*
 * Mg3a: Rewritten a lot. Reads more digits like Emacs. Does not rely
 * on selfinsert(). Always inserts.
 */

INT
quote(INT f, INT n)
{
    INT c, num, base, digit;
    INT s;

    base = quoted_char_radix;

    if (base < 2 || base > 10 + 'z' - 'a' + 1) {
    	ewprintf("Invalid base %d", base);
	return FALSE;
    }

    if (curbp->b_flag & BFREADONLY) return readonly();

    if (n < 0) return FALSE;

    if (inmacro) {
	if ((s = macro_get_INT(&c)) != TRUE) return s;
	return linsert_ucs(curbp->charset, n, c);
    }

    c = getkey(TRUE);
    digit = basedigit(c, base);

    if (digit >= 0 && !(curbp->b_flag & BFOVERWRITE)) {
	num = 0;

    	while (1) {
		if (num > MAXINT / base || num * base > 0x10ffff - digit) {
			return invalid_codepoint();
		}

		num = num * base + digit;

		c = getkey(TRUE);
		digit = basedigit(c, base);

		if (c == '\r') {
			break;
		} else if (digit < 0) {
			ungetkey(c);
			break;
		}
	}

	c = num;
    }

    if (macrodef) {
	if ((s = macro_store_INT(c)) != TRUE) return s;
    }

    return linsert_ucs(curbp->charset, n, c);
}
