/*
 * Termcap/terminfo display driver
 *
 * Mg3a: Cleaned up a bit. Uses setupterm. And sigaction.
 */

#include	"def.h"

#include <langinfo.h>
#include <signal.h>
#include <stdarg.h>

#define BEL	0x07			/* BEL character.		*/

char		*tgetstr(char *, char **);
char		*tgoto(char *, int, int);
int		setupterm(char *, int, int *);
int		tputs(const char *, int, int (*)(int));

#ifdef MOUSE
static void ttputsf(char *format, ...);
#endif

char	*CM,			/* cursor move				*/
	*CE,			/* clear to end of line			*/
	*TI,			/* term init -- start using cursor motion */
	*TE,			/* term end --- end using cursor motion */
	*SO,			/* invers video				*/
	*SE,			/* normal video				*/
	*CD,			/* clear to end of display		*/
	*CS,			/* set scroll region			*/
	*KS,			/* enter keypad mode			*/
	*KE;			/* exit keypad mode			*/

#ifdef UTF8
INT		termisutf8 = 0;			// Terminal is UTF-8
INT		termdoeswide = 1;		// Terminal handles wide characters
INT		termdoescombine = 1;		// Terminal handles combining characters
INT		termdoesonlybmp = 1;		// Terminal is restricted to BMP

charset_t	buf8bit = cp1252;		// Default 8-bit charset
charset_t	termcharset = cp1252;		// Terminal/locale charset
charset_t	bufdefaultcharset = cp1252;	// Default charset
charset_t	doscharset = cp437;		// Alternate/Dos charset

/* Internal variable. If true, assume all terminal capabilities are there */
INT		fullterm = 0;
#endif


INT refresh(INT f, INT n);


/*
 * Mg3a: Finally deal with window change.
 */

volatile sig_atomic_t winchanged = 0;

static void
winchange(int dummy)
{
	winchanged = 1;
}

/*
 * Initialize the terminal when the editor gets started up.
 *
 * Mg3a: and do a minimal re-init when resuming.
 */

void
ttinit()
{
	char *tv_stype;
	struct sigaction sa;
	static int inited = 0;
	int errret;
	char errstr[160];

	if (inited) {
		// Comes here after resuming
                mouse_mode(1);
		if (TI && *TI) putpad(TI, 1);	/* init the term */
		if (KS && *KS) putpad(KS, 1);	/* turn on keypad	*/
		refresh(0, 1);			/* Check for size, and set up to repaint */
		winchanged = 0;			/* Already done */
		return;
	}

	inited = 1;

	if((tv_stype = gettermtype()) == NULL)
		panic("Could not determine terminal type", 0);

	setupterm(tv_stype, 1, &errret);

	if (errret == -1) {
		panic("Terminfo database not found", 0);
	} else if (errret == 0) {
		snprintf(errstr, sizeof(errstr), "Terminfo entry for '%s' not found", tv_stype);
		panic(errstr, 0);
	}

	CD = tgetstr("cd", NULL);
	CM = tgetstr("cm", NULL);
	CE = tgetstr("ce", NULL);
	TI = tgetstr("ti", NULL);
	TE = tgetstr("te", NULL);
	SO = tgetstr("so", NULL);
	SE = tgetstr("se", NULL);
	CS = tgetstr("cs", NULL);	/* set scrolling region */
	KS = tgetstr("ks", NULL);	/* keypad start, keypad end	*/
	KE = tgetstr("ke", NULL);
	mouse_mode (1);

#ifdef UTF8
	termcharset = nametocharset(nl_langinfo(CODESET), CHARSETALL);
	bufdefaultcharset = termcharset;

	if (termcharset == utf_8) {
		termisutf8 = 1;
	} else {
		buf8bit = termcharset;
	}

	if (strncmp(tv_stype, "cygwin", 6) == 0) {
		termdoeswide = 0;
		termdoescombine = 0;
		termdoesonlybmp = 1;
	}
#endif

#if LF_DEFAULT
	defb_flag |= BFUNIXLF;
#endif
	if(CM == NULL)
	    panic("This terminal is too stupid to run Mg", 0);

	ttresize();			/* set nrow & ncol	*/

	/* watch out for empty capabilities (sure to be wrong)	*/
	if (CE && !*CE) CE = NULL;
	if (CS && !*CS) CS = NULL;
	if (CD && !*CD) CD = NULL;

	if (TI && *TI) putpad(TI, 1);	/* init the term */
	if (KS && *KS) putpad(KS, 1);	/* turn on keypad	*/

	sa.sa_handler = winchange;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGWINCH, &sa, NULL);
	sigaction(SIGCONT, &sa, NULL);
}


/*
 * Clean up the terminal, in anticipation of a return to the command
 * interpreter. This is a no-op on the ANSI display. On the SCALD
 * display, it sets the window back to half screen scrolling. Perhaps
 * it should query the display for the increment, and put it back to
 * what it was.
 */

void
tttidy()
{
	mouse_mode(0);                  /* turn off mouse                   */
	if (KE && *KE) putpad(KE, 1);	/* turn off keypad		    */
	if (TE && *TE) putpad(TE, 1);	/* set the term back to normal mode */
}


/*
 * Move the cursor to the specified origin 0 row and column position.
 * Try to optimize out extra moves; redisplay may have left the cursor
 * in the right location last time!
 */

void
ttmove(INT row, INT col)
{
    if (row != ttrow || col != ttcol) {
	putpad(tgoto(CM, col, row), 1);
	ttrow = row;
	ttcol = col;
    }
}


/*
 * Erase to end of line.
 */

void
tteeol()
{
    if (CE) putpad(CE, 1);
    else {
	INT i = ncol - ttcol;
	if (ttrow == nrow - 1) i--;
	while(i-- > 0) ttputc(' ');
	ttcol = HUGE;
    }
}


/*
 * Erase to end of page.
 */

void
tteeop()
{
    INT line;

    if (CD) putpad(CD, nrow - ttrow);
    else {
	tteeol();

	/* do it by hand */
	for (line = ttrow + 1; line < nrow; line++) {
		ttmove(line, 0);
		tteeol();
	}

	ttrow = ttcol = HUGE;
    }
}


/*
 * Mg3a: Controlling type of bell
 */

INT bell_type = 1;	// Audio bell default
static int inverse_video = 0;


/*
 * Make a noise.
 */

void
ttbeep() {
	char *ce = CE;

	if (bell_type == 1) {
		ttputc(BEL);
		ttflush();
	} else if (bell_type == 2) {
		CE = NULL;
		refreshbuf(NULL);
		invalidatecache();
		inverse_video = 1;
		update();
		usleep(100000);
		refreshbuf(NULL);
		invalidatecache();
		inverse_video = 0;
		update();
		CE = ce;
	}
}


/*
 * Switch to full screen scroll. This is used by "spawn.c" just before
 * is suspends the editor, and by "display.c" when it is getting ready
 * to exit.
 */

void
ttnowindow()
{
    if (CS) {
	putpad(tgoto(CS, nrow - 1, 0), nrow - ttrow);
	ttrow = HUGE;			/* Unknown.		*/
	ttcol = HUGE;
    }
}


/*
 * Set the current writing color to the specified color. Watch for
 * color changes that are not going to do anything (the color is
 * already right) and don't send anything to the display.
 *
 * Mg3a: added inverse video.
 */

void
ttcolor(INT color)
{
	if (inverse_video) {
		if (color == CTEXT) color = CMODE;
		else if (color == CMODE) color = CTEXT;
	}

	if (color != tthue) {
		if (color == CTEXT) {		/* Normal video.	*/
			putpad(SE, 1);
		} else if (color == CMODE) {	/* Reverse video.	*/
			putpad(SO, 1);
		}
		tthue = color;			/* Save the color.	*/
	}
}


/*
 * This routine is called by the "refresh the screen" command to try
 * and resize the display.
 */

void
ttresize()
{
	setttysize();			/* found in "ttyio.c",	*/
					/* ask OS for tty size	*/
	if (nrow < 1)			/* Check limits.	*/
		nrow = 1;

	if (ncol < 1)
		ncol = 1;
}

/*
 * mode  0 => disable mouse reporting
 *       1 => enable mouse reporting
 */
#ifdef MOUSE
static void
mouse_mode(INT mode)
{
  ttputsf("\e[?9%c", (mode == 1) ? 'h' : 'l');  // Enable/Disable mouse report
}

static void
ttputsf(char *format, ...)
{
  char localbuf[256];
  va_list valist;
  va_start (valist,format);
  vsnprintf(localbuf,255,format,valist);
  va_end(valist);
  for (int i=0; localbuf[i]; i++)
    ittputc(localbuf[i]);
}
#endif
