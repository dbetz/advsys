/* advcom.h - adventure compiler definitions */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdio.h>
#include <ctype.h>
#include "advavl.h"

/* limits */
#define TKNSIZE		50	/* maximum token size */
#define OSIZE		104	/* maximum object size (O_SIZE/2 + OPMAX*2) */
#define OPMAX		50	/* maximum # properties/object */
#define WMAX		800	/* maximum number of words */
#define OMAX		800	/* maximum number of objects */
#define AMAX		500	/* maximum number of actions */
#define DMAX		32767	/* maximum data space */
#define CMAX		32767	/* maximum code space */
#define FMAX		20	/* file name maximum */

/* useful definitions */
#define TRUE		1
#define FALSE		0
#define EOS		'\0'

/* token definitions */
#define T_OPEN		1
#define T_CLOSE		2
#define T_STRING	3
#define T_IDENTIFIER	4
#define T_NUMBER	5
#define T_EOF		6

/* symbol types */
#define ST_OBJECT	1
#define ST_ACTION	2
#define ST_VARIABLE	3
#define ST_CONSTANT	4
#define ST_PROPERTY	5

/* symbol structure */
typedef struct symbol {
    char *s_name;		/* symbol name */
    int s_type;			/* symbol type */
    int s_value;		/* symbol value */
    struct symbol *s_next;	/* next symbol in table */
} SYMBOL;

/* function argument structure */
typedef struct argument {
    char *arg_name;		/* argument name */
    struct argument *arg_next;	/* next argument */
} ARGUMENT;

/* advcom.c prototypes */
int main(int argc,char *argv[]);
int getvalue(void);
int dalloc(int size);
int add_word(char *str,int type);
int add_synonym(char *str,int wrd);
int getword(int off);
void putword(int off,int dat);
int getbyte(int off);
void putbyte(int off,int dat);
void output(void);
void woutput(TNODE *node);
void wtoutput(TNODE *node);
void undef_object(int n);
void str_out(char *str,int len);
void word_out(int dat);
void byte_out(int dat);
int oenter(char *name);
int ofind(char *name);
int aenter(char *name);
int venter(char *name);
int penter(char *name);
void center(char *name,int value);
SYMBOL *sfind(SYMBOL *head,char *name);
SYMBOL *senter(SYMBOL **phead,char *name,int type,int value);
void frequire(int rtkn);
void require(int tkn,int rtkn);
char *save(char *str);
int match(char *str);
void fail(char *msg);

/* advexp.c prototypes */
void do_expr(void);
int in_ntab(void);
int in_ftab(void);
int in_etab(void);
void do_cond(void);
void do_and(void);
void do_or(void);
void do_if(void);
void do_while(void);
void do_progn(void);
void do_setq(void);
void do_return(void);
void do_send(void);
void do_sndsuper(void);
void sender(void);
void do_catch(void);
void do_call(void);
void do_nary(int op,int n);
void do_literal(void);
void do_identifier(void);
void code_argument(int n);
void cd_setargument(int n);
void code_temporary(int n);
void cd_settemporary(int n);
void code_variable(int n);
void cd_setvariable(int n);
void code_literal(int n);
void do_op(int op);
int putcbyte(int b);
int putcword(int w);
void fixup(int chn,int val);

/* advfcn.c prototypes */
void do_adventure(void);
void do_word(int type);
void do_synonym(void);
void do_define(void);
void do_extended(void);
void do_variable(void);
void do_defproperty(void);
void do_default(void);
void do_dflag(int flag);
int do_object(char *cname,int class);
void do_noun(void);
void do_adjective(void);
void do_property(int flags);
void do_method(void);
void setprop(int prop,int flags,int value);
void addprop(int prop,int flags,int value);
int do_code(char *type);
void do_action(void);
void do_flag(int flag);
void do_verb(void);
void do_preposition(void);
void do_function(void);
void addargument(ARGUMENT **list,char *name);
void freelist(ARGUMENT *arg);
int findarg(char *name);
int findtmp(char *name);

/* advscn.c prototypes */
void sinit(void);
int token(void);
void stoken(int tkn);
int rtoken(void);
int getstring(void);
int getid(int ch);
int isnum(char *str,int *pval);
void wputc(int ch);
void strdone(void);
int skipspaces(void);
int isidchar(int ch);
int getchr(void);
int encode(int ch);
void error(char *msg);
void xerror(char *msg);

/* advcstuff.c prototypes */
void macinit(char *iname,char *oname);
void macpause(void);
void ad_create(char *name);
void ad_close(void);
void ad_putc(int ch);
void ad_seek(long pos);
void ad_flush(void);
