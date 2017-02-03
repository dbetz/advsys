/* advtrm.c - terminal i/o routines */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdio.h>
#include "advint.h"

/* useful definitions */
#define TRUE	1
#define FALSE	0
#define EOS	'\0'
#define LINEMAX	200
#define WORDMAX	100

/* local variables */
static char line[LINEMAX+1];
static int col,maxcol,row,maxrow;
static int scnt,wcnt;
static char word[WORDMAX+1],*wptr;
static FILE *logfp = NULL;

/* trm_init - initialize the terminal module */
void trm_init(int rows,int cols,char *name)
{
    /* initialize the terminal i/o variables */
    maxcol = cols-1; col = 0;
    maxrow = rows-1; row = 0;
    wptr = word; wcnt = 0;
    scnt = 0;

    /* open the log file */
    if (name && (logfp = fopen(name,"w")) == NULL)
	error("can't open log file");
}

/* trm_done - finish terminal i/o */
void trm_done(void)
{
    if (wcnt) trm_word();
    if (logfp) fclose(logfp);
}

/* trm_get - get a line */
char *trm_get(void)
{
    if (wcnt) trm_word();
    while (scnt--) putchr(' ');
    row = col = scnt = 0;
    return (trm_line());
}

/* trm_str - output a string */
void trm_str(char *str)
{
    while (*str)
	trm_chr(*str++);
}

/* trm_xstr - output a string without logging or word wrap */
void trm_xstr(char *str)
{
    while (*str)
	advputc(*str++,stdout);
}

/* trm_chr - output a character */
void trm_chr(int ch)
{
    switch (ch) {
    case ' ':
	    if (wcnt)
		trm_word();
	    scnt++;
	    break;
    case '\t':
	    if (wcnt)
		trm_word();
	    scnt = (col + scnt + 8) & ~7;
	    break;
    case '\n':
	    if (wcnt)
		trm_word();
	    trm_eol();
	    scnt = 0;
	    break;
    default:
	    if (wcnt < WORDMAX) {
	        *wptr++ = ch;
	        wcnt++;
	    }
	    break;
    }
}

/* trm_word - output the current word */
void trm_word(void)
{
    if (col + scnt + wcnt > maxcol)
	trm_eol();
    else
	while (scnt--)
	    { putchr(' '); col++; }
    for (wptr = word; wcnt--; col++)
	putchr(*wptr++);
    wptr = word;
    wcnt = 0;
    scnt = 0;
}

/* trm_eol - end the current line */
void trm_eol(void)
{
    putchr('\n');
    if (++row >= maxrow)
	{ trm_wait(); row = 0; }
    col = 0;
}

/* trm_wait - wait for the user to type return */
void trm_wait(void)
{
    trm_xstr("  << MORE >>\r");
    advwaitc();
    trm_xstr("            \r");
}

/* trm_line - get an input line */
char *trm_line(void)
{
    char *p;
    int ch;

    p = line;
    while ((ch = getchr()) != EOF && ch != '\n')
	switch (ch) {
	case '\177':
	case '\010':
		if (p != line) {
		    if (ch != '\010') putchr('\010');
		    putchr(' ');
		    putchr('\010');
		    p--;
		}
		break;
	default:
		if ((p - line) < LINEMAX)
		    *p++ = ch;
		break;
	}
    *p = 0;
    return (ch == EOF ? NULL : line);
}

/* getchr - input a single character */
int getchr(void)
{
    int ch;

    if ((ch = advgetc()) != EOF && logfp)
	advputc(ch,logfp);
    return (ch);
}

/* putchr - output a single character */
void putchr(int ch)
{
    if (logfp) advputc(ch,logfp);
    advputc(ch,stdout);
}
