/*
 * This file is in the public domain.
 *
 * Author: Pedro A. Aranda <paaguti@hotmail.com>
 *
 * Modified by: Bengt Larsson.
 *
 */

/*
 * Implement a mode for editing Makefiles
 */

#include "def.h"


#ifdef LANGMODE_MAKE
/*
 * Enable/toggle python-mode
 */
extern char comment_begin[20], comment_end[20];

INT
makemode(INT f, INT n)
{
	strncpy(comment_begin, "# ", 19);
	comment_end[0] = '\0';
	return changemode(curbp, f, n, "make");
}

#endif
