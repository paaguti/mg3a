/* definitions for keyboard macros */

#define MAXMACRO 256		/* maximum functs in a macro */

extern	INT inmacro;
extern	INT inkbdmacro;
extern	INT macrodef;
extern	INT macrocount;

typedef	union {
	PF	m_funct;
	INT	m_count;	/* for count-prefix	*/
	INT	m_flag;		/* Mg3a: save the flag also */
} macrorecord;

extern macrorecord macro[];

extern	LINE *maclhead;
extern	LINE *maclcur;
extern	LINE *cmd_maclhead;	/* For excline() */
