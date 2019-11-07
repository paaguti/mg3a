/*
 * Name:	MicroEMACS
 *		System V terminal I/O.
 * Version:	0
 * Last edit:	Tue Aug 26 23:57:57 PDT 1986
 * By:		gonzo!daveb
 *		{sun, amdahl, mtxinu}!rtech!gonzo!daveb
 *
 * The functions in this file negotiate with the operating system for
 * keyboard characters, and write characters to the display in a
 * barely buffered fashion.
 *
 * Mg3a: many changes, including converting to termios.
 */

#include	"def.h"

#ifdef SLOW
#include "slowputc.h"
#endif

#ifndef SELECT_FOR_TYPEAHEAD
#define SELECT_FOR_TYPEAHEAD 1		/* May be more portable		*/
#endif

#include	<sys/select.h>
#include	<sys/time.h>

#include	<sys/ioctl.h>
#include	<termios.h>
#include	<signal.h>

#define	NOBUF	512			/* Output buffer size.		*/

int tgetnum(char *);

static char	obuf[NOBUF];		/* Output buffer.		*/
static INT	nobuf = 0;		/* buffer count			*/

static struct termios	ot;		/* entry state of the terminal	*/
static struct termios	nt;		/* editor's terminal state	*/

static int ttyactivep = FALSE;		/* terminal in editor mode?	*/
static int ttysavedp = FALSE;		/* terminal state saved?	*/

#ifdef	TIOCGWINSZ
static struct	winsize winsize;	/* 4.3 BSD window sizing	*/
#endif

INT	nrow;				/* Terminal size, rows.		*/
INT	ncol;				/* Terminal size, columns.	*/

#ifdef CHARSDEBUG
INT charsoutput = 0;			/* Bytes output since last display */
INT totalkeys = 0;			/* Total bytes input		*/
#endif

INT refresh(INT f, INT n);


/*
 * This function gets called once, to set up the terminal channel.
 * This version turns off flow control. This may be wrong for your
 * system, but no good solution has really been found (daveb).
 */

void
ttopen()
{
	int i;

	if( !ttysavedp )
	{
		if (tcgetattr(0, &ot) < 0)
			panic("ttopen: tcgetattr", errno);
		nt = ot;		/* save entry state		*/
		for (i = 0; i < NCCS; i++) nt.c_cc[i] = 0;
		nt.c_cc[VMIN] = 1;	/* one character read is OK	*/
		nt.c_cc[VTIME] = 0;	/* Never time out.		*/
		nt.c_iflag |= IGNBRK;
		nt.c_iflag &= ~( ICRNL | INLCR | IGNCR | ISTRIP | IXON | IXOFF );
		nt.c_oflag &= ~OPOST;
		nt.c_cflag |= CS8;	/* allow 8th bit on input	*/
		nt.c_cflag &= ~PARENB;	/* Don't check parity		*/
		nt.c_lflag &= ~( ECHO | ICANON | ISIG | IEXTEN );

		ttysavedp = TRUE;
	}

	if (tcsetattr(0, TCSADRAIN, &nt) < 0)
		panic("ttopen: tcsetattr", errno);

	ttyactivep = TRUE;
}


/*
 * This function gets called just before we go back home to the shell.
 * Put all of the terminal parameters back.
 */

void
ttclose()
{
	if(!ttysavedp || !ttyactivep)
		return;
	ttflush();
	if (tcsetattr(0, TCSADRAIN, &ot) < 0)
		panic("ttclose: tcsetattr", errno);
	ttyactivep = FALSE;
}


/*
 * Mg3a: set/clear SIGINT on ^G. Return success status.
 */

int
setctrlg_interrupt(int on)
{
	struct termios gnt;

	if (on) {
		gnt = nt;
		gnt.c_cc[VINTR] = CCHR('G');
		gnt.c_lflag |= ISIG;
		return !tcsetattr(0, TCSANOW, &gnt);
	} else {
		return !tcsetattr(0, TCSANOW, &nt);
	}
}


/*
 * Write character to the display. Characters are buffered up, to make
 * things a little bit more efficient.
 */

void
ttputc(INT c)
{
	/* Work around cygwin term bug 	*/
	/* Possibly good to never flush in the middle of UTF-8 character */

	if ((nobuf >= NOBUF-3 && (c & 0xc0) != 0x80) || nobuf >= NOBUF)
		ttflush();

#ifdef SLOW
	if (slowspeed > 0) slowdown(c);		// May also flush
#endif

	obuf[nobuf++] = c;

#ifdef CHARSDEBUG
	charsoutput++;
#endif
}


/*
 * Flush output.
 */

void
ttflush()
{
	ssize_t s;
	char *buf = obuf;

	while (nobuf > 0) {
		s = write(1, buf, nobuf);

		if (s == -1) {
			if (errno == EINTR) continue;

			nobuf = 0;
			panic("Error writing screen", errno);
		}

		buf += s;
		nobuf -= s;
	}
}


/*
 * Mg3a: This is to fit the signature of tputs(). It might work
 * anyway, but for safety's sake.
 */

int
ittputc(int c)
{
	ttputc(c);
	return 0;
}


/*
 * Read character from terminal. All 8 bits are returned, so that you
 * can use a multi-national terminal.
 *
 * Mg3a: New dealing with typeahead. Deal with window change. Deal
 * severely with read errors other than EINTR (which is not an error).
 * using type_ahead instead of typeahead to avoid possible collisions
 * with ncurses
 */

INT
ttgetc()
{
	extern volatile sig_atomic_t winchanged;
	char c;
	ssize_t ret;

	while (1) {
		while (winchanged) {
			winchanged = 0;
			refresh(0, 1);
			update();
		}

		ret = read(0, &c, 1);

		if (ret == 1) {
			break;
		} else if (ret == 0) {
			panic("Stdin EOF", 0);
		} else if (ret == -1 && errno != EINTR) {
			panic("Stdin I/O Error", errno);
		}
	}

#ifdef CHARSDEBUG
	totalkeys++;
#endif
	return c & 0xff;
}


/*
 * Return non-FALSE if typeahead is pending.
 *
 * Mg3a: Stolen ioctl version from Portable Mg. So this doesn't work
 * on Cygwin, but it should be reliable.
 *
 * Using select() may be better. We will see.
 */

int
type_ahead()
{
#if SELECT_FOR_TYPEAHEAD
	fd_set readset;
	struct timeval t;

	FD_ZERO(&readset);
	FD_SET(0, &readset);
	t.tv_sec = 0;
	t.tv_usec = 0;
	return select(1, &readset, NULL, NULL, &t) > 0;
#else
	int	x;

#ifdef FIONREAD
	return ((ioctl(0, FIONREAD, &x) < 0) ? 0 : x);
#else   /* For platforms that don't have FIONREAD */
	return ((ioctl(0, TIOCINQ, &x) < 0) ? 0 : x);
#endif
#endif
}


/*
 * Mg3a: Wait for input for a while. Return true if there was any.
 */

int
waitforinput(INT milliseconds)
{
	fd_set readset;
	struct timeval t;

	FD_ZERO(&readset);
	FD_SET(0, &readset);
	t.tv_sec = milliseconds / 1000;
	t.tv_usec = (milliseconds - t.tv_sec*1000) * 1000;
	return select(1, &readset, NULL, NULL, &t) > 0;
}


/*
 * Mg3a: Try to reset the tty channel, then quick and rude exit.
 * Report errno.
 */

void
panic(char *s, int error)
{
	static int panicking = 0;

	if (panicking) return;
	panicking = 1;

	ttclose();

	fprintf(stderr, "\r\n********************\r\n");
	fprintf(stderr, "mg panic: %s  \r\n", s);
	if (error) fprintf(stderr, "errno = %d, %s  \r\n", error, strerror(error));
	exit(1);
}


/*
 * This should check the size of the window, and reset if needed.
 */

void
setttysize()
{
#ifdef	TIOCGWINSZ
	if (ioctl(0, TIOCGWINSZ, (char *) &winsize) == 0) {
		nrow = winsize . ws_row;
		ncol = winsize . ws_col;
	} else
#endif
	if ((nrow=tgetnum ("li")) <= 0
	|| (ncol=tgetnum ("co")) <= 0) {
		nrow = 24;
		ncol = 80;
	}
}


#ifdef SLOW

/*
 * Mg3a: Set or clear the emulation of a slow terminal.
 */

INT
slowmode(INT f, INT n)
{
	char buf[20];
	INT speed;
	int ok = 1;

	if (f & FFARG) {
		speed = n;
	} else {
		if (ereply("Baud rate: ", buf, sizeof(buf)) == ABORT) return ABORT;
		ok = getINT(buf, &speed, 0);
	}

	if (ok && speed > 0) {
		if (speed < 70) {
			ewprintf("Minimum speed is 70 baud");
			return FALSE;
		}

		slowspeed = speed;
	} else {
		slowspeed = -1;
		ewprintf("Slow mode cleared");
	}

	upmodes(NULL);
	return TRUE;
}
#endif
