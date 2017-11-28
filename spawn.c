/*
 * Spawn.
 */

#include	"def.h"

#include	<signal.h>
#include	<sys/stat.h>
#include	<fcntl.h>

/*
 * Mg3a: And finally, an attempt at suspending and resuming. ttinit(),
 * called from vtinit(), has been adapted for the restart.
 */

INT
spawncli(INT f, INT n)
{
	sigset_t oldset;

	vttidy();

	sigprocmask(SIG_SETMASK, NULL, &oldset);
	kill(0, SIGTSTP);
	sigprocmask(SIG_SETMASK, &oldset, NULL);

	vtinit();

	return TRUE;
}


/*
 * Mg3a: paraphernalia to allow interrupting subprocess
 */

static volatile sig_atomic_t subprocess_alive = 0;
static pid_t subprocess_pid;

static void
sigint(int dummy)
{
	if (subprocess_alive) {
		if (subprocess_alive == 1) {
			kill(subprocess_pid, SIGINT);
			subprocess_alive = 2;
		} else {
			kill(subprocess_pid, SIGKILL);
			subprocess_alive = 0;
		}
	}
}


/*
 * Mg3a: Do the setup so that we can interrupt subprocess. Return
 * success status when turning on interrupts.
 */

static int
set_subprocess_alive(int on)
{
	struct sigaction sa;

	if (on) {
		subprocess_alive = 1;

		sa.sa_handler = sigint;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;	// Don't break pipe IO

		if (sigaction(SIGINT, &sa, NULL) != 0 || !setctrlg_interrupt(1)) return 0;
	} else {
		setctrlg_interrupt(0);

		sa.sa_handler = SIG_DFL;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGINT, &sa, NULL);

		subprocess_alive = 0;
	}

	return 1;
}


/*
 * Mg3a: shell with override
 */

static char shellfile[NFILEN] = "";

static char *
getshell()
{
	char *cp;

	if (shellfile[0]) return shellfile;
	else if ((cp = getenv("SHELL"))) return cp;
	else return "/bin/sh";
}

INT
setshell(INT f, INT n)
{
	char input[NFILEN];

	if (eread("Set shell: ", input, sizeof(input), EFFILE) == ABORT) return ABORT;

	stringcopy(shellfile, input, sizeof(shellfile));

	if (input[0]) {
		ewprintf("Shell set to %s", input);
	} else {
		ewprintf("Shell reverted to %s", getshell());
	}

	return TRUE;
}


/*
 * Mg3a: Do this to replace popen() to redirect stderr.
 */

FILE *
pipe_open(char *cmd)
{
	int fds[2], infd;
	FILE *f;
	struct sigaction sa;
	char *shell = getshell();

	if (pipe(fds) < 0) {
		ewprintf("Error opening pipe: %s", strerror(errno));
		return NULL;
	}

	// Ignore children

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGCHLD, &sa, NULL);

	switch ((subprocess_pid = fork())) {
	case -1:
		ewprintf("Fork error: %s", strerror(errno));
		return NULL;
	case 0:
		if (	dup2(fds[1], 2) < 0 || dup2(fds[1], 1) < 0 ||
			close(fds[0]) < 0 || (infd = open("/dev/null", O_RDONLY)) < 0 ||
			dup2(infd, 0) < 0 || chdir(dirnameofbuffer(curbp)) < 0)
		{
			panic("Error in subprocess", errno);
		}

		if (execl(shell, shell, "-c", cmd, NULL) < 0) {
			printf("(Couldn't execute shell %s: %s)\n", shell, strerror(errno));
			exit(1);
		}

		return NULL;
	default:
		if ((f = fdopen(fds[0], "r")) == NULL || close(fds[1]) < 0) {
			ewprintf("Error modifying pipe: %s", strerror(errno));
			if (f) fclose(f); else close(fds[1]);
			return NULL;
		}

		return f;
	}
}


/*
 * Mg3a: Inspired by POSIX getline()
 *
   - Must be called with *lineptr == NULL first time.
   - errno is not explicitly set.
   - If maxlen > SSIZE_MAX, max SSIZE_MAX bytes are returned anyway.
   - Out of memory is reported by a return value.
   - Can only be used on one stream at a time.
   - feof() is not reliable. You have to check the return value and
     see that ferror() isn't set.
 */

#define GETLINE_EOF		(-1)
#define GETLINE_OUTOFMEM	(-2)

ssize_t
mg_getline(char **lineptr, size_t *n, size_t maxlen, FILE *stream)
{
	char *line = NULL, *line2, *nl;
	size_t len = 0, size = 0, copylen, newsize;
	static char buf[BUFSIZ];
	static size_t bufpos, buflen;
	int returnresult = 0;

	if (!*lineptr) {
		bufpos = 0;
		buflen = 0;
		size = 2*BUFSIZ;
		line = malloc(size);
		if (!line) goto outofmemory;
	} else {
		line = *lineptr;
		size = *n;
	}

	while (1) {
		if (bufpos == buflen) {
			buflen = fread(buf, 1, sizeof(buf), stream);
			bufpos = 0;

			if (buflen == 0) break;	// EOF or error
		}

		copylen = buflen - bufpos;

		if (len + copylen > maxlen) {
			copylen = maxlen - len;
			returnresult = 1;
		}

		nl = memchr(buf + bufpos, '\n', copylen);

		if (nl) {
			copylen = nl - buf - bufpos + 1;
			returnresult = 1;
		}

		if (len + copylen >= size) {
			if (len + copylen >= SSIZE_MAX) {
				goto outofmemory;
			} else if (size <= BUFSIZ) {
				newsize = 2*BUFSIZ;
			} else if (size > SSIZE_MAX - size) {
				newsize = SSIZE_MAX;
			} else {
				newsize = size + size;
			}

			line2 = realloc(line, newsize);
			if (!line2) goto outofmemory;

			line = line2;
			size = newsize;
		}

		memcpy(line + len, buf + bufpos, copylen);
		len += copylen;
		bufpos += copylen;

		if (returnresult) break;
	}

	line[len] = 0;
	*lineptr = line;
	*n = size;
	if (len == 0) return GETLINE_EOF;
	return len;

outofmemory:
	free(line);		// Note that line is the current one, not *lineptr
	*lineptr = NULL;
	*n = 0;
	return GETLINE_OUTOFMEM;
}


/* Limit on shell command data */

INT shell_command_limit = 10000000;


/*
 * Mg3a: As Emacs "shell-command" without asynch
 */

INT
shellcommand(INT f, INT n)
{
	char cmd[EXCLEN], *line = NULL;
	INT s, savedoffset = 0, len, ret = FALSE, yankp, inthisbuffer;
	FILE *pipef;
	int first = 1, nl = 0;
	BUFFER *bp = NULL;
	size_t pos = 0, linesize = 0, range;
	LINE *prevline = NULL, *lp;
	const INT overhead = sizeof(LINE) + sizeof(void*);
	INT budget;

	if ((s = ereply("Shell command: ", cmd, sizeof(cmd))) != TRUE) return s;

	ttmove(nrow - 1, 0); ttflush();	/* Like Emacs */

	if ((pipef = pipe_open(cmd)) == NULL) return FALSE;

	yankp = (f & FFRAND);
	inthisbuffer = (f & FFARG) || yankp;

	if (inthisbuffer) {
		u_modify();

		if (lnewline_noundo() == FALSE) {
			fclose(pipef);
			return FALSE;
		}

		endofpreviousline();

		isetmark();
		prevline = lback(curwp->w_dotp);
		savedoffset = curwp->w_doto;
		pos = curwp->w_dotpos;
	} else {
		if ((bp = bfind("*Shell Command Output*", TRUE)) == NULL) {
			fclose(pipef);
			return FALSE;
		}

		resetbuffer(bp);
		u_clear(bp);
		bp->b_flag &= ~BFCHG;
		bclear(bp);
	}

	if (!set_subprocess_alive(1)) {
		ewprintf("Failed to set signal handler: %s", strerror(errno));
		goto error;
	}

	budget = shell_command_limit;

	while (1) {
		if (budget <= overhead) {
			ewprintf("Data overflowing \"shell-command-limit\"");
			goto error;
		}

		len = mg_getline(&line, &linesize, budget - overhead, pipef);

		if (len == GETLINE_OUTOFMEM) {
			ewprintf("Out of memory");
			goto error;
		} else if (len < 0) {
			break;
		}

		budget -= len + overhead;

		nl = 0;

		if (len && line[len - 1] == '\n') {
			nl = 1;
			len--;
			if (len && line[len - 1] == '\r') len--;
		}

		if (inthisbuffer) {
			if (first) {
				if (linsert_str_noundo(line, len) == FALSE) goto error;
				first = 0;
			} else {
				if (laddline(line, len) == FALSE) goto error;
			}
		} else {
			if ((lp = lalloc(len)) == NULL) goto error;
			memcpy(ltext(lp), line, len);
			addlast(bp, lp);
		}
	}

	ret = TRUE;

error:
	free(line);
	set_subprocess_alive(0);
	fclose(pipef);

	if (inthisbuffer) {
		if (nl && !yankp) adjustpos(lforw(curwp->w_dotp), 0);
		else ldelnewline_noundo();

		if ((range = curwp->w_dotpos - pos) > MAXINT) {
			u_clear(curbp);
		} else if (!u_entry_range(UFL_INSERT, lforw(prevline), savedoffset, range, pos)) {
			ret = FALSE;
		}

		if (!yankp) {
			isetmark();
			position(pos);
		}
	} else {
		// setcharsetfromcontent(bp);	// ?
		if (popbuftop(bp) == FALSE) return FALSE;
	}

	return ret;
}


/*
 * Mg3a: A variant of "shell-command". Not in Emacs.
 */

INT
yank_process(INT f, INT n)
{
	return shellcommand(f | FFRAND, n);
}
