#include "def.h"
#include "macro.h"

/*
 * A linear undo.
 */


INT undo_limit = 1000000;	// Max bytes stored in undo list per buffer

int u_boundary = 1;		// An undo boundary will be made
INT undo_enabled = 1;		// Undo enabled
INT buf_modified = 0;		// True if the buffer was modified before the commmand


static int u_panic();
static int u_trim();


/*
 * Fake the general string insert into inserting raw.
 */

static INT
linsert_str_nl(char *str, INT len)
{
	return linsert_general_string(curbp->charset, 1, str, len);
}


/*
 * Insert an undo record after another, and update size
 */

static int
u_insertafter(undo_rec *p1, undo_rec *p2)
{
	if (!buf_modified) p2->flag |= UFL_CLEARMOD;
	buf_modified = BFCHG;		// Record clear modify only once per u_modify()

	p2->suc = p1->suc;
	p2->pre = p1;
	p1->suc->pre = p2;
	p1->suc = p2;

	// Include a reasonable estimate of the overhead in the size

	if (curbp->undo_size > MAXINT - (INT)offsetof(undo_rec, content) - p2->size) {
		ewprintf("Integer overflow in undo size");
		return u_panic();
	}

	curbp->undo_size += offsetof(undo_rec, content) + p2->size;
	return u_trim();
}


/*
 * Remove an undo record and update size
 */

static void
u_unlink(undo_rec *p1)
{
	p1->pre->suc = p1->suc;
	p1->suc->pre = p1->pre;

	curbp->undo_size -= offsetof(undo_rec, content) + p1->size;
	free(p1);
}


/*
 * Create a new undo record
 */

static undo_rec *
u_new(int flag, size_t pos, INT len)
{
	undo_rec *p1;
	INT size;

	if (len < UNDO_MIN) size = UNDO_MIN;
	else if (len > MAXINT - 8) size = MAXINT;
	else size = len + 8;

	p1 = malloc(offsetof(undo_rec, content) + size);

	if (p1 == NULL) {
		ewprintf("Error allocating undo entry");
		return NULL;
	}

	p1->pos = pos;
	p1->flag = flag;
	p1->size = size;
	p1->len = len;
	p1->suc = p1->pre = p1;

	return p1;
}


/*
 * Trim the undo list to 'undo-limit'
 */

static int
u_trim()
{
	undo_rec *undo_list = curbp->undo_list;

	if (undo_list && curbp->undo_size > undo_limit) {
		while (curbp->undo_size > undo_limit && undo_list->suc != undo_list) {
			u_unlink(undo_list->suc);
		}

		while (undo_list->suc != undo_list && !(undo_list->suc->flag & UFL_BOUNDARY)) {
			u_unlink(undo_list->suc);
		}
	}

	curbp->undo_current = NULL;
	return 1;
}


/*
 * Clean out the undo list in a given buffer. Deallocate everything.
 */

int
u_clear(BUFFER *bp)
{
	undo_rec *undo_list = bp->undo_list, *p1;

//	ewprintf("Clearing undo.");

	if (undo_list) {
		while (undo_list->suc != undo_list) {
			p1 = undo_list->suc;
			undo_list->suc = p1->suc;
			free(p1);
		}

		free(undo_list);
		bp->undo_list = NULL;
	}

	bp->undo_size = 0;
	bp->undo_current = NULL;

	return 1;
}


/*
 * Error. Clear the undo list to keep sync with the buffer. Return
 * error status. A message should already have been shown.
 */

static int
u_panic()
{
	u_clear(curbp);
	return 0;
}


/*
 * Kill the undo-entries beyond the current point and clear the
 * current pointer.
 */

void
u_kill_later_entries()
{
	undo_rec *list	= curbp->undo_list;
	undo_rec *p1	= curbp->undo_current;

	if (p1) {
		while (p1 != list) {
			p1 = p1->suc;
			u_unlink(p1->pre);
		}

		curbp->undo_current = NULL;
	}

	if (list) list->flag = 0;
}


/*
 * Insert an undo entry
 */

int
u_entry(int flag, size_t pos, char *str, INT len)
{
	undo_rec *last, *p1, *undo_list;

	if (!undo_enabled || len <= 0 || (curbp->b_flag & (BFSYS|BFDIRED))) {
		return 1;		// Not an error
	} else if (len > undo_limit) {
		return u_clear(curbp);	// Not an error
	}

	u_kill_later_entries();

	if (!curbp->undo_list && (curbp->undo_list = u_new(0, 0, 0)) == NULL) return u_panic();
	undo_list = curbp->undo_list;

	last = undo_list->pre;

	// Try to merge with the last entry to save space. If merging backward,
	// only do it if the entry is not too big.

	if (last != undo_list && last->len <= last->size - len && !u_boundary &&
		(flag & UFL_INSERT) == (last->flag & UFL_INSERT))
	{
		if (flag & UFL_INSERT) {		// Merge insert after
			if (last->pos + last->len == pos) {
				memcpy(&last->content[last->len], str, len);
				last->len += len;
				return u_trim();
			}
		} else if (last->pos == pos) {		// Merge delete after
			memcpy(&last->content[last->len], str, len);
			last->len += len;
			return u_trim();
		} else if (last->pos == pos + len && last->len < (INT)(sizeof(undo_rec) << 2)) {
							// Merge delete before
			memmove(&last->content[len], last->content, last->len);
			memcpy(last->content, str, len);
			last->pos = pos;
			last->len += len;
			return u_trim();
		}
	}

	if ((p1 = u_new(flag, pos, len)) == NULL) return u_panic();

	memcpy(p1->content, str, len);

	if (u_boundary) p1->flag |= UFL_BOUNDARY;
	u_boundary = 0;

	return u_insertafter(last, p1);
}


/*
 * Insert an undo entry for a range, starting at a given point.
 */

int
u_entry_range(int flag, LINE *lp, INT offset, INT len, size_t pos)
{
	undo_rec *p1 = NULL;
	char *str, *startbuf, buf[UNDO_MIN];
	INT chunk;
	LINE *blp = curbp->b_linep;	// Must check this.

	if (!undo_enabled || len <= 0 || lp == blp) return 1;		// Not an error

	// Within same line, potential merge

	if (len <= llength(lp) - offset) {
		return u_entry(flag, pos, &ltext(lp)[offset], len);
	}

	if (curbp->b_flag & (BFSYS|BFDIRED)) {
		return 1;						// Not an error
	} else if (len > undo_limit) {
		return u_clear(curbp);					// Not an error
	}

	u_kill_later_entries();

	if (!curbp->undo_list && (curbp->undo_list = u_new(0, 0, 0)) == NULL) return u_panic();

	if (len < UNDO_MIN) {
		// Small entry, potential merge

		startbuf = buf;
	} else {
		if ((p1 = u_new(flag, pos, len)) == NULL) return u_panic();
		startbuf = p1->content;
	}

	str = startbuf;
	chunk = llength(lp) - offset;
	if (chunk > len) chunk = len;

	memcpy(str, &ltext(lp)[offset], chunk);
	str += chunk;
	len -= chunk;

	while (len > 0) {
		if (lforw(lp) == blp) break;
		*str++ = '\n';
		len--;
		if (len <= 0) break;
		lp = lforw(lp);
		chunk = llength(lp);
		if (chunk > len) chunk = len;
		memcpy(str, ltext(lp), chunk);
		str += chunk;
		len -= chunk;
	}

	if (!p1) {
		return u_entry(flag, pos, buf, str - startbuf);
	}

	p1->len = str - startbuf;

	if (u_boundary) p1->flag |= UFL_BOUNDARY;
	u_boundary = 0;

	return u_insertafter(curbp->undo_list->pre, p1);
}


/*
 * Return true if undo list is empty
 */

int
u_empty()
{
	undo_rec *undo_list = curbp->undo_list;

	return (!undo_list || undo_list->suc == undo_list);
}


/*
 * Return true if we can't undo
 */

static int
u_njet()
{
	if (curbp->b_flag & (BFSYS|BFDIRED)) {
		u_clear(curbp);
		ewprintf("Undo is off in system or Dired buffers.");
		return 1;
	} else if (!undo_enabled) {
		u_clear(curbp);
		ewprintf("Undo is disabled.");
		return 1;
	} else if (u_empty()) {
		ewprintf("Undo list is empty.");
		return 1;
	} else if (curbp->b_flag & BFREADONLY) {
		readonly();
		return 1;
	}

	return 0;
}


/*
 * Undo command that goes only backward
 */

INT
undo_only(INT f, INT n)
{
	undo_rec *p1, *undo_list;
	INT s, save_undo = undo_enabled;

	if (n < 0) return FALSE;
	if (u_njet()) return FALSE;

	thisflag |= CFUNDO;
	curbp->undo_forward = 0;

	if (curbp->undo_current == NULL) p1 = curbp->undo_list->pre;
	else p1 = curbp->undo_current->pre;

	undo_list = curbp->undo_list;

	if (p1 == undo_list) {
		ewprintf("Nothing more to undo.");
		return FALSE;
	}

	ewprintf("Undo!");

	if (p1 == undo_list->pre && !(curbp->b_flag & BFCHG)) {
		undo_list->flag |= UFL_CLEARMOD;
	}

	while (1) {
		if (!position(p1->pos)) return FALSE;
		undo_enabled = 0;

		if (p1->flag & UFL_INSERT) {
			s = ldeleteraw(p1->len, KNONE);
		} else {
			s = linsert_str_nl(p1->content, p1->len);
		}

		undo_enabled = save_undo;

		if (s == FALSE) {
			u_clear(curbp);
			return s;
		}

		if (p1->flag & UFL_CLEARMOD) {
			curbp->b_flag &= ~BFCHG;
			upmodes(curbp);
		}

		curbp->undo_current = p1;

		if (p1->pre == undo_list || ((p1->flag & UFL_BOUNDARY) && n && !--n)) return TRUE;

		p1 = p1->pre;
	}
}


/*
 * Undo command that goes only forward
 */

INT
redo(INT f, INT n)
{
	undo_rec *p1, *undo_list;
	INT s, save_undo = undo_enabled;

	if (n < 0) return FALSE;
	if (u_njet()) return FALSE;

	thisflag |= CFUNDO;
	curbp->undo_forward = 1;

	p1 = curbp->undo_current;
	undo_list = curbp->undo_list;

	if (p1 == NULL) {
		ewprintf("Nothing to redo.");
		return FALSE;
	}

	if (p1 == undo_list) {
		ewprintf("Nothing more to redo.");
		return FALSE;
	}

	ewprintf("Redo!");

	while (1) {
		if (!position(p1->pos)) return FALSE;
		undo_enabled = 0;

		if (p1->flag & UFL_INSERT) {
			s = linsert_str_nl(p1->content, p1->len);
		} else {
			s = ldeleteraw(p1->len, KNONE);
		}

		undo_enabled = save_undo;

		if (s == FALSE) {
			u_clear(curbp);
			return s;
		}

		p1 = p1->suc;
		curbp->undo_current = p1;

		if (p1->flag & UFL_CLEARMOD) {
			curbp->b_flag &= ~BFCHG;
			upmodes(curbp);
		}

		if (p1 == undo_list || ((p1->flag & UFL_BOUNDARY) && n && !--n)) return TRUE;
	}
}


/*
 * Undo command that switches direction if not consecutive
 */

INT
undo(INT f, INT n)
{
	if (curbp->undo_current == NULL) {
		return undo_only(f, n);
	} else if (lastflag & CFUNDO) {
		return curbp->undo_forward ? redo(f, n) : undo_only(f, n);
	} else {
		return curbp->undo_forward ? undo_only(f, n) : redo(f, n);
	}
}


/*
 * Command to introduce an undo boundary
 */

INT
undo_boundary(INT f, INT n)
{
	u_boundary = 1;
	return TRUE;
}


/*
 * Signal that the buffer gets modified, and save modified flag
 */

void
u_modify()
{
	if (undo_enabled) buf_modified = (curbp->b_flag & BFCHG);
	if (!toplevel_undo_enabled) u_clear(curbp);
}


/*
 * Selectively inhibit undo boundary
 */

void
inhibit_undo_boundary(INT f, PF pf)
{
	if (!inmacro && pf == prevfunc && !(f & FFRAND)) u_boundary = 0;
}


#ifdef LIST_UNDO

/*
 * List the undo data
 */

INT
list_undo(INT f, INT n)
{
	BUFFER *bp;
	LINE *lp;
	undo_rec *undo_list = curbp->undo_list, *p;
	char str[200];
	INT len, plen;
	size_t count = 0;
	const INT maxlen = (MAXINT > (1 << 25)) ? (1 << 24) : MAXINT/2;

	if (undo_list == NULL) {
		ewprintf("No undo data.");
		return TRUE;
	}

	bp = emptyhelpbuffer();
	if (bp == NULL) return FALSE;

	snprintf(str, sizeof(str), "Undo list size = " INTFMT, curbp->undo_size);
	if (addline(bp, str) == FALSE) return FALSE;
	if (addline(bp, "") == FALSE) return FALSE;

	for (p = undo_list->suc; p != undo_list; p = p->suc) {
		snprintf(str, sizeof(str), "%s%s, pos = %zu, size = %zu, allocated = %zu, flag = %d",
			(p->flag & UFL_INSERT) ? "Insert" : "Delete",
			(p->flag & UFL_BOUNDARY) ? ", boundary" : "",
			p->pos, (size_t)p->len, (size_t)p->size, p->flag);

		if (addline(bp, str) == FALSE) return FALSE;

		// Truncate large record, also solves the problem of a
		// MAXINT undo record with overhead in a MAXINT line

		if (p->len > maxlen) {
			len = stringcopy(str, "data (truncated) = \"", sizeof(str));
			plen = maxlen;
		} else {
			len = stringcopy(str, "data = \"", sizeof(str));
			plen = p->len;
		}

		lp = lalloc(len + plen + 1);
		if (lp == NULL) return FALSE;

		memcpy(ltext(lp), str, len);
		memcpy(&ltext(lp)[len], p->content, plen);
		ltext(lp)[len + plen] = '"';

		addlast(bp, lp);
		if (addline(bp, "") == FALSE) return FALSE;
		count++;
	}

	snprintf(str, sizeof(str), "%zu record%s, %.2f bytes per record",
		count, count == 1 ? "" : "s", count ? (double)curbp->undo_size/count : 0.0);

	if (addline(bp, str) == FALSE) return FALSE;

	return popbuftop(bp);
}

#endif
