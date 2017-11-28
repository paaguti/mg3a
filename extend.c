/*
 *	Extended (M-X) commands, rebinding, and
 *	startup file processing.
 */

#include	"def.h"
#include	"kbd.h"
#include	"macro.h"
#include	"key.h"

static 	char 	*skipwhite(char *s);
static	char	*parsetoken(char *s);
static	void	fixmap(KEYMAP *curmap, KEYMAP *mp, KEYMAP *mt);
static	KEYMAP	*realocmap(KEYMAP *curmap);
static	INT	excline(char *line);

INT		rescan(INT f, INT n);
INT		evalexpr(INT f, INT n);

INT		toplevel_undo_enabled;		// Undo enabled at top level


/*
 * Mg3a: Function to allow general unbinding, otherwise just returns
 * FALSE.
 */

INT
nil(INT f, INT n)
{
	return FALSE;
}


/* Previous function interactively */

PF prevfunc = NULL;

/*
 * Mg3a: Do this to check for macros. A negative status return is an
 * index to a macro to execute.
 */

INT
execute(PF pf, INT f, INT n)
{
	INT status;

	if (pf == rescan) return rescan(f, n);

	// UNDO
	if (!inmacro) u_boundary = 1;
	toplevel_undo_enabled = undo_enabled;
	//

	status = (*pf)(f, n);

#ifdef USER_MACROS
	if (status < 0) status = func_macro_execute(-status-1, n);
#endif
	if (!inmacro) prevfunc = pf;

	return status;
}


/*
 * Mg3a: help function for insert() which deals with overwrite mode
 */

static INT
insert_string(INT n, char *str, INT len)
{
	INT c, i;

	if (n < 0) return FALSE;

	// Check for only one char, which may happen with repeat count
	// when macrorecording in selfinsert().

	c = ucs_strtochar(termcharset, str, len, 0, &i);

	if (c >= 0 && i == len) return linsert_overwrite(curbp->charset, n, c);

	return linsert_overwrite_general_string(termcharset, n, str, len);
}


/*
 * Insert a string, mainly for use from macros (created by selfinsert)
 */

INT
insert(INT f, INT n)
{
    char buf[NFILEN];
    INT s;

#ifndef NO_MACRO
    if (inmacro) {
    	if (outofarguments(1)) return ABORT;
	s = insert_string(n, ltext(maclcur), llength(maclcur));
	maclcur = maclcur->l_fp;
	return s;
    }
    if (n == 1) thisflag |= CFINS; /* CFINS means selfinsert can tack on end */
#endif
    if ((s = ereply("Insert: ", buf, sizeof(buf))) != TRUE) return s;
    return insert_string(n, buf, strlen(buf));
}


/*
 * Bind a key to a function. Cases range from the trivial (replacing
 * an existing binding) to the extremly complex (creating a new prefix
 * in a map_element that already has one, so the map_element must be
 * split, but the keymap doesn't have enough room for another
 * map_element, so the keymap is reallocated). No attempt is made to
 * reclaim space no longer used, if this is a problem flags must be
 * added to indicate malloced verses static storage in both keymaps
 * and map_elements. Structure assignments would come in real handy,
 * but K&R based compilers don't have them. Care is taken so running
 * out of memory will leave the keymap in a usable state.
 */

static INT
remap(KEYMAP *curmap, INT c, PF funct, KEYMAP *pref_map)
//KEYMAP	*curmap;	/* pointer to the map being changed */
//INT		c;		/* character being changed */
//PF		funct;		/* function being changed to */
//KEYMAP	*pref_map;	/* if funct==prefix, map to bind to or NULL for new */
/* extern MAP_ELEMENT *ele;	must be set before calling */
{
	INT	i;
	INT	n1, n2, nold;
	KEYMAP	*mp;
	PF	*pfp;
	MAP_ELEMENT *mep;

	if (funct == nil) funct = rescan;	/* Mg3a: for undefining key */

	if(ele >= &curmap->map_element[curmap->map_num] || c < ele->k_base) {
	    if(ele > &curmap->map_element[0] && (funct!=prefix ||
			(ele-1)->k_prefmap==NULL)) {
		n1 = c - (ele-1)->k_num;
	    } else n1 = HUGE;
	    if(ele < &curmap->map_element[curmap->map_num] && (funct!=prefix ||
			ele->k_prefmap==NULL)) {
		n2 = ele->k_base - c;
	    } else n2 = HUGE;
	    if(n1 <= MAPELEDEF && n1 <= n2) {
		ele--;
		if((pfp = (PF *)malloc((UINT)(c - ele->k_base+1)
			* sizeof(PF))) == NULL) {
		    ewprintf("Out of memory");
		    return FALSE;
		}
		nold = ele->k_num - ele->k_base + 1;
		for(i=0; i < nold; i++)
		    pfp[i] = ele->k_funcp[i];
		while(--n1) pfp[i++] = curmap->map_default;
		pfp[i] = funct;
		ele->k_num = c;
		ele->k_funcp = pfp;
	    } else if(n2 <= MAPELEDEF) {
		if((pfp = (PF *)malloc((UINT)(ele->k_num - c + 1)
			* sizeof(PF))) == NULL) {
		    ewprintf("Out of memory");
		    return FALSE;
		}
		nold = ele->k_num - ele->k_base + 1;
		for(i=0; i < nold; i++)
		    pfp[i+n2] = ele->k_funcp[i];
		while(--n2) pfp[n2] = curmap->map_default;
		pfp[0] = funct;
		ele->k_base = c;
		ele->k_funcp = pfp;
	    } else {
		if(curmap->map_num >= curmap->map_max &&
		    (curmap = realocmap(curmap)) == NULL) return FALSE;
		if((pfp = (PF *)malloc(sizeof(PF))) == NULL) {
		    ewprintf("Out of memory");
		    return FALSE;
		}
		pfp[0] = funct;
		for(mep = &curmap->map_element[curmap->map_num]; mep > ele; mep--) {
		    mep->k_base    = (mep-1)->k_base;
		    mep->k_num     = (mep-1)->k_num;
		    mep->k_funcp   = (mep-1)->k_funcp;
		    mep->k_prefmap = (mep-1)->k_prefmap;
		}
		ele->k_base = c;
		ele->k_num = c;
		ele->k_funcp = pfp;
		ele->k_prefmap = NULL;
		curmap->map_num++;
	    }
	    if(funct == prefix) {
		if(pref_map != NULL) {
		    ele->k_prefmap = pref_map;
		} else {
		    if((mp = (KEYMAP *)malloc(sizeof(KEYMAP) +
			    (MAPINIT-1)*sizeof(MAP_ELEMENT))) == NULL) {
			ewprintf("Out of memory");
			ele->k_funcp[c - ele->k_base] = curmap->map_default;
			return FALSE;
		    }
		    mp->map_num = 0;
		    mp->map_max = MAPINIT;
		    mp->map_default = rescan;
		    ele->k_prefmap = mp;
		}
	    }
	} else {
	    n1 = c - ele->k_base;
	    if(ele->k_funcp[n1] == funct && (funct!=prefix || pref_map==NULL ||
		    pref_map==ele->k_prefmap))
		return TRUE;	/* no change */
	    if(funct!=prefix || ele->k_prefmap==NULL) {
		if(ele->k_funcp[n1] == prefix)
		    ele->k_prefmap = (KEYMAP *)NULL;
		ele->k_funcp[n1] = funct;	/* easy case */
		if(funct==prefix) {
		    if(pref_map!=NULL)
			ele->k_prefmap = pref_map;
		    else {
			if((mp = (KEYMAP *)malloc(sizeof(KEYMAP) +
				(MAPINIT-1)*sizeof(MAP_ELEMENT))) == NULL) {
			    ewprintf("Out of memory");
			    ele->k_funcp[c - ele->k_base] = curmap->map_default;
			    return FALSE;
			}
			mp->map_num = 0;
			mp->map_max = MAPINIT;
			mp->map_default = rescan;
			ele->k_prefmap = mp;
		    }
		}
	    } else {
		/* this case is the splits */
		/* determine which side of the break c goes on */
		/* 0 = after break; 1 = before break */
		n2 = 1;
		for(i=0; n2 && i < n1; i++)
			n2 &= ele->k_funcp[i] != prefix;
		if(curmap->map_num >= curmap->map_max &&
		    (curmap = realocmap(curmap)) == NULL) return FALSE;
		if((pfp = (PF *)malloc((UINT)(ele->k_num - c + !n2)
			* sizeof(PF))) == NULL) {
		    ewprintf("Out of memory");
		    return FALSE;
		}
		ele->k_funcp[n1] = prefix;
		for(i=n1+n2; i <= ele->k_num - ele->k_base; i++)
		    pfp[i-n1-n2] = ele->k_funcp[i];
		for(mep = &curmap->map_element[curmap->map_num]; mep > ele; mep--) {
		    mep->k_base    = (mep-1)->k_base;
		    mep->k_num     = (mep-1)->k_num;
		    mep->k_funcp   = (mep-1)->k_funcp;
		    mep->k_prefmap = (mep-1)->k_prefmap;
		}
		ele->k_num = c - !n2;
		(ele+1)->k_base = c + n2;
		(ele+1)->k_funcp = pfp;
		ele += !n2;
		ele->k_prefmap = NULL;
		curmap->map_num++;
		if(pref_map == NULL) {
		    if((mp = (KEYMAP *)malloc(sizeof(KEYMAP) +
			    (MAPINIT-1)*sizeof(MAP_ELEMENT))) == NULL) {
			ewprintf("Out of memory");
			ele->k_funcp[c - ele->k_base] = curmap->map_default;
			return FALSE;
		    }
		    mp->map_num = 0;
		    mp->map_max = MAPINIT;
		    mp->map_default = rescan;
		    ele->k_prefmap = mp;
		} else ele->k_prefmap = pref_map;
	    }
	}
	return TRUE;
}


/* reallocate a keymap, used above */

static KEYMAP *
realocmap(KEYMAP *curmap)
{
    KEYMAP *mp;
    INT i;
    extern INT nmaps;

    if((mp = (KEYMAP *)malloc((UINT)(sizeof(KEYMAP)+
	    (curmap->map_max+(MAPGROW-1))*sizeof(MAP_ELEMENT)))) == NULL) {
	ewprintf("Out of memory");
	return NULL;
    }
    mp->map_num = curmap->map_num;
    mp->map_max = curmap->map_max + MAPGROW;
    mp->map_default = curmap->map_default;
    for(i=curmap->map_num; i--; ) {
	mp->map_element[i].k_base	= curmap->map_element[i].k_base;
	mp->map_element[i].k_num	= curmap->map_element[i].k_num;
	mp->map_element[i].k_funcp	= curmap->map_element[i].k_funcp;
	mp->map_element[i].k_prefmap	= curmap->map_element[i].k_prefmap;
    }
    for(i=nmaps; i--; ) {
	if(map_table[i].p_map == curmap) map_table[i].p_map = mp;
	else fixmap(curmap, mp, map_table[i].p_map);
    }
    ele = &mp->map_element[ele - &curmap->map_element[0]];
    return mp;
}


/* fix references to a reallocated keymap (recursive) */

static void
fixmap(KEYMAP *curmap, KEYMAP *mp, KEYMAP *mt)
{
    INT i;

    for(i = mt->map_num; i--; ) {
	if(mt->map_element[i].k_prefmap != NULL) {
	    if(mt->map_element[i].k_prefmap == curmap)
	    	mt->map_element[i].k_prefmap = mp;
	    else fixmap(curmap, mp, mt->map_element[i].k_prefmap);
	}
    }
}


/*
 * Do the input for local-set-key, global-set-key and define-key then
 * call remap to do the work.
 */

static INT
dobind(KEYMAP *curmap, char *p, INT unbind)
{
	PF	funct;
	char	prompt[(MAXSTRKEY+1)*MAXKEY+80];
	char	cmdmap[EXCLEN];
	char	*pep;
	INT	c;
	INT	s;
	KEYMAP	*pref_map = NULL;
	ucs4	temp[MAXKEY];
	INT	i, len;

	if (curmap == NULL) panic("dobind", 0);
	if (macrodef) {
	/* keystrokes arn't collected.	Not hard, but pretty useless */
	/* would not work for function keys in any case */
	    return emessage(ABORT, "Can't rebind key in macro");
	}
	if (inmacro) {
	    i = 0;
	    len = 0;
	    while (i < llength(maclcur)) {
		c = ucs_char(termcharset, maclcur, i, &i);
		if (c < 0 || len >= MAXKEY) return FALSE;
		temp[len++] = c;
	    }
	    for(s=0; s < len - 1; s++) {
		if(doscan(curmap, c=temp[s]) != prefix) {
		    if(remap(curmap, c, prefix, (KEYMAP *)NULL) != TRUE) {
			return FALSE;
		    }
		}
		curmap = ele->k_prefmap;
	    }
	    doscan(curmap, c=temp[s]);
	    maclcur = maclcur->l_fp;
	} else {
	    stringcopy(prompt, p, 80);
	    pep = prompt + strlen(prompt);
	    for(;;) {
		ewprintf("%s", prompt);
		pep[-1] = ' ';
		if (pep >= prompt + sizeof(prompt) - 1) {
			ewprintf("Max Key count reached");
			return FALSE;
		}
		pep = keyname(pep, c = getkey(FALSE));
		if(doscan(curmap,c) != prefix) break;
		*pep++ = '-';
		*pep = '\0';
		curmap = ele->k_prefmap;
	    }
	}
	if(unbind) {
		ewprintf("%s", prompt);
		funct = rescan;
	} else {
#ifdef USER_MACROS
	    if ((s=eread("%s to command: ", cmdmap, sizeof(cmdmap),
			EFFUNC|EFMACRO|EFNOSTRICTCR|EFNOSTRICTSPC, prompt)) != TRUE) return s;
    	    if ((funct = name_or_macro_to_key_function(cmdmap)) == NULL) {
		// More detailed message in called function
		return FALSE;
	    }
#else
	    if ((s=eread("%s to command: ", cmdmap, sizeof(cmdmap), EFFUNC, prompt))
			!= TRUE) return s;
    	    if ((funct = name_function(cmdmap)) == NULL) {
		ewprintf("[No match]");
		return FALSE;
	    }
#endif
	}
	return remap(curmap, c, funct, pref_map);
}


/*
 * Mg3a: Do assignments of long key sequences internally from a string
 */

INT
internal_bind(MODE mode, char *str, PF funct)
{
	INT	c;
	INT	i;
	INT	len;
	KEYMAP	*curmap;

	if (!str || str == (char *)-1 || !*str || invalid_mode(mode)) return FALSE;

	curmap = mode_map(mode);
	len = strlen(str);

	for (i = 0; i < len - 1; i++) {
		c = CHARMASK(str[i]);

		if (doscan(curmap, c) != prefix) {
			if (remap(curmap, c, prefix, NULL) != TRUE) {
				return FALSE;
			}
		}

		curmap = ele->k_prefmap;
	}

	c = CHARMASK(str[len - 1]);
	doscan(curmap, c);

	return remap(curmap, c, funct, NULL);
}



/*
 * This function modifies the fundamental keyboard map.
 */

INT
bindtokey(INT f, INT n)
{
    return dobind(mode_map(0), "Global set key: ", FALSE);
}


/*
 * This function modifies the current mode's keyboard map.
 */

INT
localbind(INT f, INT n)
{
    return dobind(mode_map(curbp->b_modes[curbp->b_nmodes]), "Local set key: ",
	FALSE);
}


/*
 * This function redefines a key in any keymap.
 */

INT
define_key(INT f, INT n)
{
	char *prompt = "Define key map: ";
	char promptbuf[80+MODENAMELEN], name[MODENAMELEN];
	KEYMAP *map;
	INT s;

	if ((s = eread(prompt, name, sizeof(name), EFKEYMAP)) != TRUE) return s;

	if ((map = name_map(name)) == NULL) {
		ewprintf("Unknown map %s", name);
		return FALSE;
	}

	snprintf(promptbuf, sizeof(promptbuf), "%s%s key: ", prompt, name);

	return dobind(map, promptbuf, FALSE);
}


INT
unbindtokey(INT f, INT n)
{
    return dobind(mode_map(0), "Global unset key: ", TRUE);
}


INT
localunbind(INT f, INT n)
{
    return dobind(mode_map(curbp->b_modes[curbp->b_nmodes]), "Local unset key: ",
	TRUE);
}


/*
 * Mg3a: Chew prefix on a command line
 */

static void
chewprefix(INT *fp, INT *np)
{
	char buf[80], *str;
	INT len;

	if (inmacro && !inkbdmacro && !outofarguments(0)) {
		len = llength(maclcur);
		str = ltext(maclcur);

		if (len > 0 && len < (INT)sizeof(buf)) {
			if ((*str >= '0' && *str <= '9') || *str == '-') {
				memcpy(buf, str, len);
				buf[len] = 0;

				if (getINT(buf, np, 0)) {
					maclcur = lforw(maclcur);
					*fp = FFARG;
				}
			}
		}
	}
}


/*
 * Extended command. Call the message line routine to read in the
 * command name and apply autocompletion to it. When it comes back,
 * look the name up in the symbol table and run the command if it is
 * found. Print an error if there is anything wrong.
 */

INT
extend(INT f, INT n)
{
	PF	funct;
	INT	s;
	char	xname[NXNAME];
#ifdef USER_MACROS
	char *commands;
#endif

	if (f & FFARG) {
		s = eread("%d M-x ", xname, NXNAME, EFFUNC|EFMACRO, n);
	} else {
		s = eread("M-x ", xname, NXNAME, EFFUNC|EFMACRO);
	}

	if (s != TRUE) {
		if (s == FALSE) ewprintf("No command name given");
		return s;
	}

	chewprefix(&f, &n);

	if ((funct = name_function(xname)) != NULL) {
		if (macrodef) {
			LINE *lp = maclcur;
			macro[macrocount-1].m_funct = funct;
			maclcur = lback(lp);
			removefromlist(lp);
			free(lp);
		}

		return (*funct)(f, n);
	}

#ifdef USER_MACROS
	if ((commands = lookup_named_macro(xname)) != NULL) {
		return excline_copy_list_n(commands, n);
	}
#endif
	ewprintf("[No match]");
	return FALSE;
}


/*
 * Mg3a: Ignore errors of a command.
 */

INT
ignore_errors(INT f, INT n)
{
	if (macrodef) {
		ewprintf("Can't ignore errors in keyboard macro");
		return FALSE;
	}

	if (!inmacro) return TRUE;

	extend(f, n);
	return TRUE;
}


/*
 * Define the commands needed to do startup-file processing. This code
 * is mostly a kludge just so we can get startup-file processing.
 *
 * If you're serious about having this code, you should rewrite it.
 * To wit:
 *	It has lots of funny things in it to make the startup-file
 *	look like a GNU startup file; mostly dealing with parens and
 *	semicolons. This should all vanish.
 *
 * We define eval-expression because it's easy. It can make *-set-key
 * or define-key set an arbitrary key sequence, so it isn't useless.
 */


/*
 * Mg3a: line too long to execute
 */

static INT
overlong_line(INT maxlen)
{
	ewprintf("Error: line longer than %d bytes", maxlen-1);
	return FALSE;
}


/*
 * evalexpr - get one line from the user, and run it.
 *
 * Mg3a: evaluates a list with a repeat count. If executed
 * interactively, don't hide messages.
 */

INT
evalexpr(INT f, INT n)
{
	INT	s;
	char	excbuf[EXCLEN];

	if (f & FFARG) {
		if ((s = ereply("%d Eval: ", excbuf, sizeof(excbuf), n)) != TRUE) return s;
	} else {
		if ((s = ereply("Eval: ", excbuf, sizeof(excbuf))) != TRUE) return s;
	}

	macro_show_message++;		// Show messages if macro_show_message was 1
	s = excline_copy_list_n(excbuf, n);
	macro_show_message--;

	return s;
}


/*
 * evalbuffer - evaluate the current buffer as line commands. Useful
 * for testing startup files.
 *
 * Mg3a: Make a copy of the buffer. Make it behave more like in Emacs.
 */

INT
evalbuffer(INT f, INT n)
{
	BUFFER	*bp = curbp;
	LINE	*lp1, *lp2, *list;
	INT	len, ret = FALSE, s;
	char	bufname[NFILEN];
	size_t	line = 0;

	if (inmacro && !inkbdmacro && !outofarguments(0)) {
		if ((s = eread("Eval Buffer: ", bufname, sizeof(bufname), EFBUF)) != TRUE) {
			return s;
		}

		if ((bp = bfind(bufname, FALSE)) == NULL) {
			return buffernotfound(bufname);
		}
	}

	if ((list = make_empty_list(NULL)) == NULL) return FALSE;

	for (lp1 = lforw(bp->b_linep); lp1 != bp->b_linep; lp1 = lforw(lp1)) {
		len = llength(lp1);
		if ((lp2 = lalloc(len + 1)) == NULL) goto cleanup;
		memcpy(ltext(lp2), ltext(lp1), len);
		lputc(lp2, len, 0);
		insertlast(list, lp2);
	}


	for (lp1 = lforw(list); lp1 != list; lp1 = lforw(lp1)) {
		line++;

		if ((s = excline(ltext(lp1))) != TRUE) {
			if (macro_show_message == 1 && s != SILENTERR) {
				ewprintf("Error in line %z", line);
			}

			goto cleanup;
		}
	}

	ret = TRUE;

cleanup:
	make_empty_list(list);
	free(list);

	return ret;
}


/*
 * evalfile - go get a file and evaluate it as line commands. You can
 * go get your own startup file if need be.
 */

INT
evalfile(INT f, INT n)
{
	INT	s;
	char		fname[NFILEN], *adjfname;

	if ((s = eread("Load file: ", fname, NFILEN, EFFILE)) != TRUE)
		return s;

	if ((adjfname = adjustname(fname)) == NULL) return FALSE;

	if (!fileexists(adjfname)) {
		ewprintf("(File not found)");
		return FALSE;
	} else if (isdirectory(adjfname)) {
		ewprintf("(File is a directory)");
		return FALSE;
	} else if (existsandnotreadable(adjfname)) {
		ewprintf("File is not readable: %s", adjfname);
		return FALSE;
	}

	return load(fname);
}


/*
 * load - go load the file name we got passed.
 *
 * Mg3a: Use ordinary file ops. Support recursion.
 */

INT
load(char *infilename)
{
	INT	nbytes, s;
	char	excbuf[EXCLEN + 4], fname[NFILEN], *adjf;
	FILE	*f;
	size_t	lineno = 0;

	if ((adjf = adjustname(infilename)) == NULL)
		return FALSE;	/* just to be careful */

	stringcopy(fname, adjf, NFILEN);	// Save for recursion

	if ((f = fopen(fname, "r")) == NULL) {
		ewprintf("Cannot open file for reading: %s", fname);
		return FALSE;
	}

	while (fgets(excbuf, sizeof(excbuf), f)) {
		lineno++;
		nbytes = strlen(excbuf);

		if (nbytes && excbuf[nbytes - 1] == '\n') {
			excbuf[--nbytes] = 0;
			if (nbytes && excbuf[nbytes - 1] == '\r') excbuf[--nbytes] = 0;
		} else if (!feof(f) && nbytes < EXCLEN) {
			fclose(f);
			ewprintf("Null in pos %d of line %z of file %s", nbytes, lineno, fname);
			return FALSE;
		}

		if (nbytes >= EXCLEN) {
			fclose(f);
			return overlong_line(EXCLEN);
		}

		if ((s = excline(excbuf)) != TRUE) {
			fclose(f);

			if (macro_show_message == 1 && s != SILENTERR) {
				ewprintf("Error on line %z of file %s", lineno, fname);
			}

			return FALSE;
		}
	}

	if (ferror(f)) {
		ewprintf("File read error: %s", strerror(errno));
		fclose(f);
		return FALSE;
	}

	fclose(f);
	return TRUE;
}


/*
 * Mg3a: Append the escape sequence for a terminfo key to the line.
 * Return a pointer to the next character to scan, or NULL if fail.
 *
 * Do some translation from easy-to-remember aliases.
 */

static char *
appendcap(char *capname, LINE *lp)
{
	char *cp1, *cp2, *cap, **p;
	extern char *tigetstr(char *cap);
	static char *aliases[] = {
		"up", "kcuu1", "down", "kcud1", "left", "kcub1", "right", "kcuf1",
		"pageup", "kpp", "pagedn", "knp", "home", "khome", "end", "kend",
		"insert", "kich1", "delete", "kdch1", NULL};

	for (cp1 = capname ;; cp1++) {
		if (*cp1 == 0 || *cp1 == '"') {
			return NULL;
		} else if (*cp1 == '}') {
			*cp1++ = 0;
			break;
		}
	}

	cp2 = ltext(lp) + llength(lp);

	for (p = aliases; *p; p += 2) {
		if (*p[0] == capname[0] && strcmp(*p, capname) == 0) {
			capname = *(p + 1);
			break;
		}
	}

	cap = tigetstr(capname);

	if (!cap || cap == (char *)-1 ||
		!*cap || llength(lp) > lsize(lp) - (INT)strlen(cap))
	{
		return NULL;
	}

	while (*cap) {
		*cp2++ = *cap++;
		lp->l_used++;
	}

	return cp1;
}


/*
 * excline - run a line from a load file or eval-expression.
 *
 * Mg3a: support recursion.
 */

static INT
excline_aux(char *line)
{
	char	*funcp, *argp = NULL;
	INT	c;
	INT		status;
	INT	f, n;
	LINE	*lp;
	PF	fp = NULL;
#ifdef USER_MACROS
	char *macrop = NULL;
#endif
#define BINDEXT 80	// Leave some room for function key sequence

	f = 0;
	n = 1;
	funcp = skipwhite(line);
	if (*funcp == '\0') return TRUE;	/* No error on blank lines */
	line = parsetoken(funcp);
	if (*line != '\0') {
		*line++ = '\0';
		line = skipwhite(line);
		if ((*line >= '0' && *line <= '9') || *line == '-') {
			argp = line;
			line = parsetoken(line);
		}
	}

	if (argp != NULL) {
		f = FFARG;
		if (!getINT(argp, &n, 1)) return FALSE;
	}
	if((fp = name_function(funcp)) == NULL) {
#ifdef USER_MACROS
	    if ((macrop = lookup_named_macro(funcp)) == NULL) {
		ewprintf("Unknown function or macro: %s", funcp);
		return FALSE;
	    }
#else
	    ewprintf("Unknown function: %s", funcp);
	    return FALSE;
#endif
	}
	/* Pack away all the args now...	*/
	if ((cmd_maclhead = make_empty_list(cmd_maclhead)) == NULL) return FALSE;
	while (*line != '\0') {
		argp = skipwhite(line);
		if (*argp == '\0') break;
		line = parsetoken(argp);
		if (*argp != '"') {
		    if (*argp == '\'') ++argp;
		    if((lp = lalloc(line - argp)) == NULL) return FALSE;
		    memcpy(ltext(lp), argp, line - argp);
		} else {	/* Quoted strings special */
		    ++argp;
		    if((lp = lalloc(line - argp + BINDEXT)) == NULL) return FALSE;
		    lsetlength(lp, 0);
		    while (*argp != '"' && *argp != '\0') {
			if (*argp != '\\') {
			    c = CHARMASK(*argp++);
			} else if (argp[1] == '\0') {
			    break;
			} else if (argp[1] == '{') {
			    argp = appendcap(argp + 2, lp);
			    if (argp == NULL) {
			        free(lp);
			    	return TRUE;	/* Skip the command, but no error */
			    }
			    continue;
			} else if (argp[1] == 'f' || argp[1] == 'F') {
				// "\fnn" emulation
				char kbuf[10];
				int num = 0, i;

				for (i = 2; i < 4; i++) {
					if (!ISDIGIT(argp[i])) break;
					num = num * 10 + argp[i] - '0';
				}

				sprintf(kbuf, "kf%d}", num);

				if (i == 2 || appendcap(kbuf, lp) == NULL) {
					free(lp);
					return TRUE;	// Fail silently if no digits
				}

				argp += i;
				continue;
			} else if (argp[1] == '^') {
				c = CHARMASK(argp[2]);		// Tag:ctrl
				if (c == 0 || c > 127) {
					free(lp);		// Error
					return TRUE;		// Should fail non-silently?
				}
				c = CCHR(ucs_toupper(c));
				argp += 3;
			} else if (argp[1] == 'u' || argp[1] == 'U') {
				INT num = 0, i, j, digit;	// Tag:unicode
				unsigned char buf[4], first = argp[2];
				INT len, status;

				if (first == '{') argp++;

				for (i = 2; i < 8; i++) {
					digit = basedigit(CHARMASK(argp[i]), 16);
					if (digit < 0) break;
					num = num * 16 + digit;
				}

				status = ucs_chartostr(termcharset, num, buf, &len);

				if (first == '{') {
					if (argp[i] == '}') argp++;
					else status = FALSE;
				}

				if (status == FALSE || i == 2 || llength(lp) > lsize(lp) - len) {
					free(lp);
					return TRUE;	// Fail silently if no digits
				}

				for (j = 0; j < len; j++) {
					ltext(lp)[lp->l_used++] = buf[j];
				}

				argp += i;
				continue;
			} else {
			    switch(*++argp) {
				case 't': case 'T':
				    c = CCHR('I');
				    break;
				case 'n': case 'N':
				    c = CCHR('J');
				    break;
				case 'r': case 'R':
				    c = CCHR('M');
				    break;
				case 'e': case 'E':
				    c = CCHR('[');
				    break;
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
				    c = *argp - '0';
				    if(argp[1] <= '7' && argp[1] >= '0') {
				    	c <<= 3;
					c += *++argp - '0';
					if(argp[1] <= '7' && argp[1] >= '0') {
					    c <<= 3;
					    c += *++argp - '0';
					}
				    }
				    break;
				case 'x': case 'X':
				    c = basedigit(argp[1], 16);

				    if (c >= 0) {
				    	INT c2;

				    	c2 = basedigit(argp[2], 16);

					if (c2 >= 0) {
						c = c*16 + c2;
						argp += 2;
					} else {
						argp++;
					}
				    } else {
				    	free(lp);
					return TRUE;	// Fail silently if no digits
				    }

				    break;
				case '\\':
				case '"':
				    c = *argp;
				    break;
				default:
				    // Ignore the command. Makes sequences backward-
				    // and forward-compatible.

				    free(lp);
				    return TRUE;
			    }
			    argp++;
			}
			if (llength(lp) < lsize(lp)) ltext(lp)[lp->l_used++] = c;
		    }
		}
		insertlast(cmd_maclhead, lp);
	}
	inmacro++;
	macro_show_message--;
#ifdef USER_MACROS
	if (fp) {
		maclcur = lforw(cmd_maclhead);
		status = (*fp)(f, n);
	} else {
		maclcur = cmd_maclhead;		// No arguments
		status = excline_copy_list_n(macrop, n);
	}
#else
	maclcur = lforw(cmd_maclhead);
	status = (*fp)(f, n);
#endif
	macro_show_message++;
	inmacro--;

	return status;
}


/*
 * Mg3a: Trick excline_aux into being recursive and to support
 * keyboard macro.
 */

#define MAXRECURSION 10

static INT
excline(char *line)
{
	LINE *old_cmd_maclhead;
	LINE *old_maclcur;
	INT old_macrodef, old_inkbdmacro;
	INT status;

	if (inmacro > MAXRECURSION) {
		return emessage(ABORT, "Max recursion level reached");
	}

	toplevel_undo_enabled = undo_enabled;

	if (macrodef || inmacro) {
		old_cmd_maclhead = cmd_maclhead;
		old_maclcur = maclcur;
		old_macrodef = macrodef;
		old_inkbdmacro = inkbdmacro;

		cmd_maclhead = NULL;
		macrodef = 0;
		inkbdmacro = 0;

		status = excline_aux(line);

		inkbdmacro = old_inkbdmacro;
		macrodef = old_macrodef;
		maclcur = old_maclcur;
		if (cmd_maclhead) {
			make_empty_list(cmd_maclhead);
			free(cmd_maclhead);
		}
		cmd_maclhead = old_cmd_maclhead;

		return status;
	} else {
		return excline_aux(line);
	}
}


/*
 * Mg3a: Whether to error check a command list
 */

INT errcheck = 1;


/*
 * Mg3a: Like excline, but copies its argument. If there is an error
 * and errorchecking is on, show an error message if interactive.
 */

INT
excline_copy(char *line)
{
	INT s;
	char excbuf[EXCLEN];

	if (stringcopy(excbuf, line, EXCLEN) >= EXCLEN) {
		return overlong_line(EXCLEN);
	}

	if ((s = excline(excbuf)) != TRUE && errcheck > 0) {
		if (macro_show_message == 1 && s != SILENTERR) {
			ewprintf("Error in \"%s\"", line);
		}

		return FALSE;
	}

	return TRUE;
}


/*
 * Mg3a: Execute a list of commands separated with ';'.
 *
 * Copies its argument. Of possibly large size.
 */

INT
excline_copy_list(char *str)
{
	char *cp, *endstr, excbuf[EXCLEN], *exc = NULL;
	INT ret = TRUE;
	size_t len;

	if ((len = stringcopy(excbuf, str, EXCLEN)) >= EXCLEN) {
		if ((exc = string_dup(str)) == NULL) return FALSE;
		str = exc;
	} else {
		str = excbuf;
	}

	endstr = str + len;

	while (1) {
		str = skipwhite(str);
		cp = str;

		while (1) {
			if (*cp == 0) break;
			cp = parsetoken(cp);
			cp = skipwhite(cp);
		}

		if ((ret = excline_copy(str)) != TRUE) break;

		if (cp == endstr) break;
		str = cp + 1;
	}

	if (exc) free(exc);

	return ret;
}


/*
 * Mg3a: execute a list with a repeat count
 */

INT
excline_copy_list_n(char *commands, INT n)
{
	if (n < 0) return FALSE;

	while (n--) {
		if (excline_copy_list(commands) != TRUE) return FALSE;
	}

	return TRUE;
}


/*
 * A pair of utility functions for the above
 */

/*
 * Skip whitespace and return a pointer to the character after it.
 *
 * If the whitespace ends at a ';', put a null there.
 */

static char *
skipwhite(char *s)
{
	while (*s == ' ' || *s == '\t' || *s == ')' || *s == '(') s++;
	if (*s == ';') *s = '\0' ;
	return s;
}

/*
 * Parse a quoted or unquoted token, and return a pointer to the
 * character after it.
 *
 * Mg3a: fixed tree bugs: ';' was taken as part of an unquoted argument,
 * you could escape out of a quoted string with '\', and the return
 * pointer pointed to the '"' at the end of a quoted argument rather
 * than after it.
 */

static char *
parsetoken(char *s)
{
	if (*s != '"') {
		while (*s && *s!=' ' && *s!='\t' && *s!=')' && *s!='(' && *s != ';') s++;
		return s;
	} else {
		// *s points to a '"'. The intent is to scan the string and
		// return a pointer to a point after it.
		while (1) {
			s++;
			if (*s == '"') return ++s;
	    		if (*s == '\\') s++;
			if (*s == '\0') return s;
		}
	}
}


/*
 * Mg3a: List of autoexec pattern and commands
 */

LINE *autoexec_list = NULL;


/*
 * Mg3a: Add an autoexecute line with code to the autoexec_list
 */

static int
add_autoexec_line(char code, char *str, int flag)
{
	INT len;
	LINE *lp;
	char *adjf;

	if (code == 'p' && *str == '~') {
		if ((adjf = adjustname(str))) str = adjf;
	}

	if (autoexec_list == NULL) {
		if (code == 'c') {
			ewprintf("The autoexec_list is empty; can't add a command without a pattern.");
			return 0;
		}
		if ((lp = lalloc(1)) == NULL) return 0;
		ltext(lp)[0] = 'h';
		lp->l_fp = lp;
		lp->l_bp = lp;
		autoexec_list = lp;
	}

	len = strlen(str);

	if ((lp = lalloc(len + 2)) == NULL) return 0;

	ltext(lp)[0] = code;
	memcpy(ltext(lp) + 1, str, len + 1);
	insertlast(autoexec_list, lp);

	lp->l_flag = flag;

	return 1;
}


/*
 * Mg3a: Add multiple autoexecute patterns to the autoexec_list
 */

static int
add_autoexec_patterns(char *str)
{
	char *cp;

	while ((cp = strchr(str, ';'))) {
		*cp = 0;
		if (!add_autoexec_line('p', str, 0)) return 0;
		str = cp + 1;
	}

	if (!add_autoexec_line('p', str, 0)) return 0;

	return 1;
}

/*
 * Mg3a: Add multiple autoexecute commands to the autoexec_list
 */

static int
add_autoexec_commands(char *str, INT f, INT n)
{
	char *cp, *endstr;
	int flag;

	endstr = str + strlen(str);

	if (f & FFARG) {
		if (n == 1 /* || n == 2 */) flag = n;
		else flag = 0;
	} else {
		flag = 0;
	}

	while (1) {
		str = skipwhite(str);
		cp = str;

		while (1) {
			if (*cp == 0) break;
			cp = parsetoken(cp);
			cp = skipwhite(cp);
		}

		if (!add_autoexec_line('c', str, flag)) return 0;
		if (cp == endstr) break;
		str = cp + 1;
	}

	return 1;
}

/*
 * Mg3a: Add an autoexecute pattern and/or a list of commands to the
 * autoexec_list. Allow for empty parameters.
 */

INT
auto_execute(INT f, INT n)
{
	char pattern[NFILEN], command[EXCLEN] = "";
	INT s;

	if ((s = ereply("Filename pattern: ", pattern, sizeof(pattern))) == ABORT) return s;

	if (!inmacro || !outofarguments(0)) {
		if ((s = ereply("Command(s): ", command, sizeof(command))) == ABORT) return s;
	}

	if (pattern[0] && !add_autoexec_line('p', pattern, 0)) return FALSE;
	if (command[0] && !add_autoexec_commands(command, f, n)) return FALSE;


	if (pattern[0] || command[0]) {
		ewprintf("Pattern and/or command(s) added.");
	}

	return TRUE;
}


/*
 * Mg3a: Add a list of autoexecute patterns and/or a list of commands
 * to the autoexec_list. Allow for empty parameters.
 */

INT
auto_execute_list(INT f, INT n)
{
	char pattern[NFILEN], command[EXCLEN] = "";
	INT s;


	if ((s = ereply("Filename pattern(s): ", pattern, sizeof(pattern))) == ABORT) return s;

	if (!inmacro || !outofarguments(0)) {
		if ((s = ereply("Command(s): ", command, sizeof(command))) == ABORT) return s;
	}

	if (pattern[0] && !add_autoexec_patterns(pattern)) return FALSE;
	if (command[0] && !add_autoexec_commands(command, f, n)) return FALSE;

	if (pattern[0] || command[0]) {
		ewprintf("Pattern(s) and/or command(s) added.");
	}

	return TRUE;
}


/*
 * Mg3a: Add a "shebang" test to the next auto-execute statement.
 */

INT
shebang(INT f, INT n)
{
	char str[NFILEN];
	INT s;

	if ((s = ereply("Shebang string: ", str, sizeof(str))) != TRUE) return s;

	if (!add_autoexec_line('s', str, 0)) return FALSE;

	ewprintf("Shebang test for '%s' added", str);

	return TRUE;
}


/*
 * Mg3a: Execute autoexecute commands for the current buffer.
 */

int
autoexec_execute(int change)
{
	LINE *lp;
	INT matched, status;
	int skipping = 0, nextskipping = 0;

	if (autoexec_list == NULL) return 0;

	lp = lforw(autoexec_list);

	while (lp != autoexec_list) {
		// In case we ever allow commands first
		matched = (ltext(lp)[0] == 'c');

		while (lp != autoexec_list && ltext(lp)[0] != 'c') {
			// Tests can come in any order
			switch (ltext(lp)[0]) {
			    case 's':
				if (isshebang(curbp, &ltext(lp)[1])) {
					if (!change) return 1;
					matched = 1;
				}
				break;
			    case 'p':
				if (filematch(&ltext(lp)[1], curbp->b_fname)) {
					if (!change) return 1;
					matched = 1;
				}
				break;
			}

			lp = lforw(lp);
		}

		nextskipping = 0;

		while (lp != autoexec_list && ltext(lp)[0] == 'c') {
			if (matched && (!skipping || lp->l_flag == 1)) {
				errcheck--;
				status = excline_copy(&ltext(lp)[1]);
				errcheck++;

				if (status != TRUE) return 0;

				nextskipping = 1;
			}

			lp = lforw(lp);
		}

		if (nextskipping) skipping = 1;
	}

	return 0;
}


/*
 * Mg3a: Execute conditionally. For autoexecute.
 */

#define IFENTRY 32

INT
ifcmd(INT f, INT n)
{
	INT match = 0, not = 0;
	char code[IFENTRY];

	if (macrodef) {
		return emessage(ABORT, "\"if\" doesn't work in a keyboard macro");
	}

	if (ereply("Code: ", code, IFENTRY) != TRUE) return FALSE;

	if (strcmp(code, "not") == 0) {
		not = 1;
		if (ereply("Code2: ", code, IFENTRY) != TRUE) return FALSE;
	}

	if (strcmp(code, "empty") == 0) {
		match = (curbp->b_linep == lforw(curbp->b_linep));
	} else if (strcmp(code, "bom") == 0) {
		match = ((curbp->b_flag & BFBOM) != 0);
	} else {
		ewprintf("Unknown code: %s", code);
		return FALSE;
	}

	if (not != match) return extend(f, n);

	return TRUE;
}


/*
 * Mg3a: List autoexec patterns
 */

INT
listpatterns(INT f, INT n)
{
	BUFFER *bp;
	LINE *lp;
	INT c, prevc = ' ';
	char buf[3*NFILEN], *s, type[80];

	bp = emptyhelpbuffer();
	if (!bp) return FALSE;

	if (!autoexec_list) {
		if (addline(bp, "** No Patterns **") == FALSE) return FALSE;
		return popbuftop(bp);
	}

	for (lp = lforw(autoexec_list); lp != autoexec_list; lp = lforw(lp)) {
		c = lgetc(lp, 0);
		if (prevc == 'c' && c != 'c' && addline(bp, "") == FALSE) return FALSE;
		switch (c) {
			case 's': s = "   #!  "; break;
			case 'p': s = "Pattern"; break;
			case 'c': s = "Command"; break;
			default: snprintf(type, sizeof(type), "Code %c ", (int)c);
				 s = type; break;
		}
		if (lp->l_flag) {
			snprintf(buf, sizeof(buf), "%s = %jd '%s'", s, (intmax_t)lp->l_flag, &ltext(lp)[1]);
		} else {
			snprintf(buf, sizeof(buf), "%s = '%s'", s, &ltext(lp)[1]);
		}
		if (addline(bp, buf) == FALSE) return FALSE;
		prevc = c;
	}

	return popbuftop(bp);
}


/*
 * Mg3a: Show a message in the echo line
 */

INT
message(INT f, INT n)
{
	char msg[NFILEN];
	INT save_message = macro_show_message;

	if (ereply("Message: ", msg, NFILEN) != TRUE) return FALSE;

	macro_show_message = 1;
	ewprintf("%s", msg);
	macro_show_message = save_message;

	return TRUE;
}


/*
 * Mg3a: Make a command show messages and check errors, even when it
 * otherwise wouldn't have.
 */

INT
with_message(INT f, INT n)
{
	INT s;
	INT save_message = macro_show_message;
	INT save_errcheck = errcheck;

	if (macrodef) {
		ewprintf("\"with-message\" cannot be recorded in a keyboard macro");
		return FALSE;
	}

	macro_show_message = 100;
	errcheck = 100;

	s = extend(f, n);

	macro_show_message = save_message;
	errcheck = save_errcheck;

	if (s != TRUE && inmacro) s = SILENTERR;	// Silent error in command line

	ttflush();	// The command doesn't necessarily flush

	return s;
}


/*
 * Mg3a: As with-message but from keystrokes.
 */

INT
with_key(INT f, INT n)
{
	INT s;
	INT save_message = macro_show_message;
	INT save_errcheck = errcheck;

	if (inmacro || macrodef || macro_show_message != 1) {
		ewprintf("\"with-key\" can not be used in a macro or recursively.");
		return FALSE;
	}

	estart();
	ewprintfc("Press keystrokes to debug: ");
	epresf = KPROMPT;
	ttflush();

	macro_show_message = 100;
	errcheck = 100;

	s = doin();

	macro_show_message = save_message;
	errcheck = save_errcheck;

	return s;
}
