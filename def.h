/* integrate the auto tools */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* ALL turns on all options */
#ifdef ALL
#define DIRED
#define PREFIXREGION
#define CHARSDEBUG
#define SLOW
#define SEARCHALL
#define LANGMODE_C
#define LANGMODE_PYTHON
#define LANGMODE_MAKE
#define LANGMODE_CLIKE
#define UCSNAMES
#define USER_MODES
#define USER_MACROS
#define PIPEIN
#endif

#ifdef NO_CMODE
# ifdef LANGMODE_C
#  undef LANGMODE_C
# endif
#endif

#ifdef NO_UCSN
# ifdef UCSNAMES
#  undef UCSNAMES
# endif
#endif

#ifdef NO_PIPEIN
# ifdef PIPEIN
#  undef PIPEIN
# endif
#endif


/* Obligatory definitions */

#undef NO_MACRO

#undef UTF8
#define UTF8
#undef NO_DPROMPT
#define NO_DPROMPT	/* Never worked anyway */
#undef INOUTOPT
#define INOUTOPT

/* Override-able definitions */

#ifndef BSMAP
#define BSMAP 0		/* Compile in BSMAP, but do not turn on by default */
#endif

#ifndef MAKEBACKUP
# define MAKEBACKUP 0	/* Default the backups to not on */
#endif

#ifdef MINIDIRED
#undef DIRED
#define DIRED
#endif

#ifdef __CYGWIN__
#undef AUTOMODE
#define AUTOMODE
#endif

#ifndef LF_DEFAULT
#define LF_DEFAULT 1
#endif

#ifndef TESTCMD
#define TESTCMD 0	/* Test instrumentation */
#endif

#undef HELP_RET		/* Used to be conditional */
#ifdef UCSNAMES
#define HELP_RET
#endif

#if defined(LANGMODE_CLIKE) || defined(LANGMODE_PYTHON) || defined(LANGMODE_MAKE) || defined(USER_MODES)
#define LOCAL_SET_MODE
#else
#undef LOCAL_SET_MODE
#endif

/*
 * This file is the general header file for all parts of the
 * MicroEMACS display editor. It contains all of the general
 * definitions and macros. It also contains some conditional
 * compilation flags. All of the per-system and per-terminal
 * definitions are in special header files. The most common reason to
 * edit this file would be to zap the definition of CVMVAS or BACKUP.
 */

#include	"sysdef.h"		/* Order is critical.		*/
#include	"ttydef.h"
#include	"chrdef.h"

#include 	"cpextern.h"


/* The default integer size for the editor */

#ifndef NEWSIZE
#define NEWSIZE 1
#endif

#if NEWSIZE
#define INT	long
#define UINT	unsigned long
#define INTFMT 	"%ld"
#define MAXINT	LONG_MAX
#define MININT	LONG_MIN
#define MAXUINT	ULONG_MAX
#else
#define INT	int
#define UINT	unsigned int
#define INTFMT 	"%d"
#define MAXINT	INT_MAX
#define MININT	INT_MIN
#define MAXUINT	UINT_MAX
#endif

typedef INT	(*PF)(INT f, INT n);	/* generally useful type	*/
typedef INT	MODE;			/* Type of a mode		*/


/*
 * Table sizes, etc.
 */

#define NFILEN	PATH_MAX		/* Length, file or buffer name.	*/
#define NLINE	1024			/* Length, line.		*/
#define PBMODES 8			/* modes per buffer		*/
#define NKBDM	256			/* Length, keyboard macro.	*/
#define NPAT	160			/* Length, pattern.		*/
#define HUGE	1000000			/* A rather large number.	*/
#define NSRCH	128			/* Undoable search commands.	*/
#define NXNAME	128			/* Length, extended command.	*/
#define MAXSTRKEY 8			/* Max length of the string	*/
					/* display of a key: U+xxxxxx.	*/
#define MAXLOCALVAR 10			/* Max number of local variables */
#define MAXLOCALSTRING 2		/* Max number of local string variables */
#define LOCALMODELEN 256		/* Size of local mode name	*/
#define VARLEN	32			/* Length of a variable name	*/
#define CHARSETLEN 20			/* Length of a charset name	*/
#define UNIDATALEN 512			/* Line in Unicode database	*/
#define MODENAMELEN NXNAME		/* Size of a mode name		*/
#define EXCLEN	(2*NFILEN)		/* Size of execution buffer	*/
#define TRUNCNAME 24			/* Width of truncated content in prompt */

/*
 * Universal.
 */

#define FALSE		0		/* False, no, bad, etc.		*/
#define TRUE		1		/* True, yes, good, etc.	*/
#define ABORT		2		/* Death, ^G, abort, etc.	*/
#define SILENTERR	3		/* Silent error			*/

#define KPROMPT		2		/* keyboard prompt		*/


/*
 * These flag bits keep track of some aspects of the last command. The
 * CFCPCN flag controls goal column setting. The CFKILL flag controls
 * the clearing versus appending of data in the kill buffer.
 */

#define CFCPCN		0x0001		/* Last command was C-P, C-N	*/
#define CFKILL		0x0002		/* Last command was a kill	*/
#define CFINS		0x0004		/* Last command was self-insert */
#define CFCL1		0x0008		/* Last command was C-L		*/
#define	CFCL2		0x0010		/* Last two commands were C-L C-L */
#define	CFCL3		0x0020		/* Last three commands were C-L C-L C-L*/	// Tag:CF
#define CFMR1		0x0040		/* Last command was M-R		*/
#define CFMR2		0x0080		/* Last two commands were M-R M-R */
#define CFKWL		0x0100		/* Last command is kill-whole-line */
#define CFNOQUIT	0x0200		/* Inhibit "Quit" message	*/
#define CFUNDO		0x0400		/* Last op was undo or redo	*/


/*
 * File I/O.
 */

#define FIOSUC	0			/* Success.			*/
#define FIOFNF	1			/* File not found.		*/
#define FIOEOF	2			/* End of file.			*/
#define FIOERR	3			/* Error.			*/
#define FIOLONG 4			/* long line partially read	*/


/*
 * Directory I/O.
 */

#define DIOSUC	0			/* Success.			*/
#define DIOEOF	1			/* End of file.			*/
#define DIOERR	2			/* Error.			*/


/*
 * Display colors.
 */

#define CNONE	0			/* Unknown color.		*/
#define CTEXT	1			/* Text color.			*/
#define CMODE	2			/* Mode line color.		*/


/* Flags for keyboard invoked functions */

#define FFUNIV		0x01		/* universal argument		*/
#define FFNUM		0x02		/* numeric argument		*/
#define FFARG		0x03		/* any user argument		*/
#define FFRAND		0x04		/* Called by other function	*/
#define FFUNIV2		0x08		/* Digit sequence terminated	*/


/*
 * Flags for "eread".
 */

#define EFFUNC		0x0001		/* Autocomplete functions.	*/
#define EFBUF		0x0002		/* Autocomplete buffers.	*/
#define EFFILE		0x0004		/* Autocomplete files.		*/
#define EFCHARSET	0x0008		/* Autocomplete charsets	*/
#define EFVAR		0x0010		/* Autocomplete variables	*/
#define EFKEYMAP	0x0020		/* Autocomplete keymaps		*/
#define EFLOCALVAR	0x0040		/* Autocomplete local variables	*/
#define EFUNICODE	0x0080		/* Autocomplete Unicode names	*/
#define EFSEARCH	0x0100		/* Fill in pattern if empty	*/
#define EFREPLACE	0x0200		/* Fill in replacement if empty	*/
#define EFMACRO		0x0400		/* Autocomplete macros		*/
#define EFAUTO		0x07ff		/* Any autocompletion		*/
#define EFAUTOCR	0x06f9		/* Autocompletion on CR		*/
#define EFAUTOSPC	0x047b		/* Autocompletion on Space	*/
#define EFNOSTRICTCR	0x1000		/* Don't demand completion on RET */
#define EFNOSTRICTSPC	0x2000		/* Don't demand completion on SPC */


/*
 * Flags for "ldelete"/"kinsert"
 */

#define KNONE	0
#define KFORW	1
#define KBACK	2
#define KKILL	3


/*
 * Mg3a: flags for "copyfile"
 */

#define COPYERRORMSG	0x01		/* Show message on error	*/
#define COPYABORTMSG	0x02		/* Show message on abort	*/
#define COPYALLMSG	(COPYERRORMSG|COPYABORTMSG)


/*
 * Mg3a: flags for complete-matching
 */

#define MATCHFULL	0x01		/* Matched a full item.		*/
#define MATCHAMBIGUOUS	0x02		/* Match was ambiguous.		*/
#define MATCHNONE	0x04		/* Something matched nothing.	*/
#define MATCHEMPTY	0x08		/* Nothing matched everything.	*/
#define MATCHERR	0x10		/* Some other error in matching.*/


/*
 * Mg3a: Function flags.
 */

#define FUNC_ALIAS	0x01		/* Name is an alias		*/
#define FUNC_HIDDEN	0x02		/* Hidden from apropos listing	*/


/*
 * Mg3a: Tab options
 */

#define TAB_ALTERNATE	0x01		/* Alternate tab feature	*/
#define TAB_MOVEMENT	0x02		/* Simulate tabs for movement	*/


/*
 * All text is kept in circularly linked lists of "LINE" structures.
 * These begin at the header line (which is the blank line beyond the
 * end of the buffer). This line is pointed to by the "BUFFER". Each
 * line contains a the number of bytes in the line (the "used" size),
 * the size of the text array, and the text. The end of line is not
 * stored as a byte; it's implied. Future additions will include
 * update hints, and a list of marks into the line.
 */

typedef struct	LINE {
	struct	LINE *l_fp;		/* Link to the next line	*/
	struct	LINE *l_bp;		/* Link to the previous line	*/
	INT	l_size;			/* Allocated size		*/ // Tag:line
	INT	l_used;			/* Used size			*/
	int	l_flag;			/* Line type			*/
	char	l_text[];		/* A bunch of characters.	*/
}	LINE;


/*
 * The rationale behind these macros is that you could (with some
 * editing, like changing the type of a line link from a "LINE *" to a
 * "REFLINE", and fixing the commands like file reading that break the
 * rules) change the actual storage representation of lines to use
 * something fancy on machines with small address spaces.
 */

#define lforw(lp)	((lp)->l_fp)
#define lback(lp)	((lp)->l_bp)
#define lgetc(lp, n)	(CHARMASK((lp)->l_text[(n)]))
#define lputc(lp, n, c) ((lp)->l_text[(n)]=(c))
#define llength(lp)	((lp)->l_used)
#define ltext(lp)	((lp)->l_text)
#define	lsetlength(lp, n) ((lp)->l_used=(n))
#define lsize(lp)	((lp)->l_size)


/* Constants for the l_flag of a LINE */

#define LCBUFNAME	1	/* A buffer line in Buffer List		*/
#define LCDIRED 	2	/* An unedited line in Dired		*/
#define LCLISTHEAD	3	/* The head of a general list of lines	*/
#define LCUNICODE	4	/* Line with a Unicode character	*/
//			5,6	// Reserved for Clike
//		       15	// Max


/*
 * Mg3a: The undo record
 */

typedef struct undo_rec {
	struct undo_rec *pre;	/* Previous record			*/
	struct undo_rec *suc;	/* Next record				*/
	size_t pos;		/* Buffer position as integer		*/
	INT size;		/* Size of content			*/
	INT len;		/* Used data size			*/
	int flag;		/* flags				*/
	char content[];		/* The undo data			*/
} undo_rec;


#define UNDO_MIN 32		/* Minimum data length for undo record	*/

/* Undo flags */

#define UFL_INSERT	0x01	/* Undo insert				*/
#define UFL_DELETE	0	/* Undo delete				*/
#define UFL_BOUNDARY	0x02	/* Undo boundary			*/
#define UFL_CLEARMOD	0x04	/* Undo reset modified flag		*/


/*
 * All repeated structures are kept as linked lists of structures. All
 * of these start with a LIST structure (except lines, which have
 * their own abstraction). This will allow for later conversion to
 * generic list manipulation routines should I decide to do that. it
 * does mean that there are four extra bytes per window. I feel that
 * this is an acceptable price, considering that there are usually
 * only one or two windows.
 */

typedef struct LIST {
	union {
		struct WINDOW	*l_wp;
		struct BUFFER	*x_bp;	/* l_bp is used by LINE */
		struct LIST	*l_nxt;
	} l_p;
	char	*l_name;
} LIST;


/*
 * Usual hack - to keep from uglifying the code with lotsa references
 * through the union, we #define something for it.
 */

#define l_next	l_p.l_nxt


/*
 * There is a window structure allocated for every active display
 * window. The windows are kept in a big list, in top to bottom screen
 * order, with the listhead at "wheadp". Each window contains its own
 * values of dot and mark. The flag field contains some bits that are
 * set by commands to guide redisplay; although this is a bit of a
 * compromise in terms of decoupling, the full blown redisplay is just
 * too expensive to run for every input character.
 */

typedef struct	WINDOW {
	LIST	w_list;			/* List header		       */
	struct	BUFFER *w_bufp;		/* Buffer displayed in window	*/
	struct	LINE *w_linep;		/* Top line in the window	*/
	struct	LINE *w_dotp;		/* Line containing "."		*/
	struct	LINE *w_markp;		/* Line containing "mark"	*/
	size_t	w_dotpos;		/* Byte position of dot		*/
	INT	w_doto;			/* Byte offset for "."		*/ // Tag:window
	INT	w_marko;		/* Byte offset for "mark"	*/
	INT	w_toprow;		/* Origin 0 top row of window	*/
	INT	w_ntrows;		/* # of rows of text in window	*/
	INT	w_force;		/* If NZ, forcing row.		*/
	int	w_flag;			/* Flags.			*/
}	WINDOW;

#define w_wndp	w_list.l_p.l_wp
#define w_name	w_list.l_name


/*
 * Window flags are set by command processors to tell the display
 * system what has happened to the buffer mapped by the window.
 * Setting "WFHARD" is always a safe thing to do, but it may do more
 * work than is necessary. Always try to set the simplest action that
 * achieves the required update. Because commands set bits in the
 * "w_flag", update will see all change flags, and do the most general
 * one.
 */

#define WFFORCE 0x01			/* Force reframe.		*/
#define WFMOVE	0x02			/* Movement from line to line.	*/
#define WFEDIT	0x04			/* Editing within a line.	*/
#define WFHARD	0x08			/* Better to a full display.	*/
#define WFMODE	0x10			/* Update mode line.		*/


/*
 * Text is kept in buffers. A buffer header, described below, exists
 * for every buffer in the system. The buffers are kept in a big list,
 * so that commands that search for a buffer by name can find the
 * buffer header. There is a safe store for the dot and mark in the
 * header, but this is only valid if the buffer is not being displayed
 * (that is, if "b_nwnd" is 0). The text for the buffer is kept in a
 * circularly linked list of lines, with a pointer to the header line
 * in "b_linep".
 */

typedef struct	BUFFER {
	LIST	b_list;			/* buffer list pointer		*/
	struct	BUFFER *b_altb;		/* Link to alternate buffer	*/
	struct	LINE *b_dotp;		/* Link to "." LINE structure	*/
	struct	LINE *b_markp;		/* ditto for mark		*/
	struct	LINE *b_linep;		/* Link to the header LINE	*/
	undo_rec *undo_list;		/* Undo list			*/
	undo_rec *undo_current;		/* Current pos in undo list	*/
	INT	undo_size;		/* Size of data in undo list	*/
	INT	undo_forward;		/* Undoing forward in time	*/
	MODE b_modes[PBMODES];		/* buffer modes		*/
	charset_t	charset;	/* Charset of buffer		*/
	size_t	b_size;			/* Byte size of buffer		*/
	size_t	b_dotpos;		/* Dot position in bytes	*/
	INT	b_doto;			/* Offset of "." in above LINE	*/
	INT	b_marko;		/* ditto for the "mark"		*/
	INT	b_nmodes;		/* number of non-fundamental modes */
	INT	b_nwnd;			/* Count of windows on buffer	*/
	INT	b_flag;			/* Flags			*/ // Tag:buffer
	INT	l_flag;			/* Used by language modes. Inited to 0.	*/

	union	{
		INT var[MAXLOCALVAR];
		struct {
			INT fillcol;
			INT fill_options;
			INT make_backup;
			INT soft_tab_size;
			INT tab_options;
			INT tabs_with_spaces;
			INT trim_whitespace;
#ifdef LANGMODE_CLIKE
			INT clike_lang;
			INT clike_options;
			INT clike_style;
#endif
		} v;
	} localvar;
	union {
		char *var[MAXLOCALSTRING];
		struct {
			char *comment_begin;
			char *comment_end;
		} v;
	} localsvar;
	char	localmodename[LOCALMODELEN]; /* Local mode name		*/
	char	b_fname[NFILEN];	/* File name			*/
}	BUFFER;

#define b_bufp	b_list.l_p.x_bp
#define b_bname b_list.l_name

#define BFCHG		0x01		/* Changed.			*/
#define BFBAK		0x02		/* Need to make a backup.	*/
#define BFNOTAB 	0x04		/* no tab mode			*/
#define BFOVERWRITE 	0x08		/* overwrite mode		*/
#define	BFUNIXLF 	0x10		/* Unix line endings		*/
#define	BFBOM		0x20		/* File BOM			*/
#define BFMODELINECHARSET 0x40		/* Always show 8-bit charset	*/
#define BFREADONLY	0x80		/* read only			*/
#define BFDIRED		0x100		/* Dired			*/
#define BFSYS		0x200		/* System buffer 		*/
#define BFSCRATCH	0x400		/* Scratch buffer		*/

#define CHARSET8BIT	0x01		/* 8-bit charset flag		*/
#define CHARSETUTF8	0x02		/* UTF-8 charset flag		*/
#define CHARSETALL	(CHARSET8BIT|CHARSETUTF8)

#define CASEDOWN	0		/* Convert to lower case	*/
#define CASEUP		1		/* Convert to upper case	*/
#define CASETITLE	2		/* Convert to title case	*/

/* Modeline control */

#define MLCHARSET	0x01		/* Always show charset		*/
#define MLCRLF		0x02		/* Show "-crlf"			*/
#define	MLNOLF		0x04		/* Don't show "-lf"		*/
#define MLNODEF		0x08		/* Don't show default charset	*/


/* Backward compatibility */

/* Choose current window after "split-window" based on cursor position	*/
/* in original window.							*/
#define BACKW_DYNWIND		0x01


/*
 * This structure holds the starting and ending positions (as
 * line/offset pairs and byte positions), the number of bytes and
 * lines, and the direction of a region of a buffer.
 */

typedef struct	{
	struct	LINE *r_linep;		/* Origin LINE address.		*/
	struct	LINE *r_endlinep;	/* End LINE address		*/
	size_t	r_dotpos;		/* Byte position of origin	*/
	size_t	r_enddotpos;		/* Byte position of end		*/
	INT	r_offset;		/* Origin LINE offset.		*/	// Tag:region
	INT	r_endoffset;		/* End LINE offset.		*/
	INT	r_size;			/* Length in bytes.		*/
	INT	r_lines;		/* Length in lines.		*/
	int	r_forward;		/* Direction			*/
}	REGION;


/*
 * A structure for holding the current position of the window and dot
 * in the form of line numbers and column. Used with getwinpos() and
 * setwinpos();
 */

typedef struct {
	size_t	line;
	size_t	dot;
	INT 	column;
} 	winpos_t;


/*
 * External variables, Tag:vars
 */

extern	INT	thisflag;
extern	INT	lastflag;
extern	INT	curgoal;
extern	INT	epresf;
extern	INT	sgarbf;
extern	WINDOW	*curwp;
extern	BUFFER	*curbp;
extern	WINDOW	*wheadp;
extern	BUFFER	*bheadp;
extern	char	pat[];
extern	INT	nrow;
extern	INT	ncol;
extern	INT	ttrow;
extern	INT	ttcol;
extern	INT	tthue;
extern	const uint_least8_t cinfo[];
extern	INT	termisutf8;
extern	INT	termdoeswide;
extern	INT	termdoescombine;
extern	INT	termdoesonlybmp;
extern	INT	fullterm;
extern	INT	width_routine;
extern	INT	case_fold_search;
extern	charset_t	buf8bit;
extern	charset_t	termcharset;
extern	charset_t	bufdefaultcharset;
extern	charset_t	doscharset;
extern	INT	kbuf_flag;
extern	charset_t	kbuf_charset;
extern	INT	kbuf_wholelines;
extern	INT	backward_compat;
extern	INT	defb_flag;
extern	char	*wdir;
extern	LINE	*gmarkp;
extern	INT	gmarko;
extern	size_t	count_lf;
extern	size_t	count_crlf;
extern	INT	macro_show_message;
extern	INT	inhibit_startup;
extern	INT	word_search;
extern	INT	soft_tab_size;
extern	INT	tabs_with_spaces;
extern	INT	tab_options;
extern	INT	kill_whole_lines;
extern	const INT	localvars;
extern	const INT	localsvars;
extern	INT	defb_nmodes;
extern	MODE	defb_modes[PBMODES];
extern	INT	defb_flag;
extern	INT	errcheck;
extern	INT	undo_enabled;
extern	INT	toplevel_undo_enabled;
extern	INT	undo_limit;
extern	int	u_boundary;
extern	INT	emacs_compat;
extern	PF	prevfunc;

/* External functions, Tag:funcs */

/* basic.c */

void	setgoal(void);
INT	getgoal(LINE *lp);
void	isetmark(void);

/* buffer.c */

INT	internal_killbuffer(BUFFER *bp);
INT	addline(BUFFER *bp, char *text);
INT	addline_extra(BUFFER *bp, char *text, int flag, char *extra);
INT	anycb(INT f);
BUFFER	*bfind(char *bname, INT cflag);
void	resetbuffer(BUFFER *bp);
INT	bclear(BUFFER *bp);
INT	showbuffer(BUFFER *bp, WINDOW *wp);
WINDOW	*popbuf(BUFFER *bp);
INT	popbuftop(BUFFER *bp);

/* cinfo.c */

char 	*keyname(char *cp, INT k);

/* dir.c */

void	dirinit(void);

/* dired.c */

#ifdef DIRED
void	diredsetoffset(void);
BUFFER	*readin_dired(BUFFER *bp, char *dirname);
#endif

/* display.c */

void	vtinit(void);
void	vttidy(void);
INT	getcolumn(WINDOW *wp, LINE *lp, INT offset);
INT	getoffset(WINDOW *wp, LINE *lp, INT column);
INT	getfullcolumn(WINDOW *wp, LINE *lp, INT offset);
INT	getfulloffset(WINDOW *wp, LINE *lp, INT column);
void	invalidatecache(void);
void	reframe_window(WINDOW *wp);
void	update(void);

/* echo.c */

void	eerase(void);
void	estart(void);
INT	eyorn(char *sp);
INT	eyesno(char *sp);
INT	ereply(char *prompt, char *buf, INT nbuf, ...);
INT	eread(char *prompt, char *buf, INT nbuf, INT flag, ...);
void	ewprintf(char *fp, ...);
void	ewprintfc(char *fp, ...);
INT	emessage(INT status, char *fp, ...);

/* extend.c */

INT	execute(PF pf, INT f, INT n);
INT	internal_bind(MODE mode, char *str, PF funct);
INT	load(char *fname);
INT	excline_copy(char *line);
INT	excline_copy_list(char *line);
INT	excline_copy_list_n(char *commands, INT n);
int	autoexec_execute(int change);

/* file.c */

BUFFER	*findbuffer(char *fname);
void	setcharsetfromcontent(BUFFER *bp);
INT	readin(char *fname);
INT	freadin(FILE *f);
INT	buffsave(BUFFER *bp);
void	upmodes(BUFFER *bp);
void	refreshbuf(BUFFER *bp);
void	winflag(BUFFER *bp, INT flag);
BUFFER	*buftotop(BUFFER *bp);

/* fileio.c */

INT	ffrset(FILE *f);
INT	ffropen(char *fn);
INT	ffwopen(char *fn);
INT	ffclose(void);
INT	ffputbuf(BUFFER *bp);
INT	ffgetline(char *buf, INT nbuf, INT *nbytes);
INT	fbackupfile(char *fn);
char	*adjustname(char *fn);
char 	*startupfile(char *suffix);
int	fncmp(char *str1, char *str2);
INT	copyfile(char *infilename, char *outfilename, INT flag, off_t *outsize);
int	filematch(char *pattern, char *filename);

/* help.c */

BUFFER	*emptyhelpbuffer(void);

/* kbd.c */

void	ungetkey(INT c);
INT	translate(charset_t charset, INT c);
INT	reverse_translate(charset_t charset, INT c);
INT	getkey(INT flag);
INT	doin(void);

/* keymap.c */
INT	exec_mode_init_commands(MODE mode);
char	*getnext_mapname(INT first);
void	do_internal_bindings(void);

/* line.c */

INT	readonly(void);
LINE 	*lalloc(INT used);
LINE 	*lallocx(INT used);
void	lfree(BUFFER *bp, LINE *lp, size_t pos);
void	lchange(INT flag);
INT	linsert_str(INT n, char *str, INT len);
INT	linsert_str_noundo(char *str, INT len);
INT	linsert(INT n, INT c);
INT	linsert_ucs(charset_t charset, INT n, INT c);
INT	linsert_overwrite(charset_t charset, INT n, INT c);
INT	laddline(char *txt, INT len);
INT	lnewline(void);
INT	lnewline_noundo(void);
INT	lnewline_n(INT n);
INT	ldelete(INT n, INT kflag);
INT	ldeleteback(INT n, INT kflag);
INT	ldeleteraw(INT n, INT kflag);
INT	ldeleterawback(INT n, INT kflag);
INT	ldeleteregion(const REGION *rp, INT kflag);
INT	ldelnewline(void);
INT	ldelnewline_noundo(void);
INT lduplicate(INT f, INT n);
INT lmovedown(INT f, INT n);
INT lmoveup(INT f, INT n);
INT	lreplace(INT plen, char *st, INT case_exact);
void	kdelete(void);
INT	kinsert(INT c, INT dir);
INT	kinsert_str(char *str, INT len, INT dir);
INT	kgrow(INT back, INT size);
INT	kremove(INT n);
void	getkillbuffer(char **kbuf, INT *ksize);
int	find_and_remove_bom(BUFFER *bp);
INT	normalizebuffer(BUFFER *bp);
INT	normalizebuffer_noundo(BUFFER *bp);
INT	linsert_general_string(charset_t str_charset, INT n, char *str, INT len);
INT	linsert_overwrite_general_string(charset_t str_charset, INT n, char *str, INT len);
void	ltrim(BUFFER *bp, LINE *lp, INT len, size_t pos);

/* macro.c */

#ifdef USER_MACROS
char	*lookup_named_macro(char *name);
char	*getnext_macro(INT first);
PF 	name_or_macro_to_key_function(char *fname);
char 	*key_function_to_name_or_macro(PF pf);
INT	func_macro_execute(INT i, INT n);
#endif

/* match.c */

INT	leftparenof(INT c);
INT	rightparenof(INT c);
void	displaymatch(LINE *clp, INT cbo);

/* modes.c */

INT	changemode(BUFFER *bp, INT f, INT n, char *mode);
INT	setbufmode(BUFFER *bp, char *mode);
INT	clearbufmode(BUFFER *bp, char *mode);
int	isbufmode(BUFFER *bp, char *mode);
INT	internal_unixlf(INT n);
INT	nocharsets(void);
INT	internal_utf8(INT n);
INT	internal_dos(INT n);
INT	internal_bom(INT n);
INT	internal_iso(INT n);
int	charsetequal(char *str1, char *str2);
charset_t	nametocharset(char *str, INT flags);
char 	*charsettoname(charset_t charset);

/* random.c */

INT	getcolpos(void);
INT	internal_deletetrailing(BUFFER *bp, int msg);
INT	tabtocolumn(INT fromcol, INT tocol);
INT	getindent(LINE *lp, INT *outoffset, INT *outcolumn);
INT	setindent(INT col);
INT	gettabsize(BUFFER *bp);
INT	deltrailwhite(void);
INT	delleadwhite(void);
INT	inserttabs(INT n);

/* region.c */
INT	getregion_mark(REGION *rp, LINE *markp, INT marko);
INT	getregion(REGION *rp);

/* search.c */

#ifdef SEARCHSIMPLE
INT internal_searchsimple(char *str, INT circular);
#endif

/* tty.c */

void	ttinit(void);
void	tttidy(void);
void	ttmove(INT row, INT col);
void	tteeol(void);
void	tteeop(void);
void	ttbeep(void);
void	ttnowindow(void);
void	ttcolor(INT color);
void	ttresize(void);

/* ttyio.c */

void	ttopen(void);
void	ttclose(void);
int	setctrlg_interrupt(int on);
void	ttputc(INT c);
void	ttflush(void);
INT	ttgetc(void);
int	ittputc(int c);
int	waitforinput(INT milliseconds);
int	type_ahead(void);
void	panic(char *s, int error);
void	setttysize(void);

/* ttykbd.c */

void	ttykeymapinit(void);

/* ucs.c */

INT	ucs_backward(charset_t charset, LINE *lp, INT offset);
INT	ucs_forward(charset_t charset, LINE *lp, INT offset);
INT	ucs_prevc(charset_t charset, LINE *lp, INT offset);
void	ucs_adjust(void);
void	ucs_put(INT c);
INT	ucs_termwidth(INT c);
INT	ucs_fulltermwidth(INT c);
INT	ucs_termwidth_str(charset_t charset, char *str, INT len);
INT	ucs_strtochar(charset_t charset, char *str, INT len, INT offset, INT *outoffset);
INT	ucs_char(charset_t charset, LINE *lp, INT offset, INT *outoffset);
INT	ucs_chartostr(charset_t charset, INT c, unsigned char *str, INT *len);
int	ucs_islower(INT c);
int	ucs_isupper(INT c);
INT	ucs_tolower(INT c);
INT	ucs_toupper(INT c);
int	ucs_isalpha(INT c);
int	ucs_isalnum(INT c);
int	ucs_isword(INT c);
int	ucs_changecase(INT tocase, INT c, INT **out, INT *outlen);
INT	ucs_case_fold(INT c);
int	ucs_strcasecmp(charset_t charset, char *str1, char *str2);
INT	ucs_strchars(charset_t charset, char *str);
INT	ucs_strscan(charset_t charset, char *str1, char *str2, INT case_sensitive);
INT	ucs_complete(char *instr, char *outstr, INT outstr_size,
		char *(*getnext)(INT first), INT case_insensitive, INT *statusptr);

/* ucsnames.c */

#ifdef UCSNAMES
int	unicode_database_exists(void);
char	*getnext_unicode(INT first);
INT	ucs_nametonumber(char *name);
char	*ucs_numbertoname(INT val);
INT	advanced_display(INT f, INT n);
#endif

/* undo.c */

int	u_clear(BUFFER *bp);
int	u_empty(void);
int	u_entry(int flag, size_t pos, char *str, INT len);
int	u_entry_range(int flag, LINE *lp, INT offset, INT len, size_t pos);
void	u_modify(void);
void	inhibit_undo_boundary(INT f, PF pf);


/* util.c */

size_t	stringcopy(char *dest, char *src, size_t size);
size_t	stringconcat(char *dest, char *src, size_t size);
size_t	stringconcat2(char *dest, char *src1, char *src2, size_t size);
char	*string_dup(char *s);
size_t	findnewline(char *str, size_t len);
void	getwinpos(winpos_t *w);
void	setwinpos(winpos_t w);
int	isdirectory(char *filename);
int	isregularfile(char *filename);
int	issymlink(char *filename);
int	fileexists(char *filename);
int	existsandnotreadable(char *filename);
char	*filenameof(char *filename);
char	*dirnameofbuffer(BUFFER *bp);
INT	column_overflow(void);
void	*outofmem(size_t size);
char	*overlong(void);
INT	invalid_codepoint(void);
INT	buffernotfound(char *name);
void	addlast(BUFFER *bp, LINE *lp);
void	insertafter(LINE *lp1, LINE *lp2);
void	insertbefore(LINE *lp1, LINE *lp2);
void	insertlast(LINE *listhead, LINE *lp);
void	insertfirst(LINE *listhead, LINE *lp);
void	removefromlist(LINE *lp);
LINE	*make_empty_list(LINE *listhead);
INT	basedigit(INT c, INT base);
int	outofarguments(int msg);
char 	*dirname_addslash(char *dirname);
int	isblankline(LINE *lp);
void	upcase_ascii(char *s);
void	make_underline(char *s);
int	ascii_strcasecmp(const char *s1, const char *s2);
void	chomp(char *s);
void	initkill(void);
int	isshebang(BUFFER *bp, char *str);
INT	concat_limited(char *str, char *add, INT maxwidth, INT size);
void	*malloc_msg(size_t size);
int	getINT(char *str, INT *outint, int msg);
int	in_leading_whitespace(void);
INT	macro_store_INT(INT i);
INT	macro_get_INT(INT *outint);
int	sortlist(LINE *listhead, int (*sortfunc)(const void *, const void *));
int	sortlist_content(LINE *listhead);
void	adjustpos(LINE *to_line, INT to_offset);
void	adjustpos3(LINE *to_line, INT to_offset, size_t to_pos);
int	position(size_t to_pos);
void	endofpreviousline(void);

/* variables.c */
void variable_completion_init(void);

char	*getnext_varname(INT first);
char	*getnext_localvarname(INT first);
INT	get_variable(INT localvar, INT globalvar);

/* version.c */
extern char version[];

/* width.c */

int	ucs_nonspacing(INT c);
int	ucs_wide(INT c);
INT	ucs_width(INT c);

/* window.c */

WINDOW	*wpopup(void);

/* word.c */

int	inword(void);
