/*
 * kbd.h: type definitions for symbol.c and kbd.c for mg experimental
 */

typedef struct	{
	KCHAR	k_base;		/* first key in element			*/
	KCHAR	k_num;		/* last key in element			*/
	PF	*k_funcp;	/* pointer to array of pointers to functions */
	struct	keymap_s	*k_prefmap;	/* keymap of ONLY prefix key in element */
}	MAP_ELEMENT;


/*
 * Predefined keymaps are NOT type KEYMAP because final array needs
 * dimension. If any changes are made to this struct, they must be
 * reflected in all keymap declarations.
 */

#define KEYMAPE(NUM)	{\
	KCHAR	map_num;\
	KCHAR	map_max;\
	PF	map_default;\
	MAP_ELEMENT map_element[NUM];\
}
		/* elements used		*/
		/* elements allocated		*/
		/* default function		*/
		/* realy [e_max]		*/
typedef struct keymap_s KEYMAPE(1) KEYMAP;

#define none	ctrlg
#define prefix	(PF)NULL


/* number of map_elements to grow an overflowed keymap by */

#define MAPGROW 3
#define MAPINIT (MAPGROW+1)


/* max number of default bindings added to avoid creating new element */

#define MAPELEDEF 4

#define MODE_NAMEDYN 1		// Mode name was dynamically allocated

typedef struct {
	KEYMAP	*p_map;
	char	*p_name;
	int	p_flags;
	char	*p_commands;
}	MAPS;

extern	MAPS	*map_table;

typedef const struct {
	PF	n_funct;
	char	*n_name;
	int	n_flag;
}	FUNCTNAMES;

extern	const FUNCTNAMES functnames[];
extern	const INT	 nfunct;

PF	doscan(KEYMAP *map, INT c);
char	*map_name(KEYMAP *map);
MODE 	name_mode(char *name);
char	*mode_name(MODE mode);
char	*mode_name_check(MODE mode);
KEYMAP	*mode_map(MODE mode);
int	invalid_mode(MODE mode);
KEYMAP	*name_map(char *name);
PF	name_function(char *fname);
INT	complete_function(char *fname, INT c);
char 	*function_name(PF fpoint);

extern	MAP_ELEMENT *ele;
