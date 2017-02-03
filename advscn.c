/* advscn.c - a lexical scanner for the adventure compiler */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "advavl.h"
#include "advcom.h"

/* useful definitions */
#define maplower(ch)	(isupper(ch) ? tolower(ch) : ch)

/* global variables */
int errcount=0;		/* error count */
int t_value;		/* numeric value */
char t_token[TKNSIZE+1];/* token string */
char *t_names[] = {
	0,
	"(",
	")",
	"STRING",
	"IDENTIFIER",
	"NUMBER",
	"EOF"
};

/* external variables */
extern FILE *ifp;	/* input file pointer */
extern int msgoff;	/* message section offset */

/* local variables */
static int savetkn = 0;	/* look ahead token */
static int savech = 0;	/* look ahead character */
static char fname[200];	/* include file name */
static char line[200];	/* current input line */
static char *lptr;	/* line pointer */
static int lnum;	/* line number */
static int ieof;	/* input end of file flag */
static int save_lnum;	/* saved lnum */
static FILE *save_ifp;	/* saved ifp */
static int scnt;	/* count of characters in string */

/* prototypes */
static int getch(void);

/* sinit - initialize the scanner */
void sinit(void)
{
    /* setup the line buffer */
    lptr = line; *lptr = 0;
    lnum = 0;

    /* no include file yet */
    save_ifp = NULL;

    /* no lookahead yet */
    savech = 0;
    savetkn = 0;

    /* not eof yet */
    ieof = FALSE;
}

/* token - get the next token */
int token(void)
{
    int tkn;

    if ((tkn = savetkn) != 0)
	savetkn = 0;
    else
	tkn = rtoken();
    return (tkn);
}

/* stoken - save a token */
void stoken(int tkn)
{
    savetkn = tkn;
}

/* rtoken - read the next token */
int rtoken(void)
{
    int ch;

    /* check the next character */
    for (;;)
	switch (ch = skipspaces()) {
	case EOF:	return (T_EOF);
	case '(':	strcpy(t_token,"("); return (T_OPEN);
	case ')':	strcpy(t_token,")"); return (T_CLOSE);
	case '"':	return (getstring());
	case ';':	while (getch() != '\n'); break;
	default:	return (getid(ch));
	}
}

/* getstring - get a string */
int getstring(void)
{
    int ch,sflag;

    t_value = msgoff;
    sflag = FALSE;
    scnt = 0;
    while ((ch = getch()) != EOF && ch != '"')
	if (isspace(ch))
	    sflag = TRUE;
	else {
	    if (ch == '\\')
		switch (ch = getch()) {
		case 'n':  ch = '\n'; break;
		case 't':  ch = '\t'; break;
		}
	    if (sflag)
		{ wputc(' '); sflag = FALSE; }
	    wputc(ch);
	}
    if (sflag)
	wputc(' ');
    strdone();
    strcpy(t_token,"{string}");
    return (T_STRING);
}

/* getid - get an identifier */
int getid(int ch)
{
    char *p;

    p = t_token; *p++ = maplower(ch);
    while ((ch = getch()) != EOF && isidchar(ch))
	*p++ = maplower(ch);
    *p = EOS;
    savech = ch;
    return (isnum(t_token,&t_value) ? T_NUMBER : T_IDENTIFIER);
}

/* isnum - check if this string is a number */
int isnum(char *str,int *pval)
{
    int digits;
    char *p;

    /* initialize */
    p = str; digits = 0;

    /* check for a sign */
    if (*p == '+' || *p == '-')
	p++;

    /* check for a string of digits */
    while (isdigit(*p))
	p++, digits++;

    /* make sure there was at least one digit and this is the end */
    if (digits == 0 || *p)
	return (FALSE);

    /* convert the string to an integer and return successfully */
    if (*str == '+') ++str;
    *pval = atoi(str);
    return (TRUE);
}

/* wputc - put a character into the output file */
void wputc(int ch)
{
    ad_putc(encode(ch));
    scnt++;
}

/* strdone - finish a string */
void strdone(void)
{
    wputc('\0');
    while (scnt & 3)
	wputc('\0');
    msgoff += scnt >> 2;
}

/* skipspaces - skip leading spaces */
int skipspaces(void)
{
    int ch;

    while ((ch = getch()) && isspace(ch))
	;
    return (ch);
}

/* isidchar - is this an identifier character */
int isidchar(int ch)
{
    return (!isspace(ch) && ch != '(' && ch != ')' && ch != '"');
}

/* getch - get the next character */
static int getch(void)
{
    FILE *fp;
    int ch;

    /* check for a lookahead character */
    if ((ch = savech) != 0)
	savech = 0;

    /* check for a buffered character */
    else if ((ch = *lptr) != 0)
	lptr++;

    /* check for end of file */
    else if (ieof)
	ch = EOF;

    /* read another line */
    else {

	/* read the line */
	for (lptr = line; (ch = getchr()) != EOF && (*lptr++ = ch) != '\n'; )
	    ;
	*lptr = 0;
	lnum++;

	/* check for an included file */
	if (line[0] == '@') {

	    /* open the file */
	    strcpy(fname,&line[1]); fname[strlen(fname)-1] = 0;
	    if ((fp = fopen(fname,"r")) == NULL) {
		printf("Can't open include file: %s\n",fname);
		exit(0);
	    }
	    printf("[ including %s ]\n",fname);

	    /* setup input from the file */
	    save_lnum = lnum;
	    save_ifp = ifp;
	    ifp = fp;

	    /* setup for the first line */
	    lptr = line; *lptr = 0;
	    lnum = 0;
	}

	/* otherwise this must be an input line */
	else {

	    /* terminate the line with a newline */
	    *lptr++ = '\n'; *lptr = 0;

	    /* check for end of file */
	    if (ch == EOF)
		ieof = TRUE;

	    /* update the line number and setup for the new line */
	    lptr = line;
	}

	/* get a character */
	ch = getch();
    }

    /* return the current character */
    return (ch);
}

/* getchr - get a character checking for end of file */
int getchr(void)
{
    int ch;

    if ((ch = getc(ifp)) == EOF || ch == '\032') {
	if (save_ifp) {
	    printf("[ end of %s ]\n",fname);
	    fclose(ifp);
	    lnum = save_lnum;
	    ifp = save_ifp;
	    save_ifp = NULL;
	    ch = getchr();
	}
	else
	    ch = EOF;
    }
    else if (ch == '\r')
	ch = getchr();
    return (ch);
}

/* encode - encode a single character */
int encode(int ch)
{
    return ((ch - 30) & 0xFF);
}

/* error - report an error in the current line */
void error(char *msg)
{
    char *p;

    printf(">>> %s <<<\n>>> in line %d <<<\n%s",msg,lnum,line);
    for (p = line; p < lptr; p++)
	if (*p == '\t')
	    putchar('\t');
	else
	    putchar(' ');
    printf("^\n");
    errcount++;
}

/* xerror - report an error in the current line */
void xerror(char *msg)
{
    printf(">>> %s <<<\n",msg);
    errcount++;
}
