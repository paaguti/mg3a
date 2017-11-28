/* key.h: Insert file for mg 2 functions that need to reference key pressed */

#define MAXKEY	16			/* maximum number of prefix chars */

typedef	struct {			/* the chacter sequence in a key */
	INT	k_count;		/* number of chars		*/
	KCHAR	k_chars[MAXKEY];	/* chars			*/
}	key_struct;

extern key_struct key;
