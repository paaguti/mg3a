/*
 *		Mainline
 */
#include	"def.h"
#include	"macro.h"

#include <locale.h>

INT	thisflag = 0;			/* Flags, this command		*/
INT	lastflag = 0;			/* Flags, last command		*/
INT	curgoal = 0;			/* Goal column			*/
BUFFER	*curbp;				/* Current buffer		*/
WINDOW	*curwp;				/* Current window		*/
BUFFER	*bheadp = NULL;			/* BUFFER listhead		*/
WINDOW	*wheadp = NULL;			/* WINDOW listhead		*/
char	pat[NPAT] = "";			/* Pattern			*/
#ifndef NO_DPROMPT
extern char prompt[], *promptp;		/* delayed prompting		*/
#endif
INT	inhibit_startup = 0;		/* Inhibit startup files	*/

static void	edinit(void);

INT	gotoline(INT f, INT n);


static int
usage(char *progname)
{
	fprintf(stderr, "Usage: %s [-q] [-p pattern] [-L loadfile] [-l loadfile]"
		" [-e commands] [+line] [files...]\n", progname);
	return 1;
}


int
main(int argc, char *argv[])
{
	char	*cp;
	BUFFER	*bp;
	INT	c, n;
	char	*linearg = NULL;
	INT	lineno;
	char	*loadfile = NULL, *startexpr = NULL, *pre_loadfile = NULL;
#ifdef PIPEIN
	FILE *pipein = NULL;

 	if (!isatty(fileno(stdin))) {
 		/*
		 * Create a copy of stdin for future use
 		 */
		pipein = fdopen(dup(fileno(stdin)),"r");
 		/*
 		 * Reopen the console device /dev/tty as stdin
 		 */
 		stdin = freopen("/dev/tty","r",stdin);
	}
#else
	if (!isatty(0)) {
		fprintf(stderr, "mg: standard input is not a tty\n");
		exit(1);
	}
#endif
	setlocale(LC_ALL, "");

	while ((c = getopt(argc, argv, "qp:L:l:e:")) != -1) {
		switch (c) {
		case 'q':
			inhibit_startup = 1;
			break;
		case 'p':
			stringcopy(pat, optarg, NPAT);
			break;
		case 'L':
			pre_loadfile = optarg;
			break;
		case 'l':
			loadfile = optarg;
			break;
		case 'e':
			startexpr = optarg;
			break;
		default:
			return usage(argv[0]);
		}
	}

	vtinit();				/* Virtual terminal.	*/
	dirinit();				/* Get current directory */
	edinit();				/* Buffers, windows.	*/

	/*
	 * Doing update() before reading files causes the error
	 * messages from the file I/O show up on the screen. (and also
	 * an extra display of the mode line if there are files
	 * specified on the command line.)
	 */

	update();

	ttykeymapinit();			/* Symbols, bindings.	*/

	if (!inhibit_startup && (cp = startupfile(NULL)) != NULL) {
		load(cp);
	}

	/* "-L" argument */

	if (pre_loadfile) load(pre_loadfile);

	if ((bp = bfind("*scratch*", TRUE)) != NULL) {
		resetbuffer(bp);		/* Buffer "*scratch*" will need reset of modes */
	}

	/* Goto line if two arguments */

	if (argc - optind == 2) {
		if (argv[optind][0] == '+') {
			linearg = argv[optind];
			optind++;
		} else if (argv[argc - 1][0] == '+') {
			linearg = argv[argc - 1];
			argc--;
		}
	}

	for (n = argc - 1; n >= optind; n--) {	/* Backwards, for prettier list	*/
		if ((cp = adjustname(argv[n])) == NULL) continue;
#ifndef DIRED
		if (isdirectory(cp)) continue;
#endif
		if (existsandnotreadable(cp)) continue;
		if ((bp = findbuffer(cp)) == NULL) break;

		curbp = bp;
		showbuffer(curbp, curwp);

		if (readin(cp) != TRUE) break;
	}
#ifdef PIPEIN
	if (NULL != pipein) {
		if ((bp = findbuffer("*stdin*")) != NULL) {
			curbp = bp;
			showbuffer(curbp, curwp);
			freadin(pipein);
			curbp->b_flag |= BFREADONLY;
		}
	}
#endif
	if (linearg) {
		if (linearg[1] == 0) gotoline(FFARG, 0);
		else if (getINT(linearg + 1, &lineno, 1)) gotoline(FFARG, lineno);
	}

	if (loadfile) load(loadfile);
	if (startexpr) excline_copy_list(startexpr);

	for(;;) {
#ifndef NO_DPROMPT
	    *(promptp = prompt) = '\0';
#endif
	    if (epresf == KPROMPT) eerase();
	    else if (epresf == TRUE) epresf = KPROMPT;

	    ucs_adjust();
	    update();

	    lastflag = thisflag;
	    thisflag = 0;

	    switch(doin()) {
		case TRUE: break;
		case ABORT:
		    while (typeahead()) ttgetc();
		    if (!(thisflag & CFNOQUIT)) ewprintf("Quit");
		    /* fallthrough */
		case FALSE:
		default:
		    ttbeep();
#ifndef NO_MACRO
		    macrodef = FALSE;
#endif
	    }
	}
}


/*
 * Initialize default buffer and window.
 */

static void
edinit()
{
	BUFFER *bp;
	WINDOW *wp;

	bheadp = NULL;
	bp = bfind("*scratch*", TRUE);		/* Text buffer.		*/
	wp = (WINDOW *)malloc(sizeof(WINDOW));	/* Initial window.	*/
	if (bp==NULL || wp==NULL) panic("edinit", 0);
	curbp  = bp;				/* Current ones.	*/
	wheadp = wp;
	curwp  = wp;
	wp->w_wndp  = NULL;			/* Initialize window.	*/
	wp->w_bufp  = bp;
	bp->b_nwnd  = 1;			/* Displayed.		*/
	wp->w_linep = wp->w_dotp = bp->b_linep;
	wp->w_doto  = 0;
	wp->w_markp = NULL;
	wp->w_marko = 0;
	wp->w_toprow = 0;
	wp->w_ntrows = nrow-2;			/* 2 = mode, echo.	*/
	wp->w_force = 0;
	wp->w_flag  = WFMODE|WFHARD;		/* Full.		*/
	wp->w_dotpos = 0;
}


/*
 * Mg3a: Warn for new behavior of "save-buffers-kill-emacs" with an
 * argument. Return TRUE if OK to save and exit.
 */

static INT
quit_warning()
{
	BUFFER *bp;
	INT s;

	bp = emptyhelpbuffer();
	if (!bp) return FALSE;

	if ((s = addline(bp, "Mg now behaves like GNU Emacs when \"save-buffers-kill-emacs\" is called")) != TRUE)
		return s;

	if ((s = addline(bp, "with an argument: all modified buffers will be saved before exiting.")) != TRUE)
		return s;

	if ((s = addline(bp, "")) != TRUE)
		return s;

	if ((s = addline(bp, "To turn off this warning set bit 1 in the variable \"emacs-compat\".")) != TRUE)
		return s;

	if ((s = addline(bp, "")) != TRUE)
		return s;

	if ((s = addline(bp, "To exit easily without saving, assign \"kill-emacs\" to a key.")) != TRUE)
		return s;

	if ((s = popbuftop(bp)) != TRUE) return s;

	update();

	return eyesno("Save all modified buffers and exit");
}


/*
 * Quit command.
 */

INT
quit(INT f, INT n)
{
	INT	s, saveall = 0;

	if (inmacro) {
		if (anycb(1) != FALSE) {
			return emessage(FALSE, "Some buffers failed to be written; not exiting");
		}

		vttidy();
		exit(GOOD);
	}

	if (f & FFRAND) {
		saveall = 1;
	} else if (f & FFARG) {
		if (!(emacs_compat & 1) && (s = quit_warning()) != TRUE) return s;
		saveall = 1;
	}

	if ((s = anycb(saveall)) == ABORT) return ABORT;

	if (s == FALSE
	|| eyesno("Some modified buffers exist, really exit") == TRUE) {
		vttidy();
		exit(GOOD);
	}
	return TRUE;
}


/*
 * Mg3a: Quick exit
 */

INT
hardquit(INT f, INT n)
{
	vttidy();
	exit(GOOD);

	return TRUE;
}


/*
 * Mg3a: Save all files and exit (not in Emacs)
 */

INT
save_and_exit(INT f, INT n)
{
	return quit(FFRAND, 1);
}


/*
 * User abort. Should be called by any input routine that sees a C-g
 * to abort whatever C-g is aborting these days. Currently does
 * nothing.
 */

INT
ctrlg(INT f, INT n)
{
	return ABORT;
}
