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
extern INT reverse_mousewheel;
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

typedef const struct {char *name; char vtype; union{INT *value; char *svalue[20];};} var_entry;

static const var_entry variables[] = {
	{"case-fold-search",	'i', &case_fold_search},
	{"",			0, NULL},
	{"termdoescombine", 	'i', &termdoescombine},
	{"termdoeswide", 	'i', &termdoeswide},
	{"termdoesonlybmp",	'i', &termdoesonlybmp},
	{"",			0, NULL},
	{"width-routine",	'i', &width_routine},
	{"manual-locale", 	'i', &manual_locale},
	{"",			0, NULL},
	{"word-search",	'i', &word_search},
	{"fill-column",	'i', &fillcol},
	{"",			0, NULL},
	{"bsmap",		'i', &bs_map},
	{"make-backup",	'i', &makebackup},
	{"",			0, NULL},
	{"window-scroll", 	'i', &scrollbyone},
	{"recenter-redisplay",'i', &recenter_redisplay},
	{"reverse-mousewheel", 'i', &reverse_mousewheel},
	{"",			0, NULL},
	{"modeline-show",	'i', &modeline_show},
	{"kill-whole-lines",	'i', &kill_whole_lines},
	{"",			0, NULL},
	{"backward-compat",	'i', &backward_compat},
	{"quoted-char-radix", 'i', &quoted_char_radix},
	{"",			0, NULL},
	{"fill-options",	'i', &fill_options},
	{"trim-whitespace",	'i', &trim_whitespace},
	{"",			0, NULL},
	{"insert-default-directory", 'i', &insert_default_directory},
	{"preserve-ligatures", 'i', &preserve_ligatures},
	{"",			0, NULL},
	{"complete-fold-file", 'i', &complete_fold_file},
	{"compare-fold-file",	'i', &compare_fold_file},
	{"",			0, NULL},
	{"blink-wait",	'i', &blink_wait},
	{"",			0, NULL},
	{"soft-tab-size",	'i', &soft_tab_size},
	{"tabs-with-spaces",	'i', &tabs_with_spaces},
	{"tab-options",	'i', &tab_options},
	{"",			0, NULL},
	{"buffer-name-width",	'i', &buffer_name_width},
	{"",			0, NULL},
	{"undo-limit",	'i', &undo_limit},
	{"undo-enabled",	'i', &undo_enabled},
	{"",			0, NULL},
	{"bell-type",		'i', &bell_type},
	{"emacs-compat",	'i', &emacs_compat},
	{"",			0, NULL},
	{"shell-command-limit", 'i', &shell_command_limit},
#if defined(LANGMODE_C) || defined(LANGMODE_CLIKE)
	{"",			0, NULL},
	{"# Language mode variables", 0, NULL},
	{"# -----------------------", 0, NULL},
#endif
#ifdef LANGMODE_C
	{"?",			0, NULL},			// "?" is skipped if previous line is comment,
	{"cc-strip-trailp",	'i', &cc_strip_trailp},	// otherwise lists as a blank line.
	{"cc-basic-indent",	'i', &cc_basic_indent},
	{"cc-cont-indent",	'i', &cc_cont_indent},
	{"cc-colon-indent",	'i', &cc_colon_indent},
#endif
#ifdef LANGMODE_CLIKE
	{"?",			0, NULL},
	{"clike-lang",	'i', &clike_lang},
	{"clike-options",	'i', &clike_options},
	{"clike-style",	'i', &clike_style},
#endif
  {NULL, 0, NULL},
};

static const var_entry obsolete_variables[] = {
	{"advancedinput",	      'i', &dummy},
	{"advancedinputdebug",    'i', &dummy},
	{"auto-trim-whitespace",  'i', &trim_whitespace},
	{"python-indent-offset",  'i', &dummy},
	{NULL, 0, NULL},
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

static char *local_svariables[] = {
	"comment-begin",
	"comment-end",
	NULL
};

const INT localvars = (sizeof(local_variables)/sizeof(char *) - 1);
const INT localsvars = (sizeof(local_svariables)/sizeof(char *) - 1);

static char **local_var_complete;

/* Call this from main() to initialise local variable completion
 * This is basically concat the list of integer and string local variable names
 */

void variable_completion_init(void) {
	int complete_size = localvars+localsvars+1;
	int i;
	/* ALlocate the list */
	local_var_complete = (char **)calloc(complete_size+1, sizeof(char *));
	/* copy the integer variable names */
	for (i = 0; local_variables[i] != NULL; i++)
		local_var_complete[i] = local_variables[i];
	/* copy the string variable names */
	for (; local_svariables[i-localvars]; i++)
		local_var_complete[i] = local_svariables[i-localvars];

}
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
	if (p->vtype == 'i') {
		if (!getINT(numstring, &num, 1)) return FALSE;

		*p->value = num;

		ewprintf("%s set to %d", p->name, *p->value);
	}
	if (p->vtype == 's') {
		strncpy(*p -> svalue, numstring, 19);
		ewprintf("%s set to '%s'", p->name, *p->svalue);
	}
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
		if (curbp->localvar.var[i] != MININT) {
			haslocal = 1;
			break;
		}
	}
	for (i = 0; i < localsvars; i++) {
		if (curbp->localsvar.var[i] != NULL) {
			haslocal = 1;
			break;
		}
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
		for (i = 0; i < localsvars; i++) {
			char *val = "";
			if (curbp -> localsvar.var[i] != NULL)
				val = curbp->localsvar.var[i];
			if ((f & FFARG) || (curbp->localsvar.var[i] != NULL))
				sprintf(line," %-24s\"%s\"",
						local_svariables[i], val);
			if (addline(bp, line) == FALSE) return FALSE;
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
			if (p->vtype == 'i')
				sprintf(line, " %-24s %7jd", p->name, (intmax_t)(*p->value));
			else
				sprintf(line, " %-24s %s", p->name, *p->svalue);
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
	char varname[VARLEN], val_value[20];
	INT num;
	char **p;
	INT is_string = 0;

	if ((s = eread("Local Set Variable: ",
		varname, sizeof(varname), EFLOCALVAR)) != TRUE) return s;

	for (p = local_variables; *p; p++) {
		if (strcmp(varname, *p) == 0) break;
	}

	if (*p == NULL) {
		/* Retry with the local string variables */
		for (p = local_svariables; *p; p++) {
			if (strcmp(varname, *p) == 0) {
				is_string = 1;	/* found, signal it is a string variable! */
				break;
			}
		}
		/* bad luck! */
		if (*p == NULL) {
			ewprintf("No such local variable");
			return FALSE;
		}
	}

	s = ereply("Local Set %s to: ", val_value, sizeof(val_value), varname);

	if (s == ABORT) {
		return s;
	} else if (s == FALSE) {
		ewprintf("No value given");
		return FALSE;
	}

	if (is_string == 0) {
		/* This is a number, convert and store */
		if (!getINT(val_value, &num, 1)) return FALSE;

		curbp->localvar.var[p - local_variables] = num;
		ewprintf("Local variable %s set to %ld", *p, num);
	} else{
		INT voffs = p - local_svariables;
		/* Clean up any previous value; */
		if (curbp->localsvar.var[voffs] != NULL)
			free(curbp->localsvar.var[voffs]);
		/* store a copy */
		curbp->localsvar.var[voffs] = strdup(val_value);
		ewprintf("Local variable %s set to '%s'", *p, curbp->localsvar.var[voffs]);
	}
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

	/* if (first) p = local_variables; */
	if (first) p = local_var_complete;
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
