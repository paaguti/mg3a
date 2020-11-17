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

INT
makemode(INT f, INT n)
{
	return changemode(curbp, f, n, "make");
}

#endif
