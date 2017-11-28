/*
 * New functions dealing with Unicode in various ways
 */


#include "def.h"

/*
 * Mg3a: Show the bytes of the current character, in hex
 */

INT
showbytes(INT f, INT n)
{

	INT c;
	INT i, nexti;

	i = curwp->w_doto;

	if (i >= llength(curwp->w_dotp)) {

		if (curbp->b_flag & BFUNIXLF) {
			ewprintf("0a");
		} else {
			ewprintf("0d 0a");
		}

		return TRUE;
	}

	estart();

	ucs_char(curbp->charset, curwp->w_dotp, i, &nexti);

	c = lgetc(curwp->w_dotp, i);

	if (c < 0x10) ewprintfc("0%x",c);
	else ewprintfc("%x",c);

	i++;

	while (i < nexti) {
		c = lgetc(curwp->w_dotp, i);

		if (c < 0x10) ewprintfc(" 0%x",c);
		else ewprintfc(" %x",c);

		i++;
	}

	while (i < llength(curwp->w_dotp) &&
		ucs_nonspacing(ucs_char(curbp->charset, curwp->w_dotp, i, &nexti))) {
		ewprintfc(" +");

		while (i < nexti) {
			c = lgetc(curwp->w_dotp, i);

			if (c < 0x10) ewprintfc(" 0%x",c);
			else ewprintfc(" %x",c);

			i++;
		}
	}

	return TRUE;
}


/*
 * Mg3a:
 *
 * Return the offset of the previous character, according to the
 * charset. Punt and report one byte back if no valid character found.
 * Handle being in the middle of an UTF-8 character. Skip backward past
 * nonspacing characters.
 */

INT
ucs_backward(charset_t charset, LINE *lp, INT offset)
{
	INT i, endofchar;
	INT c;

	/* Scan backward until spacing character */

	while (offset > 0) {
		i = offset - 1;

		if (charset == utf_8) {		/* If UTF-8, skip backward */
			for (; i > 0 && i > offset-4; i--) {
				if ((lgetc(lp, i) & 0xc0) != 0x80) break;
			}
		}

		if ((c = ucs_char(charset, lp, i, &endofchar)) >= 0 && endofchar >= offset) {
			if (ucs_nonspacing(c)) {
				/* Nonspacing, continue backward. */
				offset = i;
			} else {
				return i;
			}
		} else {
			return offset - 1;
		}
	}

	return 0;
}


/*
 * Mg3a:
 *
 * Return the offset of the next character, according to the charset. Punt
 * and report one byte forward if no valid character found. Skip forward
 * past nonspacing characters.
 */

INT
ucs_forward(charset_t charset, LINE *lp, INT offset)
{
	INT outoffset, nextoffset, len;
	INT c;

	len = llength(lp);
	outoffset = offset;

	if (outoffset >= len) return len;

	while (outoffset < len) {
		c = ucs_char(charset, lp, outoffset, &nextoffset);

		if (outoffset > offset && !ucs_nonspacing(c)) break;

		outoffset = nextoffset;
	}

	return outoffset;
}


/*
 * Mg3a:
 *
 * Return the offset of the previous character, according to the
 * charset. Punt and report one byte back if no valid character found.
 * Handle being in the middle of an UTF-8 character. Do not skip
 * backward past nonspacing characters.
 */

INT
ucs_prevc(charset_t charset, LINE *lp, INT offset)
{
	INT i, endofchar;
	INT c;

	if (offset > 0) {
		i = offset - 1;

		if (charset == utf_8) {		/* If UTF-8, skip backward */
			for (; i > 0 && i > offset-4; i--) {
				if ((lgetc(lp, i) & 0xc0) != 0x80) break;
			}
		}

		if ((c = ucs_char(charset, lp, i, &endofchar)) >= 0 && endofchar >= offset) {
			return i;
		} else {
			return offset - 1;
		}
	}

	return 0;
}


/*
 * Mg3a: Adjust to the beginning of a valid character.
 */

void
ucs_adjust()
{
	LINE *dotp;
	INT doto;
	INT c;
	charset_t charset;

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	charset = curwp->w_bufp->charset;

	if (doto < llength(dotp)) {
		c = ucs_char(charset, dotp, doto, NULL);

		if (c < 0 || ucs_nonspacing(c)) {
			adjustpos(dotp,
				ucs_backward(charset, dotp,
					ucs_forward(charset, dotp, doto)));
		}
	}
}


/*
 * Mg3a: Output a UCS-4 character as UTF-8 to the screen, raw
 */

static void
utf8put(INT c)
{
	if (c < 0) ttputc('?');
	else if (c < 0x80) ttputc(c);
	else if (c < 0x800) {
		ttputc(0xc0 | (c >> 6));
		ttputc(0x80 | (c & 0x3f));
	} else if (c < 0x10000) {
		ttputc(0xe0 | (c >> 12));
		ttputc(0x80 | ((c >> 6) & 0x3f));
		ttputc(0x80 | (c & 0x3f));
	} else if (c < 0x110000) {
		ttputc(0xf0 | (c >> 18));
		ttputc(0x80 | ((c >> 12) & 0x3f));
		ttputc(0x80 | ((c >> 6) & 0x3f));
		ttputc(0x80 | (c & 0x3f));
	} else {
		ttputc('?');
	}
}


/*
 * Mg3a: Output a UCS-4 character to the screen, appropriate for
 * the editor.
 */

void
ucs_put(INT c)
{
	INT width;

	if (c < 0) {
		ttputc('?');
	} else if (c < 256 && ISCTRL(c)) {
		ttputc('^');
		ttputc(CCHR(c));
	} else if (c < 128 || !termcharset) {
		ttputc(c);
	} else {
		width = ucs_width(c);

		if ((width == 0 && !termdoescombine) || (width == 2 && !termdoeswide) ||
			(c > 0xffff && termdoesonlybmp))
		{
			if (width == 2) ttputc('?');
			ttputc('?');
		} else {
			if (c >= 128 && c < 160) {
				c = translate(cp1252, c);	/* For compatibility display */
			}

			if (termcharset == utf_8) {
				utf8put(c);
			} else {
				c = reverse_translate(termcharset, c);

				if (width == 2 && c == '?') ttputc('?');

				ttputc(c);
			}
		}
	}
}


/*
 * Mg3a: Return the screen width of an UCS-4 character,
 * appropriate for the editor.
 */

INT
ucs_termwidth(INT c)
{
	INT width;

	if (c < 0) {
		return 1;
	} else if (c < 256 && ISCTRL(c)) {
		return 2;
	} else if (c < 128 || !termcharset) {
		return 1;
	} else {
		width = ucs_width(c);

		if (width == 0 && !fullterm) {
			if (!termdoescombine || (c > 0xffff && termdoesonlybmp)) {
				return 1;
			} else if (termisutf8 || reverse_translate(termcharset, c) != '?') {
				return 0;
			} else {
				return 1;
			}
		} else {
			return width;
		}
	}
}


/*
 * Mg3a: Return the screen width of an UCS-4 character,
 * appropriate for the editor for a terminal of full
 * capability.
 */

INT
ucs_fulltermwidth(INT c)
{
	if (c < 0) {
		return 1;
	} else if (c < 256 && ISCTRL(c)) {
		return 2;
	} else if (c < 128 || !termcharset) {
		return 1;
	} else {
		return ucs_width(c);
	}
}


/*
 * Mg3a: The screen width of a string
 */

INT
ucs_termwidth_str(charset_t charset, char *str, INT len)
{
	INT c, width, i;

	width = 0;

	for (i = 0; i < len;) {
		c = ucs_strtochar(charset, str, len, i, &i);
		width += ucs_termwidth(c);
	}

	return width;
}


/*
 * Mg3a: Encode an UCS-4 character as UTF-8 bytes
 */

static INT
utf8chartostr(INT c, unsigned char *dest, INT *outlen) {
	INT len = 1;

	if (c < 0) len = 0;
	else if (c < 0x80) dest[0] = c;
	else if (c < 0x800) {
		dest[0] = (0xc0 | (c >> 6));
		dest[1] = (0x80 | (c & 0x3f));
		len = 2;
	} else if (c < 0x10000) {
		dest[0] = (0xe0 | (c >> 12));
		dest[1] = (0x80 | ((c >> 6) & 0x3f));
		dest[2] = (0x80 | (c & 0x3f));
		len = 3;
	} else if (c < 0x110000) {
		dest[0] = (0xf0 | (c >> 18));
		dest[1] = (0x80 | ((c >> 12) & 0x3f));
		dest[2] = (0x80 | ((c >> 6) & 0x3f));
		dest[3] = (0x80 | (c & 0x3f));
		len = 4;
	} else {
		len = 0;
	}

	if (outlen) *outlen = len;

	if (len == 0) return FALSE;
	return TRUE;
}


/*
 * Mg3a: Return the UTF-8 character at offset in the string. Set
 * outoffset, if non-NULL, to the offset of the next character.
 * Return negative of first byte if not valid UTF-8. Skip the
 * offset range check since it's only called from ucs_strtochar.
 */

static INT
utf8strtochar(char *instr, INT len, INT offset, INT *outoffset)
{
	INT c1, c2, c3, c4;
	INT result;
	INT nextoffset;
	unsigned char *str = (unsigned char *) instr;

	c1 = str[offset];
	result = -c1;
	nextoffset = offset + 1;

	if ((c1 & 0x80) == 0) {
		result = c1;
	} else if ((c1 & 0xe0) == 0xc0 && offset < len - 1) {
		c2 = str[offset+1];

		if ((c2 & 0xc0) == 0x80) {
			result = ((c1 & 0x1f) << 6) | (c2 & 0x3f);
			nextoffset = offset + 2;

			/* Validity checking. */

			if (result < 0x80) {
				result = -c1;
				nextoffset = offset + 1;
			}
		}
	} else if ((c1 & 0xf0) == 0xe0 && offset < len - 2) {
		c2 = str[offset+1];
		c3 = str[offset+2];

		if ((c2 & 0xc0) == 0x80 && (c3 & 0xc0) == 0x80) {
			result = ((c1 & 0x0f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f);
			nextoffset = offset + 3;

			/* Validity checking. */

			if (result < 0x800 ||   /* should we really check for surrogates? Debatable. */
			    (result >= 0xd800 && result < 0xe000) ||
			    (result >= 0xfdd0 && result <= 0xfdef) ||
			    (result & 0xfffe) == 0xfffe) {
				result = -c1;
				nextoffset = offset + 1;
			}
		}
	} else if ((c1 & 0xf8) == 0xf0 && offset < len - 3) {
		c2 = str[offset+1];
		c3 = str[offset+2];
		c4 = str[offset+3];

		if ((c2 & 0xc0) == 0x80 && (c3 & 0xc0) == 0x80 && (c4 & 0xc0) == 0x80) {
			result = ((c1 & 0x07) << 18) | ((c2 & 0x3f) << 12) |
				 ((c3 & 0x3f) << 6) | (c4 & 0x3f);

			nextoffset = offset + 4;

			/* Validity checking. */

			if (result < 0x10000 || result >= 0x110000 ||
			    (result & 0xfffe) == 0xfffe) {
				result = -c1;
				nextoffset = offset + 1;
			}
		}
	}

	if (outoffset != NULL) *outoffset = nextoffset;
	return result;
}


/*
 * Mg3a: Return the UCS-4 character at offset, according to the charset.
 * Set outoffset, if non-NULL, to the next offset. Return negative of
 * first byte if decoding error, -0xfffd if offset out of range.
 */

INT
ucs_strtochar(charset_t charset, char *str, INT len, INT offset, INT *outoffset)
{
	INT nextoffset;
	INT result;

	if (offset < 0 || offset >= len) {
		result = -0xfffd;
		nextoffset = offset;
	} else if (charset == utf_8) {
		return utf8strtochar(str, len, offset, outoffset);
	} else {
		result = translate(charset, CHARMASK(str[offset]));
		nextoffset = offset + 1;
	}

	if (outoffset != NULL) *outoffset = nextoffset;
	return result;
}


/*
 * Mg3a: As ucs_strtochar, but the parameter is a line structure.
 */

INT
ucs_char(charset_t charset, LINE *lp, INT offset, INT *outoffset)
{
	return ucs_strtochar(charset, ltext(lp), llength(lp), offset, outoffset);
}


/*
 * Mg3a: Decode an UCS-4 character to a string of bytes, according to
 * the charset. Return TRUE if the conversion was successful, FALSE
 * otherwise.
 */

INT
ucs_chartostr(charset_t charset, INT c, unsigned char *str, INT *outlen)
{
	INT c2;

	if (charset == utf_8) {
		return utf8chartostr(c, str, outlen);
	} else {
		c2 = str[0] = reverse_translate(charset, c);
		if (outlen) *outlen = 1;
		if (c2 == '?' && c != '?') return FALSE;
		return TRUE;
	}
}


/*
 * Mg3a: Explode a combined chararacter by inserting no-break spaces
 * (if possible, otherwise spaces) before each combining mark.
 */

INT
explode(INT f, INT n)
{
	INT saved_doto;
	INT outoffset;
	INT c, explodec;
	charset_t charset;
	unsigned char buf[4];
	INT len;

	if (curbp->b_flag & BFREADONLY) return readonly();

	saved_doto = curwp->w_doto;
	charset = curbp->charset;

	explodec = 160;

	if (ucs_chartostr(charset, explodec, buf, &len) == FALSE) {
		explodec = ' ';	// no-break space not in buffer charset
	}

	c = ucs_char(charset, curwp->w_dotp, saved_doto, &outoffset);

	if (c == explodec) {
		ewprintf("Nothing to explode");
		return FALSE;
	}

	while (outoffset < llength(curwp->w_dotp)) {
		if (!ucs_nonspacing(ucs_char(charset, curwp->w_dotp, outoffset, NULL)))
			break;
		adjustpos(curwp->w_dotp, outoffset);
		if (linsert_ucs(charset, 1, explodec) == FALSE) return FALSE;
		ucs_char(charset, curwp->w_dotp, curwp->w_doto, &outoffset);
	}

	if (curwp->w_doto == saved_doto) {
		ewprintf("Nothing to explode");
		return FALSE;
	} else {
		adjustpos(curwp->w_dotp, saved_doto);
		return TRUE;
	}
}


/*
 * Mg3a: Pull together combining marks preceded with spaces or no-break
 * spaces with a base character. The opposite of explode.
 */

INT
implode(INT f, INT n)
{
	LINE *lp;
	INT saved_doto;
	INT i, temp;
	INT c1, c2;
	INT done = 0;
	charset_t charset;
	const INT explodec = 160;

	if (curbp->b_flag & BFREADONLY) return readonly();

	lp = curwp->w_dotp;
	i = curwp->w_doto;

	charset = curbp->charset;

	c1 = ucs_char(charset, lp, i, &temp);
	c2 = ucs_char(charset, lp, temp, NULL);

	while (i > 0 && (c1 == ' ' || c1 == explodec) && ucs_nonspacing(c2)) {
		i = ucs_backward(charset, lp, i);
		c1 = ucs_char(charset, lp, i, &temp);
		c2 = ucs_char(charset, lp, temp, NULL);
	}

	saved_doto = i;

	if (c1 == ' ' || c1 == explodec) {
		ewprintf("Nothing to implode");
		return FALSE;
	}

	i = ucs_forward(charset, lp, i);

	while (i < llength(curwp->w_dotp)) {
		c1 = ucs_char(charset, lp, i, &temp);
		c2 = ucs_char(charset, lp, temp, NULL);

		if ((c1 == ' ' || c1 == explodec) && ucs_nonspacing(c2)) {
			adjustpos(curwp->w_dotp, i);
			if (ldeleteraw(temp - i, KNONE) == FALSE) return FALSE;
			lp = curwp->w_dotp;
			i = ucs_forward(charset, lp, i);
			done = 1;
		} else {
			break;
		}
	}

	adjustpos(curwp->w_dotp, saved_doto);

	if (done) {
		return TRUE;
	} else {
		ewprintf("Nothing to implode");
		return FALSE;
	}
}


/*
 * Mg3a: Ask for a number as a C integer, insert as UTF-8.
 */

INT
insert_unicode(INT f, INT n)
{
	char input[20];
	int s;
	INT val;

	if ((s = ereply("Insert Unicode: ", input, 20)) != TRUE) return s;

	if (!getINT(input, &val, 1)) return FALSE;

	if (val < 0 || val > 0x10ffff) return invalid_codepoint();

	return linsert_ucs(utf_8, n, val);
}


/*
 * Mg3a: Ask for a number in hex, insert as UTF-8.
 */

INT
insert_unicode_hex(INT f, INT n)
{
	char input[20];
	int s;
	int val;

	if ((s = ereply("Insert Unicode U+", input, 20)) != TRUE) return s;

	if (sscanf(input, "%x", &val) != 1) {
		ewprintf("Invalid number");
		return FALSE;
	}

	if (val < 0 || val > 0x10ffff) return invalid_codepoint();

	return linsert_ucs(utf_8, n, val);
}


/*
 * Mg3a: Ask for a number as a C integer, insert as a raw byte.
 */

INT
insert_8bit(INT f, INT n)
{
	char input[20];
	int s;
	INT val;

	if ((s = ereply("Insert 8bit: ", input, 20)) != TRUE) return s;

	if (!getINT(input, &val, 1)) return FALSE;

	if (val < 0 || val > 0xff) return invalid_codepoint();

	return linsert(n, val);
}


/*
 * Mg3a: Ask for a number in hex, insert as a raw byte.
 */

INT
insert_8bit_hex(INT f, INT n)
{
	char input[20];
	int s;
	int val;

	if ((s = ereply("Insert 8bit 0x", input, 20)) != TRUE) return s;

	if (sscanf(input, "%x", &val) != 1) {
		ewprintf("Invalid number");
		return FALSE;
	}

	if (val < 0 || val > 0xff) return invalid_codepoint();

	return linsert(n, val);
}


/*
 * Mg3a: Emulation of Emacs "ucs-insert".
 */

INT
ucs_insert(INT f, INT n)
{
	char input[UNIDATALEN], basestr[80], *endstr;
	int s, base, ok = 0, end;
	long val;
#ifdef UCSNAMES
	int has_unicode_data = unicode_database_exists();
#else
	const int has_unicode_data = 0;
#endif

	if (has_unicode_data) {
		if ((s = eread("Unicode (name or hex): ",
			input, sizeof(input), EFUNICODE)) != TRUE) return s;
	} else {
		if ((s = eread("Unicode (hex): ",
			input, sizeof(input), 0)) != TRUE) return s;
	}

	upcase_ascii(input);

	if (	sscanf(input, " #O%lo", &val) == 1 ||
		sscanf(input, " #X%lx", &val) == 1 ||
			(sscanf(input, " #B%79s", basestr) == 1 &&
				(((val = strtol(basestr, &endstr, 2)) != 0 ||
					endstr != basestr))) ||
			(sscanf(input, " #%uR%79s", &base, basestr) == 2 &&
				(((val = strtol(basestr, &endstr, base)) != 0 ||
					endstr != basestr))) ||
		(sscanf(input, "%lx %n", &val, &end) == 1 && !input[end]))
	{
		ok = 1;
	}

	if (!ok) {
		if (!has_unicode_data) {
			ewprintf("Invalid number");
			return FALSE;
		} else {
#ifdef UCSNAMES
			val = ucs_nametonumber(input);

			if (val < 0) {
				ewprintf("Character name not found");
				return FALSE;
			}
#endif
		}
	}

	if (val < 0 || val > 0x10ffff) return invalid_codepoint();

	return linsert_ucs(curbp->charset, n, val);
}


/*
 * Mg3a: Various routines for upcase/downcase and classification.
 * Make it work for Deseret even on 16-bit systems.
 */

int
ucs_islower(INT c)
{
	if (!termcharset) return ISLOWER(c);

	if (c >= WCHAR_MIN && c <= WCHAR_MAX) return iswlower(c);
	else if (c >= 0x10428 && c <= 0x1044f) return 1;
	else return 0;
}

int
ucs_isupper(INT c)
{
	if (!termcharset) return ISUPPER(c);

	if (c >= WCHAR_MIN && c <= WCHAR_MAX) return iswupper(c);
	else if (c >= 0x10400 && c <= 0x10427) return 1;
	else return 0;
}

INT
ucs_tolower(INT c)
{
	if (!termcharset) {
		if (ISUPPER(c)) c = TOLOWER(c);
		return c;
	}

	if (c >= WCHAR_MIN && c <= WCHAR_MAX) return towlower(c);
	else if (c >= 0x10400 && c <= 0x10427) return c + 40;
	else return c;
}

INT
ucs_toupper(INT c)
{
	if (!termcharset) {
		if (ISLOWER(c)) c = TOUPPER(c);
		return c;
	}

	if (c >= WCHAR_MIN && c <= WCHAR_MAX) return towupper(c);
	else if (c >= 0x10428 && c <= 0x1044f) return c - 40;
	else return c;
}

int
ucs_isalpha(INT c)
{
	if (!termcharset) return ISALPHA(c);

	if (c >= WCHAR_MIN && c <= WCHAR_MAX) return iswalpha(c);
	else if (c > 0xffff) return 1;
	else return 0;
}

int
ucs_isalnum(INT c)
{
	if (!termcharset) return ISALNUM(c);

	if (c >= WCHAR_MIN && c <= WCHAR_MAX) return iswalnum(c);
	else if (c > 0xffff) return 1;
	else return 0;
}

/*
 * Return true if in "word" for the editor.
 */

int
ucs_isword(INT c)
{
	if (!termcharset) return ISWORD(c);

	if (c < 0) return 0;
	else return ucs_isalnum(c) || ucs_nonspacing(c);
}


/*
 * Mg3a: Upcase/downcase/titlecase handling for the Latin one-to-one
 * and one-to-many translations. Return 1 if change, 0 otherwise.
 */

#define	SPEC_FLAGS	0xfff0
#define SPEC_NORM	0x10
#define SPEC_TR_AZ	0x20
#define SPEC_LENGTH	0x0f

INT manual_locale = 0;

int
ucs_changecase(INT tocase, INT c, INT **out, INT *outlen)
{
	static const ucs2 downcase_table[] = {
		0x0130, SPEC_TR_AZ | 1, 0x0069,
		0x0049, SPEC_TR_AZ | 1, 0x0131,	/*	49=I 69=i 130=İ 131=ı */
		0
	};

	static const ucs2 upcase_table[] = {		/* only the latin ones */
		0x00DF, SPEC_NORM  | 2, 0x0053, 0x0053,
		0xFB00, SPEC_NORM  | 2, 0x0046, 0x0046,
		0xFB01, SPEC_NORM  | 2, 0x0046, 0x0049,
		0xFB02, SPEC_NORM  | 2, 0x0046, 0x004C,
		0xFB03, SPEC_NORM  | 3, 0x0046, 0x0046, 0x0049,
		0xFB04, SPEC_NORM  | 3, 0x0046, 0x0046, 0x004C,
		0xFB05, SPEC_NORM  | 2, 0x0053, 0x0054,
		0xFB06, SPEC_NORM  | 2, 0x0053, 0x0054,
		0x0149, SPEC_NORM  | 2, 0x02bc, 0x004e,
		0x01f0, SPEC_NORM  | 2, 0x004a, 0x030c,
		0x1e96, SPEC_NORM  | 2, 0x0048, 0x0331,
		0x1e97, SPEC_NORM  | 2, 0x0054, 0x0308,
		0x1e98, SPEC_NORM  | 2, 0x0057, 0x030a,
		0x1e99, SPEC_NORM  | 2, 0x0059, 0x030a,
		0x1e9a, SPEC_NORM  | 2, 0x0041, 0x02be,
		0x0069, SPEC_TR_AZ | 1, 0x0130,
		0x0131, SPEC_TR_AZ | 1, 0x0049,
		0
	};

	static const ucs2 titlecase_table[] = {		/* only the latin ones */
		0x00DF, SPEC_NORM  | 2, 0x0053, 0x0073,
		0xFB00, SPEC_NORM  | 2, 0x0046, 0x0066,
		0xFB01, SPEC_NORM  | 2, 0x0046, 0x0069,
		0xFB02, SPEC_NORM  | 2, 0x0046, 0x006C,
		0xFB03, SPEC_NORM  | 3, 0x0046, 0x0066, 0x0069,
		0xFB04, SPEC_NORM  | 3, 0x0046, 0x0066, 0x006C,
		0xFB05, SPEC_NORM  | 2, 0x0053, 0x0074,
		0xFB06, SPEC_NORM  | 2, 0x0053, 0x0074,
		0x0149, SPEC_NORM  | 2, 0x02bc, 0x004e,
		0x01f0, SPEC_NORM  | 2, 0x004a, 0x030c,
		0x1e96, SPEC_NORM  | 2, 0x0048, 0x0331,
		0x1e97, SPEC_NORM  | 2, 0x0054, 0x0308,
		0x1e98, SPEC_NORM  | 2, 0x0057, 0x030a,
		0x1e99, SPEC_NORM  | 2, 0x0059, 0x030a,
		0x1e9a, SPEC_NORM  | 2, 0x0041, 0x02be,
		0x01f1, SPEC_NORM  | 1, 0x01f2,
		0x01f2, SPEC_NORM  | 1, 0x01f2,
		0x01f3, SPEC_NORM  | 1, 0x01f2,
		0x01c4, SPEC_NORM  | 1, 0x01c5,
		0x01c5, SPEC_NORM  | 1, 0x01c5,
		0x01c6, SPEC_NORM  | 1, 0x01c5,
		0x01c7, SPEC_NORM  | 1, 0x01c8,
		0x01c8, SPEC_NORM  | 1, 0x01c8,
		0x01c9, SPEC_NORM  | 1, 0x01c8,
		0x01ca, SPEC_NORM  | 1, 0x01cb,
		0x01cb, SPEC_NORM  | 1, 0x01cb,
		0x01cc, SPEC_NORM  | 1, 0x01cb,
		0x0069, SPEC_TR_AZ | 1, 0x0130,
		0x0131, SPEC_TR_AZ | 1, 0x0049,
		0
	};

	int this_locale = SPEC_NORM;
	INT i, j, len;
	const ucs2 *p = downcase_table;
	static INT result[3];

	if (!termcharset) {
		this_locale = 0;
	} else if (manual_locale) {
		this_locale = SPEC_TR_AZ | SPEC_NORM;
	}

	if (tocase == CASEUP) {
		p = upcase_table;
	} else if (tocase == CASETITLE) {
		p = titlecase_table;
	}

	i = 0;
	*out = result;

	while (p[i]) {
		len = p[i+1] & SPEC_LENGTH;

		if (p[i] == c && (p[i+1] & this_locale)) {
			for (j = 0; j < len; j++) result[j] = p[i+2+j];
			*outlen = len;
			return result[0] != c;
		}

		i = i + len + 2;
	}

	if (tocase == CASEDOWN) {
		result[0] = ucs_tolower(c);
	} else {
		result[0] = ucs_toupper(c);
	}

	*outlen = 1;
	return result[0] != c;
}


/*
 * Mg3a: Case-fold is a bit more tolerant on Turkish than tolower.
 */

INT
ucs_case_fold(INT c)
{
	if (c == 0x49 || (c >= 0x130 && c <= 0x131)) return 'i';

	return ucs_tolower(c);
}


/*
 * Mg3a: Case insensitive compare, with charset.
 */

int
ucs_strcasecmp(charset_t charset, char *str1, char *str2)
{
	INT i, j;
	INT len1, len2;
	int c1, c2;

	len1 = strlen(str1);
	len2 = strlen(str2);

	i = 0;
	j = 0;

	while (i < len1 && j < len2) {
		c1 = ucs_strtochar(charset, str1, len1, i, &i);
		c2 = ucs_strtochar(charset, str2, len2, j, &j);

		c1 = ucs_case_fold(c1);
		c2 = ucs_case_fold(c2);

		if (c1 != c2) return c1 - c2;
	}

	if (len1 - i == len2 - j) return 0;
	else if (len1 - i < len2 - j) return -1;
	else return 1;
}


/*
 * Mg3a: Count number of Unicode codepoints in the string in charset.
 */

INT
ucs_strchars(charset_t charset, char *str)
{
	INT count;
	INT i, len;

	len = strlen(str);
	count = 0;

	for (i = 0; i < len; ) {
		ucs_strtochar(charset, str, len, i, &i);
		count++;
	}

	return count;
}


/*
 * Mg3a: Count number of initially-same codepoints in two strings,
 * optionally case-insensitive.
 */

INT
ucs_strscan(charset_t charset, char *str1, char *str2, INT case_insensitive)
{
	INT count;
	INT i, j, len1, len2, c1, c2;

	i = 0;
	j = 0;

	count = 0;

	len1 = strlen(str1);
	len2 = strlen(str2);

	while (i < len1 && j < len2) {
		c1 = ucs_strtochar(charset, str1, len1, i, &i);
		c2 = ucs_strtochar(charset, str2, len2, j, &j);

		if (case_insensitive) {
			c1 = ucs_case_fold(c1);
			c2 = ucs_case_fold(c2);
		}

		if (c1 != c2) break;

		count++;
	}

	return count;
}


/*
 * Mg3a: Complete instr to outstr, returning number of Unicode
 * codepoints in the completion. Optionally indicate whether it's a
 * full or ambiguous completion.
 */

INT
ucs_complete(char *instr, char *outstr, INT outstr_size,
	char *(*getnext)(INT first), INT case_insensitive, INT *statusptr)
{
	INT incount, count, outcount, strcount, minstrcount, first, status;
	char *str;
	size_t matches;

	first = 1;
	matches = 0;
	outcount = 0;
	status = 0;
	minstrcount = 0;

	incount = ucs_strchars(termcharset, instr);
	stringcopy(outstr, instr, outstr_size);

	while ((str = getnext(first))) {
		first = 0;
		count = ucs_strscan(termcharset, instr, str, case_insensitive);

		if (count == incount) {
			strcount = ucs_strchars(termcharset, str);

			if (matches++ == 0) {
				stringcopy(outstr, str, outstr_size);
				outcount = strcount;
				minstrcount = strcount;
			} else {
				count = ucs_strscan(termcharset, str, outstr, case_insensitive);

				if (count < outcount) outcount = count;

				if (strcount < minstrcount) {
					stringcopy(outstr, str, outstr_size);
					minstrcount = strcount;
				}
			}
		}
	}

	if (outcount != 0 && minstrcount == outcount) status |= MATCHFULL;
	if (outcount == incount && matches > 1) status |= MATCHAMBIGUOUS;
	if (outcount == 0 && incount != 0) status |= MATCHNONE;
	if (incount == 0) status |= MATCHEMPTY;

	if (statusptr) *statusptr = status;

	return outcount;
}
