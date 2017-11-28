/*
 *	Termcap terminal file, nothing special, just make it big
 *	enough for windowing systems.
 */

// #define GOSLING			/* Compile in fancy display.	*/
// #define	MEMMAP		*/	/* Not memory mapped video.	*/

#define NROW	100			/* (maximum) Rows.		*/
#define NCOL	200			/* (maximum) Columns.		*/
#define TERMCAP				/* for possible use in ttyio.c	*/

#define getkbd()	(ttgetc())
#define	putpad(str, num)	tputs(str, num, ittputc)

#define	KFIRST	K00
#define	KLAST	K00
