/*
 * Keyboard maps.  This is character set dependent.
 * The terminal specific parts of building the
 * keymap has been moved to a better place.
 */

#include	"def.h"
#include	"kbd.h"
#include	"macro.h"
#include	"key.h"

/*
 * Defined by "basic.c".
 */
extern	INT	gotobol();		/* Move to start of line	*/
extern	INT	backchar();		/* Move backward by characters	*/
extern	INT	gotoeol();		/* Move to end of line		*/
extern	INT	forwchar();		/* Move forward by characters	*/
extern	INT	backtoindent();		/* Move to start of indent	*/
extern	INT	beginvisualline();	/* Move to begin of visual line	*/
extern	INT	endvisualline();	/* Move to end of visual line	*/
extern	INT	gotobob();		/* Move to start of buffer	*/
extern	INT	gotoeob();		/* Move to end of buffer	*/
extern	INT	forwline();		/* Move forward by lines	*/
extern	INT	backline();		/* Move backward by lines	*/
extern	INT	forwpage();		/* Move forward by pages	*/
extern	INT	backpage();		/* Move backward by pages	*/
extern	INT	pagenext();		/* Page forward next window	*/
extern	INT	pagenext_backward();	/* Page backward next window	*/
extern	INT	setmark();		/* Set mark			*/
extern	INT	markwholebuffer();	/* Mark whole buffer		*/
extern	INT	swapmark();		/* Swap "." and mark		*/
extern	INT	gotoline();		/* Go to a specified line.	*/
extern	INT	forw1page();		/* move forward by lines	*/
extern	INT	back1page();		/* move back by lines		*/

/*
 * Defined by "buffer.c".
 */
extern	INT	listbuffers();		/* Display list of buffers	*/
extern	INT	usebuffer();		/* Switch a window to a buffer	*/
extern	INT	poptobuffer();		/* Other window to a buffer	*/
extern	INT	killbuffer();		/* Make a buffer go away.	*/
extern	INT	nextbuffer();		/* Switch to the next buffer.	*/
extern	INT	prevbuffer();		/* Switch to the previous buffer. */
extern	INT	savebuffers();		/* Save unmodified buffers	*/
extern	INT	bufferinsert();		/* Insert buffer into another	*/
extern	INT	notmodified();		/* Reset modification flag	*/
extern	INT	bufed();		/* Bufed			*/
extern	INT	bufed_otherwindow();	/* Bufed in other window	*/
extern	INT	bufed_gotobuf();	/* Bufed goto buffer		*/
extern	INT	bufed_gotobuf_one();	/* Bufed goto buffer in one window */
extern	INT	bufed_gotobuf_other();	/* Bufed goto buffer in other window */
extern	INT	bufed_gotobuf_switch_other();	/* Bufed goto buffer in other window,	*/
						/* without switching window 		*/
extern	INT	bufed_gotobuf_two();	/* Bufed goto buffer in other window, and	*/
					/* back to original buffer in current window	*/
extern	INT	killbuffer_andwindow();	/* Kill buffer and window	*/
extern	INT	killbuffer_quickly();	/* Kill buffer without asking for name */
extern	INT	killthisbuffer();	/* Kill this buffer a la Emacs	*/

/*
 * Defined by "dir.c"
 */
extern	INT	changedir();		/* change current directory	*/
extern	INT	showcwdir();		/* show current directory	*/

#ifdef DIRED
/*
 * defined by "dired.c"
 */
extern	INT	dired();		/* dired			*/
extern	INT	d_otherwindow();	/* dired other window		*/
extern	INT	d_findfile();		/* dired find file		*/
extern	INT	d_ffonewindow();	/* dired find file one window	*/
extern	INT	d_ffotherwindow();	/* dired find file other window */
extern	INT	d_del();		/* dired mark for deletion	*/
extern	INT	d_undel();		/* dired unmark			*/
extern	INT	d_undelbak();		/* dired unmark backwards	*/
extern	INT	d_expunge();		/* dired expunge		*/
extern	INT	d_copy();		/* dired copy			*/
extern	INT	d_rename();		/* dired rename			*/
extern	INT	d_updir();		/* dired up directory		*/
extern	INT	d_jump();		/* dired show dir of file	*/
extern	INT	d_jump_otherwindow();	/* dired show dir of file other window */
extern	INT	d_nextdir();		/* dired next subdirectory	*/
extern	INT	d_prevdir();		/* dired previous subdirectory	*/
extern	INT	d_createdir();		/* dired create directory	*/
#endif

/*
 * Defined by "display.c".
 */
#ifdef CHARSDEBUG
extern	INT	charsdebug_set();	/* Toggle chars debug		*/
extern	INT	charsdebug_zero();	/* Zero chars debug counters	*/
#endif

/*
 * Defined by "extend.c".
 */
extern	INT	extend();		/* Extended commands.		*/
extern	INT	bindtokey();		/* Modify global key bindings.	*/
extern	INT	localbind();		/* Modify mode key bindings.	*/
extern	INT	define_key();		/* modify any key map		*/
extern	INT	unbindtokey();		/* delete global binding	*/
extern	INT	localunbind();		/* delete local binding		*/
extern	INT	insert();		/* insert string		*/
extern	INT	evalexpr();		/* Evaluate expression		*/
extern	INT	evalbuffer();		/* Evaluate current buffer	*/
extern	INT	evalfile();		/* Evaluate a file		*/
extern	INT	ignore_errors();	/* Like execute-extended-command but ignore errors */
extern	INT	nil();			/* Function for general unbinding */
extern	INT	auto_execute();		/* Add autoexec command		*/
extern	INT	auto_execute_list();	/* Add autoexec command list	*/
extern	INT	shebang();		/* Add autoexec shebang test	*/
extern	INT	ifcmd();		/* Conditional execution for autoexec */
extern	INT	listpatterns();		/* List autoexec patterns	*/
extern	INT	message();		/* Show message in echo line	*/
extern	INT	with_message();		/* Make command show message in macro */
extern	INT	with_key();		/* Debug command bound to key	*/

/*
 * Defined by "file.c".
 */
extern	INT	filevisit();		/* Get a file, read write	*/
extern	INT	filevisit_readonly();	/* Visit file read only		*/
extern	INT	poptofile();		/* Get a file, other window	*/
extern	INT	poptofile_readonly();	/* Get a file, other window, readonly */
extern	INT	filewrite();		/* Write a file			*/
extern	INT	filesave();		/* Save current file		*/
extern	INT	fileinsert();		/* Insert file into buffer	*/
extern	INT	makebkfile();		/* Control backups on saves	*/
extern	INT	filerevert();		/* Revert to file on disk	*/
extern	INT	filerevert_forget();	/* Revert to file on disk, Mg style */

/*
 * defined by "help.c".
 */
extern	INT	desckey();		/* describe key			*/
extern	INT	wallchart();		/* Make wall chart.		*/
extern	INT	help_help();		/* help help			*/
extern	INT	apropos_command();	/* apropos			*/
#ifdef HELP_RET
extern	INT	help_ret();		/* RET in a help buffer		*/
#endif

/*
 * defined by "kbd.c"
 */
#ifdef	BSMAP
extern	INT	bsmap();		/* backspace mapping		*/
#endif
extern	INT	universal_argument();	/* Ctrl-U			*/
extern	INT	digit_argument();	/* M-1, etc.			*/
extern	INT	negative_argument();	/* M--				*/
extern	INT	selfinsert();		/* Insert character		*/
extern	INT	rescan();		/* internal try again function	*/

#ifdef LANGMODE_C
/*
 * Defined by "langmode_c.c"
 */
extern	INT	cmode();		/* Toggle C-mode		*/
extern	INT	cc_char();		/* Insert char, indent		*/
extern	INT	cc_brace();		/* Insert char, blink, indent	*/
extern	INT	cc_tab();		/* Insert tab or re-indent	*/
extern 	INT	cc_indent();		/* Re-indent current line	*/
extern	INT	cc_lfindent();		/* C newline and indent		*/
extern	INT	cc_preproc();		/* C preprocessor		*/
#endif

#ifdef LANGMODE_CLIKE
/*
 * defined by "langmode_clike.c"
 */
extern	INT	clike_newline_and_indent();	/* Clike newline and indent	*/
extern	INT	clike_insert();			/* Insert and re-indent on blank line */
extern	INT	clike_insert_special();		/* Insert ':' and re-indent if case statement */
extern	INT	clike_align_after_paren();	/* Align after a '(' in previous line */
extern	INT	clike_align_after();		/* Align after a '=' or other char in previous line */
extern	INT	clike_align_back();		/* Align with a smaller earlier indent */
extern	INT	clike_tab_or_indent();		/* Clike tab or indent		*/
extern	INT	clike_indent();			/* Clike re-indent line		*/
extern	INT	clike_indent_next();		/* Clike re-indent line, go to next line */
extern	INT	clike_indent_region();		/* Clike re-indent region	*/
extern	INT	clike_mode();			/* Clike mode switch		*/
extern	INT	clike_showmatch();		/* Clike blink-and-insert	*/
#endif

#ifdef LANGMODE_PYTHON
/*
 * defined by "langmode_python.c"
 */
extern	INT	pymode();		/* Python mode switch		*/
extern	INT	py_indent();		/* Python re-indent line	*/
extern	INT	py_lfindent();		/* Python newline and indent	*/
#endif

/*
 * Defined by "line.c".
 */
#ifdef UPDATE_DEBUG
extern INT	do_update_debug();	/* Turn on debugging of screen updates */
#endif
extern INT lduplicate();
extern INT lmoveup();
extern INT lmovedown();
/*
 * defined by "macro.c"
 */
extern	INT	start_kbd_macro();	/* Begin keyboard macro		*/
extern	INT	end_kbd_macro();	/* End keyboard macro		*/
extern	INT	call_last_kbd_macro();	/* Execute keyboard macro	*/
extern	INT	list_kbd_macro();	/* List keyboard macro		*/
#ifdef USER_MACROS
extern	INT	create_macro();		/* Create named macro		*/
extern	INT	list_macros();		/* List named macros		*/
#endif

/*
 * Defined by "main.c".
 */
extern	INT	keyboard_quit();		/* Abort out of things		*/
extern	INT	ctrlg();		/* Abort out of things		*/
extern	INT	quit();			/* Quit, asking to save files	*/
extern	INT	hardquit();		/* Quit the editor immediately	*/
extern	INT	save_and_exit();	/* Save and quit (not in Emacs)	*/

/*
 * Defined by "match.c"
 */
extern	INT	showmatch();		/* Hack to show matching paren	 */

/*
 * defined by "modes.c"
 */
extern	INT	indentmode();		/* set auto-indent mode		*/
extern	INT	fillmode();		/* set word-wrap mode		*/
extern	INT	notabmode();		/* no tab mode			*/
extern	INT	overwrite();		/* overwrite mode		*/
extern	INT	unixlf();		/* Unix line endings		*/
extern	INT	crlf();			/* DOS/TCP line endings		*/
extern 	INT	cmdutf8();		/* UTF-8 toggle			*/
extern	INT	bom();			/* Toggle BOM - Byte Order Mark	*/
extern	INT	ubom();			/* Toggle BOM and UTF-8		*/
extern	INT	nobom();		/* Turn off BOM 		*/
extern	INT	cmddos();		/* DOS toggle			*/
extern	INT	cmdiso();		/* 8-bit toggle			*/
extern	INT	set_default_charset();	/* Set default charset		*/
extern	INT	set_default_8bit_charset();	/* Set default charset for 8 bit */
extern	INT	set_dos_charset();	/* Set dos charset	 	*/
extern	INT	local_set_charset();	/* Set charset in current buffer */
extern	INT	listcharsets();		/* List supported charsets	*/
extern	INT	toggle_readonly();	/* Toggle readonly		*/
#ifdef SLOW
extern	INT	slowmode();		/* Slow mode			*/
#endif
extern	INT	set_default_mode();	/* set default modes		*/
extern	INT	localmodename();	/* set name of buffer-local mode */

/*
 * defined by "paragraph.c" - the paragraph justification code.
 */
extern	INT	gotobop();		/* Move to start of paragraph.	*/
extern	INT	gotoeop();		/* Move to end of paragraph.	*/
extern	INT	fillpara();		/* Justify a paragraph.		*/
extern	INT	killpara();		/* Delete a paragraph.		*/
extern	INT	markpara();		/* Mark a paragraph		*/
extern	INT	twiddlepara();		/* Transpose paragraphs		*/
extern	INT	setfillcol();		/* Set fill column for justify. */
extern	INT	fillword();		/* Insert char with word wrap.	*/

/*
 * Defined by "random.c".
 */
extern	INT	showcpos();		/* Show the cursor position	*/
extern	INT	twiddle();		/* Twiddle characters		*/
extern	INT	twiddleword();		/* twiddle words		*/
extern	INT	quote();		/* Insert literal		*/
extern	INT	openline();		/* Open up a blank line		*/
extern	INT	newline();		/* Insert newline		*/
extern	INT	newlineclassic();	/* Insert newline, classic	*/
extern	INT	deblank();		/* Delete blank lines		*/
extern	INT	justone();		/* Delete extra whitespace	*/
extern	INT	delwhite();		/* Delete all whitespace	*/
extern	INT	indent();		/* Insert newline, then indent	*/
extern	INT	indentsame();		/* Newline, indent same 	*/
extern	INT	forwdel();		/* Forward delete		*/
extern	INT	backdel();		/* Backward delete in		*/
extern	INT	backdeluntab();		/* Backward delete untabify	*/
extern	INT	killline();		/* Kill forward			*/
extern	INT	killwholeline();	/* Kill whole line		*/
extern	INT	yank();			/* Yank back from killbuffer.	*/
extern	INT	deletetrailing();	/* Delete trailing whitespace	*/
extern	INT	no_break();		/* Change/insert no-break character */
extern	INT	joinline();		/* Join lines			*/
extern	INT	joinline_forward();	/* Join lines, default forward	*/ // Mg3a extension
extern	INT	insert_tab();		/* Insert soft/hard tab		*/
extern	INT	insert_tab_8();		/* Insert tab of size 8		*/
extern  INT comment_line();      /* comment line */

/*
 * Defined by "region.c".
 */
extern	INT	killregion();		/* Kill region.			*/
extern	INT	copyregion();		/* Copy region to kill buffer.	*/
extern	INT	deleteregion();		/* Delete the region		*/
extern	INT	lowerregion();		/* Lower case region.		*/
extern	INT	upperregion();		/* Upper case region.		*/
extern	INT	prefixregion();		/* Prefix all lines in region	*/
extern	INT	setprefix();		/* Set line prefix string	*/
extern	INT	appendnextkill();	/* Append next kill		*/
extern	INT	tabregionright();	/* Shift region n tabs right	*/
extern	INT	tabregionleft();	/* Shift region n tabs left	*/
#ifdef INDENT_RIGIDLY
extern	INT	indent_rigidly();	/* Shift region like in Emacs	*/
#endif

/*
 * Defined by "search.c".
 */
extern	INT	word_search_mode();	/* Word search mode		*/
extern	INT	case_fold_mode();	/* Case fold search mode	*/
extern	INT	forwsearch();		/* Search forward		*/
extern	INT	backsearch();		/* Search backwards		*/
extern	INT	searchagain();		/* Repeat last search command	*/
extern	INT	forwisearch();		/* Incremental search forward	*/
extern	INT	backisearch();		/* Incremental search backwards */
extern	INT	queryrepl();		/* Query replace		*/
#ifdef SEARCHALL
extern	INT	forwsearchall();	/* Forward search in all buffers */
extern	INT	backsearchall();	/* Backward search in all buffers */
#endif
#ifdef SEARCHSIMPLE
extern	INT	searchsimple();		/* Simple search		*/
#ifdef SEARCHALL
extern	INT	searchallsimple();	/* Simple search, all buffers	*/
#endif
#endif

/*
 * Defined by "spawn.c".
 */
extern	INT	spawncli();		/* Run CLI in a subjob.		*/
extern	INT	shellcommand();		/* Run command, capture output	*/
extern	INT	yank_process();		/* Variant of shell-command	*/
extern	INT	setshell();		/* Override shell		*/

#if TESTCMD
/*
 * Defined by "testcmd.inc".
 */
extern	INT	testcmd();		/* Test command			*/
#endif

/*
 * Defined by "ucs.c".
 */
extern 	INT	showbytes();		/* Show bytes of character	*/
extern	INT 	explode();		/* Explode combined key sequence */
extern	INT 	implode();		/* Implode combined key sequence */
extern	INT 	insert_unicode();	/* Insert Unicode character	*/
extern	INT 	insert_unicode_hex();	/* Insert Unicode character (hex) */
extern	INT 	insert_8bit();		/* Insert 8-bit character	*/
extern	INT 	insert_8bit_hex();	/* Insert 8-bit character (hex)	*/
extern	INT	ucs_insert();		/* Emulate Emacs ucs-insert	*/

#ifdef UCSNAMES
/*
 * Defined by "ucsnames.c"
 */
extern	INT	set_unicode_data();	/* Set Unicode data file	*/
extern	INT	list_unicode();		/* List Unicode data		*/
#endif

/*
 * Defined by "undo.c"
 */
extern	INT	undo();			/* Emacs-like undo		*/
extern	INT	undo_only();		/* Emacs-like undo-only		*/
extern	INT	redo();			/* Redo				*/
extern	INT	undo_boundary();	/* Introduce undo boundary	*/
#ifdef LIST_UNDO
extern	INT	list_undo();		/* List undo list contents	*/
#endif

/*
 * Defined by "variables.c".
 */
extern	INT	setvar();		/* Set named variable		*/
extern	INT	listvars();		/* List variables		*/
extern	INT	localsetvar();		/* Set local named variable	*/
extern	INT	localunsetvar();	/* Unset local named variable	*/
extern	INT	localsettabs();		/* Set local tab variables	*/

/* defined by "version.c" */

extern	INT	showversion();		/* Show version numbers, etc.	*/

/*
 * Defined by "window.c".
 */
extern	INT	recenter();		/* Recenter window		*/
extern	INT	recentertopbottom();	/* Recenter like in Emacs	*/
extern	INT	movetoline();		/* Move to window line		*/
extern	INT	movetoline_topbottom();	/* Move to window line top bottom */
extern	INT	refresh();		/* Refresh the screen		*/
extern	INT	nextwind();		/* Move to the next window	*/
extern	INT	prevwind();		/* Move to the previous window	*/
extern	INT	onlywind();		/* Make current window only one */
extern	INT	splitwind();		/* Split current window		*/
extern	INT	delwind();		/* Delete current window	*/
extern	INT	enlargewind();		/* Enlarge display window.	*/
extern	INT	shrinkwind();		/* Shrink window.		*/
extern	INT	quitwind();		/* Quit window from Emacs	*/
extern	INT	balancewind();		/* Balance window areas		*/
extern	INT	shrinkwind_iflarge();	/* Shrink window if larger than buffer */

/*
 * Defined by "word.c".
 */
extern	INT	backword();		/* Backup by words		*/
extern	INT	forwword();		/* Advance by words		*/
extern	INT	upperword();		/* Upper case word.		*/
extern	INT	lowerword();		/* Lower case word.		*/
extern	INT	capword();		/* Initial capitalize word.	*/
extern	INT	delfword();		/* Delete forward word.		*/
extern	INT	delbword();		/* Delete backward word.	*/


/* initial keymap declarations, deepest first. Tag:keymaps */

static 	PF	metalbA[] = {	/* <ESC>[ */
	backline,	/* A */
	forwline,	/* B */
	forwchar,	/* C */
	backchar,	/* D */
};

static 	PF	metaOA[] = {	/* <ESC>O */
	backline,	/* A */
	forwline,	/* B */
	forwchar,	/* C */
	backchar,	/* D */
};

static struct KEYMAPE(1) metalbmap = {
	1,
	1,
	rescan,
	{
		{'A',	'D',	metalbA,	NULL},
	}
};

static struct KEYMAPE(1) metaOmap = {
	1,
	1,
	rescan,
	{
		{'A',	'D',	metaOA,		NULL},
	}
};

static	PF	cHcG[] = {
	ctrlg,		/* ^G */
	help_help,	/* ^H */
};
static	PF	cHa[]	= {
	apropos_command,/* a */
	wallchart,	/* b */
	desckey,	/* c */
};
static	struct	KEYMAPE(2) helpmap = {
	2,
	2,
	rescan,
	{
		{CCHR('G'),CCHR('H'),	cHcG,	NULL},
		{'a',	'c',		cHa,	NULL},
	}
};

static	PF	cX4cF[] = {
	poptofile,	/* ^f */
	ctrlg,		/* ^g */
	rescan,		/* ^h */
	rescan,		/* ^i */
#ifdef DIRED
	d_jump_otherwindow, /* ^j */
#else
	rescan,		/* ^j */
#endif
};
static	PF	cX40[] = {
	killbuffer_andwindow, /* 0 */
};
static	PF	cX4b[] = {
	poptobuffer,	/* b */
	rescan,		/* c */
#ifdef DIRED
	d_otherwindow,	/* d */
#else
	rescan,		/* d */
#endif
	rescan,		/* e */
	poptofile,	/* f */
};
static PF	cX4r[] = {
	poptofile_readonly, /* r */
};
static	struct	KEYMAPE(4) cX4map	= {
	4,
	4,
	rescan,
	{
		{CCHR('F'),CCHR('J'),	cX4cF,	NULL},
		{'0',	'0',		cX40,	NULL},
		{'b',	'f',		cX4b,	NULL},
		{'r',	'r',		cX4r,	NULL},
	}
};

static	PF	cXcB[] = {
	listbuffers,	/* ^B */
	quit,		/* ^C */
	rescan,		/* ^D */
	rescan,		/* ^E */
	filevisit,	/* ^F */
	ctrlg,		/* ^G */
	rescan,		/* ^H */
#ifdef INDENT_RIGIDLY
	indent_rigidly,	/* ^I */
#else
	rescan,		/* ^I */
#endif
#ifdef DIRED
	d_jump,		/* ^J */
#else
	rescan,		/* ^J */
#endif
};
static	PF	cXcL[] = {
	lowerregion,	/* ^L */
	rescan,		/* ^M */
	rescan,		/* ^N */
	deblank,	/* ^O */
	rescan,		/* ^P */
	toggle_readonly,/* ^Q */
	filevisit_readonly, /* ^R */
	filesave,	/* ^S */
	rescan,		/* ^T */
	upperregion,	/* ^U */
	rescan,		/* ^V */
	filewrite,	/* ^W */
	swapmark,	/* ^X */
};
static	PF	cXlp[]	= {
	start_kbd_macro, /* ( */
	end_kbd_macro,	/* ) */
	rescan,		/* * */
	balancewind,	/* + */
	rescan,		/* , */
	shrinkwind_iflarge, /* - */
	rescan,		/* . */
	rescan,		/* / */
	delwind,	/* 0 */
	onlywind,	/* 1 */
	splitwind,	/* 2 */
	rescan,		/* 3 */
	prefix,		/* 4 */
};
static	PF	cXeq[]	= {
	showcpos,	/* = */
};
static	PF	cXcar[] = {
	enlargewind,	/* ^ */
	rescan,		/* _ */
	rescan,		/* ` */
	rescan,		/* a */
	usebuffer,	/* b */
	rescan,		/* c */
#ifdef DIRED
	dired,		/* d */
#else
	rescan,		/* d */
#endif
	call_last_kbd_macro,	/* e */
	setfillcol,	/* f */
	gotoline,	/* g */
	markwholebuffer,	/* h */
	fileinsert,	/* i */
	rescan,		/* j */
	killbuffer,	/* k */
	rescan,		/* l */
	rescan,		/* m */
	nextwind,	/* n */
	nextwind,	/* o */
	prevwind,	/* p */
	rescan,		/* q */
	rescan,		/* r */
	savebuffers,	/* s */
	rescan,		/* t */
	undo,		/* u */
};

static	struct	KEYMAPE(5) cXmap = {
	5,
	5,
	rescan,
	{
		{CCHR('B'),CCHR('J'),	cXcB,	NULL},
		{CCHR('L'),CCHR('X'),	cXcL,	NULL},
		{'(',	'4',		cXlp,	(KEYMAP *)&cX4map},
		{'=',	'=',		cXeq,	NULL},
		{'^',	'u',		cXcar,	NULL},
	}
};

static	PF	metacG[] = {
	ctrlg,		/* ^G */
};
static	PF	metacV[] = {
	pagenext,	/* ^V */
	appendnextkill,	/* ^W */
};
static	PF	metasp[] = {
	justone,	/* space */
	shellcommand,	/* ! */
};
static	PF	metapct[] = {
	queryrepl,	/* % */
};
static	PF	metami[] = {
	negative_argument, /* - */
	rescan,		/* . */
	rescan,		/* / */
	digit_argument, /* 0 */
	digit_argument, /* 1 */
	digit_argument, /* 2 */
	digit_argument, /* 3 */
	digit_argument, /* 4 */
	digit_argument, /* 5 */
	digit_argument, /* 6 */
	digit_argument, /* 7 */
	digit_argument, /* 8 */
	digit_argument, /* 9 */
	evalexpr,	    /* : */
	comment_line,	/* ; */
	gotobob,	/* < */
	rescan,		/* = */
	gotoeob,	/* > */
};
static	PF	metaO[] = {
	prefix,		/* O */
};
static	PF	metalb[] = {
	prefix,		/* [ */
	delwhite,	/* \ */
	rescan,		/* ] */
	joinline,	/* ^ */
	rescan,		/* _ */
	rescan,		/* ` */
	rescan,		/* a */
	backword,	/* b */
	capword,	/* c */
	delfword,	/* d */
	rescan,		/* e */
	forwword,	/* f */
	rescan,		/* g */
	markpara,	/* h */
	insert_tab_8,	/* i */
};
static	PF	metal[] = {
	lowerword,	/* l */
	backtoindent,	/* m */
	rescan,		/* n */
	rescan,		/* o */
	rescan,		/* p */
	fillpara,	/* q */
	movetoline_topbottom,	/* r */
	forwsearch,	/* s */
	twiddleword,	/* t */
	upperword,	/* u */
	backpage,	/* v */
	copyregion,	/* w */
	extend,		/* x */
	rescan,		/* y */
	rescan,		/* z */
	gotobop,	/* lbrace */
	rescan,		/* | */
	gotoeop,	/* rbrace */
	notmodified,	/* ~ */
	delbword,	/* DEL */
};

static	struct	KEYMAPE(8) metamap = {
	8,
	8,
	rescan,
	{
		{CCHR('G'),CCHR('G'),	metacG,		NULL},
		{CCHR('V'),CCHR('W'),	metacV,		NULL},
		{' ',	'!',		metasp,		NULL},
		{'%',	'%',		metapct,	NULL},
		{'-',	'>',		metami,		NULL},
		{'O',	'O',		metaO,		(KEYMAP *)&metaOmap},
		{'[',	'i',		metalb,		(KEYMAP *)&metalbmap},
		{'l',	CCHR('?'),	metal,		NULL},
	}
};

static	PF	fund_at[] = {
	setmark,	/* ^@ */
	gotobol,	/* ^A */
	backchar,	/* ^B */
	rescan,		/* ^C */
	forwdel,	/* ^D */
	gotoeol,	/* ^E */
	forwchar,	/* ^F */
	keyboard_quit,		/* ^G */
	prefix,		/* ^H */
};

static	PF	fund_CI[] = {
	insert_tab,	/* ^I */
	indent,		/* ^J */
	killline,	/* ^K */
	recentertopbottom, /* ^L */
	newline,	/* ^M */
	forwline,	/* ^N */
	openline,	/* ^O */
	backline,	/* ^P */
	quote,		/* ^Q */
	backisearch,	/* ^R */
	forwisearch,	/* ^S */
	twiddle,	/* ^T */
	universal_argument, /* ^U */
	forwpage,	/* ^V */
	killregion,	/* ^W */
	prefix,		/* ^X */
	yank,		/* ^Y */
	spawncli, 	/* ^Z */
};
static	PF	fund_esc[] = {
	prefix,		/* esc */
	rescan,		/* ^\ */	/* selfinsert is default on fundamental */
	rescan,		/* ^] */
	rescan,		/* ^^ */
	undo,		/* ^_ */
};
static	PF	fund_rp[] = {
	showmatch,	/* ) */
};
static	PF	fund_del[] = {
	backdel,	/* DEL */
};

static	struct	KEYMAPE(5) fundmap = {
	5,
	5,
	selfinsert,
	{
		{CCHR('@'),CCHR('H'),	fund_at,	(KEYMAP *)&helpmap},
		{CCHR('I'),CCHR('Z'),	fund_CI,	(KEYMAP *)&cXmap},
		{CCHR('['),CCHR('_'),	fund_esc,	(KEYMAP *)&metamap},
		{')',	')',		fund_rp,	NULL},
		{CCHR('?'),CCHR('?'),	fund_del,	NULL},
	}
};

static	PF	fill_sp[] = {
	fillword,	/* ' ' */
};
static struct KEYMAPE(1) fillmap = {
	1,
	1,
	rescan,
	{
		{' ',	' ',	fill_sp,	NULL},
	}
};

static	PF	indent_lf[] = {
	newline,	/* ^J */
	rescan,		/* ^K */
	rescan,		/* ^L */
	indent,		/* ^M */
};
static	struct	KEYMAPE(1) indntmap = {
	1,
	1,
	rescan,
	{
		{CCHR('J'), CCHR('M'),	indent_lf,	NULL},
	}
};


#ifdef DIRED
/* Tag:diredmaps */

static	PF	diredctrld[] = {
	d_del,		/* ^D */
};
static	PF	diredcm[] = {
	d_findfile,	/* ^M */
};
static	PF	diredsp[] = {
	forwline,	/* SP */
};
static	PF	diredplus[] = {
	d_createdir,	/* + */
};
static	PF	dired1[] = {
	d_ffonewindow,	/* 1 */
};
static	PF	diredlt[] = {
	d_prevdir,	/* < */
	rescan,		/* = */
	d_nextdir,	/* > */
	wallchart,	/* ? */
};
static	PF	diredup[] = {
	d_updir,	/* ^ */
};
static	PF	diredc[] = {
	d_copy,		/* c */
	d_del,		/* d */
	d_findfile,	/* e */
	d_findfile,	/* f */
	filerevert,	/* g */
	wallchart,	/* h */
};
static	PF	diredn[] = {
	forwline,	/* n */
	d_ffotherwindow,/* o */
	backline,	/* p */
	quitwind,	/* q */
	d_rename,	/* r */
	rescan,		/* s */
	rescan,		/* t */
	d_undel,	/* u */
	rescan,		/* v */
	rescan,		/* w */
	d_expunge,	/* x */
};
static	PF	direddl[] = {
	d_undelbak,	/* del */
};

static	struct	KEYMAPE(10) diredmap = {
	10,
	10,
	rescan,
	{
		{CCHR('D'),	CCHR('D'),	diredctrld,	NULL},
		{CCHR('M'),	CCHR('M'),	diredcm,	NULL},
		{' ',		' ',		diredsp,	NULL},
		{'+',		'+',		diredplus,	NULL},
		{'1',		'1',		dired1,		NULL},
		{'<',		'?',		diredlt,	NULL},
		{'^',		'^',		diredup,	NULL},
		{'c',		'h',		diredc,		NULL},
		{'n',		'x',		diredn,		NULL},
		{CCHR('?'),	CCHR('?'),	direddl,	NULL},
	}
};

#endif


/* Tag:bufedmaps, Tag:bufmenumaps */

static	PF	bufedctrlm[] = {
	bufed_gotobuf,	/* ^M */
	rescan,		/* ^N */
	bufed_gotobuf_switch_other, /* ^O */
};

static	PF	bufed1[] = {
	bufed_gotobuf_one,	/* 1 */
	bufed_gotobuf_two,	/* 2 */
};

static	PF	bufedquest[] = {
	wallchart,		/* ? */
};

static	PF	bufede[] = {
	bufed_gotobuf,		/* e */
	bufed_gotobuf,		/* f */
	rescan,			/* g */
	wallchart,		/* h */
};

static	PF	bufedo[] = {
	bufed_gotobuf_other,	/* o */
	rescan,			/* p */
	quitwind,		/* q */
};

static	struct	KEYMAPE(5) bufedmap = {
	5,
	5,
	rescan,
	{
		{CCHR('M'),	CCHR('O'),	bufedctrlm,	NULL},
		{'1', 		'2',		bufed1,		NULL},
		{'?', 		'?',		bufedquest,	NULL},
		{'e', 		'h',		bufede,		NULL},
		{'o', 		'q',		bufedo,		NULL},
	}
};


/* Tag:helpbufmaps */

static	PF	helpbufquest[] = {
	wallchart,		/* ? */
};

static	PF	helpbufh[] = {
	wallchart,		/* h */
};

static	PF	helpbufq[] = {
	quitwind,		/* q */
};

static	struct	KEYMAPE(3) helpbufmap = {
	3,
	3,
	rescan,
	{
		{'?', 		'?',		helpbufquest,	NULL},
		{'h', 		'h',		helpbufh,	NULL},
		{'q', 		'q',		helpbufq,	NULL},
	}
};

#ifdef LANGMODE_C

/* Keymaps */

static PF cmode_preproc[] = {
	cc_preproc,	/* # */
};

static PF cmode_brace[] = {
	cc_brace,	/* } */
};

static PF cmode_cCP[] = {
	rescan,
//	compile,		/* C-c P */
};

static PF cmode_cc[] = {
	prefix,		/* ^C */
//	NULL,		/* ^C */
	rescan,		/* ^D */
	rescan,		/* ^E */
	rescan,		/* ^F */
	rescan,		/* ^G */
	rescan,		/* ^H */
	cc_tab,		/* ^I */
	rescan,		/* ^J */
	rescan,		/* ^K */
	rescan,		/* ^L */
	cc_lfindent,	/* ^M */
};

static PF cmode_spec[] = {
	cc_char,	/* : */
};

static struct KEYMAPE (1) cmode_cmap = {
	1,
	1,
	rescan,
	{
		{ 'P', 'P', cmode_cCP, NULL }
	}
};

static struct KEYMAPE (4) cmodemap = {
	4,
	4,
	rescan,
	{
		{ CCHR('C'), CCHR('M'), cmode_cc, (KEYMAP *) &cmode_cmap },
		{ '#', '#', cmode_preproc, NULL },
		{ ':', ':', cmode_spec, NULL },
		{ '}', '}', cmode_brace, NULL }
	}
};

#endif

#ifdef LANGMODE_PYTHON

/*
 * Python mode key maps
 */

static PF pymode_ret[] = {
	py_lfindent,	/* ^M */
};

static struct KEYMAPE (1) pythonmap = {
	1,
	1,
	rescan,
	{
	    { CCHR('M'), CCHR('M'), pymode_ret,  NULL },
	}
};

#endif

#ifdef LANGMODE_CLIKE

/*
 * Clike mode key maps
 */

static struct KEYMAPE (0) clikemap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) stdcmap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) wsmithcmap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) perlmap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) gnucmap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) javamap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) csharpmap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) jsmap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) swiftmap = {
	0,
	0,
	rescan,
	{
	}
};

static struct KEYMAPE (0) gomap = {
	0,
	0,
	rescan,
	{
	}
};

#endif


/*
 * Give names to the maps, for use by help etc. If the map is to be
 * bindable, it must also be listed in the function name table below
 * with the same name. Maps created dynamicly currently don't get
 * added here, thus are unnamed. Modes are just named keymaps with
 * functions to add/subtract them from a buffer's list of modes. If
 * you change a mode name, change it in modes.c also.
 */

static MAPS	map_table_static[] = {					// Tag:maps
	/* fundamental map MUST be first entry */
	{(KEYMAP *)&fundmap,	"fundamental", 0, NULL},
	{(KEYMAP *)&fillmap,	"fill", 0, NULL},
	{(KEYMAP *)&indntmap,	"indent", 0, NULL},
	{(KEYMAP *)&metamap,	"esc prefix", 0, NULL},
	{(KEYMAP *)&cXmap,	"c-x prefix", 0, NULL},
	{(KEYMAP *)&cX4map,	"c-x 4 prefix", 0, NULL},
	{(KEYMAP *)&helpmap,	"help", 0, NULL},
#ifdef DIRED
	{(KEYMAP *)&diredmap,	"dired", 0, NULL},
#endif
	{(KEYMAP *)&bufedmap,	"bufmenu", 0, NULL},
	{(KEYMAP *)&helpbufmap,	"helpbuf", 0, NULL},
#ifdef LANGMODE_C
	{(KEYMAP *)&cmodemap,	"c", 0, NULL},
#endif
#ifdef LANGMODE_PYTHON
	{(KEYMAP *)&pythonmap,  "python", 0, "local-set-tabs 4 1 1"},
#endif
#ifdef LANGMODE_CLIKE
	{(KEYMAP *)&clikemap,	"clike", 0,	NULL},
	{(KEYMAP *)&stdcmap,	"stdc",	0,	"lv clike-lang 1"},
	{(KEYMAP *)&wsmithcmap,	"whitesmithc", 0, "lv clike-lang 1; lv clike-options 8"},
	{(KEYMAP *)&gnucmap,	"gnuc",	0,	"lv clike-lang 1; lv clike-options 16; local-set-tabs 2 1 1"},
	{(KEYMAP *)&perlmap,	"perl",	0,	"lv clike-lang 2"},
	{(KEYMAP *)&javamap,	"java",	0,	"lv clike-lang 3; local-set-tabs 4 1 1"},
	{(KEYMAP *)&csharpmap,	"c#", 0,	"lv clike-lang 4; local-set-tabs 4 1 1"},
	{(KEYMAP *)&jsmap,	"javascript", 0,  "lv clike-lang 5"},
	{(KEYMAP *)&swiftmap,	"swift", 0,	"lv clike-lang 6; local-set-tabs 4 1 1"},
	{(KEYMAP *)&gomap,	"go", 0,	"lv clike-lang 7"},
#endif /* LANGMODE_CLIKE */
};

INT	nmaps = (sizeof map_table_static/sizeof(MAPS));
#ifdef USER_MODES
static INT nmaps_size = (sizeof map_table_static/sizeof(MAPS));
#endif

MAPS	*map_table = map_table_static;


#ifdef USER_MODES

/*
 * Mg3a: copy a keymap
 */

KEYMAP *
copymap(KEYMAP *map, INT depth)
{
	INT i, j, newnum, oldnum, size;
	KEYMAP *newmap;
	PF *fp, *fpold;

	if (depth++ > MAXKEY) {
		ewprintf("Circular key definition not allowed; not copied.");
		return NULL;
	}

	oldnum = map->map_num;
	newnum = oldnum + MAPGROW;

	if ((newmap = malloc_msg(sizeof(KEYMAP) + (newnum - 1)*sizeof(MAP_ELEMENT))) == NULL) {
		return NULL;
	}

	newmap->map_num = oldnum;
	newmap->map_max = newnum;
	newmap->map_default = map->map_default;

	for (i = 0; i < oldnum; i++) {
		newmap->map_element[i] = map->map_element[i];
		size = map->map_element[i].k_num - map->map_element[i].k_base + 1;
		if ((fp = malloc_msg(size * sizeof(PF))) == NULL) return NULL;
		newmap->map_element[i].k_funcp = fp;
		fpold = map->map_element[i].k_funcp;
		for (j = 0; j < size; j++) {
			fp[j] = fpold[j];
		}
		if (map->map_element[i].k_prefmap) {
			if ((newmap->map_element[i].k_prefmap =
				copymap(map->map_element[i].k_prefmap, depth)) == NULL)
			{
				return NULL;
			}
		}
	}

	return newmap;
}


/*
 * Mg3a: Internal create a mode.
 */

static INT
internal_create_mode(KEYMAP *map, char *name, char *commands)
{
	INT i;
	MAPS *new_map_table;
	char *pname, *pcommands = NULL;

	if ((pname = string_dup(name)) == NULL) {
		free(map);
		return FALSE;
	}

	if (commands[0] && (pcommands = string_dup(commands)) == NULL) {
		free(map);
		free(pname);
		return FALSE;
	}

	if (nmaps >= nmaps_size) {
		if (nmaps > MAXINT/2/(INT)sizeof(MAPS)) {
			free(map);
			free(pname);
			if (pcommands) free(pcommands);
			ewprintf("A ridiculous number of keymaps were attempted");
			return FALSE;
		}

		new_map_table = (MAPS *) malloc_msg(2*nmaps*sizeof(MAPS));

		if (new_map_table == NULL) {
			free(map);
			free(pname);
			if (pcommands) free(pcommands);
			return FALSE;
		}

		for (i = 0; i < nmaps; i++) {
			new_map_table[i] = map_table[i];
		}

		if (map_table != map_table_static) free(map_table);

		map_table = new_map_table;
		nmaps_size = nmaps*2;
	}

	map_table[nmaps].p_map = map;
	map_table[nmaps].p_name = pname;
	map_table[nmaps].p_flags = MODE_NAMEDYN;
	map_table[nmaps].p_commands = pcommands;

	nmaps++;

	ewprintf("Created mode \"%s\"", name);
	return TRUE;
}


/*
 * Mg3a: Create a mode.
 */

INT
create_mode(INT f, INT n)
{
	char name[MODENAMELEN];
	char commands[EXCLEN] = "";
	INT s;
	KEYMAP *map;

	if ((s = ereply("Create Mode: ", name, sizeof(name))) != TRUE) return s;

	if (!inmacro || !outofarguments(0)) {
		if ((s = ereply("Commands: (default none) ", commands, sizeof(commands))) == ABORT) {
			return s;
		}
	}

	if (name_map(name)) {
		ewprintf("Mode %s already exists", name);
		return FALSE;
	}

	if ((map = (KEYMAP *)malloc_msg(sizeof(KEYMAP))) == NULL) return FALSE;

	map->map_num = 0;
	map->map_max = 1;
	map->map_default = rescan;

	return internal_create_mode(map, name, commands);
}


/*
 * Mg3a: Copy a mode
 */

INT
copy_mode(INT f, INT n)
{
	char oldname[MODENAMELEN];
	char name[MODENAMELEN];
	char commands[EXCLEN] = "";
	INT s;
	KEYMAP *oldmap, *map;

	if ((s = eread("Copy Mode: ", oldname, sizeof(oldname), EFKEYMAP)) != TRUE) return s;
	if ((s = ereply("New Mode: ", name, sizeof(name))) != TRUE) return s;

	if (!inmacro || !outofarguments(0)) {
		if ((s = ereply("Commands: (default none) ", commands, sizeof(commands))) == ABORT) {
			return s;
		}
	}

	if ((oldmap = name_map(oldname)) == NULL) {
		ewprintf("Mode %s does not exist", oldname);
		return FALSE;
	}

	if (name_map(name)) {
		ewprintf("Mode %s already exists", name);
		return FALSE;
	}

	if ((map = copymap(oldmap, 0)) == NULL) return FALSE;

	return internal_create_mode(map, name, commands);
}

#endif


/*
 * Mg3a: Run mode init commands
 */

INT
exec_mode_init_commands(MODE mode)
{
	INT s;

	if (!invalid_mode(mode) && map_table[mode].p_commands) {
		errcheck--;
		s = excline_copy_list(map_table[mode].p_commands);
		errcheck++;

		return s;
	}

	return TRUE;
}


#ifdef LOCAL_SET_MODE

/*
 * Mg3a: Check whether the mode is user-manipulable.
 */

static int
checkmap(char *name, char *why)
{
	MODE mode;

	mode = name_mode(name);

	if (invalid_mode(mode)) {
		ewprintf("No such mode: %s", name);
		return 0;
	} else if (mode <= name_mode("helpbuf")) {
		ewprintf("Built-in mode cannot be %s", why);
		return 0;
	} else {
		return 1;
	}
}


/*
 * Mg3a: Set a mode in the current buffer.
 */

INT
local_set_mode(INT f, INT n)
{
	char name[MODENAMELEN];
	INT s;

	if ((s = eread("Local Set Mode: ", name, sizeof(name), EFKEYMAP)) != TRUE) return s;

	if (!checkmap(name, "set")) return FALSE;

	return setbufmode(curbp, name);
}


/*
 * Mg3a: Clear a mode from the current buffer.
 */

INT
local_unset_mode(INT f, INT n)
{
	char name[MODENAMELEN];
	INT s;

	if ((s = eread("Local Unset Mode: ", name, sizeof(name), EFKEYMAP)) != TRUE) return s;

	if (!checkmap(name, "unset")) return FALSE;

	return clearbufmode(curbp, name);
}


/*
 * Mg3a: Toggle a mode in the current buffer.
 */

INT
local_toggle_mode(INT f, INT n)
{
	char name[MODENAMELEN];
	INT s;

	if (f & FFARG) return (n > 0) ? local_set_mode(f, n) : local_unset_mode(f, n);

	if ((s = eread("Local Toggle Mode: ", name, sizeof(name), EFKEYMAP)) != TRUE) return s;

	if (!checkmap(name, "toggled")) return FALSE;

	return changemode(curbp, f, n, name);
}

#endif


#ifdef USER_MODES

/*
 * Mg3a: Clean the mode from a mode array, and renumber modes if
 * needed.
 */

static void
remove_mode_from_array(MODE a[], INT *arraylen, MODE remove)
{
	INT i, len = *arraylen;

	for (i = 1; i <= len; i++) {
		if (a[i] == remove) {
			for (; i < len; i++) {
				a[i] = a[i + 1];
			}

			len--;
			*arraylen = len;
			break;
		}
	}

	for (i = 1; i <= len; i++) {
		if (a[i] > remove) a[i]--;
	}
}


/*
 * Mg3a: Delete a mode. There so we can replace builtin language
 * modes.
 */

INT
delete_mode(INT f, INT n)
{
	char name[MODENAMELEN];
	INT s;
	MODE i, mode;
	BUFFER *bp;

	if ((s = eread("Delete Mode: ", name, sizeof(name), EFKEYMAP)) != TRUE) return s;

	if (!checkmap(name, "deleted")) return FALSE;

	if (invalid_mode((mode = name_mode(name)))) return FALSE;

	for (bp = bheadp; bp; bp = bp->b_bufp) {
		remove_mode_from_array(bp->b_modes, &bp->b_nmodes, mode);
		upmodes(bp);
	}

	remove_mode_from_array(defb_modes, &defb_nmodes, mode);

	for (i = mode; i < nmaps - 1; i++) {
		map_table[i] = map_table[i + 1];
	}

	nmaps--;

	ewprintf("Deleted mode \"%s\"", name);

	return TRUE;
}


/*
 * Rename a mode. Easy to do and for completeness.
 */

INT
rename_mode(INT f, INT n)
{
	char name[MODENAMELEN], newname[MODENAMELEN];
	INT s;
	char *p;
	MODE mode;
	MAPS *mp;

	if ((s = eread("Rename Mode: ", name, sizeof(name), EFKEYMAP)) != TRUE) return s;

	if (!checkmap(name, "renamed")) return FALSE;

	if ((s = ereply("New name: ", newname, sizeof(newname))) != TRUE) return s;

	if (name_map(newname)) {
		ewprintf("Mode %s already exists", newname);
		return FALSE;
	}

	mode = name_mode(name);
	if (invalid_mode(mode)) return FALSE;

	mp = &map_table[mode];

	if ((p = string_dup(newname)) == NULL) return FALSE;

	if (mp->p_flags & MODE_NAMEDYN) free(mp->p_name);
	mp->p_flags |= MODE_NAMEDYN;
	mp->p_name = p;

	upmodes(NULL);
	ewprintf("Mode \"%s\" renamed to \"%s\"", name, newname);
	return TRUE;
}

#endif


#ifdef LOCAL_SET_MODE

/*
 * Mg3a: List the modes and their initialization commands.
 */

INT
list_modes(INT f, INT n)
{
#define MODENAMEWIDTH 31
	BUFFER *bp;
	char line[MODENAMELEN + 32 + EXCLEN];
	MAPS *p;
	INT len;

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	snprintf(line, sizeof(line), " %-*s %s", MODENAMEWIDTH, "Mode name", "Commands");
	if (addline(bp, line) == FALSE) return FALSE;
	snprintf(line, sizeof(line), " %-*s %s", MODENAMEWIDTH, "---------", "--------");
	if (addline(bp, line) == FALSE) return FALSE;

	for (p = map_table; p < &map_table[nmaps]; p++) {
		if (p->p_commands) {
			strcpy(line, " ");
			len = concat_limited(line, p->p_name, MODENAMEWIDTH, sizeof(line));
			snprintf(line + len, sizeof(line) - len, " \"%s\"", p->p_commands);
		} else {
			snprintf(line, sizeof(line), " %s", p->p_name);
		}
		if (addline(bp, line) == FALSE) return FALSE;
	}

	return popbuftop(bp);
}


#endif

char *
map_name(KEYMAP *map)
{
	MAPS *mp = &map_table[0];

	do {
	    if(mp->p_map == map) return mp->p_name;
	} while(++mp < &map_table[nmaps]);
	return NULL;
}


MODE
name_mode(char *name)
{
	MODE i;

	for (i = 0; i < nmaps; i++) {
		if (strcmp(map_table[i].p_name, name) == 0) return i;
	}

	return -1;
}


KEYMAP *
name_map(char *name)
{
	MODE i;

	for (i = 0; i < nmaps; i++) {
		if (strcmp(map_table[i].p_name, name) == 0) return map_table[i].p_map;
	}

	return NULL;
}

KEYMAP *
mode_map(MODE mode)
{
	if (mode < 0 || mode >= nmaps) return NULL;
	return map_table[mode].p_map;
}

char *
mode_name(MODE mode)
{
	if (mode < 0 || mode >= nmaps) return NULL;
	return map_table[mode].p_name;
}

char *
mode_name_check(MODE mode)
{
	char *cp = mode_name(mode);

	if (cp) return cp;
	return "INVALID_MODE";
}

int
invalid_mode(MODE mode)
{
	return mode < 0 || mode >= nmaps;
}


char *
getnext_mapname(INT first)
{
	static MAPS *p = NULL;

	if (first) p = &map_table[0];
	else if (p >= &map_table[nmaps-1]) return NULL;
	else p++;

	return p->p_name;
}


#ifndef LOCAL_SET_MODE

/*
 * Mg3a: If we don't have user modes, we call it "list-keymaps" and
 * it's simpler.
 */

INT
listkeymaps(INT f, INT n)
{
	BUFFER *bp;
	char line[MODENAMELEN + 80];
	MAPS *p;

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	if (addline(bp, "List of keymaps:") == FALSE) return FALSE;
	if (addline(bp, "") == FALSE) return FALSE;

	for (p = map_table; p < &map_table[nmaps]; p++) {
		snprintf(line, sizeof(line), " %s", p->p_name);
		if (addline(bp, line) == FALSE) return FALSE;
	}

	return popbuftop(bp);
}

#endif


/*
 * A dummy function for compatibility.
 */

INT
truefunc(INT f, INT n)
{
	return TRUE;
}


/*
 * Warning: functnames MUST be in alphabetical order! (due to binary
 * search in name_function.) If the function is prefix, it must be
 * listed with the same name in the map_table above.
 */

/* Tag:commandlist */

const FUNCTNAMES functnames[] = {
	{cmdiso,	"8bit", 0},
	{bufed_gotobuf_one,	"Buffer-menu-1-window", 0},
	{bufed_gotobuf_two,	"Buffer-menu-2-window", 0},
	{bufed_gotobuf_other,	"Buffer-menu-other-window", 0},
	{bufed_gotobuf_switch_other,	"Buffer-menu-switch-other-window", 0},
	{bufed_gotobuf,	"Buffer-menu-this-window", 0},
	{appendnextkill,"append-next-kill", 0},
	{apropos_command, "apropos", 0},
	{auto_execute,	"auto-execute", 0},
	{auto_execute_list,	"auto-execute-list", 0},
	{fillmode,	"auto-fill-mode", 0},
	{indentmode,	"auto-indent-mode", 0},
	{backtoindent,	"back-to-indentation", 0},
	{backchar,	"backward-char", 0},
	{backdeluntab,	"backward-delete-char-untabify", 0},
	{delbword,	"backward-kill-word", 0},
	{gotobop,	"backward-paragraph", 0},
	{backword,	"backward-word", 0},
	{balancewind,	"balance-windows", 0},
	{gotobob,	"beginning-of-buffer", 0},
	{gotobol,	"beginning-of-line", 0},
	{beginvisualline, "beginning-of-visual-line", 0},
	{showmatch,	"blink-and-insert", 0},
	{truefunc,	"blink-matching-paren", FUNC_HIDDEN},
	{showmatch,	"blink-matching-paren-hack", FUNC_ALIAS|FUNC_HIDDEN},
	{bom,		"bom", 0},
#ifdef	BSMAP
	{bsmap,		"bsmap-mode", 0},
#endif
	{bufed,		"buffer-menu", 0},
	{bufed_otherwindow, "buffer-menu-other-window", 0},
#ifdef LANGMODE_C
	{cc_brace,	"c-handle-special-brace", 0},
	{cc_char,	"c-handle-special-char", 0},
	{cc_indent,	"c-indent", 0},
	{cc_lfindent,	"c-indent-and-newline", 0},
	{cmode, 	"c-mode", 0},
  	{cc_preproc,	"c-preproc", 0},
	{cc_tab,	"c-tab-or-indent", 0},
#endif
//	{prefix,	"c-x 4 prefix", 0},
//	{prefix,	"c-x prefix", 0},
	{call_last_kbd_macro,	"call-last-kbd-macro", 0},
	{capword,	"capitalize-word", 0},
	{case_fold_mode,"case-fold-mode", 0},
	{changedir,	"cd", 0},
#ifdef CHARSDEBUG
	{charsdebug_set,	"charsdebug", 0},
	{charsdebug_zero,	"charsdebug-zero", 0},
#endif
#ifdef LANGMODE_CLIKE
	{clike_align_after,	"clike-align-after", 0},
	{clike_align_after_paren,	"clike-align-after-paren", 0},
	{clike_align_back,	"clike-align-back", 0},
	{clike_showmatch,	"clike-blink-and-insert", 0},
	{clike_indent,	"clike-indent", 0},
	{clike_indent_next,	"clike-indent-next", 0},
	{clike_indent_region,	"clike-indent-region", 0},
	{clike_insert,	"clike-insert", 0},
	{clike_insert_special,	"clike-insert-special", 0},
	{clike_mode,	"clike-mode", 0},
	{clike_newline_and_indent,	"clike-newline-and-indent", 0},
	{clike_tab_or_indent,		"clike-tab-or-indent", 0},
#endif
#ifdef USER_MODES
	{copy_mode,	"copy-mode", 0},
#endif
	{copyregion,	"copy-region-as-kill", 0},
#ifdef USER_MACROS
	{create_macro,	"create-macro", 0},
#endif
#ifdef USER_MODES
	{create_mode,	"create-mode", 0},
#endif
	{crlf,		"crlf", 0},
	{define_key,	"define-key", 0},
	{backdel,	"delete-backward-char", 0},
	{deblank,	"delete-blank-lines", 0},
	{forwdel,	"delete-char", 0},
	{delwhite,	"delete-horizontal-space", 0},
//	{delete_macro,	"delete-macro", 0},
#ifdef USER_MODES
	{delete_mode,	"delete-mode", 0},
#endif
	{onlywind,	"delete-other-windows", 0},
	{deleteregion,	"delete-region", 0},
	{deletetrailing,"delete-trailing-whitespace", 0},
	{delwind,	"delete-window", 0},
	{wallchart,	"describe-bindings", 0},
	{desckey,	"describe-key-briefly", 0},
	{digit_argument,"digit-argument", 0},
#ifdef DIRED
	{dired,		"dired", 0},
	{d_copy,	"dired-copy-file", 0},
	{d_createdir,	"dired-create-directory", 0},
	{d_expunge,	"dired-do-deletions", 0},
	{d_findfile,	"dired-find-file", 0},
	{d_ffonewindow,	"dired-find-file-one-window", 0},
	{d_ffotherwindow, "dired-find-file-other-window", 0},
	{d_del,		"dired-flag-file-deleted", 0},
	{d_jump,	"dired-jump", 0},
	{d_jump_otherwindow, "dired-jump-other-window", 0},
	{d_nextdir,	"dired-next-dirline", 0},
	{d_otherwindow, "dired-other-window", 0},
	{d_prevdir,	"dired-prev-dirline", 0},
	{d_rename,	"dired-rename-file", 0},
	{d_undel,	"dired-unflag", 0},
	{d_undelbak,	"dired-unflag-backward", 0},
	{d_updir,	"dired-up-directory", 0},
#endif
	{cmddos,	"dos", 0},
	{lowerregion,	"downcase-region", 0},
	{lowerword,	"downcase-word", 0},
	{lduplicate, "duplicate-line", 0},
	{showversion,	"emacs-version", 0},
	{end_kbd_macro,	"end-kbd-macro", 0},
	{gotoeob,	"end-of-buffer", 0},
	{gotoeol,	"end-of-line", 0},
	{endvisualline,	"end-of-visual-line", 0},
	{enlargewind,	"enlarge-window", 0},
//	{prefix,	"esc prefix", 0},
	{evalbuffer,	"eval-buffer", 0},
	{evalbuffer,	"eval-current-buffer", FUNC_ALIAS},
	{evalexpr,	"eval-expression", 0},
	{swapmark,	"exchange-point-and-mark", 0},
	{extend,	"execute-extended-command", 0},
	{explode,	"explode-character", 0},
	{fillpara,	"fill-paragraph", 0},
	{filevisit,	"find-file", 0},
	{poptofile,	"find-file-other-window", 0},
	{filevisit_readonly,	"find-file-read-only", 0},
	{poptofile_readonly,	"find-file-read-only-other-window", 0},
	{forwchar,	"forward-char", 0},
	{gotoeop,	"forward-paragraph", 0},
	{forwword,	"forward-word", 0},
	{bindtokey,	"global-set-key", 0},
	{unbindtokey,	"global-unset-key", 0},
	{gotoline,	"goto-line", 0},
//	{prefix,	"help", 0},
	{help_help,	"help-help", 0},
#ifdef HELP_RET
	{help_ret,	"help-ret", 0},
#endif
	{ifcmd,		"if", 0},
	{ignore_errors,	"ignore-errors", 0},
	{implode,	"implode-character", 0},
#ifdef INDENT_RIGIDLY
	{indent_rigidly, "indent-rigidly", 0},
#endif
	{insert,	"insert", 0},
	{insert_8bit,	"insert-8bit", 0},
	{insert_8bit_hex, "insert-8bit-hex", 0},
	{bufferinsert,	"insert-buffer", 0},
	{ucs_insert,	"insert-char", FUNC_ALIAS},
	{fileinsert,	"insert-file", 0},
	{insert_tab,	"insert-tab", 0},
	{insert_tab_8,	"insert-tab-8", 0},
	{insert_unicode,"insert-unicode", 0},
	{insert_unicode_hex, "insert-unicode-hex", 0},
	{fillword,	"insert-with-wrap", 0},
	{backisearch,	"isearch-backward", 0},
	{forwisearch,	"isearch-forward", 0},
	{joinline,	"join-line", 0},
	{joinline_forward,	"join-line-forward", 0},	// Mg3a extension
	{justone,	"just-one-space", 0},
	{keyboard_quit,		"keyboard-quit", 0},
	{killbuffer,	"kill-buffer", 0},
	{killbuffer_andwindow, "kill-buffer-and-window", 0},
	{killbuffer_quickly, "kill-buffer-quickly", 0},
	{hardquit,	"kill-emacs", 0},
	{killline,	"kill-line", 0},
	{killpara,	"kill-paragraph", 0},
	{killregion,	"kill-region", 0},
	{killthisbuffer, "kill-this-buffer", 0},
	{killwholeline,	"kill-whole-line", 0},
	{delfword,	"kill-word", 0},
	{unixlf,	"lf", 0},
#if 0							/* Not yet ready */
	{lmovedown, "line-move-down", 0},
	{lmoveup,   "line-move-up", 0},
#endif
	{listbuffers,	"list-buffers", 0},
	{listcharsets,	"list-charsets", 0},
	{list_kbd_macro, "list-kbd-macro", 0},
#ifdef LOCAL_SET_MODE
	{list_modes,	"list-keymaps", FUNC_ALIAS},
#else
	{listkeymaps,	"list-keymaps", 0},
#endif
	{list_kbd_macro, "list-macro", FUNC_ALIAS|FUNC_HIDDEN},
#ifdef USER_MACROS
	{list_macros,	"list-macros", 0},
#endif
#ifdef LOCAL_SET_MODE
	{list_modes,	"list-modes", 0},
#endif
	{listpatterns,	"list-patterns", 0},
#ifdef LIST_UNDO
	{list_undo,	"list-undo", 0},
#endif
#ifdef UCSNAMES
	{list_unicode,	"list-unicode", 0},
#endif
	{listvars, 	"list-variables", 0},
	{evalfile,	"load", 0},
	{localmodename,	"local-mode-name", 0},
	{local_set_charset, "local-set-charset", 0},
	{localbind,	"local-set-key", 0},
#ifdef LOCAL_SET_MODE
	{local_set_mode, "local-set-mode", 0},
#endif
	{localsettabs,	"local-set-tabs", 0},
	{localsetvar,	"local-set-variable", 0},
#ifdef LOCAL_SET_MODE
	{local_toggle_mode, "local-toggle-mode", 0},
#endif
	{localunbind,	"local-unset-key", 0},
#ifdef LOCAL_SET_MODE
	{local_unset_mode, "local-unset-mode", 0},
#endif
	{localunsetvar,	"local-unset-variable", 0},
	{localsetvar,	"lv", FUNC_ALIAS},
	{makebkfile,	"make-backup-files", 0},
	{markpara,	"mark-paragraph", 0},
	{markwholebuffer, "mark-whole-buffer", 0},
	{message,	"message", 0},
	{movetoline,	"move-to-window-line", 0},
	{movetoline_topbottom, "move-to-window-line-top-bottom", 0},
	{negative_argument, "negative-argument", 0},
	{newline,	"newline", 0},
	{indent,	"newline-and-indent", 0},
	{indentsame,	"newline-and-indent-same", 0},
	{newlineclassic, "newline-classic", 0},
	{nextbuffer,	"next-buffer", 0},
	{forwline,	"next-line", 0},
	{nil,		"nil", 0},
	{no_break,	"no-break", 0},
	{notabmode,	"no-tab-mode", 0},
	{nobom,		"nobom", 0},
	{notmodified,	"not-modified", 0},
	{openline,	"open-line", 0},
	{nextwind,	"other-window", 0},
	{overwrite,	"overwrite-mode", 0},
#ifdef PREFIXREGION
	{prefixregion,	"prefix-region", 0},
#endif
	{prevbuffer,	"previous-buffer", 0},
	{backline,	"previous-line", 0},
	{prevwind,	"previous-window", 0},
	{showcwdir,	"pwd", 0},
#ifdef LANGMODE_PYTHON
	{py_indent,     "python-indent", 0},
	{pymode,	"python-mode", 0},
	{py_lfindent,   "python-newline-and-indent", 0},
#endif
	{queryrepl,	"query-replace", 0},
	{quitwind,	"quit-window", 0},
	{quote,		"quoted-insert", 0},
	{recenter,	"recenter", 0},
	{recentertopbottom,	"recenter-top-bottom", 0},
	{redo,		"redo", 0},
	{refresh,	"redraw-display", 0},
#ifdef USER_MODES
	{rename_mode,	"rename-mode", 0},
#endif
	{filerevert,	"revert-buffer", 0},			/* Gnu Emacs compatible */
	{filerevert_forget,	"revert-buffer-forget", 0},	/* Better revert	*/
	{filerevert,	"revert-file", FUNC_ALIAS|FUNC_HIDDEN},
	{save_and_exit,	"save-and-exit", 0},	/* Not in GNU Emacs */
	{filesave,	"save-buffer", 0},
	{quit,		"save-buffers-kill-emacs", 0},
	{savebuffers,	"save-some-buffers", 0},
	{backpage,	"scroll-down", 0},
	{back1page,	"scroll-one-line-down", 0},
	{forw1page,	"scroll-one-line-up", 0},
	{pagenext,	"scroll-other-window", 0},
	{pagenext_backward, "scroll-other-window-down", 0},
	{forwpage,	"scroll-up", 0},
	{searchagain,	"search-again", 0},
#ifdef SEARCHALL
	{backsearchall,	"search-all-backward", 0},
	{forwsearchall,	"search-all-forward", 0},
#ifdef SEARCHSIMPLE
	{searchallsimple, "search-all-simple", 0},
#endif
#endif
	{backsearch,	"search-backward", 0},
	{forwsearch,	"search-forward", 0},
#ifdef SEARCHSIMPLE
	{searchsimple,	"search-simple", 0},
#endif
	{selfinsert,	"self-insert-command", 0},
	{set_dos_charset,"set-alternate-charset", 0},
	{set_default_8bit_charset, "set-default-8bit-charset", 0},
	{set_default_charset, "set-default-charset", 0},
	{set_default_mode, "set-default-mode", 0},
	{setfillcol,	"set-fill-column", 0},
	{setmark,	"set-mark-command", 0},
#ifdef PREFIXREGION
	{setprefix,	"set-prefix-string", 0},
#endif
	{setshell,	"set-shell", 0},
#ifdef UCSNAMES
	{set_unicode_data, "set-unicode-data", 0},
#endif
	{setvar,	"set-variable", 0},
	{shebang,	"shebang", 0},
	{shellcommand,	"shell-command", 0},
	{showbytes,	"show-bytes", 0},
	{shrinkwind,	"shrink-window", 0},
	{shrinkwind_iflarge,	"shrink-window-if-larger-than-buffer", 0},
#ifdef SLOW
	{slowmode,	"slow-mode", 0},
#endif
	{splitwind,	"split-window-vertically", 0},
	{start_kbd_macro,	"start-kbd-macro", 0},
	{spawncli,	"suspend-emacs", 0},
	{setvar,	"sv", FUNC_ALIAS},
	{usebuffer,	"switch-to-buffer", 0},
	{poptobuffer,	"switch-to-buffer-other-window", 0},
	{tabregionleft,	"tab-region-left", 0},
	{tabregionright,"tab-region-right", 0},
#if TESTCMD
	{testcmd,	"test", 0},
#endif
	{toggle_readonly, "toggle-read-only", 0},
	{twiddle,	"transpose-chars", 0},
	{twiddlepara,	"transpose-paragraphs", 0},
	{twiddleword,	"transpose-words", 0},
	{ubom,		"ubom", 0},
	{ucs_insert,	"ucs-insert", 0},
	{undo,		"undo", 0},
	{undo_boundary,	"undo-boundary", 0},
	{undo_only,	"undo-only", 0},
	{universal_argument, "universal-argument", 0},
	{upperregion,	"upcase-region", 0},
	{upperword,	"upcase-word", 0},
#ifdef UPDATE_DEBUG
	{do_update_debug, "update-debug", 0},
#endif
	{cmdutf8,	"utf8", 0},
	{showcpos,	"what-cursor-position", 0},
	{with_key,	"with-key", 0},
	{with_message,	"with-message", 0},
	{word_search_mode, "word-search-mode", 0},
	{filewrite,	"write-file", 0},
	{yank,		"yank", 0},
	{yank_process,	"yank-process", 0},
};

#define NFUNCT	((INT)(sizeof(functnames)/sizeof(FUNCTNAMES)))

const INT nfunct = NFUNCT;		/* used by help.c */


/*
 * The general-purpose version of ROUND2 blows osk C (2.0) out of the
 * water. (reboot required) If you need to build a version of mg with
 * less than 32 or more than 511 functions, something better must be
 * done. The version that should work, but doesn't is:
 * #define ROUND2(x) (1+((x>>1)|(x>>2)|(x>>3)|(x>>4)|(x>>5)|(x>>6)|(x>>7)|\
 *	(x>>8)|(x>>9)|(x>>10)|(x>>11)|(x>>12)|(x>>13)|(x>>14)|(x>>15)))
 */

#define ROUND2(x) (x<128?(x<64?32:64):(x<256?128:256))

static	INT
name_fent(char *fname, INT flag)
{
	INT	try;
	INT	x = ROUND2(NFUNCT);
	INT	base = 0;
	INT	notit;

	do {
	    /* + can be used instead of | here if more efficent.	*/
	    if ((try = base | x) < NFUNCT) {
		if ((notit = strcmp(fname, functnames[try].n_name)) >= 0) {
		    if (!notit) return try;
		    base = try;
		}
	    }
	} while ((x>>=1) || (try==1 && base==0));    /* try 0 once if needed */
	return flag ? base : -1;
}


/*
 * Translate from function name to function pointer, using binary
 * search.
 */

PF
name_function(char *fname)
{
	INT i;
	if ((i = name_fent(fname, FALSE)) >= 0) return functnames[i].n_funct;
	return NULL;
}


/* translate from function pointer to function name. */

char *
function_name(PF fpoint)
{
	FUNCTNAMES	*fnp = &functnames[0];

	if (fpoint == prefix) return NULL;		/* ambiguous */
	do {
	    if (fnp->n_funct == fpoint && !(fnp->n_flag & FUNC_ALIAS)) return fnp->n_name;
	} while (++fnp < &functnames[NFUNCT]);
#ifdef USER_MACROS
	return key_function_to_name_or_macro(fpoint);
#else
	return NULL;
#endif
}


#ifdef LANGMODE_CLIKE

static void
clike_basic(MODE mode, int lbrace)
{
	internal_bind(mode, "\r",	clike_newline_and_indent);
	if (lbrace) {
		internal_bind(mode, "{",	clike_insert);
	}
	internal_bind(mode, "}",	clike_insert);
	internal_bind(mode, "\033(",	clike_align_after_paren);
	internal_bind(mode, "\033=",	clike_align_after);		// Tag:bind
	internal_bind(mode, "\033)",	clike_align_back);
	internal_bind(mode, "\033\t",	clike_indent);
	internal_bind(mode, "\033\034",	clike_indent_region);
	internal_bind(mode, ")",	clike_showmatch);
}

static void
clike_c_common(MODE mode, int hash)
{
	clike_basic(mode, 1);

	internal_bind(mode, ":",	clike_insert_special);

	if (hash) {
		internal_bind(mode, "#",	clike_insert);
	}
}

static void
swiftgobind(MODE mode)
{
	clike_basic(mode, 1);

	internal_bind(mode, ":",	clike_insert_special);
	internal_bind(mode, "]",	clike_insert);
}

#endif

/*
 * Mg3a: for stuff that doesn't depend on "term.h".
 */

static void
commonbind()
{
	const MODE fundamental = 0;
	MODE mode;

	mode = fundamental;		// Shut up compiler complaint
	internal_bind(mode, "\033gg", 		gotoline);
	internal_bind(mode, "\033g\033g",	gotoline);
	internal_bind(mode, "\030" "8\r",	ucs_insert);

#ifdef HELP_RET
	internal_bind(name_mode("helpbuf"), "\r",	help_ret);
#endif

#ifdef LANGMODE_PYTHON
	internal_bind(name_mode("python"), "\033\t",	py_indent);
#endif

#ifdef LANGMODE_CLIKE
	clike_basic(	name_mode("clike"), 1);
	clike_basic(	name_mode("perl"), 0);

	clike_c_common(	name_mode("stdc"), 1);
	clike_c_common(	name_mode("whitesmithc"), 1);
	clike_c_common(	name_mode("gnuc"), 1);
	clike_c_common(	name_mode("java"), 0);
	clike_c_common(	name_mode("c#"), 1);
	clike_c_common(	name_mode("javascript"), 0);

	swiftgobind(	name_mode("swift"));
	swiftgobind(	name_mode("go"));
#endif /* LANGMODE_CLIKE */
}


#ifdef NO_TERMH

void
do_internal_bindings()
{
	commonbind();
}

#else

#ifdef __CYGWIN__
#include <ncurses/term.h>
#else
#include <term.h>
#endif

void
do_internal_bindings()
{
	MODE fund, esc, ctrlx;

	fund = 0;
	esc = name_mode("esc prefix");
	if (invalid_mode(esc)) panic("esc prefix", 0);
	ctrlx = name_mode("c-x prefix");
	if (invalid_mode(ctrlx)) panic("c-x prefix", 0);

	internal_bind(fund,  key_up,	backline);
	internal_bind(fund,  key_down,	forwline);
	internal_bind(fund,  key_left,	backchar);
	internal_bind(fund,  key_right,	forwchar);
	internal_bind(fund,  key_beg,	gotobol);
	internal_bind(fund,  key_home,	gotobol);
	internal_bind(fund,  key_end,	gotoeol);
	internal_bind(fund,  key_dc,	forwdel);
	internal_bind(fund,  key_npage,	forwpage);
	internal_bind(fund,  key_ppage,	backpage);

	internal_bind(esc,   key_left,	backword);
	internal_bind(esc,   key_right,	forwword);
	internal_bind(esc,   key_beg,	gotobob);
	internal_bind(esc,   key_home,	gotobob);
	internal_bind(esc,   key_end,	gotoeob);
	internal_bind(esc,   key_dc,	delfword);
	internal_bind(esc,   key_npage,	pagenext);
	internal_bind(esc,   key_ppage,	pagenext_backward);

	internal_bind(ctrlx, key_left,	prevbuffer);
	internal_bind(ctrlx, key_right,	nextbuffer);

	internal_bind(fund,  key_f1,	help_help);

#if 0
	internal_bind(fund,  key_f3,	searchagain);
	internal_bind(fund,  key_f5,	filerevert);
	internal_bind(fund,  key_f6,	nextwind);
	internal_bind(esc,   key_f6,	prevwind);
	internal_bind(fund,  key_f10,	extend);
#endif

	commonbind();
}
#endif
