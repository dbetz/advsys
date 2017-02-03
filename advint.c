/* advint.c - an interpreter for adventure games */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <setjmp.h>
#include <stdlib.h>
#include <stdarg.h>
#include "advint.h"
#include "advdbs.h"

/* global variables */
jmp_buf restart;

/* external variables */
extern int h_main;

/* prototypes */
int main(int argc,char *argv[]);

/* main - the main routine */
int main(int argc,char *argv[])
{
    char *fname,*lname;
    int rows,cols,i;

    printf("ADVINT v1.3 - Copyright (c) 1986, by David Betz\n");
    fname = NULL;
    lname = NULL;
    rows = 24;
    cols = 80;

    /* parse the command line */
    for (i = 1; i < argc; i++)
	if (argv[i][0] == '-')
	    switch (argv[i][1]) {
	    case 'r':
	    case 'R':
		    rows = atoi(&argv[i][2]);
		    break;
	    case 'c':
	    case 'C':
		    cols = atoi(&argv[i][2]);
		    break;
	    case 'l':
	    case 'L':
		    lname = &argv[i][2];
	    	    break;
	    }
	else
	    fname = argv[i];
    if (fname == NULL) {
	printf("usage: advint [-r<rows>] [-c<columns>] [-l<log-file>] <file>\n");
	exit(1);
    }

    /* initialize terminal i/o */
    trm_init(rows,cols,lname);

    /* initialize the database */
    db_init(fname);

    /* come here on restart */
    setjmp(restart);

    /* play the game */
    execute(h_main);

    /* finish with the terminal */
    trm_done();

    /* return successfully */
    return 0;
}

/* error - print an error message and exit */
void error(char *msg,...)
{
    char buf[100];
    va_list ap;
    va_start(ap,msg);
    vsprintf(buf,msg,ap);
    trm_str(buf);
    trm_chr('\n');
    exit(0);
}
