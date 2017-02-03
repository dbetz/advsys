/* advdbs.c - adventure database access routines */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "advint.h"
#include "advdbs.h"

/* global variables */
int h_main;	/* main code */
FILE *datafp;	/* data file pointer */

/* external variables */
extern jmp_buf restart;

/* table base addresses */
char *wtable;	/* word table */
char *wtypes;	/* word type table */
int wcount;	/* number of words */
char *otable;	/* object table */
int ocount;	/* number of objects */
char *atable;	/* action table */
int acount;	/* number of actions */
char *vtable;	/* variable table */
int vcount;	/* number of variables */
char *data;	/* base of data tables */
char *base;	/* current base address */
char *dbase;	/* base of the data space */
char *cbase;	/* base of the code space */
int length;	/* length of resident data structures */

/* data file header */
static char hdr[HDR_SIZE];

/* save parameters */
static long saveoff;	/* save data file offset */
static char *save;	/* save area base address */
static int slen;	/* save area length */

/* prototypes */

/* db_init - read and decode the data file header */
void db_init(char *name)
{
    int woff,ooff,aoff,voff,n;
    char fname[50];

    /* get the data file name */
    strcpy(fname,name);
    strcat(fname,".dat");

    /* open the data file */
    if (!(datafp = fopen(fname,"rb")))
	error("can't open data file: %s",fname);

    /* read the header */
    if (fread(hdr,1,HDR_SIZE,datafp) != HDR_SIZE)
	error("bad data file - header");
    complement(hdr,HDR_SIZE);
    base = hdr;

    /* check the magic information */
    if (strncmp(&hdr[HDR_MAGIC],"ADVSYS",6) != 0)
	error("not an adventure data file");

    /* check the version number */
    if ((n = getword(HDR_VERSION)) < 103 || n > VERSION)
	error("wrong version number: %d %d",n,VERSION);

    /* decode the resident data length header field */
    length = getword(HDR_LENGTH);

    /* allocate space for the resident data structure */
    if ((data = malloc(length)) == 0)
	error("insufficient memory");

    /* compute the offset to the data */
    saveoff = (long)getword(HDR_DATBLK) * 512L;	

    /* read the resident data structure */
    fseek(datafp,saveoff,SEEK_SET);
    if (fread(data,1,length,datafp) != length)
	error("bad data file - resident data");
    complement(data,length);

    /* get the table base addresses */
    wtable = data + (woff = getword(HDR_WTABLE));
    wtypes = data + getword(HDR_WTYPES) - 1;
    otable = data + (ooff = getword(HDR_OTABLE));
    atable = data + (aoff = getword(HDR_ATABLE));
    vtable = data + (voff = getword(HDR_VTABLE));

    /* get the save data area */
    saveoff += (long)getword(HDR_SAVE);
    save = data + getword(HDR_SAVE);
    slen = getword(HDR_SLEN);

    /* get the base of the data and code spaces */
    dbase = data + getword(HDR_DBASE);
    cbase = data + getword(HDR_CBASE);

    /* initialize the message routines */
    msg_init(datafp,getword(HDR_MSGBLK));

    /* get the main code pointer */
    h_main = getword(HDR_MAIN);

    /* get the table lengths */
    base = data;
    wcount = getword(woff); 
    ocount = getword(ooff);
    acount = getword(aoff);
    vcount = getword(voff);

    /* setup the base of the resident data */
    base = dbase;

    /* set the object count */
    setvalue(V_OCOUNT,ocount);
}

/* db_save - save the current database */
int db_save(void)
{
    return (advsave(&hdr[HDR_ANAME],20,save,slen) ? T : NIL);
}

/* db_restore - restore a saved database */
int db_restore(void)
{
    return (advrestore(&hdr[HDR_ANAME],20,save,slen) ? T : NIL);
}

/* db_restart - restart the current game */
int db_restart(void)
{
    fseek(datafp,saveoff,SEEK_SET);
    if (fread(save,1,slen,datafp) != slen)
	return (NIL);
    complement(save,slen);
    setvalue(V_OCOUNT,ocount);
    longjmp(restart,1);
}

/* complement - complement a block of memory */
void complement(char *adr,int len)
{
    for (; len--; adr++)
	*adr = ~(*adr + 30);
}

/* findword - find a word in the dictionary */
int findword(char *word)
{
    char sword[WRDSIZE+1];
    int wrd,i;

    /* shorten the word */
    strncpy(sword,word,WRDSIZE); sword[WRDSIZE] = 0;

    /* look up the word */
    for (i = 1; i <= wcount; i++) {
	wrd = getwloc(i);
	if (strcmp(base+wrd+2,sword) == 0)
	    return (getword(wrd));
    }
    return (NIL);
}

/* wtype - return the type of a word */
int wtype(int wrd)
{
    return (wtypes[wrd]);
}

/* match - match an object against a name and list of adjectives */
int match(int obj,int noun,int *adjs)
{
    int *aptr;

    if (!hasnoun(obj,noun))
	return (FALSE);
    for (aptr = adjs; *aptr != NIL; aptr++)
	if (!hasadjective(obj,*aptr))
	    return (FALSE);
    return (TRUE);
}

/* checkverb - check to see if this is a valid verb */
int checkverb(int *verbs)
{
    int act;

    /* look up the action */
    for (act = 1; act <= acount; act++)
	if (hasverb(act,verbs))
	    return (act);
    return (NIL);
}

/* findaction - find an action matching a description */
int findaction(int *verbs,int preposition,int flag)
{
    int act,mask;

    /* look up the action */
    for (act = 1; act <= acount; act++) {
	if (preposition && !haspreposition(act,preposition))
	    continue;
	if (!hasverb(act,verbs))
	    continue;
	mask = ~getabyte(act,A_MASK);
	if ((flag & mask) == (getabyte(act,A_FLAG) & mask))
	    return (act);
    }
    return (NIL);
}

/* getp - get the value of an object property */
int getp(int obj,int prop)
{
    int p;

    for (; obj; obj = getofield(obj,O_CLASS))
	if ((p = findprop(obj,prop)) != 0)
	    return (getofield(obj,p));
    return (NIL);
}

/* setp - set the value of an object property */
int setp(int obj,int prop,int val)
{
    int p;

    for (; obj; obj = getofield(obj,O_CLASS))
	if ((p = findprop(obj,prop)) != 0)
	    return (putofield(obj,p,val));
    return (NIL);
}

/* findprop - find a property */
int findprop(int obj,int prop)
{
    int n,i,p;

    n = getofield(obj,O_NPROPERTIES);
    for (i = p = 0; i < n; i++, p += 4)
	if ((getofield(obj,O_PROPERTIES+p) & 0x7FFF) == prop)
	    return (O_PROPERTIES+p+2);
    return (NIL);
}

/* hasnoun - check to see if an object has a specified noun */
int hasnoun(int obj,int noun)
{
    while (obj) {
	if (inlist(getofield(obj,O_NOUNS),noun))
	    return (TRUE);
	obj = getofield(obj,O_CLASS);
    }
    return (FALSE);
}

/* hasadjective - check to see if an object has a specified adjective */
int hasadjective(int obj,int adjective)
{
    while (obj) {
	if (inlist(getofield(obj,O_ADJECTIVES),adjective))
	    return (TRUE);
	obj = getofield(obj,O_CLASS);
    }
    return (FALSE);
}

/* hasverb - check to see if this action has this verb */
int hasverb(int act,int *verbs)
{
    int link,word,*verb;

    /* get the list of verbs */
    link = getafield(act,A_VERBS);

    /* look for this verb */
    while (link != NIL) {
	verb = verbs;
	word = getword(link+L_DATA);
	while (*verb != NIL && word != NIL) {
	    if (*verb != getword(word+L_DATA))
		break;
	    verb++;
	    word = getword(word+L_NEXT);
	}
	if (*verb == NIL && word == NIL)
	    return (TRUE);
	link = getword(link+L_NEXT);
    }
    return (FALSE);
}

/* haspreposition - check to see if an action has a specified preposition */
int haspreposition(int act,int preposition)
{
    return (inlist(getafield(act,A_PREPOSITIONS),preposition));
}

/* inlist - check to see if a word is an element of a list */
int inlist(int link,int word)
{
    while (link != NIL) {
	if (word == getword(link+L_DATA))
	    return (TRUE);
	link = getword(link+L_NEXT);
    }
    return (FALSE);
}

/* getofield - get a field from an object */
int getofield(int obj,int off)
{
    return (getword(getoloc(obj)+off));
}

/* putofield - put a field into an object */
int putofield(int obj,int off,int val)
{
    return (putword(getoloc(obj)+off,val));
}

/* getafield - get a field from an action */
int getafield(int act,int off)
{
    return (getword(getaloc(act)+off));
}

/* getabyte - get a byte field from an action */
int getabyte(int act,int off)
{
    return (getbyte(getaloc(act)+off));
}

/* getoloc - get an object from the object table */
int getoloc(int n)
{
    if (n < 1 || n > ocount)
	error("object number out of range: %d",n);
    return (getdword(otable+n+n));
}

/* getaloc - get an action from the action table */
int getaloc(int n)
{
    if (n < 1 || n > acount)
	error("action number out of range: %d",n);
    return (getdword(atable+n+n));
}

/* getvalue - get the value of a variable from the variable table */
int getvalue(int n)
{
    if (n < 1 || n > vcount)
	error("variable number out of range: %d",n);
    return (getdword(vtable+n+n));
}

/* setvalue - set the value of a variable in the variable table */
int setvalue(int n,int v)
{
    if (n < 1 || n > vcount)
	error("variable number out of range: %d",n);
    return (putdword(vtable+n+n,v));
}

/* getwloc - get a word from the word table */
int getwloc(int n)
{
    if (n < 1 || n > wcount)
	error("word number out of range: %d",n);
    return (getdword(wtable+n+n));
}

/* getword - get a word from the data array */
int getword(int n)
{
    return (getdword(base+n));
}

/* putword - put a word into the data array */
int putword(int n,int w)
{
    return (putdword(base+n,w));
}

/* getbyte - get a byte from the data array */
int getbyte(int n)
{
    return (*(base+n) & 0xFF);
}

/* getcbyte - get a code byte */
int getcbyte(int n)
{
    return (*(cbase+n) & 0xFF);
}

/* getcword - get a code word */
int getcword(int n)
{
    return (getdword(cbase+n));
}

/* getdword - get a word from the data array */
int getdword(char *p)
{
    return ((*p & 0xFF) | (*(p+1) << 8));
}

/* putdword - put a word into the data array */
int putdword(char *p,int w)
{
    *p = w; *(p+1) = w >> 8;
    return (w);
}

