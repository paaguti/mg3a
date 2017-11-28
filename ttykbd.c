#include	"def.h"

/*
 * Mg3a: Define keys and run terminal startup file.
 */

void
ttykeymapinit()
{
	char *cp;

	do_internal_bindings();

	if (!inhibit_startup && (cp = gettermtype())) {
		if ((cp = startupfile(cp)) != NULL)
			load(cp);
	}
}
