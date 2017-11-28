/*
 * Name:	MG 2a
 *		Directory management functions
 * Created:	Ron Flax (ron@vsedev.vse.com)
 *		Modified for MG 2a by Mic Kaczmarczik 03-Aug-1987
 */

#include "def.h"

char	*wdir;
static char cwd[NFILEN];


/*
 * Initialize anything the directory management routines need
 */

void
dirinit()
{
	if (!(wdir = getcwd(cwd, NFILEN)))
		panic("Can't get current directory!", errno);
}


/*
 * Change current working directory
 */

INT
changedir(INT f, INT n)
{
	INT s;
	char bufc[NFILEN], *adjf;

	if ((s=eread("Change default directory: ", bufc, NFILEN, EFFILE)) != TRUE)
		return(s);

	if (bufc[0] == '~') {
		if ((adjf = adjustname(bufc)) == NULL) return FALSE;
		stringcopy(bufc, adjf, NFILEN);
	}

	if (chdir(bufc) == -1) {
		ewprintf("Can't change dir to %s", bufc);
		return(FALSE);
	} else {
		if (!(wdir = getcwd(cwd, NFILEN)))
			panic("Can't get current directory!", errno);
		ewprintf("Current directory is now %s", wdir);
		return(TRUE);
	}
}


/*
 * Show current directory
 */

INT
showcwdir(INT f, INT n)
{
	ewprintf("Current directory: %s", wdir);
	return(TRUE);
}
