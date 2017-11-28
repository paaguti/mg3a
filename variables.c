#include "def.h"
#include "macro.h"

INT dummy;	// For obsolete variables

INT width_routine = 0;
INT backward_compat = 0;
INT soft_tab_size = 0;
INT tabs_with_spaces = 0;
INT emacs_compat = 0;
INT tab_options = 0;

extern INT case_fold_search;
extern INT manual_locale;
extern INT fillcol;
extern INT bs_map;
extern INT makebackup;
extern INT scrollbyone;
extern INT recenter_redisplay;
extern INT modeline_show;
extern INT kill_whole_lines;
extern INT quoted_char_radix;
extern INT fill_options;
extern INT trim_whitespace;
extern INT complete_fold_file;
extern INT insert_default_directory;
extern INT preserve_ligatures;
extern INT blink_wait;
extern INT compare_fold_file;
extern INT buffer_name_width;
extern INT bell_type;
extern INT shell_command_limit;

#ifdef LANGMODE_C
extern INT cc_strip_trailp;
extern INT cc_basic_indent;
extern INT cc_cont_indent;
extern INT cc_colon_indent;
#endif

#ifdef LANGMODE_CLIKE
extern INT clike_options;
extern INT clike_lang;
extern INT clike_style;
#endif

typedef const struct {char *name; INT *value;} var_entry;

static const var_entry variables[] = {
  {"case-fold-search",	&case_fold_search},
  {"",			NULL},
  {"termdoescombine", 	&termdoescombine},
  {"termdoeswide", 	&termdoeswide},
  {"termdoesonlybmp",	&termdoesonlybmp},
  {"",			NULL},
  {"width-routine",	&width_routine},
  {"manual-locale", 	&manual_locale},
  {"",			NULL},
  {"word-search",	&word_search},
  {"fill-column",	&fillcol},
  {"",			NULL},
  {"bsmap",		&bs_map},
  {"make-backup",	&makebackup},
  {"",			NULL},
  {"window-scroll", 	&scrollbyone},
  {"recenter-redisplay",&recenter_redisplay},
  {"",			NULL},
  {"modeline-show",	&modeline_show},
  {"kill-whole-lines",	&kill_whole_lines},
  {"",			NULL},
  {"backward-compat",	&backward_compat},
  {"quoted-char-radix", &quoted_char_radix},
  {"",			NULL},
  {"fill-options",	&fill_options},
  {"trim-whitespace",	&trim_whitespace},
  {"",			NULL},
  {"insert-default-directory", &insert_default_directory},
  {"preserve-ligatures", &preserve_ligatures},
  {"",			NULL},
  {"complete-fold-file", &complete_fold_file},
  {"compare-fold-file",	&compare_fold_file},
  {"",			NULL},
  {"blink-wait",	&blink_wait},
  {"",			NULL},
  {"soft-tab-size",	&soft_tab_size},
  {"tabs-with-spaces",	&tabs_with_spaces},
  {"tab-options",	&tab_options},
  {"",			NULL},
  {"buffer-name-width",	&buffer_name_width},
  {"",			NULL},
  {"undo-limit",	&undo_limit},
  {"undo-enabled",	&undo_enabled},
  {"",			NULL},
  {"bell-type",		&bell_type},
  {"emacs-compat",	&emacs_compat},
  {"",			NULL},
  {"shell-command-limit", &shell_command_limit},
#if defined(LANGMODE_C) || defined(LANGMODE_CLIKE)
  {"",			NULL},
  {"# Language mode variables", NULL},
  {"# -----------------------", NULL},
#endif
#ifdef LANGMODE_C
  {"?",			NULL},			// "?" is skipped if previous line is comment,
  {"cc-strip-trailp",	&cc_strip_trailp},	// otherwise lists as a blank line.
  {"cc-basic-indent",	&cc_basic_indent},
  {"cc-cont-indent",	&cc_cont_indent},
  {"cc-colon-indent",	&cc_colon_indent},
#endif
#ifdef LANGMODE_CLIKE
  {"?",			NULL},
  {"clike-lang",	&clike_lang},
  {"clike-options",	&clike_options},
  {"clike-style",	&clike_style},
#endif
  {NULL, NULL},
};

static const var_entry obsolete_variables[] = {
  {"advancedinput",	&dummy},
  {"advancedinputdebug",&dummy},
  {"auto-trim-whitespace",	&trim_whitespace},
  {"python-indent-offset", &dummy},
  {NULL, NULL},
};

static char *local_variables[] = {
	"fill-column",
	"fill-options",
	"make-backup",
	"soft-tab-size",
	"tab-options",
	"tabs-with-spaces",
	"trim-whitespace",
#ifdef LANGMODE_CLIKE
	"clike-lang",
	"clike-options",
	"clike-style",
#endif
	NULL
};

const INT localvars = (sizeof(local_variables)/sizeof(char *) - 1);

/*
 * A separator line or comment in the global variable list
 */

static int
notvar(var_entry *p)
{
	if (p->name[0] == 0 || p->name[0] == '#' || p->name[0] == '?') return 1;
	return 0;
}


/*
 * Set a global variable
 */

INT
setvar(INT f, INT n)
{
	INT s;
	char varname[VARLEN], numstring[20];
	INT num;
	var_entry *p;

	if ((s = eread("Set Variable: ", varname, sizeof(varname), EFVAR)) != TRUE) return s;

	for (p = variables; p->name; p++) {
		if (notvar(p)) continue;
		if (strcmp(varname, p->name) == 0) break;
	}

	if (p->name == NULL) {
		for (p = obsolete_variables; p->name; p++) {
			if (strcmp(varname, p->name) == 0) break;
		}
	}

	if (p->name == NULL) {
		ewprintf("No such variable");
		return FALSE;
	}

	s = ereply("Set %s to: ", numstring, sizeof(numstring), varname);

	if (s == ABORT) {
		return s;
	} else if (s == FALSE) {
		ewprintf("No value given");
		return FALSE;
	}

	if (!getINT(numstring, &num, 1)) return FALSE;

	*p->value = num;

	ewprintf("%s set to %d", p->name, *p->value);

	invalidatecache();	/* Translations may have changed */
	refreshbuf(NULL);

	return TRUE;
}


/*
 * List local variables (if set) and global variables
 */

INT
listvars(INT f, INT n)
{
	BUFFER *bp;
	char line[128 + NFILEN];
	var_entry *p;
	INT i, haslocal, prev = ' ';

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	haslocal = (f & FFARG);

	for (i = 0; i < localvars; i++) {
		if (curbp->localvar.var[i] != MININT) haslocal = 1;
	}

	if (haslocal) {
		sprintf(line, " %-24s %7s      Local in buffer %s",
			"Local Variable", "Value", curbp->b_bname);

		if (addline(bp, line) == FALSE) return FALSE;
		sprintf(line, " %-24s %7s","--------------", "-----");
		if (addline(bp, line) == FALSE) return FALSE;

		for (i = 0; i < localvars; i++) {
			if (curbp->localvar.var[i] != MININT) {
				sprintf(line, " %-24s %7jd",
					local_variables[i],
					(intmax_t)curbp->localvar.var[i]);
				if (addline(bp, line) == FALSE) return FALSE;
			} else if (f & FFARG) {
				sprintf(line, " %-24s %7s",
					local_variables[i],
					"(unset)");
				if (addline(bp, line) == FALSE) return FALSE;
			}
		}
		if (addline(bp, "") == FALSE) return FALSE;
	}

	sprintf(line, " %-24s %7s", "Variable", "Value");
	if (addline(bp, line) == FALSE) return FALSE;
	sprintf(line, " %-24s %7s", "--------", "-----");
	if (addline(bp, line) == FALSE) return FALSE;

	for (p = variables; p->name; p++) {
		if (p->name[0] == 0) {
			strcpy(line, "");
		} else if (p->name[0] == '#') {
			strcpy(line, &p->name[1]);
		} else if (p->name[0] == '?') {
			if (prev == '#') continue;
			strcpy(line, "");
		} else {
			sprintf(line, " %-24s %7jd", p->name, (intmax_t)(*p->value));
		}

		if (addline(bp, line) == FALSE) return FALSE;
		prev = p->name[0];
	}

	return popbuftop(bp);
}


/*
 * Get next global variable name for completion
 */

char *
getnext_varname(INT first)
{
	static var_entry *p = NULL;

	if (first) p = variables;
	else if (!p->name) return NULL;
	else {
		p++;
		while (p->name && notvar(p)) p++;
		if (!p->name) return NULL;
	}

	return p->name;
}


/*
 * Set local variable
 */

INT
localsetvar(INT f, INT n)
{
	INT s;
	char varname[VARLEN], numstring[20];
	INT num;
	char **p;

	if ((s = eread("Local Set Variable: ",
		varname, sizeof(varname), EFLOCALVAR)) != TRUE) return s;

	for (p = local_variables; *p; p++) {
		if (strcmp(varname, *p) == 0) break;
	}

	if (*p == NULL) {
		ewprintf("No such local variable");
		return FALSE;
	}

	s = ereply("Local Set %s to: ", numstring, sizeof(numstring), varname);

	if (s == ABORT) {
		return s;
	} else if (s == FALSE) {
		ewprintf("No value given");
		return FALSE;
	}

	if (!getINT(numstring, &num, 1)) return FALSE;

	curbp->localvar.var[p - local_variables] = num;

	ewprintf("Local variable %s set to %d", *p, num);

	return TRUE;
}


/*
 * Unset local variable
 */

INT
localunsetvar(INT f, INT n)
{
	INT s;
	char varname[VARLEN];
	char **p;

	if ((s = eread("Local Unset Variable: ",
		varname, sizeof(varname), EFLOCALVAR)) != TRUE) return s;

	for (p = local_variables; *p; p++) {
		if (strcmp(varname, *p) == 0) break;
	}

	if (*p == NULL) {
		ewprintf("No such local variable");
		return FALSE;
	}

	curbp->localvar.var[p - local_variables] = MININT;

	ewprintf("Local variable %s cleared", *p);

	return TRUE;
}


/*
 * Get next local variable name for completion
 */

char *
getnext_localvarname(INT first)
{
	static char **p = NULL;

	if (first) p = local_variables;
	else if (!*p)  return NULL;
	else p++;

	return *p;
}


/*
 * Get value of variable, defaulting to the global one when the local
 * one is MININT.
 */

INT
get_variable(INT localvar, INT globalvar)
{
	if (localvar != MININT) return localvar;
	return globalvar;
}


/*
 * The "local-set-tabs" command.
 *
 * "local-set-tabs n m k" is shorthand for "local-set-variable
 * soft-tab-size n; local-set-variable tabs-with-spaces m;
 * local-set-variable tab-options k"
 *
 * An empty parameter changes nothing. A negative parameter unsets the
 * local variable. If the first parameter is numeric it may have been
 * parsed as a prefix count.
 */

INT
localsettabs(INT f, INT n)
{
	char buf[20];
	INT size, spaces, options, s;

	if (f & FFARG) {
		curbp->localvar.v.soft_tab_size = (n < 0) ? MININT : n;
	} else {
		if ((s = ereply("Soft Tab Size: ", buf, sizeof(buf))) == ABORT) return s;

		if (s == TRUE) {
			if (!getINT(buf, &size, 1)) return FALSE;

			curbp->localvar.v.soft_tab_size = (size < 0) ? MININT : size;
		}
	}

	if (!inmacro || !outofarguments(0)) {
		if ((s = ereply("Tabs With Spaces: ", buf, sizeof(buf))) == ABORT) return s;

		if (s == TRUE) {
			if (!getINT(buf, &spaces, 1)) return FALSE;

			curbp->localvar.v.tabs_with_spaces = (spaces < 0) ? MININT : spaces;
		}
	}

	if (!inmacro || !outofarguments(0)) {
		if ((s = ereply("Tab Options: ", buf, sizeof(buf))) == ABORT) return s;

		if (s == TRUE) {
			if (!getINT(buf, &options, 1)) return FALSE;

			curbp->localvar.v.tab_options = (options < 0) ? MININT : options;
		}
	}

	estart();

	ewprintfc("Soft-tab-size = ");

	if (curbp->localvar.v.soft_tab_size == MININT) {
		ewprintfc("(default %d)", soft_tab_size);
	} else {
		ewprintfc("%d", curbp->localvar.v.soft_tab_size);
	}

	ewprintfc(", tabs-with-spaces = ");

	if (curbp->localvar.v.tabs_with_spaces == MININT) {
		ewprintfc("(default %d)", tabs_with_spaces);
	} else {
		ewprintfc("%d", curbp->localvar.v.tabs_with_spaces);
	}

	ewprintfc(", tab-options = ");

	if (curbp->localvar.v.tab_options == MININT) {
		ewprintfc("(default %d)", tab_options);
	} else {
		ewprintfc("%d", curbp->localvar.v.tab_options);
	}

	return TRUE;
}
