/* keyboard macros for MicroGnuEmacs 1x */

/* Mg3a: Includes named macros */

#include "def.h"
#include "key.h"
#include "macro.h"
#include "kbd.h"


INT inmacro = 0;
INT inkbdmacro = 0;
INT macrodef = FALSE;
INT macrocount = 0;

macrorecord macro[MAXMACRO];

LINE *maclhead = NULL;
LINE *maclcur;
LINE *cmd_maclhead = NULL;	/* For excline() */

INT	universal_argument(INT f, INT n);


INT
start_kbd_macro(INT f, INT n)
{
	macrocount = 0;

	if (macrodef || inmacro) {
		return emessage(ABORT, "Already defining macro");
	}

	if ((maclhead = make_empty_list(maclhead)) == NULL) return FALSE;
	maclcur = maclhead;

	ewprintf("Defining Keyboard Macro...");
	return macrodef = TRUE;
}


INT
end_kbd_macro(INT f, INT n)
{
	macrodef = FALSE;
	ewprintf("End Keyboard Macro Definition");
	return TRUE;
}


/*
 * Execute a keyboard macro.
 *
 * Mg3a: deal with preserved flags.
 */

INT
call_last_kbd_macro(INT f, INT n)
{
	INT	i, j;
	PF	funct;
	INT	universal_argument();
	INT	flag, num, s;

	if (macrodef || n < 0 ||
		(macrocount >= MAXMACRO &&
		macro[MAXMACRO-1].m_funct != end_kbd_macro)) return FALSE;

	if (macrocount == 0 || n == 0) return TRUE;

	inmacro++;
	inkbdmacro++;

	for (i = n; i > 0; i--) {
		maclcur = maclhead->l_fp;
		flag = 0;
		num = 1;

		for (j = 0; j < macrocount-1; j++) {
			funct = macro[j].m_funct;

			if (funct == universal_argument) {
				if (j >= MAXMACRO - 2) break;

				flag = macro[++j].m_flag;
				num = macro[++j].m_count;
				continue;
			}

			ucs_adjust();

			s = execute(funct, flag, num);

			if (s != TRUE) {
				inmacro--;
				inkbdmacro--;
				return FALSE;
			}

			lastflag = thisflag;
			thisflag = 0;
			flag = 0;
			num = 1;
		}
	}

	inmacro--;
	inkbdmacro--;
	return TRUE;
}


/*
 * Mg3a: list current macro
 */

INT
list_kbd_macro(INT f, INT n)
{
	INT i;
	char buf[NFILEN], *name;
	BUFFER *bp;
	LINE *lp, *argp;
	INT buflen, lplen;

	if (macrocount == 0) {
		ewprintf("No keyboard macro defined");
		return FALSE;
	}

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	for (i = 0; i < macrocount; i++) {
		name = function_name(macro[i].m_funct);

		if (name == NULL) name = "(unknown function)";
		if (addline(bp, name) == FALSE) return FALSE;

		for (lp = lforw(maclhead); lp != maclhead; lp = lforw(lp)) {
			if (lp->l_flag == i + 1) {
				strcpy(buf, " arg = \"");
				buflen = strlen(buf);
				lplen = llength(lp);
				if ((argp = lalloc(buflen + lplen + 1)) == NULL) return FALSE;
				memcpy(ltext(argp), buf, buflen);
				memcpy(&ltext(argp)[buflen], ltext(lp), lplen);
				ltext(argp)[buflen + lplen] = '"';
				addlast(bp, argp);
			}
		}

		if (macro[i].m_funct == universal_argument) {
			if (i >= MAXMACRO - 2) break;
			strcpy(buf, "f =");
			buflen = strlen(buf);

			f = macro[i + 1].m_flag;
			n = macro[i + 2].m_count;

			if (f & FFUNIV)  strcat(buf, "|FFUNIV");
			if (f & FFNUM)   strcat(buf, "|FFNUM");
			if (f & FFRAND)  strcat(buf, "|FFRAND");
			if (f & FFUNIV2) strcat(buf, "|FFUNIV2");
			if (buf[buflen] == '|') buf[buflen] = ' ';

			sprintf(buf + strlen(buf), ", n = " INTFMT, n);

			if (addline(bp, buf) == FALSE) return FALSE;

			i += 2;
		}
	}

	return popbuftop(bp);
}


#ifdef USER_MACROS


/* Mg3a: Named macros */

typedef struct {
	char *name;
	char *commands;
} macrorec;

static macrorec *named_macros = NULL;
static INT named_macros_count = 0, named_macros_size = 0;


/*
 * Sorting named macros
 */

static int named_macros_sorted = 0;

static int
compare_named_macro(const void *a, const void *b)
{
	return strcmp(((macrorec *)a)->name, ((macrorec *)b)->name);
}

static void
sort_named_macros()
{
	if (named_macros) {
		qsort(named_macros, named_macros_count, sizeof(macrorec), compare_named_macro);
	}

	named_macros_sorted = 1;
}


/*
 * Mg3a: Look up a named macro. Return NULL if not found.
 */

char *
lookup_named_macro(char *name)
{
	INT low = 0, high = named_macros_count - 1, mid;
	int cmp;

	if (!named_macros_sorted) sort_named_macros();

	while (low <= high) {
		mid = (low + high) >> 1;
		cmp = strcmp(name, named_macros[mid].name);

		if (cmp > 0) {
			low = mid + 1;
		} else if (cmp < 0) {
			high = mid - 1;
		} else {
			return named_macros[mid].commands;
		}
	}

	return NULL;
}


/*
 * Mg3a: A linear lookup for create_macro.
 */

static char *
lookup_named_macro_simple(char *name)
{
	INT i;

	for (i = 0; i < named_macros_count; i++) {
		if (strcmp(name, named_macros[i].name) == 0) {
			return named_macros[i].commands;
		}
	}

	return NULL;
}


/*
 * Mg3a: Create a named macro.
 */

INT
create_macro(INT f, INT n)
{
	INT s, new_size;
	char macroname[NXNAME], commands[EXCLEN], *pmacroname, *pcommands;
	macrorec *new_named_macros;

	if ((s = ereply("Create Macro: ", macroname, sizeof(macroname))) != TRUE) return s;

	if (name_function(macroname)) {
		ewprintf("%s already exists as a function", macroname);
		return FALSE;
	} else if (lookup_named_macro_simple(macroname)) {
		ewprintf("%s already exists as a macro", macroname);
		return FALSE;
	}

	if ((s = ereply("Commands: ", commands, sizeof(commands))) != TRUE) return s;

	if ((pmacroname = string_dup(macroname)) == NULL) return FALSE;

	if ((pcommands = string_dup(commands)) == NULL) {
		free(pmacroname);
		return FALSE;
	}

	if (named_macros_count >= named_macros_size) {
		if (named_macros_size == 0) new_size = 4;
		else new_size = 2 * named_macros_size;

		if (named_macros_size > MAXINT / (2 * (INT)sizeof(macrorec)) ||
			(new_named_macros = realloc(named_macros, new_size*sizeof(macrorec))) == NULL)
		{
			free(pmacroname);
			free(pcommands);
 			ewprintf("Couldn't allocate new macro array");
			return FALSE;
		}

		named_macros = new_named_macros;
		named_macros_size = new_size;
	}

	named_macros[named_macros_count].name = pmacroname;
	named_macros[named_macros_count].commands = pcommands;

	named_macros_count++;

	named_macros_sorted = 0;

	ewprintf("Created macro \"%s\"", macroname);

	return TRUE;
}


/*
 * Mg3a: deliver next named macro for completion
 */

char *
getnext_macro(INT first)
{
	static INT i = 0;

	if (named_macros_count == 0) return NULL;

	if (first) {
		i = 0;
	} else if (i >= named_macros_count - 1) {
		return NULL;
	} else {
		i++;
	}

	return named_macros[i].name;
}


/*
 * Mg3a: List named macros.
 */

INT
list_macros(INT f, INT n)
{
#define MACROWIDTH 31
	char line[NXNAME + EXCLEN + 80];
	BUFFER *bp;
	INT i, len;

	if (!named_macros_sorted) sort_named_macros();

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	if (named_macros_count == 0) {
		if (addline(bp, "** No named macros **") == FALSE) return FALSE;
		return popbuftop(bp);
	}

	snprintf(line, sizeof(line), " %-*s %s", MACROWIDTH, "Macro Name", "Commands");
	if (addline(bp, line) == FALSE) return FALSE;

	snprintf(line, sizeof(line), " %-*s %s", MACROWIDTH, "----------", "--------");
	if (addline(bp, line) == FALSE) return FALSE;

	for (i = 0; i < named_macros_count; i++) {
		strcpy(line, " ");
		len = concat_limited(line, named_macros[i].name, MACROWIDTH, sizeof(line));
		snprintf(line + len, sizeof(line) - len, " \"%s\"", named_macros[i].commands);
		if (addline(bp, line) == FALSE) return FALSE;
	}

	return popbuftop(bp);
}


/*
 * Mg3a: Implement portable keybinding of macros. Max 100. // Tag:keymacro
 */

#define KEY_MACRO_MAX 100

static INT m0(INT f, INT n){ return -1;} static INT m1(INT f, INT n){ return -2;} static
INT m2(INT f, INT n){ return -3;} static INT m3(INT f, INT n){ return -4;} static INT
m4(INT f, INT n){ return -5;} static INT m5(INT f, INT n){ return -6;} static INT m6(INT
f, INT n){ return -7;} static INT m7(INT f, INT n){ return -8;} static INT m8(INT f, INT
n){ return -9;} static INT m9(INT f, INT n){ return -10;} static INT m10(INT f, INT n){
return -11;} static INT m11(INT f, INT n){ return -12;} static INT m12(INT f, INT n){
return -13;} static INT m13(INT f, INT n){ return -14;} static INT m14(INT f, INT n){
return -15;} static INT m15(INT f, INT n){ return -16;} static INT m16(INT f, INT n){
return -17;} static INT m17(INT f, INT n){ return -18;} static INT m18(INT f, INT n){
return -19;} static INT m19(INT f, INT n){ return -20;} static INT m20(INT f, INT n){
return -21;} static INT m21(INT f, INT n){ return -22;} static INT m22(INT f, INT n){
return -23;} static INT m23(INT f, INT n){ return -24;} static INT m24(INT f, INT n){
return -25;} static INT m25(INT f, INT n){ return -26;} static INT m26(INT f, INT n){
return -27;} static INT m27(INT f, INT n){ return -28;} static INT m28(INT f, INT n){
return -29;} static INT m29(INT f, INT n){ return -30;} static INT m30(INT f, INT n){
return -31;} static INT m31(INT f, INT n){ return -32;} static INT m32(INT f, INT n){
return -33;} static INT m33(INT f, INT n){ return -34;} static INT m34(INT f, INT n){
return -35;} static INT m35(INT f, INT n){ return -36;} static INT m36(INT f, INT n){
return -37;} static INT m37(INT f, INT n){ return -38;} static INT m38(INT f, INT n){
return -39;} static INT m39(INT f, INT n){ return -40;} static INT m40(INT f, INT n){
return -41;} static INT m41(INT f, INT n){ return -42;} static INT m42(INT f, INT n){
return -43;} static INT m43(INT f, INT n){ return -44;} static INT m44(INT f, INT n){
return -45;} static INT m45(INT f, INT n){ return -46;} static INT m46(INT f, INT n){
return -47;} static INT m47(INT f, INT n){ return -48;} static INT m48(INT f, INT n){
return -49;} static INT m49(INT f, INT n){ return -50;} static INT m50(INT f, INT n){
return -51;} static INT m51(INT f, INT n){ return -52;} static INT m52(INT f, INT n){
return -53;} static INT m53(INT f, INT n){ return -54;} static INT m54(INT f, INT n){
return -55;} static INT m55(INT f, INT n){ return -56;} static INT m56(INT f, INT n){
return -57;} static INT m57(INT f, INT n){ return -58;} static INT m58(INT f, INT n){
return -59;} static INT m59(INT f, INT n){ return -60;} static INT m60(INT f, INT n){
return -61;} static INT m61(INT f, INT n){ return -62;} static INT m62(INT f, INT n){
return -63;} static INT m63(INT f, INT n){ return -64;} static INT m64(INT f, INT n){
return -65;} static INT m65(INT f, INT n){ return -66;} static INT m66(INT f, INT n){
return -67;} static INT m67(INT f, INT n){ return -68;} static INT m68(INT f, INT n){
return -69;} static INT m69(INT f, INT n){ return -70;} static INT m70(INT f, INT n){
return -71;} static INT m71(INT f, INT n){ return -72;} static INT m72(INT f, INT n){
return -73;} static INT m73(INT f, INT n){ return -74;} static INT m74(INT f, INT n){
return -75;} static INT m75(INT f, INT n){ return -76;} static INT m76(INT f, INT n){
return -77;} static INT m77(INT f, INT n){ return -78;} static INT m78(INT f, INT n){
return -79;} static INT m79(INT f, INT n){ return -80;} static INT m80(INT f, INT n){
return -81;} static INT m81(INT f, INT n){ return -82;} static INT m82(INT f, INT n){
return -83;} static INT m83(INT f, INT n){ return -84;} static INT m84(INT f, INT n){
return -85;} static INT m85(INT f, INT n){ return -86;} static INT m86(INT f, INT n){
return -87;} static INT m87(INT f, INT n){ return -88;} static INT m88(INT f, INT n){
return -89;} static INT m89(INT f, INT n){ return -90;} static INT m90(INT f, INT n){
return -91;} static INT m91(INT f, INT n){ return -92;} static INT m92(INT f, INT n){
return -93;} static INT m93(INT f, INT n){ return -94;} static INT m94(INT f, INT n){
return -95;} static INT m95(INT f, INT n){ return -96;} static INT m96(INT f, INT n){
return -97;} static INT m97(INT f, INT n){ return -98;} static INT m98(INT f, INT n){
return -99;} static INT m99(INT f, INT n){ return -100;}

static const PF macrofuncs[KEY_MACRO_MAX] = {
	m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17,
	m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33,
	m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49,
	m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62, m63, m64, m65,
	m66, m67, m68, m69, m70, m71, m72, m73, m74, m75, m76, m77, m78, m79, m80, m81,
	m82, m83, m84, m85, m86, m87, m88, m89, m90, m91, m92, m93, m94, m95, m96, m97,
	m98, m99,
};

static INT key_macro_count = 0;		/* Number of macros in the table      */

static char *key_macro[KEY_MACRO_MAX];


/*
 * Mg3a: Translate a named function, named macro or macro string to a
 * function pointer. A macro string must contain either " " or ";".
 */

PF
name_or_macro_to_key_function(char *fname)
{
	PF pf;
	char *macrotext;

	if ((pf = name_function(fname)) != NULL) return pf;

	if (!lookup_named_macro(fname) && !strchr(fname, ' ') && !strchr(fname, ';')) {
		ewprintf("Not a function or macro: %s", fname);
		return NULL;
	}

	if (key_macro_count < KEY_MACRO_MAX) {
		if ((macrotext = string_dup(fname)) == NULL) return NULL;
		key_macro[key_macro_count] = macrotext;
		return macrofuncs[key_macro_count++];
	} else {
		ewprintf("Max key macros allocated (%d)", (INT)KEY_MACRO_MAX);
		return NULL;
	}
}


/*
 * Mg3a: Return a presentation string for a function pointer known to
 * not be an ordinary function.
 */

char *
key_function_to_name_or_macro(PF pf)
{
	static char name[EXCLEN+2];
	INT i;

	for (i = 0; i < key_macro_count; i++) {
		if (pf == macrofuncs[i]) {
			snprintf(name, sizeof(name), "\"%s\"", key_macro[i]);
			return name;
		}
	}

	return NULL;
}


/*
 * Mg3a: execute a macro by number.
 */

INT
func_macro_execute(INT i, INT n)
{
	if (i >= 0 && i < key_macro_count) {
		return excline_copy_list_n(key_macro[i], n);
	}

	ewprintf("Macro %d not found, not one of %d.", i, key_macro_count);
	return FALSE;
}

#endif
