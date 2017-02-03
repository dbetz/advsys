/* advcom.c - a compiler for adventure games */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "advcom.h"
#include "advdbs.h"

/* symbol tables */
SYMBOL *symbols;
SYMBOL *exfunctions;
ARGUMENT *arguments;
ARGUMENT *temporaries;

/* adventure id information */
char aname[19];
int aversion;

/* word table */
int wtable[WMAX+1],wcnt;

/* object table */
int otable[OMAX+1],ocnt;

/* action table */
int atable[AMAX+1],acnt;

/* constant, variable and property symbol counts */
int ccnt,vcnt,pcnt;

/* data and code space */
char *data,*code;
int dptr,cptr;

/* buffer for building an object */
int objbuf[OSIZE];
int nprops;

/* global variables */
char ifile[FMAX];	/* input file name */
char ofile[FMAX];	/* output file name */
FILE *ifp;		/* input file pointer */
unsigned int msgoff;	/* message section offset */
TREE *words;		/* word tree */
int curwrd;		/* current word number */
int curobj;		/* current object */
int curact;		/* current action */
int def_flag;		/* default action flag value */
int def_mask;		/* default action mask value */

/* header information variables */
int h_main;		/* main code */

/* external variables */
extern int errcount;	/* error count */
extern int t_value;	/* token value */
extern char t_token[];	/* token string */
extern char *t_names[];	/* token names */
extern long ad_foff;	/* data file offset */

/* forward declarations */
SYMBOL *sfind();
SYMBOL *senter();
char *save();

/* main - the main routine */
int main(int argc,char *argv[])
{
    int tkn,obj,i;

    /* initialize */
    printf("ADVCOM v1.2 - Copyright (c) 1986, by David Betz\n");
    wcnt = ocnt = acnt = ccnt = vcnt = pcnt = msgoff = 0;
    symbols = exfunctions = NULL;
    arguments = temporaries = NULL;
    def_flag = def_mask = 0;
    aname[0] = '\0';
    h_main = NIL;
    sinit();

    /* setup the code and data space */
    if ((data = calloc(1,DMAX)) == 0)
	fail("insufficient memory");
    if ((code = calloc(1,CMAX)) == 0)
	fail("insufficient memory");
    dptr = cptr = 1;	/* make sure nothing has a zero offset */

    /* get the file name */
    if (argc < 2)
	fail("usage: advcom <file> [ <ofile> ]");
    strcpy(ifile,argv[1]); strcat(ifile,".adv");
    strcpy(ofile,(argc < 3 ? argv[1] : argv[2])); strcat(ofile,".dat");

    /* open the input file */
    if ((ifp = fopen(ifile,"r")) == NULL)
	fail("can't open input file");

    /* create and initialize the output file */
    ad_create(ofile);
    for (i = 0; i++ < 512; ad_putc('\0'))
	;

    /* create the word tree */
    words = tnew();

    /* enter builtin constants */
    center("t",-1);
    center("nil",0);
 
    /* enter the builtin variables */
    venter("$actor");
    venter("$action");
    venter("$dobject");
    venter("$ndobjects");
    venter("$iobject");
    venter("$ocount");

    /* enter the preposition "to" */
    add_word("to",WT_PREPOSITION);

    /* process statements until end of file */
    while ((tkn = token()) == T_OPEN) {
	frequire(T_IDENTIFIER);

	/* identification statement */
	if (match("adventure"))
	    do_adventure();

	/* vocabulary statements */
	else if (match("adjective"))
	    do_word(WT_ADJECTIVE);
	else if (match("preposition"))
	    do_word(WT_PREPOSITION);
	else if (match("conjunction"))
	    do_word(WT_CONJUNCTION);
	else if (match("article"))
	    do_word(WT_ARTICLE);
	else if (match("synonym"))
	    do_synonym();

	/* constant, variable, function and default definition statements */
	else if (match("define"))
	    do_define();
	else if (match("variable"))
	    do_variable();
	else if (match("default"))
	    do_default();

	/* extended function definition statement */
	else if (match("extended"))
	    do_extended();
	    
	/* property definition statement */
	else if (match("property"))
	    do_defproperty();

	/* handle the main code statement */
	else if (match("main"))
	    h_main = do_code(t_token);

	/* action definition statement */
	else if (match("action"))
	    do_action();

	/* object definition statements */
        else if (match("object"))
	    do_object(t_token,NIL);

	/* object instance definition statements */
	else if ((obj = ofind(t_token)) != 0)
	    do_object(t_token,obj);

	/* error, unknown statement */
	else
	    error("Unknown statement type");
    }
    require(tkn,T_EOF);

    /* close the input file */
    fclose(ifp);

    /* output the data structures */
    output();

    /* close the output file */
    ad_close();

    /* return successfully */
    return 0;
}

/* getvalue - get a value */
int getvalue(void)
{
    SYMBOL *sym;

    switch (token()) {
    case T_IDENTIFIER:	if ((sym = sfind(symbols,t_token)) != 0)
			    return (sym->s_value);
			return (oenter(t_token));
    case T_NUMBER:	return (t_value);
    case T_STRING:	return (t_value);
    default:		error("Expecting identifier, number or string");
			return (0);
    }
}

/* dalloc - allocate data space */
int dalloc(int size)
{
    if ((dptr += size) > DMAX)
	fail("out of data space");
    return (dptr - size);
}

/* add_word - add a word to the dictionary */
int add_word(char *str,int type)
{
    if ((curwrd = tfind(words,str)) == NIL) {
	if (wcnt < WMAX) {
	    curwrd = ++wcnt;
	    wtable[curwrd] = type;
	    tenter(words,str);
	}
	else {
	    error("too many words");
	    curwrd = 0;
	}
    }
    else if (wtable[curwrd] == WT_UNKNOWN)
	wtable[curwrd] = type;
    else if (type != WT_UNKNOWN && type != wtable[curwrd])
	error("Ambiguous word type");
    return (curwrd);
}

/* add_synonym - add a synonym to a word */
int add_synonym(char *str,int wrd)
{
    curwrd = wrd;
    return (tenter(words,str));
}

/* getword - get a word from an object field */
int getword(int off)
{
    return ((data[off] & 0xFF) | (data[off+1] << 8));
}

/* putword - put a word into an object field */
void putword(int off,int dat)
{
    data[off] = dat;
    data[off+1] = dat >> 8;
}

/* getbyte - get a byte from an object field */
int getbyte(int off)
{
    return (data[off]);
}

/* putbyte - put a byte into an object field */
void putbyte(int off,int dat)
{
    data[off] = dat;
}

/* output - output the binary data structures */
void output(void)
{
    int woff,wsize;	/* word table offset and size */
    int ooff,osize;	/* object table offset and size */
    int aoff,asize;	/* action table offset and size */
    int toff,tsize;	/* word type table offset and size */
    int voff,vsize;	/* variable table offset and size */
    int soff,ssize;	/* save area offset and size */
    int dsize;		/* data size without dictionary */
    int dbase,cbase,size,mblk,dblk,i;

    /* make sure the adventure id information is present */
    if (aname[0] == 0) {
	xerror("no adventure identification information");
	strcpy(aname,"ADVENTURE");
	aversion = 0;
    }

    /* pad the remainder of this message block */
    while (msgoff & 0x007F)
	{ ad_putc('\0'); ad_putc('\0'); ad_putc('\0'); ad_putc('\0'); msgoff++; }

    /* save the size of the data area before the dictionary */
    dsize = dptr;

    /* insert the vocabulary into the data array */
    woutput(words->tr_root);

    /* compute table offsets */
    woff = 0;            wsize = tentries(words) * 2 + 2;
    toff = woff + wsize; tsize = wcnt;
    ooff = toff + tsize; osize = ocnt * 2 + 2;
    aoff = ooff + osize; asize = acnt * 2 + 2;
    voff = aoff + asize; vsize = vcnt * 2 + 2;
    dbase = voff + vsize;
    cbase = dbase + dptr;

    /* compute the resident structure size */
    size = wsize+tsize+osize+asize+vsize+dptr+cptr;

    /* set the save area parameters */
    soff = voff; ssize = vsize + dsize;

    /* compute the first block for message text */
    mblk = 1;
    dblk = (int)(ad_foff >> 9);

    /* output the word table */
    word_out(tentries(words));
    wtoutput(words->tr_root);

    /* output the word type table */
    for (i = 1; i <= wcnt; i++)
	byte_out(wtable[i]);

    /* output the object table */
    word_out(ocnt);
    for (i = 1; i <= ocnt; i++) {
	if (otable[i] == NIL)
	    undef_object(i);
	word_out(otable[i]);
    }

    /* output the action table */
    word_out(acnt);
    for (i = 1; i <= acnt; i++)
	word_out(atable[i]);

    /* beginning of saveable data */

    /* output the variable table */
    word_out(vcnt);
    for (i = 1; i <= vcnt; i++)
	word_out(NIL);

    /* output the data space */
    for (i = 0; i < dptr; )
	byte_out(data[i++]);

    /* end of saveable data */

    /* output the code space */
    for (i = 0; i < cptr; )
	byte_out(code[i++]);

    /* output the file header */
    ad_seek(0L);
    word_out(size);	/* resident structure size */
    str_out("ADVSYS",6);/* magic information */
    word_out(VERSION);	/* data structure version number */
    str_out(aname,18);	/* adventure name */
    word_out(aversion);	/* adventure version number */
    word_out(woff);	/* word table offset */
    word_out(toff);	/* word type table offset */
    word_out(ooff);	/* object table offset */
    word_out(aoff);	/* action table offset */
    word_out(voff);	/* variable table offset */
    word_out(dbase);	/* base of data */
    word_out(cbase);	/* base of code */
    word_out(dblk);	/* first data block */
    word_out(mblk);	/* first message text block */
    word_out(h_main);	/* main code */
    word_out(soff);	/* save area offset */
    word_out(ssize);	/* save area size */

    /* show statistics */
    printf("[ words:      %d ]\n",tentries(words));
    printf("[ word types: %d ]\n",wcnt);
    printf("[ objects:    %d ]\n",ocnt);
    printf("[ actions:    %d ]\n",acnt);
    printf("[ variables:  %d ]\n",vcnt);
    printf("[ data:       %d ]\n",dsize);
    printf("[ code:       %d ]\n",cptr);
    printf("[ dictionary: %d ]\n",dptr-dsize);
    printf("[ text:       %ld ]\n",(long) msgoff * 4L);
    printf("[ save area:  %d ]\n",ssize);
    printf("[ errors:     %d ]\n",errcount);
}

/* woutput - output the word data */
void woutput(TNODE *node)
{
    int wnum,wrd;

    if (node) {
	woutput(LLINK(node));
	wnum = WORD(node);
	wrd = WORD(node) = dalloc(strlen(KEY(node))+3);
	putword(wrd,wnum);
	strcpy(data+wrd+2,KEY(node));
	if (wtable[wnum] == WT_UNKNOWN)
	    printf("Type of word %s is unknown\n",KEY(node));
	woutput(RLINK(node));
    }
}

/* wtoutput - output the word table */
void wtoutput(TNODE *node)
{
    if (node) {
	wtoutput(LLINK(node));
	word_out(WORD(node));
	wtoutput(RLINK(node));
    }
}

/* undef_object - complain about an undefined object */
void undef_object(int n)
{
    char msg[100];
    SYMBOL *sym;

    for (sym = symbols; sym != NULL; sym = sym->s_next)
	if (sym->s_type == ST_OBJECT && n == sym->s_value) {
	    sprintf(msg,"Object %s is undefined",sym->s_name);
	    xerror(msg);
	    break;
	}
}

/* str_out - output a string */
void str_out(char *str,int len)
{
    while (len--)
	byte_out(*str++);
}

/* word_out - output a word */
void word_out(int dat)
{
    byte_out(dat);
    byte_out(dat >> 8);
}

/* byte_out - output a byte */
void byte_out(int dat)
{
    ad_putc((~dat - 30) & 0xFF);
}

/* oenter - enter an object into the symbol table */
int oenter(char *name)
{
    SYMBOL *sym;

    if ((sym = sfind(symbols,name)) != 0) {
	if (sym->s_type != ST_OBJECT)
	    error("Not an object");
	return (sym->s_value);
    }
    if (ocnt < OMAX) {
        senter(&symbols,name,ST_OBJECT,++ocnt);
        otable[ocnt] = NIL;
    }
    else
	error("too many objects");
    return (ocnt);
}

/* ofind - find an object in the symbol table */
int ofind(char *name)
{
    SYMBOL *sym;

    if ((sym = sfind(symbols,name)) != 0) {
	if (sym->s_type != ST_OBJECT)
	    return (NIL);
	return (sym->s_value);
    }
    return (NIL);
}

/* aenter - enter an action into the symbol table */
int aenter(char *name)
{
    SYMBOL *sym;

    if ((sym = sfind(symbols,name)) != 0) {
	if (sym->s_type != ST_ACTION)
	    error("Not an action");
	return (sym->s_value);
    }
    if (acnt < AMAX) {
        senter(&symbols,name,ST_ACTION,++acnt);
        atable[acnt] = NIL;
    }
    else
	error("too many actions");
    return (acnt);
}

/* venter - enter a variable into the symbol table */
int venter(char *name)
{
    SYMBOL *sym;

    if ((sym = sfind(symbols,name)) != 0) {
	if (sym->s_type != ST_VARIABLE)
	    error("Not a variable");
	return (sym->s_value);
    }
    senter(&symbols,name,ST_VARIABLE,++vcnt);
    return (vcnt);
}

/* penter - enter a property into the symbol table */
int penter(char *name)
{
    SYMBOL *sym;

    if ((sym = sfind(symbols,name)) != 0) {
	if (sym->s_type != ST_PROPERTY)
	    error("Not a property");
	return (sym->s_value);
    }
    senter(&symbols,name,ST_PROPERTY,++pcnt);
    return (pcnt);
}

/* center - enter a constant into the symbol table */
void center(char *name,int value)
{
    if (sfind(symbols,name)) {
	error("Already defined");
	return;
    }
    senter(&symbols,name,ST_CONSTANT,value);
}

/* sfind - find a symbol in the symbol table */
SYMBOL *sfind(SYMBOL *head,char *name)
{
    SYMBOL *sym;

    for (sym = head; sym != NULL; sym = sym->s_next)
	if (strcmp(name,sym->s_name) == 0)
	    break;
    return (sym);
}

/* senter - enter a symbol into the symbol table */
SYMBOL *senter(SYMBOL **phead,char *name,int type,int value)
{
    SYMBOL *sym;

    if ((sym = (SYMBOL *)malloc(sizeof(SYMBOL))) == NULL)
	fail("out of memory");
    sym->s_name = save(name);
    sym->s_type = type;
    sym->s_value = value;
    sym->s_next = *phead;
    *phead = sym;
    return (sym);
}

/* frequire - fetch a token and check it */
void frequire(int rtkn)
{
    require(token(),rtkn);
}

/* require - check for a required token */
void require(int tkn,int rtkn)
{
    char msg[100];
    if (tkn != rtkn) {
	sprintf(msg,"Expecting %s",t_names[rtkn]);
	error(msg);
    }
}

/* save - allocate memory for a string */
char *save(char *str)
{
    char *new;

    if ((new = malloc(strlen(str)+1)) == NULL)
	fail("out of memory");
    strcpy(new,str);
    return (new);
}

/* match - compare a string with the current token */
int match(char *str)
{
    return (strcmp(str,t_token) == 0);
}

/* fail - print an error message and exit */
void fail(char *msg)
{
    printf("%s\n",msg);
    exit(0);
}
