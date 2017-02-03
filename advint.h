/* advint.h - adventure interpreter definitions */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdio.h>
#include <ctype.h>

/* useful definitions */
#define TRUE		1
#define FALSE		0
#define EOS		'\0'

/* program limits */
#define STKSIZE		500

/* advdbs.c prototypes */
void db_init(char *name);
int db_save(void);
int db_restore(void);
int db_restart(void);
void complement(char *adr,int len);
int findword(char *word);
int wtype(int wrd);
int match(int obj,int noun,int *adjs);
int checkverb(int *verbs);
int findaction(int *verbs,int preposition,int flag);
int getp(int obj,int prop);
int setp(int obj,int prop,int val);
int findprop(int obj,int prop);
int hasnoun(int obj,int noun);
int hasadjective(int obj,int adjective);
int hasverb(int act,int *verbs);
int haspreposition(int act,int preposition);
int inlist(int link,int word);
int getofield(int obj,int off);
int putofield(int obj,int off,int val);
int getafield(int act,int off);
int getabyte(int act,int off);
int getoloc(int n);
int getaloc(int n);
int getvalue(int n);
int setvalue(int n,int v);
int getwloc(int n);
int getword(int n);
int putword(int n,int w);
int getbyte(int n);
int getcbyte(int n);
int getcword(int n);
int getdword(char *p);
int putdword(char *p,int w);

/* advexe.c prototypes */
void execute(int code);

/* advint.c prototypes */
void error(char *msg,...);

/* advmsg.c prototypes */
void msg_init(FILE *fp,int base);
void msg_open(unsigned int msg);
int msg_byte(void);
int decode(int ch);
void get_block(unsigned int blk);

/* advprs.c prototypes */
int parse(void);
int next(void);
void show_noun(int n);

/* advtrm.c prototypes */
void trm_init(int rows,int cols,char *name);
void trm_done(void);
char *trm_get(void);
void trm_str(char *str);
void trm_xstr(char *str);
void trm_chr(int ch);
void trm_word(void);
void trm_eol(void);
void trm_wait(void);
char *trm_line(void);
int getchr(void);
void putchr(int ch);

/* advistuff.c prototypes */
void macinit(char *iname,int *prows,int *pcols);
int advopen(char *name);
int advgetc(void);
int advputc(int ch,FILE *fp);
void advwaitc(void);
void advpause(void);
int advsave(char *hdr,int hlen,char *save,int slen);
int advrestore(char *hdr,int hlen,char *save,int slen);
