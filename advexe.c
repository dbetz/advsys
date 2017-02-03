/* advexe.c - adventure code executer */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <setjmp.h>
#include "advint.h"
#include "advdbs.h"

/* external variables */
extern int nouns[],*adjectives[];

/* local variables */
static int *sp,*fp,*efp,*top,pc,opcode;
static int stack[STKSIZE];
static jmp_buf trap;

/* external routines */
extern char *trm_get();

/* forward declarations */
extern void (*optab[])(void);

static void opCALL(void);
static void opSEND(void);
static void opRETURN(void);
static void opTSPACE(void);
static void opTMP(void);
static void opTSET(void);
static void opARG(void);
static void opASET(void);
static void opBRT(void);
static void opBRF(void);
static void opBR(void);
static void opT(void);
static void opNIL(void);
static void opPUSH(void);
static void opNOT(void);
static void opADD(void);
static void opSUB(void);
static void opMUL(void);
static void opDIV(void);
static void opREM(void);
static void opBAND(void);
static void opBOR(void);
static void opBNOT(void);
static void opLT(void);
static void opEQ(void);
static void opGT(void);
static void opLIT(void);
static void opSPLIT(void);
static void opSNLIT(void);
static void opVAR(void);
static void opSVAR(void);
static void opSET(void);
static void opSSET(void);
static void opGETP(void);
static void opSETP(void);
static void opPRINT(void);
static void opPNUMBER(void);
static void opPNOUN(void);
static void opTERPRI(void);
static void opEXIT(void);
static void opYORN(void);
static void opCLASS(void);
static void opMATCH(void);
static void opSAVE(void);
static void opRESTORE(void);
static void opRESTART(void);
static void opCATCH(void);
static void opCDONE(void);
static void opTHROW(void);
static void opPARSE(void);
static void opNEXT(void);
static void opXVAR(void);
static void opXSET(void);
static void opXPLIT(void);
static void opXNLIT(void);
static void badopcode(void);
static int getboperand(void);
static int getwoperand(void);

/* execute - execute adventure code */
void execute(int code)
{
    /* initialize */
    sp = fp = top = stack + STKSIZE;
    efp = NULL;
    pc = code;

    /* trap exits */
    if (setjmp(trap))
	return;

    /* execute the code */
    for (;;) {
    
	/* get the opcode */
	opcode = getcbyte(pc); ++pc;
/* printf("%04x: %02x\n",pc-1,opcode); */

	/* execute the instruction */
	((*optab[opcode]))();
    }
}

static void opCALL(void)
{
    *--sp = getboperand();
    *--sp = pc;
    *--sp = (int)(top - fp);
    fp = sp;
    pc = getafield(fp[fp[2]+3],A_CODE);
}

static void opSEND(void)
{
    register int p2;
    *--sp = getboperand();
    *--sp = pc;
    *--sp = (int)(top - fp);
    fp = sp;
    p2 = ((p2 = fp[fp[2]+3]) ? getofield(p2,O_CLASS) : fp[fp[2]+2]);
    if (p2 && (p2 = getp(p2,fp[fp[2]+1])))
	pc = getafield(p2,A_CODE);
    else {
	*sp = NIL;
	opRETURN();
    }
}

static void opRETURN(void)
{
    register int p2,p3;
    if (fp == top)
	longjmp(trap,1);
    else {
	p2 = *sp;
	sp = fp;
	fp = top - *sp++;
	pc = *sp++;
	p3 = *sp++;
	sp += p3;
	*sp = p2;
    }
}

static void opTSPACE(void)
{
    sp -= getboperand();
}

static void opTMP(void)
{
    *sp = fp[-getboperand()-1];
}

static void opTSET(void)
{
    fp[-getboperand()-1] = *sp;
}

static void opARG(void)
{
    register int p2;
    p2 = getboperand();
    if (p2 >= fp[2])
	error("too few arguments: %d %d",p2+1,fp[2]);
    *sp = fp[p2+3];
}

static void opASET(void)
{
    register int p2;
    p2 = getboperand();
    if (p2 >= fp[2])
	error("too few arguments: %d %d",p2+1,fp[2]);
    fp[p2+3] = *sp;
}

static void opBRT(void)
{
    pc = (*sp ? getwoperand() : pc+2);
}

static void opBRF(void)
{
    pc = (*sp ? pc+2 : getwoperand());
}

static void opBR(void)
{
    pc = getwoperand();
}

static void opT(void)
{
    *sp = T;
}

static void opNIL(void)
{
    *sp = NIL;
}

static void opPUSH(void)
{
    *--sp = NIL;
}

static void opNOT(void)
{
    *sp = (*sp ? NIL : T);
}

static void opADD(void)
{
    register int p2;
    p2 = *sp++;
    *sp += p2;
}

static void opSUB(void)
{
    register int p2;
    p2 = *sp++;
    *sp -= p2;
}

static void opMUL(void)
{
    register int p2;
    p2 = *sp++;
    *sp *= p2;
}

static void opDIV(void)
{
    register int p2;
    p2 = *sp++;
    *sp = (p2 == 0 ? 0 : *sp / p2);
}

static void opREM(void)
{
    register int p2;
    p2 = *sp++;
    *sp = (p2 == 0 ? 0 : *sp % p2);
}

static void opBAND(void)
{
    register int p2;
    p2 = *sp++;
    *sp &= p2;
}

static void opBOR(void)
{
    register int p2;
    p2 = *sp++;
    *sp |= p2;
}

static void opBNOT(void)
{
    *sp = ~*sp;
}

static void opLT(void)
{
    register int p2;
    p2 = *sp++;
    *sp = (*sp < p2 ? T : NIL);
}

static void opEQ(void)
{
    register int p2;
    p2 = *sp++;
    *sp = (*sp == p2 ? T : NIL);
}

static void opGT(void)
{
    register int p2;
    p2 = *sp++;
    *sp = (*sp > p2 ? T : NIL);
}

static void opLIT(void)
{
    *sp = getwoperand();
}

static void opSPLIT(void)
{
    *sp = getboperand();
}

static void opSNLIT(void)
{
    *sp = -getboperand();
}

static void opVAR(void)
{
    *sp = getvalue(getwoperand());
}

static void opSVAR(void)
{
    *sp = getvalue(getboperand());
}

static void opSET(void)
{
    setvalue(getwoperand(),*sp);
}

static void opSSET(void)
{
    setvalue(getboperand(),*sp);
}

static void opGETP(void)
{
    register int p2;
    p2 = *sp++;
    *sp = getp(*sp,p2);
}

static void opSETP(void)
{
    register int p2,p3;
    p3 = *sp++;
    p2 = *sp++;
    *sp = setp(*sp,p2,p3);
}

static void opPRINT(void)
{
    register int ch;
    msg_open(*sp);
    while ((ch = msg_byte()) != 0)
	trm_chr(ch);
}

static void opPNUMBER(void)
{
    char buf[10];
    sprintf(buf,"%d",*sp);
    trm_str(buf);
}

static void opPNOUN(void)
{
    show_noun(*sp);
}

static void opTERPRI(void)
{
    trm_chr('\n');
}

static void opEXIT(void)
{
    longjmp(trap,1);
}

static void opYORN(void)
{
    char *p;
    p = trm_get();
    *sp = (p && (*p == 'Y' || *p == 'y') ? T : NIL);
}

static void opCLASS(void)
{
    *sp = getofield(*sp,O_CLASS);
}

static void opMATCH(void)
{
    register int p2;
    p2 = *sp++;
    *sp = (match(*sp,nouns[p2-1],adjectives[p2-1]) ? T : NIL);
}

static void opSAVE(void)
{
    *sp = db_save();
}

static void opRESTORE(void)
{
    *sp = db_restore();
}

static void opRESTART(void)
{
    *sp = db_restart();
}

static void opCATCH(void)
{
    *--sp = pc;
    *--sp = (int)(top - fp);
    *--sp = (int)(top - efp);
    efp = sp;
    *--sp = NIL;
    pc += 2;
}

static void opCDONE(void)
{
    register int p2;
    p2 = *sp++;
    efp = top - *sp++;
    fp = top - *sp++;
    *++sp = p2;
}

static void opTHROW(void)
{
    register int p2;
    for (; efp != NULL; efp = top - *efp)
	if (sp[1] == efp[3])
	    break;
    if (efp) {
	p2 = *sp;
	sp = efp;
	efp = top - *sp++;
	fp = top - *sp++;
	pc = *sp++;
	pc = getwoperand();
	*sp = p2;
    }
    else
	error("no target for throw: %d",sp[1]);
}

static void opPARSE(void)
{
    *sp = parse();
}

static void opNEXT(void)
{
    *sp = next();
}

static void opXVAR(void)
{
    *sp = getvalue(opcode - OP_XVAR);
}

static void opXSET(void)
{
    setvalue(opcode - OP_XSET,*sp);
}

static void opXPLIT(void)
{
    *sp = opcode - OP_XPLIT;
}

static void opXNLIT(void)
{
    *sp = OP_XNLIT - opcode;
}

static void badopcode(void)
{
    error("bad opcode: %d",opcode);
}

/* getboperand - get data byte */
static int getboperand(void)
{
    int data;
    data = getcbyte(pc); pc += 1;
    return (data);
}

/* getwoperand - get data word */
static int getwoperand(void)
{
    int data;
    data = getcword(pc); pc += 2;
    return (data);
}

/* opcode dispatch table */
void (*optab[])(void) = {
badopcode,	/* 00   (undefined) */
opBRT,		/* 01	branch on true */
opBRF,		/* 02	branch on false */
opBR,		/* 03	branch unconditionally */
opT,		/* 04	load top of stack with t */
opNIL,		/* 05	load top of stack with nil */
opPUSH,		/* 06	push nil onto stack */
opNOT,		/* 07	logical negate top of stack */
opADD,		/* 08	add two numeric expressions */
opSUB,		/* 09	subtract two numeric expressions */
opMUL,		/* 0A	multiply two numeric expressions */
opDIV,		/* 0B	divide two numeric expressions */
opREM,		/* 0C	remainder of two numeric expressions */
opBAND,		/* 0D	bitwise and of two numeric expressions */
opBOR,		/* 0E	bitwise or of two numeric expressions */
opBNOT,		/* 0F	bitwise not of two numeric expressions */
opLT,		/* 10	less than */
opEQ,		/* 11	equal to */
opGT,		/* 12	greater than */
opLIT,		/* 13	load literal */
opVAR,		/* 14	load a variable value */
opGETP,		/* 15	get the value of an object property */
opSETP,		/* 16	set the value of an object property */
opSET,		/* 17	set the value of a variable */
opPRINT,	/* 18	print messages */
opTERPRI,	/* 19	terminate the print line */
opPNUMBER,	/* 1A	print a number */
opEXIT,		/* 1B	exit the program */
opRETURN,	/* 1C	return from interpreter */
opCALL,		/* 1D	call a function */
opSVAR,		/* 1E	short load a variable */
opSSET,		/* 1F	short set a variable */
opSPLIT,	/* 20	short load a positive literal */
opSNLIT,	/* 21	short load a negative literal */
opYORN,		/* 22	yes-or-no predicate */
opSAVE,		/* 23	save data structures */
opRESTORE,	/* 24	restore data structures */
opARG,		/* 25	load an argument value */
opASET,		/* 26	set an argument value */
opTMP,		/* 27	load a temporary variable value */
opTSET,		/* 28	set a temporary variable */
opTSPACE,	/* 29	allocate temporary variable space */
opCLASS,	/* 2A	get the class of an object */
opMATCH,	/* 2B	match a noun phrase with an object */
opPNOUN,	/* 2C	print a noun phrase */
opRESTART,	/* 2D	restart the current game */
badopcode,	/* 2E	(undefined) */
badopcode,	/* 2F	(undefined) */
opSEND,		/* 30	send a message to an object */
opCATCH,	/* 31	catch a signal (establish a catch frame) */
opCDONE,	/* 32	remove a catch frame from the stack */
opTHROW,	/* 33	throw a signal */
opPARSE,	/* 34	parse the next input line */
opNEXT,		/* 35	get the next command */
badopcode,	/* 36   (undefined) */
badopcode,	/* 37   (undefined) */
badopcode,	/* 38   (undefined) */
badopcode,	/* 39   (undefined) */
badopcode,	/* 3A   (undefined) */
badopcode,	/* 3B   (undefined) */
badopcode,	/* 3C   (undefined) */
badopcode,	/* 3D   (undefined) */
badopcode,	/* 3E   (undefined) */
badopcode,	/* 3F   (undefined) */

opXVAR,		/* 40	extra short load a variable */
opXVAR,		/* 41	extra short load a variable */
opXVAR,		/* 42	extra short load a variable */
opXVAR,		/* 43	extra short load a variable */
opXVAR,		/* 44	extra short load a variable */
opXVAR,		/* 45	extra short load a variable */
opXVAR,		/* 46	extra short load a variable */
opXVAR,		/* 47	extra short load a variable */
opXVAR,		/* 48	extra short load a variable */
opXVAR,		/* 49	extra short load a variable */
opXVAR,		/* 4A	extra short load a variable */
opXVAR,		/* 4B	extra short load a variable */
opXVAR,		/* 4C	extra short load a variable */
opXVAR,		/* 4D	extra short load a variable */
opXVAR,		/* 4E	extra short load a variable */
opXVAR,		/* 4F	extra short load a variable */
opXVAR,		/* 50	extra short load a variable */
opXVAR,		/* 51	extra short load a variable */
opXVAR,		/* 52	extra short load a variable */
opXVAR,		/* 53	extra short load a variable */
opXVAR,		/* 54	extra short load a variable */
opXVAR,		/* 55	extra short load a variable */
opXVAR,		/* 56	extra short load a variable */
opXVAR,		/* 57	extra short load a variable */
opXVAR,		/* 58	extra short load a variable */
opXVAR,		/* 59	extra short load a variable */
opXVAR,		/* 5A	extra short load a variable */
opXVAR,		/* 5B	extra short load a variable */
opXVAR,		/* 5C	extra short load a variable */
opXVAR,		/* 5D	extra short load a variable */
opXVAR,		/* 5E	extra short load a variable */
opXVAR,		/* 5F	extra short load a variable */

opXSET,		/* 60	extra short set a variable */
opXSET,		/* 61	extra short set a variable */
opXSET,		/* 62	extra short set a variable */
opXSET,		/* 63	extra short set a variable */
opXSET,		/* 64	extra short set a variable */
opXSET,		/* 65	extra short set a variable */
opXSET,		/* 66	extra short set a variable */
opXSET,		/* 67	extra short set a variable */
opXSET,		/* 68	extra short set a variable */
opXSET,		/* 69	extra short set a variable */
opXSET,		/* 6A	extra short set a variable */
opXSET,		/* 6B	extra short set a variable */
opXSET,		/* 6C	extra short set a variable */
opXSET,		/* 6D	extra short set a variable */
opXSET,		/* 6E	extra short set a variable */
opXSET,		/* 6F	extra short set a variable */
opXSET,		/* 70	extra short set a variable */
opXSET,		/* 71	extra short set a variable */
opXSET,		/* 72	extra short set a variable */
opXSET,		/* 73	extra short set a variable */
opXSET,		/* 74	extra short set a variable */
opXSET,		/* 75	extra short set a variable */
opXSET,		/* 76	extra short set a variable */
opXSET,		/* 77	extra short set a variable */
opXSET,		/* 78	extra short set a variable */
opXSET,		/* 79	extra short set a variable */
opXSET,		/* 7A	extra short set a variable */
opXSET,		/* 7B	extra short set a variable */
opXSET,		/* 7C	extra short set a variable */
opXSET,		/* 7D	extra short set a variable */
opXSET,		/* 7E	extra short set a variable */
opXSET,		/* 7F	extra short set a variable */

opXPLIT,	/* 80	extra short load a positive literal */
opXPLIT,	/* 81	extra short load a positive literal */
opXPLIT,	/* 82	extra short load a positive literal */
opXPLIT,	/* 83	extra short load a positive literal */
opXPLIT,	/* 84	extra short load a positive literal */
opXPLIT,	/* 85	extra short load a positive literal */
opXPLIT,	/* 86	extra short load a positive literal */
opXPLIT,	/* 87	extra short load a positive literal */
opXPLIT,	/* 88	extra short load a positive literal */
opXPLIT,	/* 89	extra short load a positive literal */
opXPLIT,	/* 8A	extra short load a positive literal */
opXPLIT,	/* 8B	extra short load a positive literal */
opXPLIT,	/* 8C	extra short load a positive literal */
opXPLIT,	/* 8D	extra short load a positive literal */
opXPLIT,	/* 8E	extra short load a positive literal */
opXPLIT,	/* 8F	extra short load a positive literal */
opXPLIT,	/* 90	extra short load a positive literal */
opXPLIT,	/* 91	extra short load a positive literal */
opXPLIT,	/* 92	extra short load a positive literal */
opXPLIT,	/* 93	extra short load a positive literal */
opXPLIT,	/* 94	extra short load a positive literal */
opXPLIT,	/* 95	extra short load a positive literal */
opXPLIT,	/* 96	extra short load a positive literal */
opXPLIT,	/* 97	extra short load a positive literal */
opXPLIT,	/* 98	extra short load a positive literal */
opXPLIT,	/* 99	extra short load a positive literal */
opXPLIT,	/* 9A	extra short load a positive literal */
opXPLIT,	/* 9B	extra short load a positive literal */
opXPLIT,	/* 9C	extra short load a positive literal */
opXPLIT,	/* 9D	extra short load a positive literal */
opXPLIT,	/* 9E	extra short load a positive literal */
opXPLIT,	/* 9F	extra short load a positive literal */
opXPLIT,	/* A0	extra short load a positive literal */
opXPLIT,	/* A1	extra short load a positive literal */
opXPLIT,	/* A2	extra short load a positive literal */
opXPLIT,	/* A3	extra short load a positive literal */
opXPLIT,	/* A4	extra short load a positive literal */
opXPLIT,	/* A5	extra short load a positive literal */
opXPLIT,	/* A6	extra short load a positive literal */
opXPLIT,	/* A7	extra short load a positive literal */
opXPLIT,	/* A8	extra short load a positive literal */
opXPLIT,	/* A9	extra short load a positive literal */
opXPLIT,	/* AA	extra short load a positive literal */
opXPLIT,	/* AB	extra short load a positive literal */
opXPLIT,	/* AC	extra short load a positive literal */
opXPLIT,	/* AD	extra short load a positive literal */
opXPLIT,	/* AE	extra short load a positive literal */
opXPLIT,	/* AF	extra short load a positive literal */
opXPLIT,	/* B0	extra short load a positive literal */
opXPLIT,	/* B1	extra short load a positive literal */
opXPLIT,	/* B2	extra short load a positive literal */
opXPLIT,	/* B3	extra short load a positive literal */
opXPLIT,	/* B4	extra short load a positive literal */
opXPLIT,	/* B5	extra short load a positive literal */
opXPLIT,	/* B6	extra short load a positive literal */
opXPLIT,	/* B7	extra short load a positive literal */
opXPLIT,	/* B8	extra short load a positive literal */
opXPLIT,	/* B9	extra short load a positive literal */
opXPLIT,	/* BA	extra short load a positive literal */
opXPLIT,	/* BB	extra short load a positive literal */
opXPLIT,	/* BC	extra short load a positive literal */
opXPLIT,	/* BD	extra short load a positive literal */
opXPLIT,	/* BE	extra short load a positive literal */
opXPLIT,	/* BF	extra short load a positive literal */

opXNLIT,	/* C0	extra short load a negative literal */
opXNLIT,	/* C1	extra short load a negative literal */
opXNLIT,	/* C2	extra short load a negative literal */
opXNLIT,	/* C3	extra short load a negative literal */
opXNLIT,	/* C4	extra short load a negative literal */
opXNLIT,	/* C5	extra short load a negative literal */
opXNLIT,	/* C6	extra short load a negative literal */
opXNLIT,	/* C7	extra short load a negative literal */
opXNLIT,	/* C8	extra short load a negative literal */
opXNLIT,	/* C9	extra short load a negative literal */
opXNLIT,	/* CA	extra short load a negative literal */
opXNLIT,	/* CB	extra short load a negative literal */
opXNLIT,	/* CC	extra short load a negative literal */
opXNLIT,	/* CD	extra short load a negative literal */
opXNLIT,	/* CE	extra short load a negative literal */
opXNLIT,	/* CF	extra short load a negative literal */
opXNLIT,	/* D0	extra short load a negative literal */
opXNLIT,	/* D1	extra short load a negative literal */
opXNLIT,	/* D2	extra short load a negative literal */
opXNLIT,	/* D3	extra short load a negative literal */
opXNLIT,	/* D4	extra short load a negative literal */
opXNLIT,	/* D5	extra short load a negative literal */
opXNLIT,	/* D6	extra short load a negative literal */
opXNLIT,	/* D7	extra short load a negative literal */
opXNLIT,	/* D8	extra short load a negative literal */
opXNLIT,	/* D9	extra short load a negative literal */
opXNLIT,	/* DA	extra short load a negative literal */
opXNLIT,	/* DB	extra short load a negative literal */
opXNLIT,	/* DC	extra short load a negative literal */
opXNLIT,	/* DD	extra short load a negative literal */
opXNLIT,	/* DE	extra short load a negative literal */
opXNLIT,	/* DF	extra short load a negative literal */
opXNLIT,	/* E0	extra short load a negative literal */
opXNLIT,	/* E1	extra short load a negative literal */
opXNLIT,	/* E2	extra short load a negative literal */
opXNLIT,	/* E3	extra short load a negative literal */
opXNLIT,	/* E4	extra short load a negative literal */
opXNLIT,	/* E5	extra short load a negative literal */
opXNLIT,	/* E6	extra short load a negative literal */
opXNLIT,	/* E7	extra short load a negative literal */
opXNLIT,	/* E8	extra short load a negative literal */
opXNLIT,	/* E9	extra short load a negative literal */
opXNLIT,	/* EA	extra short load a negative literal */
opXNLIT,	/* EB	extra short load a negative literal */
opXNLIT,	/* EC	extra short load a negative literal */
opXNLIT,	/* ED	extra short load a negative literal */
opXNLIT,	/* EE	extra short load a negative literal */
opXNLIT,	/* EF	extra short load a negative literal */
opXNLIT,	/* F0	extra short load a negative literal */
opXNLIT,	/* F1	extra short load a negative literal */
opXNLIT,	/* F2	extra short load a negative literal */
opXNLIT,	/* F3	extra short load a negative literal */
opXNLIT,	/* F4	extra short load a negative literal */
opXNLIT,	/* F5	extra short load a negative literal */
opXNLIT,	/* F6	extra short load a negative literal */
opXNLIT,	/* F7	extra short load a negative literal */
opXNLIT,	/* F8	extra short load a negative literal */
opXNLIT,	/* F9	extra short load a negative literal */
opXNLIT,	/* FA	extra short load a negative literal */
opXNLIT,	/* FB	extra short load a negative literal */
opXNLIT,	/* FC	extra short load a negative literal */
opXNLIT,	/* FD	extra short load a negative literal */
opXNLIT,	/* FE	extra short load a negative literal */
opXNLIT		/* FF	extra short load a negative literal */
};
