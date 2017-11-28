#include "def.h"

/*
 * Mg3a: Routines to deal with and show Unicode character names.
 */


#ifdef UCSNAMES


/* Name of Unicode database file */

static char unicode_data[NFILEN] = "";


/*
 * Test if we have a Unicode database
 */

int
unicode_database_exists()
{
	return unicode_data[0] && !access(unicode_data, R_OK);
}


/*
 * Set name of Unicode database file. Also allow clearing it.
 */

INT
set_unicode_data(INT f, INT n)
{
	char buf[NFILEN], *adjf;
	INT s;

	s = eread("Unicode Data File: (default none) ", buf, sizeof(buf), EFFILE);

	if (s == ABORT) {
		return s;
	} else if (s == FALSE) {
		unicode_data[0] = 0;
		eerase();
		return TRUE;
	}

	if ((adjf = adjustname(buf)) == NULL) return FALSE;

	stringcopy(unicode_data, adjf, NFILEN);

	return TRUE;
}


/*
 * Put a null after the second database field (the character name) and
 * return a pointer to that field. Return NULL if error.
 */

static char *
ucs_name(char *s)
{
	char *cp1, *cp2;

	if ((cp1 = strchr(s, ';')) == NULL) return NULL;
	if ((cp2 = strchr(cp1 + 1, ';')) == NULL) return NULL;

	*cp2 = 0;

	return cp1 + 1;
}


/*
 * Get next Unicode character name for completion.
 */

char *
getnext_unicode(INT first)
{
	static FILE *f;
	static char line[UNIDATALEN];
	char *cp;

	if (unicode_data[0] == 0) return NULL;

	if (first) {
		if ((f = fopen(unicode_data, "r")) == NULL) return NULL;
	}

	while (1) {
		if (!fgets(line, sizeof(line), f) || ((cp = ucs_name(line)) == NULL)) {
			fclose(f);
			return NULL;
		}

		if (ISALPHA(*cp)) break;
	}

	return cp;
}


/*
 * Given a (full, case-insensitive) Unicode character name, return its
 * number. Return -1 on error.
 */

INT
ucs_nametonumber(char *s)
{
	FILE *f;
	char line[UNIDATALEN], *cp;
	int val = -1;

	if (unicode_data[0] == 0 || !ISALPHA(*s)) return -1;

	if ((f = fopen(unicode_data, "r")) == NULL) return -1;

	while (fgets(line, sizeof(line), f)) {
		if ((cp = ucs_name(line))) {
			if (ascii_strcasecmp(cp, s) == 0) {
				if (sscanf(line, "%x", &val) != 1) val = -1;
				break;
			}
		} else {
			break;
		}
	}

	fclose(f);
	return val;
}


/*
 * Return the character name for a Unicode codepoint.
 * Return the block name (if possible) if the character is not found.
 * Return NULL on error.
 */

char *
ucs_numbertoname(INT val)
{
	FILE *f;
	static char line[UNIDATALEN];
	char *cp = NULL, *endstr, *cp2;
	long i;

	if (unicode_data[0] == 0) return NULL;

	if ((f = fopen(unicode_data, "r")) == NULL) return NULL;

	while (fgets(line, sizeof(line), f)) {
		i = strtol(line, &endstr, 16);		// Faster than sscanf

		if (endstr != line && i >= val) {
			cp = ucs_name(line);

			if (i != val) {
				if (cp && *cp == '<' && (cp2 = strstr(cp, ", Last>"))) {
					strcpy(cp2, ">");
				} else {
					cp = NULL;
				}
			}

			break;
		}
	}

	fclose(f);
	return cp;
}


/*
 * List matching Unicode codepoints, with character names. You can
 * match either codepoint (with "U+(hex)") or a substring of the two
 * first database fields (code and name).
 *
 * If given an argument, match and show the whole database line.
 */

INT
list_unicode(INT f, INT n)
{
	char line[UNIDATALEN], match[UNIDATALEN], matchupper[UNIDATALEN];
	char *cp = NULL, outline[UNIDATALEN];
	char charnum[30];
	FILE *uf;
	INT matched, all;
	BUFFER *bp;
	int val, matchval;

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	all = (f & FFARG);

	if (unicode_data[0] == 0) {
		ewprintf("No Unicode data");
		return FALSE;
	}

	if (eread("Match%s (U+(hex) or string): ", match, sizeof(match), EFUNICODE,
		all ? " all" : "") == ABORT)
	{
		return ABORT;
	}

	if ((uf = fopen(unicode_data, "r")) == NULL) {
		ewprintf("Error opening Unicode Data file: %s", strerror(errno));
		return FALSE;
	}

	stringcopy(matchupper, match, sizeof(matchupper));
	upcase_ascii(matchupper);

	if (sscanf(matchupper, " U+%x", &matchval) != 1) matchval = -1;

	while (fgets(line, sizeof(line), uf)) {
		chomp(line);

		if ((!all && (cp = ucs_name(line)) == NULL) ||
			sscanf(line, "%x;", &val) != 1)
		{
			ewprintf("Wrong format: %s", line);
			fclose(uf);
			return FALSE;
		}

		if (matchval >= 0) {
			matched = (matchval == val);
		} else {
			matched = (strstr(line, match) || strstr(line, matchupper));
		}

		if (matched) {
			snprintf(charnum, sizeof(charnum), "%04X", val);

			if (all) {
				stringcopy(outline, line, sizeof(outline));
			} else {
				snprintf(outline, sizeof(outline), "U+%-6s  %s", charnum, cp);
			}

			if (addline_extra(bp, outline, LCUNICODE, charnum) == FALSE) {
				fclose(uf);
				return FALSE;
			}
		}
	}

	fclose(uf);
	return popbuftop(bp);
}


/*
 * Add a codepoint in a line to a buffer for advanced display of the
 * current character.
 */

#define ADVFORMAT "%-6s %-20s %-9s %s"

static INT
add_advanced_display_line(BUFFER *bp, INT f, INT n, int c,
	char *charptr, INT charlen, INT screenwidth)
{
	char line[UNIDATALEN + 256];
	char width[10];
	char bytes[30], bytetemp[10];
	char unicode[15], hexnum[10];
	char *name;
	INT decimal = 0, hex = 1;
	INT i;

	if (((f & FFARG) == FFUNIV && n == 16) || n == 2) {decimal = 1; hex = 0;}
	else if (((f & FFARG) == FFUNIV && n == 64) || n == 3) hex = 0;

	snprintf(width, sizeof(width), "%d [%d]", (int)ucs_width(c), (int)screenwidth);

	bytes[0] = 0;

	for (i = 0; i < charlen; i++) {
		if (hex) {
			snprintf(bytetemp, sizeof(bytetemp), "0x%02x", CHARMASK(charptr[i]));
		} else if (decimal) {
			snprintf(bytetemp, sizeof(bytetemp), "%d", CHARMASK(charptr[i]));
		} else {
			snprintf(bytetemp, sizeof(bytetemp), "0%03o", CHARMASK(charptr[i]));
		}

		stringconcat2(bytes, (i == 0) ? "" : " ", bytetemp, sizeof(bytes));
	}

	if (hex) {
		snprintf(unicode, sizeof(unicode), "U+%04X", c);
	} else if (decimal) {
		snprintf(unicode, sizeof(unicode), "%d", c);
	} else {
		snprintf(unicode, sizeof(unicode), "0%03o", c);
	}

	if ((name = ucs_numbertoname(c)) == NULL) name = "(undefined)";

	snprintf(line, sizeof(line), ADVFORMAT, width, bytes, unicode, name);
	snprintf(hexnum, sizeof(hexnum), "%x", c);

	return addline_extra(bp, line, LCUNICODE, hexnum);
}

#ifdef UCSNAMES2

/*
 * Add a database line for "ch" to "bp"
 */

static int
showdatabaseline(BUFFER *bp, int ch)
{
	FILE *f;
	char line[UNIDATALEN];
	char *endstr, hexnum[10];
	long i;

	if (unicode_data[0] == 0) return 0;

	if ((f = fopen(unicode_data, "r")) == NULL) return 0;

	while (fgets(line, sizeof(line), f)) {
		i = strtol(line, &endstr, 16);

		if (endstr != line && i >= ch) {
			fclose(f);

			if (i != ch) {
				snprintf(line, sizeof(line), "(U+%04X not found)", ch);
			} else {
				chomp(line);
			}

			snprintf(hexnum, sizeof(hexnum), "%x", ch);
			return addline_extra(bp, line, LCUNICODE, hexnum) == TRUE;
		}
	}

	fclose(f);
	return 0;
}


/*
 * Do a raw database display for the current character.
 */

static INT
showdatabase(BUFFER *bp, LINE *dotp, INT doto, charset_t charset, INT unixlf)
{
	INT ret = FALSE, ch, startdoto = doto, nextdoto;

	if (doto == llength(dotp)) {
		if (!unixlf && !showdatabaseline(bp, 13)) goto end;
		if (!showdatabaseline(bp, 10)) goto end;
	} else {
		while (doto < llength(dotp)) {
			ch = ucs_char(charset, dotp, doto, &nextdoto);

			if (doto > startdoto && !ucs_nonspacing(ch)) break;
			if (!showdatabaseline(bp, ch)) goto end;

			doto = nextdoto;
		}
	}

	ret = popbuftop(bp);
end:
	free(dotp);	// Not lfree
	return ret;
}

#endif

/*
 * Advanced display of the current character.
 */

INT
advanced_display(INT f, INT n)
{
	INT ch, screenwidth;
	BUFFER *bp;
	INT nextdoto, doto = curwp->w_doto, startdoto = doto;
	LINE *dotp;
	char line[256];
	INT ret = FALSE, bom, unixlf;
	charset_t charset;

	charset = curbp->charset;
	bom = (curbp->b_flag & BFBOM);
	unixlf = (curbp->b_flag & BFUNIXLF);

	// Must make a copy of the current line if dot is in the help buffer

	if ((dotp = lalloc(llength(curwp->w_dotp))) == NULL) return FALSE;
	memcpy(ltext(dotp), ltext(curwp->w_dotp), llength(dotp));

	bp = emptyhelpbuffer();
	if (bp == NULL) goto end;

#ifdef UCSNAMES2
	if (((f & FFNUM) && n == 4) || ((f & FFARG) == FFUNIV && n == 256)) {
		return showdatabase(bp, dotp, doto, charset, unixlf);
	}
#endif
	snprintf(line, sizeof(line), "Charset: %s", charsettoname(charset));

	if (bom) stringconcat(line, " (with BOM)", sizeof(line));

	if (addline(bp, line) == FALSE) goto end;
	if (addline(bp, "") == FALSE) goto end;

	snprintf(line, sizeof(line), ADVFORMAT, "Width", "Byte(s)", "Unicode", "Name");

	if (addline(bp, line) == FALSE) goto end;

	make_underline(line);

	if (addline(bp, line) == FALSE) goto end;

	if (doto == llength(dotp)) {
		if (!unixlf) {
			if (add_advanced_display_line(bp, f, n, 13, "\xd", 1, 0) == FALSE) goto end;
		}

		if (add_advanced_display_line(bp, f, n, 10, "\xa", 1, 0) == FALSE) goto end;
	} else {
		while (doto < llength(dotp)) {
			ch = ucs_char(charset, dotp, doto, &nextdoto);

			if (doto > startdoto && !ucs_nonspacing(ch)) break;

			if (ch == 9) {
				screenwidth = getcolumn(curwp, dotp, nextdoto) - getcolumn(curwp, dotp, doto);
			} else {
				screenwidth = ucs_termwidth(ch);
			}

			if (add_advanced_display_line(bp, f, n, ch,
				&ltext(dotp)[doto], nextdoto - doto, screenwidth) == FALSE) goto end;

			doto = nextdoto;
		}
	}

	ret = popbuftop(bp);
end:
	free(dotp);	// Not lfree
	return ret;
}

#endif /* UCSNAMES */
