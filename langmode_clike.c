#include "def.h"
#include "macro.h"
#include "key.h"

/*
 * Mg3a: A mode for C-like languages like C, Perl, Java etc.
 *
 * Written by Bengt Larsson. It is public domain.
 *
 * paaguti: TODO: integrate comment_begin and comment_end variables
 */


#ifdef LANGMODE_CLIKE


#define CLIKE_CCOMMENTS		0x00001	// Parse C-style comments ("/*", "*/" and "//").
#define CLIKE_PREPROC		0x00002	// Parse C preprocessor statements
#define CLIKE_STRINGS		0x00004	// Parse C strings, character constants and parenthesis levels.
#define CLIKE_WHITESMITH	0x00008	// Whitesmith/Ratliff formatting
#define CLIKE_GNU		0x00010	// Gnu formatting
#define CLIKE_KEYWORDS		0x00020	// C keyword heuristics for if, else, for, while statements
#define CLIKE_CASE		0x00040	// C keyword heuristics for case, default statements.
#define CLIKE_HASHCOMMENT	0x00080	// Parse '#' as comment to end of line
#define CLIKE_JSCOMMENT		0x00100	// Parse "<!--" as comment to end of line
#define CLIKE_ANCHOR		0x00200	// Find anchors
#define CLIKE_CPP		0x00400	// Parse C++-style "public:", "private:", "protected:"
#define CLIKE_NESTED		0x00800	// Make "/*".."*/" comments nested.
#define CLIKE_IFNOPAREN		0x01000	// Keyword heuristics for "if", "else if", "for", "while",
					// "switch" without parentheses.
#define CLIKE_RAW		0x02000	// Parse Go raw strings
#define CLIKE_LABELS		0x04000	// Parse C-style labels
#define CLIKE_SIMPLELABEL	0x08000	// Simplistic labels

#define CLIKE_SWIFT		0x10000	// Swift keywords
#define CLIKE_CSHARP		0x20000	// C# keywords and literal strings


#define COLUMN_ERROR		(-1)	// Column overflow
#define NO_COLUMN		(-2)	// No column adjustment

#define STATE_NORMAL		0	// Parse states
#define STATE_STRING		1
#define STATE_CHARCONST		2
#define STATE_EOLCOMMENT	3
#define STATE_RAWSTRING		4

#define allopt(opt, test) (((opt) & (test)) == (test))

#define uchar unsigned char	// In case there is a typedef


typedef struct {
	LINE *lp;
	ssize_t parenlevel;
	ssize_t bracelevel;
	ssize_t bracketlevel;
	ssize_t incomment;
	ssize_t endofcomment;
	INT firstindex;
	INT lastindex;
	INT scanlen;
	int lines;
	uchar first;
	uchar last;
	uchar lastparen;
	uchar single;
	uchar seencontent;
	uchar seenstartcomment;
	uchar seenendcomment;
} parseresult;


INT selfinsert(INT f, INT n);
INT forwline(INT f, INT n);


INT clike_options = 0;		// Option bits for C-like mode
INT clike_lang = 0;		// Clike language bundle
INT clike_style = 0;		// Clike style number

static const int debug = 0;
static INT parsecount = 0, commentcount = 0;

static int is_label(const parseresult *rp, INT opt, int strong);


/*
 * Utility function.
 */

static int
is_blank(INT c)
{
	return c == ' ' || c == '\t' || c == '\f';
}


/*
 * Get clike options
 */

static INT
get_clike_options()
{
	INT opt, lang;
	static const uint_least32_t opts[] = {
		/* C */
		(CLIKE_CCOMMENTS|CLIKE_PREPROC|CLIKE_STRINGS|CLIKE_KEYWORDS|
		 CLIKE_CASE|CLIKE_ANCHOR|CLIKE_CPP|CLIKE_LABELS),
		/* Perl */
		0,
		/* Java */
		(CLIKE_CCOMMENTS|CLIKE_STRINGS|CLIKE_KEYWORDS|CLIKE_CASE|
		 CLIKE_ANCHOR),
		/* C# */
		(CLIKE_CCOMMENTS|CLIKE_PREPROC|CLIKE_STRINGS|CLIKE_KEYWORDS|
		 CLIKE_CASE|CLIKE_ANCHOR|CLIKE_LABELS|CLIKE_CSHARP),
		/* JavaScript */
		(CLIKE_KEYWORDS|CLIKE_CASE),
		/* Swift */
		(CLIKE_CCOMMENTS|CLIKE_NESTED|CLIKE_STRINGS|CLIKE_CASE|
		 CLIKE_ANCHOR|CLIKE_IFNOPAREN|CLIKE_SWIFT),
		/* Go */
		(CLIKE_CCOMMENTS|CLIKE_STRINGS|CLIKE_CASE|CLIKE_ANCHOR|
		 CLIKE_IFNOPAREN|CLIKE_RAW|CLIKE_LABELS),
	};

	opt = get_variable(curbp->localvar.v.clike_options, clike_options) & 0xffff;
	lang = get_variable(curbp->localvar.v.clike_lang, clike_lang);

	if (lang != 0) opt &= (CLIKE_WHITESMITH|CLIKE_GNU);

	if (lang >= 1 && lang <= (INT)(sizeof opts/sizeof opts[0])) {
		opt |= opts[lang - 1];
	}

	return opt;
}


/*
 * Get clike style number
 */

static INT
get_clike_style()
{
	return get_variable(curbp->localvar.v.clike_style, clike_style);
}


/*
 * Parse a line in useful ways, with comments and strings.
 */

#define reset_parse(r) do {\
	r.first = ' ';\
	r.last = ' ';\
	r.lastparen= ' ';\
	r.firstindex = len;\
	r.lastindex = -1;\
	r.scanlen = len;\
	r.seenstartcomment = 0;\
	r.seenendcomment = 0;\
	r.incomment = 0;\
	r.seencontent = 0;\
	r.parenlevel = 0;\
	r.bracelevel = 0;\
	r.bracketlevel = 0;\
	r.lines = 1;} while (0)

static void
parseline(LINE *lp, INT opt, parseresult *result)
{
	int c, c2;
	INT i, j, len, index;
	parseresult r;

	if (debug) parsecount++;

	r.lp = lp;
	len = llength(lp);

	r.endofcomment = 0;	// Not reset by reset_parse

	reset_parse(r);

	for (i = 0; i < len; i++) {
		c = lgetc(lp, i);
		c2 = (i < len - 1) ? lgetc(lp, i + 1) : '\n';
		index = i;

		if (!ISDELIM(c)) {
			/* Nothing */
		} else if ((opt & CLIKE_CCOMMENTS) && c == '/') {
			if (c2 == '*') {
				r.seenstartcomment = 1;
				r.incomment = 1;
				for (i += 3; i < len; i++) {
					if ((opt & CLIKE_NESTED) &&
						lgetc(lp, i - 1) == '/' && lgetc(lp, i) == '*')
					{
						r.incomment++;
						i++;
					} else if (lgetc(lp, i - 1) == '*' && lgetc(lp, i) == '/') {
						if (--r.incomment == 0) break;
						i++;
					}
				}
				if (r.incomment) break;
				r.seenendcomment = 1;
				continue;
			} else if (c2 == '/') {
				r.scanlen = i;
				break;
			}
		} else if ((opt & CLIKE_CCOMMENTS) && c == '*' && c2 == '/') {
			if (r.endofcomment == 0 || (opt & CLIKE_NESTED)) {
				reset_parse(r);
				r.endofcomment++;
				r.seenendcomment = 1;
				i++;
				continue;
			}
		} else if ((opt & CLIKE_HASHCOMMENT) && c == '#') {
			r.scanlen = i;
			break;
		} else if ((opt & CLIKE_JSCOMMENT) && c == '<' && c2 == '!' &&
			i < len - 3 && lgetc(lp, i + 2) == '-' && lgetc(lp, i + 3) == '-')
		{
			r.scanlen = i;
			break;
		} else if ((opt & CLIKE_RAW) && c == '`') {
			for (j = i + 1; j < len; j++) {
				if (lgetc(lp, j) == '`') {
					i = j;
					break;
				}
			}
		} else if ((opt & CLIKE_CSHARP) && c == '@' && c2 == '"') {
			i++;
			for (j = i + 1; j < len; j++) {
				if (lgetc(lp, j) == '"') {
					if (j < len - 1 && lgetc(lp, j + 1) == '"') {
						j++;
					} else {
						i = j;
						break;
					}
				}
			}
		} else if (opt & CLIKE_STRINGS) {
			switch (c) {
			case '"':
			case '\'':
				for (j = i + 1; j < len; j++) {
					c2 = lgetc(lp, j);
					if (c2 == '\\') {
						j++;
					} else if (c2 == c) {
						i = j;
						break;
					}
				}
				break;

			case '(': r.parenlevel--; break;
			case ')': r.parenlevel++; r.lastparen = c; break;
			case '{': r.bracelevel--; break;
			case '}': r.bracelevel++; break;
			case '[': r.bracketlevel--; break;
			case ']': r.bracketlevel++; r.lastparen = c; break;
			}
		}

		if (!is_blank(c)) {
			if (r.first == ' ') {
				r.first = c;
				r.firstindex = index;
			}
			r.last = c;
			r.lastindex = index;
		}

	}

	r.seencontent = (r.first != ' ');
	r.single = (r.firstindex == r.lastindex);

	*result = r;
}


/*
 * Find the first non-blank line before "lp". Return the line and the
 * parsed result of that line. "incomment" deals with starting from a
 * line that has an end of a block comment followed by content.
 */

static LINE *
findnonblank(LINE *lp, INT opt, parseresult *result, ssize_t incomment)
{
	int count = 0;
	parseresult r;
	LINE *blp = curbp->b_linep;

	if (lp != blp) lp = lback(lp);

	for (; lp != blp; lp = lback(lp)) {
		if (count++ >= 10000) {
			lp = blp;
			break;
		}

		parseline(lp, opt, &r);

		if (opt & CLIKE_CCOMMENTS) {
			if (opt & CLIKE_NESTED) {
				if (incomment > r.incomment) {
					incomment += r.endofcomment - r.incomment;
					continue;
				}

				incomment += r.endofcomment - r.incomment;
			} else {
				if (incomment) {
					if (r.seenstartcomment) {
						incomment = 0;
					} else {
						continue;
					}
				}

				if (r.endofcomment) incomment = 1;
			}
		}

		if (!r.seencontent) continue;

		if ((opt & CLIKE_PREPROC) && r.first == '#') continue;
		if (r.last == ':' && is_label(&r, opt, 1)) continue;

		break;
	}

	if (result) {
		if (lp == blp) parseline(lp, opt, &r);
		r.lines = count;
		*result = r;
	}

	return lp;
}


/*
 * Parse a possibly parenthesized statement before line "lp", and
 * return the first line and the parsed, possibly multiline, result.
 * The last unbalanced ']' or ')' is used for grouping.
 */

static LINE *
parse_statement(LINE *lp, INT opt, parseresult *result, ssize_t incomment)
{
	parseresult r, r2;
	LINE *lp2;
	int usebrackets;

	lp = findnonblank(lp, opt, &r, incomment);

	usebrackets = (r.lastparen == ']' && r.bracketlevel > 0);

	if (usebrackets || r.parenlevel > 0) {
		while (r.lines < 10000) {
			lp2 = findnonblank(lp, opt, &r2, r.endofcomment);

			if (lp2 == curbp->b_linep) break;

			lp = lp2;
			r.lp = lp2;
			r.first = r2.first;
			r.firstindex = r2.firstindex;
			r.lastindex = r2.lastindex;
			r.single = 0;
			r.parenlevel += r2.parenlevel;
			r.bracelevel += r2.bracelevel;
			r.bracketlevel += r2.bracketlevel;

			r.endofcomment = r2.endofcomment;
			r.lines += r2.lines;

			if (r.firstindex == 0) break;
			if (usebrackets ? r.bracketlevel <= 0 : r.parenlevel <= 0) break;
		}
	}

	*result = r;
	return lp;
}


/*
 * Check if c is one of the characters
 */

static int
isoneof(INT c, char *s)
{
	for (; *s; s++) if (c == CHARMASK(*s)) return 1;
	return 0;
}


/*
 * Is left parenthesis.
 */

static int
isleftparen(INT c)
{
	return c == '{' || c == '(' || c == '[';
}


/*
 * Is right parenthesis.
 */

static int
isrightparen(INT c)
{
	return c == '}' || c == ')' || c == ']';
}


/*
 * Increase the tab level and check for overflow
 */

static INT
increasetab(INT col, INT tabsize)
{
	if (col < 0) return col;	// Error return
	if (tabsize <= 0) return col;	// No change

	col -= col % tabsize;

	if (col > MAXINT - tabsize) {
		column_overflow();
		return COLUMN_ERROR;
	}

	return col + tabsize;
}


/*
 * Decrease the tab level
 */

static INT
decreasetab(INT col, INT tabsize)
{
	INT decrease;

	if (col < 0) return col;	// Error return
	if (tabsize <= 0) return col;	// No change

	if (col > 0) {
		decrease = col % tabsize;
		if (decrease == 0) decrease = tabsize;
		col -= decrease;
	}

	return col;
}


/*
 * Identifier character
 */

static int
ident(INT c)
{
	return ISWORD(c) || c == '_';
}


/*
 * Check if one of the words, delimited by "," start the line. An
 * embedded space in a word means a word boundary and arbitrary
 * whitespace. "$" means last non-whitespace on the line.
 */

static int
firstword(const parseresult *rp, char *inwords)
{
	INT i, j, k, len;
	LINE *lp = rp->lp;
	unsigned char *words = (unsigned char *)inwords;

	i = rp->firstindex;
	len = rp->lastindex + 1;

	while (*words) {
		for (j = i, k = 0; j < len; j++, k++) {
			if (words[k] == '$') {
				break;
			} else if (words[k] == 0 || words[k] == ',') {
				if (!ident(lgetc(lp, j)) || !ident(words[k - 1])) {
					return 1;
				} else {
					break;
				}
			} else if (words[k] == ' ') {
				if (is_blank(lgetc(lp, j))) {
					while (j < len - 1 && is_blank(lgetc(lp, j + 1))) j++;
				} else if (!ident(lgetc(lp, j)) || !ident(words[k - 1])) {
					j--;
				} else {
					break;
				}
			} else if (words[k] != lgetc(lp, j)) {
				break;
			}
		}
		if (j == len && (words[k] == 0 || words[k] == ',' || words[k] == '$')) return 1;
		while (*words && *words++ != ',') {}
	}

	return 0;
}


/*
 * Special cases for case statements
 */

static int
case_statement(const parseresult *rp)
{
	return isoneof(rp->first, "cd") && isoneof(rp->last, ":;}{") &&
	       firstword(rp, "case,default :");
}


/*
 * Special cases for statements without braces.
 */

static int
keywordspecial(const parseresult *rp, INT opt, int full)
{
	if (rp->last == ')' || !full) {
		if (opt & CLIKE_CSHARP) {
			return firstword(rp, "if (,else if (,} else if (,while (,for (,foreach (");
		} else {
			return firstword(rp, "if (,else if (,} else if (,while (,for (");
		}
	} else {
		return firstword(rp, "else$,} else$");
	}
}


/*
 * Test for C++ access specifiers
 */

static int
labelspecial(const parseresult *rp)
{
	return firstword(rp, "public :,private :,protected :");
}


/*
 * Test for parenthesis-less keywords. Swift has evolved.
 */

static int
noparen_keywords(const parseresult *rp, INT opt)
{
	if (opt & CLIKE_SWIFT) {
		return firstword(rp, "if,else,} else,for,while,switch,guard,catch,} catch");
	} else {
		return firstword(rp, "if,else,} else,for,while,switch");
	}
}


/*
 * Check for a label. If we are doing "case"-statements, "default:" is
 * not a label. "something::" is not a label.
 *
 * "strong" checks that the line is nothing but label.
 */

static int
is_label(const parseresult *rp, INT opt, int strong)
{
	parseresult r = *rp;
	INT i;

	if (!(opt & CLIKE_LABELS)) return 0;

	if (!ident(r.first) || ISDIGIT(r.first)) return 0;

	for (i = r.firstindex + 1; i <= r.lastindex; i++) if (!ident(lgetc(r.lp, i))) break;
	for (; i <= r.lastindex; i++) if (!is_blank(lgetc(r.lp, i))) break;

	if (i > r.lastindex || lgetc(r.lp, i) != ':') return 0;
	if (i < r.lastindex && lgetc(r.lp, i + 1) == ':') return 0;

	if (strong && i != r.lastindex) return 0;
	if ((opt & CLIKE_CASE) && firstword(rp, "default :")) return 0;

	return 1;
}


/*
 * Find an anchor before line "lp". An anchor is the statement of the
 * '{' of the enclosing block, or a case statement at the same
 * bracelevel. When in trouble, parse just one statement back.
 */

static LINE *
findanchor(LINE *lp, INT opt, parseresult *result)
{
	parseresult r, r2;
	int count = 0;
	LINE *lp2, *blp = curbp->b_linep;
	ssize_t bracelevel;

	parseline(lp, opt, &r);

	lp2 = lp;
	bracelevel = 0;
	r2.endofcomment = r.endofcomment;

	while (1) {
		lp2 = parse_statement(lp2, opt, &r2, r2.endofcomment);
		count += r2.lines;

		if (lp2 == blp) break;

		if (count > 10000) {
			return parse_statement(lp, opt, result, r.endofcomment);
		}

		bracelevel += r2.bracelevel;

		if (opt & CLIKE_CASE) {
			if (r.first != '}' && bracelevel == 0 && case_statement(&r2)) {
				// Neutralize any braces
				r2.last = ';';
				r2.bracelevel = 0;
				break;
			}
		}

		if (bracelevel < 0 || (r2.firstindex == 0 && isoneof(r2.last, "})"))) break;
	}

	r2.lines = count;
	*result = r2;
	return lp2;
}


/*
 * Figure out the normal column for the statement in "rp", even for
 * multiline "if", "while" etc. expressions without parentheses.
 * Multi-line braced expressions within such an expression are not
 * allowed.
 */

static INT
noparen_getcolumn(const parseresult *rp, INT opt)
{
	LINE *blp = curbp->b_linep;
	parseresult r;

	if (	(isoneof(rp->first, "{}") && rp->single) ||
		((opt & CLIKE_CASE) && case_statement(rp)) ||
		noparen_keywords(rp, opt))
	{
		return getfullcolumn(curwp, rp->lp, rp->firstindex);
	}

	r = *rp;					// Tag:noparen

	while (1) {
		parse_statement(r.lp, opt, &r, r.endofcomment);

		if (r.lp == blp || (r.first == '}' && !r.single) - r.bracelevel != 0 ||
			r.parenlevel < 0 || r.bracketlevel < 0 ||
			((opt & CLIKE_CASE) && case_statement(&r)))
		{
			return getfullcolumn(curwp, rp->lp, rp->firstindex);
		} else if (noparen_keywords(&r, opt)) {
			return getfullcolumn(curwp, r.lp, r.firstindex);
		} else if (r.firstindex == 0 || r.bracelevel != 0) {
			return getfullcolumn(curwp, rp->lp, rp->firstindex);
		}
	}
}


/*
 * Check whether in nested block comment or raw string, with caching.
 */

static int
inblockcomment(LINE *lpend, INT opt)
{
	const int incr = 0x200, mask = ~0x1f0;
	int cache_count, c1, c2;
	int state = 0, startstate = 0, flag, special_buffer;
	int alwaysparse, cachescan = 20, storecomment;
	size_t incomment = 0, startincomment = 0;
	INT i, len;
	LINE *lp, *blp = curbp->b_linep;

	if (lpend == blp) return 0;

	// Don't cache in special buffers.
	special_buffer = (curbp->b_flag & (BFSYS|BFDIRED));

	cache_count = curbp->l_flag;

	if (special_buffer) {
		lp = blp;
	} else if ((cache_count & 15) != 6) {
		cache_count = 6;
		lp = blp;
	} else {
		for (lp = lpend; lp != blp; lp = lback(lp)) {
			flag = lp->l_flag;

			if ((flag & mask) == cache_count) {
				state = (flag >> 4) & 7;
				incomment = (flag >> 7) & 3;
				break;
			} else if (cachescan-- <= 0) {
				lp = blp;
				break;
			}
		}
	}

	len = llength(lp);
	i = len;

	if (debug) commentcount = 0;

	while (1) {
		if (i >= len) {
			if (state >= STATE_STRING && state <= STATE_EOLCOMMENT) {
				if (len == 0 || lgetc(lp, len - 1) != '\\') state = STATE_NORMAL;
			}

			if (lp == lpend) break;

			if (special_buffer) {
				lp = lforw(lp);
			} else {
				// Store a parse state for each line. Lines with the same
				// end-state are normally skipped as an optimization. Lines
				// with different beginning and end state are always parsed.

				storecomment = (incomment > (INT_MAX >> 8)) ? (INT_MAX >> 8) : incomment;
				flag = 5 | (state << 5) | (storecomment << 8);

				alwaysparse = (state != startstate || incomment != startincomment);
				lp->l_flag = flag | (alwaysparse << 4);		// Tag:parsecomment

				do {
					lp = lforw(lp);
				} while (lp != lpend && lp->l_flag == flag);
			}

			i = 0;
			len = llength(lp);

			startstate = state;
			startincomment = incomment;

			if (debug) commentcount++;
			if (len == 0) continue;
		}

		c1 = lgetc(lp, i);
		c2 = (i < len - 1) ? lgetc(lp, i + 1) : '\n';

		if (!ISDELIM(c1)) {
			/* Nothing */
		} else if (incomment) {
			if ((opt & CLIKE_NESTED) && c1 == '/' && c2 == '*') {
				incomment++;
				i++;
			} else if (c1 == '*' && c2 == '/') {
				incomment--;
				i++;
			}
		} else if (state == STATE_RAWSTRING) {
			if (opt & CLIKE_CSHARP) {
				if (c1 == '"') {
					if (c2 == '"') i++; else state = STATE_NORMAL;
				}
			} else {
				if (c1 == '`') state = STATE_NORMAL;
			}
		} else if (state == STATE_STRING) {
			if (c1 == '"') state = STATE_NORMAL;
			else if (c1 == '\\') i++;
		} else if (state == STATE_CHARCONST) {
			if (c1 == '\'') state = STATE_NORMAL;
			else if (c1 == '\\') i++;
		} else if ((opt & CLIKE_CCOMMENTS) && c1 == '/') {
			if (c2 == '*') {
				incomment = 1;
				i++;
			} else if (c2 == '/') {
				state = STATE_EOLCOMMENT;
				i = len;
			}
		} else if ((opt & CLIKE_HASHCOMMENT) && c1 == '#') {
			state = STATE_EOLCOMMENT;
			i = len;
		} else if ((opt & CLIKE_JSCOMMENT) && c1 == '<' && c2 == '!' && i < len - 3 &&
			lgetc(lp, i + 2) == '-' && lgetc(lp, i + 3) == '-')
		{
			state = STATE_EOLCOMMENT;
			i = len;
		} else if ((opt & CLIKE_RAW) && c1 == '`') {
			state = STATE_RAWSTRING;
		} else if ((opt & CLIKE_CSHARP) && c1 == '@' && c2 == '"') {
			state = STATE_RAWSTRING;
			i++;
		} else if (opt & CLIKE_STRINGS) {
			if (c1 == '"') state = STATE_STRING;
			else if (c1 == '\'') state = STATE_CHARCONST;
		}

		i++;
	}

	if (!special_buffer) {
		if (cache_count > INT_MAX - incr) {
			for (lp = lforw(blp); lp != blp; lp = lforw(lp)) {
				if ((lp->l_flag & 15) == 6) lp->l_flag = 0;
			}

			cache_count = 6;
		}

		cache_count += incr;

		if (incomment <= 3) {
			lpend->l_flag = cache_count | (state << 4) | (incomment << 7);
		}

		curbp->l_flag = cache_count;
	}

	return (state != STATE_NORMAL) | ((incomment != 0) << 1);
}


/*
 * Get the desired indentation column for line "lp1". "inserted" is true
 * if called by an active key after inserting something.
 *
 * Subtracting tablevels is additive, but adding tablevels mostly not.
 */

static INT
getclikeindent(LINE *lp1, int inserted)
{
	INT col, tabsize, opt, style;
	LINE *lp2, *lp3;
	parseresult r1, r2, r3;
	int addtablevel= 0, subtablevel = 0, addhalftab = 0, subhalftab = 0, tryanchor = 1;
	int incomment, iscase = 0, isprecase = 0;

	if (lp1 == curbp->b_linep) return NO_COLUMN;

	opt = get_clike_options();
	style = get_clike_style();

	if (opt & (CLIKE_CCOMMENTS|CLIKE_RAW)) {
		incomment = inblockcomment(lback(lp1), opt);

		if (incomment) {
			if (inserted) return NO_COLUMN;

			lp2 = findnonblank(lp1, 0, NULL, 0);
			getindent(lp2, NULL, &col);

			if (incomment > 1) {				// Tag:comment
				parseline(lp2, opt, &r2);

				if (r2.seenstartcomment && !r2.seenendcomment && !r2.seencontent) {
					col++;
				}
			}

			return col;
		}
	}

	if ((opt & CLIKE_CCOMMENTS) && llength(lp1) > 1 &&
		lgetc(lp1, 0) == '/' && lgetc(lp1, 1) == '/')
	{
		return NO_COLUMN;
	}

	tabsize = gettabsize(curbp);

	parseline(lp1, opt, &r1);

	lp2 = parse_statement(lp1, opt, &r2, r1.endofcomment);
	lp3 = parse_statement(lp2, opt, &r3, r2.endofcomment);

	col = getfullcolumn(curwp, lp2, r2.firstindex);

	if ((opt & CLIKE_PREPROC) && r1.first == '#') return 0;

	if ((opt & CLIKE_IFNOPAREN) && !isoneof(r1.first, "{}") && !isoneof(r2.last, "{}") &&
		noparen_keywords(&r2, opt) && !noparen_keywords(&r1, opt))
	{
		addtablevel = 1;

		if (style & 4) col = increasetab(col, tabsize);		// Additive
		else if (style & 1) addhalftab = 1;
	} else if (r2.parenlevel < 0 || r2.bracketlevel < 0) {
		addtablevel = 1;
		tryanchor = 0;	// If you try a '}' with negative paren level something is wrong

		if ((style & 4) && (opt & CLIKE_KEYWORDS) && keywordspecial(&r2, opt, 0)) {
			col = increasetab(col, tabsize);		// Additive, Tag:if
		} else if (style & 1) {
			addhalftab = 1;
		}
	} else if (opt & CLIKE_KEYWORDS) {
		tryanchor = (r1.first == '}');

		if (r1.first != '{' && keywordspecial(&r2, opt, 1)) {
			addtablevel = 1;
		} else if (r2.last == ';' && r2.first != '{' && keywordspecial(&r3, opt, 1)) {
			col = getfullcolumn(curwp, lp3, r3.firstindex);
#if 0
			tryanchor = tryanchor || !firstword(&r3, "if (,else if (,} else if (");
#endif
		} else {
			tryanchor = 1;
		}
	}

	if (allopt(opt, CLIKE_STRINGS|CLIKE_ANCHOR) && tryanchor) {
		int endmultiassign = (r2.last == ';' && !isoneof(r3.last, ":;}{"));
		int matched = 0;

		if (opt & CLIKE_IFNOPAREN) {
			if (r1.first == '}' || endmultiassign || (r2.last == '}' && !r2.single)) {
				lp2 = findanchor(lp1, opt, &r2);
				col = noparen_getcolumn(&r2, opt);
				matched = 1;
			} else if (r1.first == '{' || r2.last == '{') {
				col = noparen_getcolumn(&r2, opt);
			}
		} else if (r1.first == '}' || endmultiassign || r2.last == '}') {
			lp2 = findanchor(lp1, opt, &r2);		// Tag:anchor
			col = getfullcolumn(curwp, lp2, r2.firstindex);
			matched = 1;
		}

		if (!matched && r2.last == '=') {
			addtablevel = 1;
			if (style & 1) addhalftab = 1;
		}
	}

	if (opt & CLIKE_CASE) {
		if (case_statement(&r1)) {
			// Try to make case statements line up
			if (opt & CLIKE_ANCHOR) {
				lp3 = findanchor(lp1, opt, &r3);
				if (case_statement(&r3)) {
					return getfullcolumn(curwp, lp3, r3.firstindex);
				}
			}

			if (!(style & 2)) subtablevel++;

			iscase = 1;
		}

		if (case_statement(&r2)) {
			col = increasetab(col, tabsize);	// Additive, Tag:case
			isprecase = 1;
		}
	}

	if (!isprecase) {
		if (is_label(&r2, opt, 0)) {
			col = increasetab(col, tabsize);	// Additive
		} else if ((opt & CLIKE_SIMPLELABEL) && r2.last == ':') {
			addtablevel = 1;
		}
	}

	if (!iscase && is_label(&r1, opt, 0)) {
		subtablevel++;
	}

	if ((style & 1) && isoneof(r1.first, ")]")) {
		subhalftab++;
	}

	if (r2.last != '{' && r2.bracelevel < 0) {
		addtablevel = 1;				// Can happen in Swift, Tag:swift
	}

	if ((opt & CLIKE_SWIFT) && firstword(&r1, "else") && r2.last != '}') {
		col = noparen_getcolumn(&r2, opt);		// Swift guard ... else
	}


	if (opt & CLIKE_GNU) {
		int preknr, preopen, preclose, preloneopen;
		int currentopen, currentclose;

		// Gnu

		preknr = isleftparen(r2.last) && !r2.single;

		preopen = isleftparen(r2.first);
		preclose = isrightparen(r2.first);
		preloneopen = preopen && r2.single;

		currentopen = isleftparen(r1.first) && r1.firstindex != 0;
		currentclose = isrightparen(r1.first);

		if (preknr) addtablevel = 2;
		else if (preloneopen || (currentopen && !preopen)) addtablevel = 1;

		if (preclose && currentclose) subtablevel += 2;
		else if (preclose || currentclose) subtablevel++;
	} else if (opt & CLIKE_WHITESMITH) {
		int ratlifflead, preclose, currentopen, preopen;

		// Whitesmith and Ratliff, combined			 Tag:simple

		ratlifflead = isleftparen(r2.last) && !r2.single;

		preopen = isleftparen(r2.first);
		preclose = isrightparen(r2.first);

		currentopen = isleftparen(r1.first);

		if (ratlifflead || (currentopen && !preopen)) addtablevel = 1;
		if (preclose) subtablevel++;
	} else {
		// 1TBS, Allman, K&R

		if (isleftparen(r2.last)) addtablevel = 1;
		if (isrightparen(r1.first)) subtablevel = 1;
	}

	while (addtablevel-- > addhalftab)	col = increasetab(col, tabsize);
	while (addhalftab--)			col = increasetab(col, tabsize/2);
	while (subtablevel-- > subhalftab)	col = decreasetab(col, tabsize);
	while (subhalftab--)			col = decreasetab(col, tabsize/2);

	return col;
}


/*
 * Re-indent the current line, but possibly not if something was
 * inserted by an active key.
 */

static INT
re_indent(int inserted)
{
	INT col;

	col = getclikeindent(curwp->w_dotp, inserted);

	if (debug) {
		ewprintf("Parsed lines = %d, Comment parsed lines = %d", parsecount, commentcount);
		parsecount = 0;
		commentcount = 0;
	}

	if (col == NO_COLUMN) return TRUE;

	return setindent(col);
}


/*
 * Re-indent the current line.
 */

INT
clike_indent(INT f, INT n)
{
	return re_indent(0);
}


/*
 * Re-indent the current line, go to the next line.
 */

INT
clike_indent_next(INT f, INT n)
{
	INT s;

	if (n < 0) return FALSE;

	while (n-- > 0) {
		if ((s = re_indent(0)) != TRUE) return s;
		if (lforw(curwp->w_dotp) == curbp->b_linep) break;
		forwline(0, 1);
	}

	return TRUE;
}


/*
 * Re-indent the region
 */

INT
clike_indent_region(INT f, INT n)
{
	INT s;
	REGION r;
	LINE *prevline;
	INT origcol;

	if ((s = getregion(&r)) != TRUE) return s;

	prevline = lback(r.r_linep);
	origcol = getcolumn(curwp, curwp->w_dotp, curwp->w_doto);

	adjustpos(r.r_linep, 0);

	while (r.r_lines-- > 0) {
		if ((s = re_indent(0)) != TRUE) return s;
		if (r.r_lines > 0) adjustpos(lforw(curwp->w_dotp), 0);
	}

	if (r.r_forward) {
		adjustpos(curwp->w_dotp, llength(curwp->w_dotp));
	} else {
		adjustpos(lforw(prevline), getoffset(curwp, lforw(prevline), origcol));
	}

	return TRUE;
}


/*
 * Insert n newlines and indent.
 */

INT
clike_newline_and_indent(INT f, INT n)
{
	INT doto = curwp->w_doto;

	if (n == 0) return TRUE;
	if (lnewline_n(n) == FALSE) return FALSE;
	if (doto && !isblankline(curwp->w_dotp)) linsert(1, ' '); // Avoid beginning-of-line behavior
	return re_indent(0);
}


/*
 * If the current line is blank, insert and re-indent. Otherwise just
 * insert.
 */

INT
clike_insert(INT f, INT n)
{
	if (n == 0) return TRUE;

	if (inkbdmacro) return re_indent(1);

	if (isblankline(curwp->w_dotp)) {
		if (selfinsert(f|FFRAND, n) == FALSE) return FALSE;

		if (macrodef && macrocount < MAXMACRO) {
			// Record the re-indent
			macro[macrocount++].m_funct = clike_insert;
			// Disallow continued insert
			thisflag &= ~CFINS;
		}

		return re_indent(1);
	}

	return selfinsert(f|FFRAND, n);
}


/*
 * Special insert for the colon. Only re-indents a case statement, or
 * "public:", "private:", "protected:".
 */

INT
clike_insert_special(INT f, INT n)
{
	INT opt;
	parseresult r;

	if (n == 0) return TRUE;

	if (linsert_overwrite(curbp->charset, n, ':') == FALSE) return FALSE;

	opt = get_clike_options();
	parseline(curwp->w_dotp, opt, &r);

	if (	((opt & CLIKE_CASE) && case_statement(&r)) ||
		(allopt(opt, CLIKE_LABELS|CLIKE_CPP) && labelspecial(&r)))
	{
		return re_indent(1);
	}

	return TRUE;
}


/*
 * Align the current line after the next "calign" character and
 * following whitespace in the previous non-blank line. If "calign2"
 * is not 0, search for that, too.
 */

static INT
align_after(INT calign, INT calign2)
{
	LINE *dotp, *lp;
	INT i, j, len, col, opt, c, tries;
	parseresult r;
	charset_t charset = curbp->charset;

	if (calign < 0 || calign > 0x10ffff) return invalid_codepoint();

	opt = get_clike_options();

	dotp = curwp->w_dotp;
	lp = findnonblank(dotp, 0, NULL, 0);	// No special parsing

	len = llength(lp);

	parseline(dotp, opt, &r);
	col = getfullcolumn(curwp, dotp, r.firstindex);
	i = getfulloffset(curwp, lp, col);

	for (tries = 0; tries < 2; tries++) {
		while (i < len) {
			c = ucs_char(charset, lp, i, &i);
			if (c == calign || (calign2 && c == calign2)) {
				for (j = i; j < len && is_blank(lgetc(lp, j)); j++) {}
				if (j == len) j = i;
				adjustpos(curwp->w_dotp, 0);
				return setindent(getfullcolumn(curwp, lp, j));
			}
		}

		i = 0;
	}

	if (calign2) {
		ewprintf("No '%K' or '%K'", calign, calign2);
	} else {
		ewprintf("No '%K'", calign);
	}

	return FALSE;
}


/*
 * Align the current line to after the next '(' or '[' in the previous
 * non-blank line.
 */

INT
clike_align_after_paren(INT f, INT n)
{
	return align_after('(', '[');
}


/*
 * Align the current line to after the next '=' (by default) in the
 * previous non-blank line.
 *
 * When given an argument, prompt for a key to match.
 */

INT
clike_align_after(INT f, INT n)
{
	INT c, s;

	if (inmacro) {
		if (f & FFARG) c = n;
		else c = '=';

		if (!outofarguments(0)) {
			if ((s = macro_get_INT(&c)) != TRUE) return s;
		}

		return align_after(c, 0);
	}

	if (f & FFARG) {
		estart();
		ewprintfc("Character to search for: ");
		ttflush();
		c = getkey(FALSE);
		ewprintfc("%C", c);
	} else {
		c = '=';
	}

	if (macrodef) {
		if ((s = macro_store_INT(c)) != TRUE) return s;
	}

	return align_after(c, 0);
}


/*
 * Backup to an earlier alignment
 */

INT
clike_align_back(INT f, INT n)
{
	INT col, col2, opt;
	int count = 0;
	LINE *lp;
	parseresult r;

	opt = get_clike_options();

	lp = curwp->w_dotp;

	parseline(lp, opt, &r);
	col = getfullcolumn(curwp, lp, r.firstindex);

	if (col <= 0) col = MAXINT;

	while (count < 10000) {
		lp = findnonblank(lp, opt, &r, r.endofcomment);
		count += r.lines;

		col2 = getfullcolumn(curwp, lp, r.firstindex);

		if (col2 < col) {
			adjustpos(curwp->w_dotp, 0);
			return setindent(col2);
		}
	}

	ewprintf("(No smaller indent found; max lines scanned)");
	return FALSE;
}


/*
 * C-like version of parentheses-balancing (from match.c).
 */

static INT
clike_balance()
{
	LINE	*clp;
	INT	cbo, cbsave, size = 0;
	INT	c1, c2;
	INT	rbal, lbal;
	ssize_t	depth;
	charset_t charset = curbp->charset;
	parseresult r;
	INT	opt, lastindex;
	ssize_t incomment = 0;
	int	instring = 0, incharconst = 0, inraw = 0;

	rbal = key.k_chars[key.k_count-1];

	if (ucs_width(rbal) == 0) return FALSE;		/* Not useful */

	/* See if there is a matching character -- default to the same */

	lbal = leftparenof(rbal);
	if (lbal == 0) lbal = rbal;

	/* Move behind the inserted character.	We are always guaranteed    */
	/* that there is at least one character on the line, since one was  */
	/* just self-inserted by blinkparen.				    */

	opt = get_clike_options();

	clp = curwp->w_dotp;
	cbsave = curwp->w_doto;
	cbo = ucs_prevc(charset, clp, curwp->w_doto);
	lastindex = llength(clp) + 1;

	depth = 0;				/* init nesting depth	*/

	for (;;) {
		size += cbsave - cbo;
		if (size > 102400) return FALSE;

		if (cbo == 0) {			/* beginning of line	*/
			clp = lback(clp);
			if (clp == curbp->b_linep)
				return (FALSE);
			parseline(clp, opt, &r);
			lastindex = r.scanlen + 1;
			cbo = lastindex;
			instring = 0;
			incharconst = 0;
		}

		cbsave = cbo;

		if (cbo == lastindex) {		/* end of line		*/
			cbo--;
			c1 = '\n';
		} else {			/* somewhere in middle	*/
			c1 = lgetc(clp, cbo - 1);

			if (c1 < 128) {		/* speed up ASCII	*/
				cbo--;
			} else {
				cbo = ucs_prevc(charset, clp, cbo);
				c1 = ucs_char(charset, clp, cbo, NULL);
			}
		}

		// "c1" can be Unicode but "c2" is ASCII.

		c2 = (cbo > 0) ? lgetc(clp, cbo - 1) : -1;

		if (incomment) {
			if ((opt & CLIKE_NESTED) && c1 == '/' && c2 == '*') {
				cbo--;
				incomment++;
			} else if (c1 == '*' && c2 == '/') {
				cbo--;
				incomment--;
			}
			continue;
		} else if (instring) {
			if (c1 == '"') {
				// In C#, approximately correct since '""'
				// will work out to a string again.

				if (c2 == '\\') cbo--;
				else instring = 0;
			}
			continue;
		} else if (incharconst) {
			if (c1 == '\'') {
				if (c2 == '\\') cbo--;
				else incharconst = 0;
			}
			continue;
		} else if (inraw) {
			if (c1 == '`') inraw = 0;
			continue;
		} else if ((opt & CLIKE_CCOMMENTS) && c1 == '/' && c2 == '*') {
			incomment = 1;
			cbo--;
			continue;
		} else if ((opt & CLIKE_RAW) && c1 == '`') {
			inraw = 1;
		} else if (opt & CLIKE_STRINGS) {
			if (c1 == '"') {
				instring = 1;
			} else if (c1 == '\'') {
				incharconst = 1;
			}
		}

		/* Check for a matching character.  If still in a nested */
		/* level, pop out of it and continue search.  This check */
		/* is done before the nesting check so single-character	 */
		/* matches will work too.				 */
		if (c1 == lbal) {
			if (depth == 0) {
				displaymatch(clp, cbo);
				return (TRUE);
			}
			else
				depth--;
		}
		/* Check for another level of nesting.	*/
		if (c1 == rbal)
			depth++;
	}
}


/*
 * C-like version of "blink-and-insert".
 */

INT
clike_showmatch(INT f, INT n)
{
	INT  s;

	if ((s = clike_insert(f|FFRAND, n)) != TRUE) return s;
	if (clike_balance() != TRUE) ttbeep();

	return TRUE;
}


/*
 * If the current line is blank, insert a tab. Otherwise re-indent.
 * Modeled on "c-tab-or-indent" in c-mode.
 */

INT
clike_tab_or_indent(INT f, INT n)
{
	if (n < 0) return FALSE;
	if (n == 0) return TRUE;

	if (isblankline(curwp->w_dotp)) {
		return inserttabs(n);
	} else {
		return re_indent(0);
	}
}


/*
 * "clike" mode switch.
 */
extern char comment_begin[20], comment_end[20];

INT
clike_mode(INT f, INT n)
{
	comment_begin[0] = 0;
	comment_end[0] = 0;
	return changemode(curbp, f, n, "clike");
}

#endif
