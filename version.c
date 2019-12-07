/*
 * This file contains the string that get written out by the emacs-
 * version command.
 */

#include	"def.h"

char version[] = "Mg3a, version " VERSION;


/*
 * Display the version. All this does is copy the version string onto
 * the echo line.
 */

INT
showversion(INT f, INT n)
{
	ewprintf(version);
	return TRUE;
}
