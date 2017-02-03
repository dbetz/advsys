/* advexp.c - expression compiler for adventure games */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <string.h>
#include "advavl.h"
#include "advcom.h"
#include "advdbs.h"

/* external routines */
extern SYMBOL *sfind();

/* external variables */
extern SYMBOL *symbols,*exfunctions;
extern char t_token[];
extern int t_value;
extern int curobj;
extern char *code;
extern int cptr;

/* opcode tables */
static struct { char *nt_name; int nt_code,nt_args; } *nptr,ntab[] = {
{	"not",		OP_NOT,		1	},
{	"+",		OP_ADD,		2	},
{	"-",		OP_SUB,		2	},
{	"*",		OP_MUL,		2	},
{	"/",		OP_DIV,		2	},
{	"%",		OP_REM,		2	},
{	"&",		OP_BAND,	2	},
{	"|",		OP_BOR,		2	},
{	"~",		OP_BNOT,	1	},
{	"<",		OP_LT,		2	},
{	"=",		OP_EQ,		2	},
{	">",		OP_GT,		2	},
{	"getp",		OP_GETP,	2	},
{	"setp",		OP_SETP,	3	},
{	"class",	OP_CLASS,	1	},
{	"match",	OP_MATCH,	2	},
{	"print",	OP_PRINT,	1	},
{	"print-number",	OP_PNUMBER,	1	},
{	"print-noun",	OP_PNOUN,	1	},
{	"terpri",	OP_TERPRI,	0	},
{	"exit",		OP_EXIT,	0	},
{	"save",		OP_SAVE,	0	},
{	"restore",	OP_RESTORE,	0	},
{	"restart",	OP_RESTART,	0	},
{	"yes-or-no",	OP_YORN,	0	},
{	"throw",	OP_THROW,	2	},
{	"parse",	OP_PARSE,	0	},
{	"next",		OP_NEXT,	0	},
{	0,		0,		0	}
};
static struct { char *ft_name; void (*ft_fcn)(void); } *fptr,ftab[] = {
{	"cond",		do_cond		},
{	"and",		do_and		},
{	"or",		do_or		},
{	"if",		do_if		},
{	"while",	do_while	},
{	"progn",	do_progn	},
{	"setq",		do_setq		},
{	"return",	do_return	},
{	"send",		do_send		},
{	"send-super",	do_sndsuper	},
{	"catch",	do_catch	},
{	0,		0		}
};

/* do_expr - compile a subexpression */
void do_expr(void)
{
    int tkn;

    switch (token()) {
    case T_OPEN:
	switch (tkn = token()) {
	case T_IDENTIFIER:
	    if (in_ntab() || in_ftab() || in_etab())
		break;
	default:
	    stoken(tkn);
	    do_call();
	}
	break;
    case T_NUMBER:
	do_literal();
	break;
    case T_STRING:
	do_literal();
	break;
    case T_IDENTIFIER:
	do_identifier();
	break;
    default:
	error("Expecting expression");
    }
}

/* in_ntab - check for a function in ntab */
int in_ntab(void)
{
    for (nptr = ntab; nptr->nt_name; ++nptr)
	if (strcmp(t_token,nptr->nt_name) == 0) {
	    do_nary(nptr->nt_code,nptr->nt_args);
	    return (TRUE);
	}
    return (FALSE);
}

/* in_ftab - check for a function in ftab */
int in_ftab(void)
{
    for (fptr = ftab; fptr->ft_name; ++fptr)
	if (strcmp(t_token,fptr->ft_name) == 0) {
	    (*fptr->ft_fcn)();
	    return (TRUE);
	}
    return (FALSE);
}

/* in_etab - compile an extended function call */
int in_etab(void)
{
    SYMBOL *fcn;
    int tkn,n;
    
    /* find the extended function definition */
    if ((fcn = sfind(exfunctions,t_token)) == NULL)
	return (FALSE);
    
    /* compile each argument expression */
    for (n = 0; (tkn = token()) != T_CLOSE; ++n) {
	stoken(tkn);
	putcbyte(OP_PUSH);
	do_expr();
    }

    /* compile the extended function call */
    putcbyte(OP_EXTEND);
    putcbyte(fcn->s_value);
    putcbyte(n);

    /* return successfully */
    return (TRUE);
}

/* do_cond - compile the (COND ... ) expression */
void do_cond(void)
{
    int tkn,nxt,end;

    /* initialize the fixup chain */
    end = NIL;

    /* compile each COND clause */
    while ((tkn = token()) != T_CLOSE) {
	require(tkn,T_OPEN);
	do_expr();
	putcbyte(OP_BRF);
	nxt = putcword(NIL);
	while ((tkn = token()) != T_CLOSE) {
	    stoken(tkn);
	    do_expr();
	}
	putcbyte(OP_BR);
	end = putcword(end);
	fixup(nxt,cptr);
    }

    /* fixup references to the end of statement */
    if (end)
	fixup(end,cptr);
    else
	putcbyte(OP_NIL);
}

/* do_and - compile the (AND ... ) expression */
void do_and(void)
{
    int tkn,end;

    /* initialize the fixup chain */
    end = NIL;

    /* compile each expression */
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
	putcbyte(OP_BRF);
	end = putcword(end);
    }

    /* fixup references to the end of statement */
    if (end)
	fixup(end,cptr);
    else
	putcbyte(OP_NIL);
}

/* do_or - compile the (OR ... ) expression */
void do_or(void)
{
    int tkn,end;

    /* initialize the fixup chain */
    end = NIL;

    /* compile each expression */
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
	putcbyte(OP_BRT);
	end = putcword(end);
    }

    /* fixup references to the end of statement */
    if (end)
	fixup(end,cptr);
    else
	putcbyte(OP_T);
}

/* do_if - compile the (IF ... ) expression */
void do_if(void)
{
    int tkn,nxt,end;

    /* compile the test expression */
    do_expr();

    /* skip around the 'then' clause if the expression is false */
    putcbyte(OP_BRF);
    nxt = putcword(NIL);

    /* compile the 'then' clause */
    do_expr();

    /* compile the 'else' clause */
    if ((tkn = token()) != T_CLOSE) {
	putcbyte(OP_BR);
	end = putcword(NIL);
	fixup(nxt,cptr);
	stoken(tkn);
	do_expr();
	frequire(T_CLOSE);
	nxt = end;
    }

    /* handle the end of the statement */
    fixup(nxt,cptr);
}

/* do_while - compile the (WHILE ... ) expression */
void do_while(void)
{
    int tkn,nxt,end;

    /* compile the test expression */
    nxt = cptr;
    do_expr();

    /* skip around the 'then' clause if the expression is false */
    putcbyte(OP_BRF);
    end = putcword(NIL);

    /* compile the loop body */
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
    }

    /* branch back to the start of the loop */
    putcbyte(OP_BR);
    putcword(nxt);

    /* handle the end of the statement */
    fixup(end,cptr);
}

/* do_progn - compile the (PROGN ... ) expression */
void do_progn(void)
{
    int tkn,n;

    /* compile each expression */
    for (n = 0; (tkn = token()) != T_CLOSE; ++n) {
	stoken(tkn);
	do_expr();
    }

    /* check for an empty statement list */
    if (n == 0)
	putcbyte(OP_NIL);
}

/* do_setq - compile the (SETQ v x) expression */
void do_setq(void)
{
    char name[TKNSIZE+1];
    int n;

    /* get the symbol name */
    frequire(T_IDENTIFIER);
    strcpy(name,t_token);

    /* compile the value expression */
    do_expr();

    /* check for this being a local symbol */
    if ((n = findarg(name)) >= 0)
	cd_setargument(n);
    else if ((n = findtmp(name)) >= 0)
	cd_settemporary(n);
    else {
	n = venter(name);
	cd_setvariable(n);
    }
    frequire(T_CLOSE);
}

/* do_return - handle the (RETURN [expr]) expression */
void do_return(void)
{
    int tkn;

    /* look for a result expression */
    if ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
	frequire(T_CLOSE);
    }

    /* otherwise, default the result to nil */
    else
	putcbyte(OP_NIL);

    /* insert the return opcode */
    putcbyte(OP_RETURN);
}

/* do_send - handle the (SEND obj msg [expr]...) expression */
void do_send(void)
{
    /* start searching for the method at the object itself */
    putcbyte(OP_NIL);

    /* compile the object expression */
    putcbyte(OP_PUSH);
    do_expr();

    /* call the general message sender */
    sender();
}

/* do_sndsuper - handle the (SEND-SUPER msg [expr]...) expression */
void do_sndsuper(void)
{
    /* start searching for the method at the current class object */
    code_literal(curobj);

    /* pass the message to "self" */
    putcbyte(OP_PUSH);
    code_argument(findarg("self"));

    /* call the general message sender */
    sender();
}

/* sender - compile an expression to send a message to an object */
void sender(void)
{
    int tkn,n;
    
    /* compile the selector expression */
    putcbyte(OP_PUSH);
    do_expr();

    /* compile each argument expression */
    for (n = 2; (tkn = token()) != T_CLOSE; ++n) {
	stoken(tkn);
	putcbyte(OP_PUSH);
	do_expr();
    }
    putcbyte(OP_SEND);
    putcbyte(n);
}

/* do_catch - comple a (CATCH ...) expression */
void do_catch(void)
{
    int tkn,end;

    /* compile the catch tag */
    do_expr();

    /* setup the catch frame */
    putcbyte(OP_CATCH);
    end = putcword(NIL);

    /* compile the body of the catch */
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
    }

    /* remove the catch frame */
    putcbyte(OP_CDONE);

    /* handle the end of the statement */
    fixup(end,cptr);
}

/* do_call - compile a function call */
void do_call(void)
{
    int tkn,n;
    
    /* compile the function itself */
    do_expr();

    /* compile each argument expression */
    for (n = 0; (tkn = token()) != T_CLOSE; ++n) {
	stoken(tkn);
	putcbyte(OP_PUSH);
	do_expr();
    }
    putcbyte(OP_CALL);
    putcbyte(n);
}

/* do_nary - compile nary operator expressions */
void do_nary(int op,int n)
{
    while (n--) {
	do_expr();
	if (n) putcbyte(OP_PUSH);
    }
    putcbyte(op);
    frequire(T_CLOSE);
}

/* do_literal - compile a literal */
void do_literal(void)
{
    code_literal(t_value);
}

/* do_identifier - compile an identifier */
void do_identifier(void)
{
    SYMBOL *sym;
    int n;
    
    if (match("t"))
	putcbyte(OP_T);
    else if (match("nil"))
	putcbyte(OP_NIL);
    else if ((n = findarg(t_token)) >= 0)
	code_argument(n);
    else if ((n = findtmp(t_token)) >= 0)
	code_temporary(n);
    else if ((sym = sfind(symbols,t_token)) != 0) {
	if (sym->s_type == ST_VARIABLE)
	    code_variable(sym->s_value);
	else
	    code_literal(sym->s_value);
    }
    else
	code_literal(oenter(t_token));
}

/* code_argument - compile an argument reference */
void code_argument(int n)
{
    putcbyte(OP_ARG);
    putcbyte(n);
}

/* cd_setargument - compile a set argument reference */
void cd_setargument(int n)
{
    putcbyte(OP_ASET);
    putcbyte(n);
}

/* code_temporary - compile an temporary reference */
void code_temporary(int n)
{
    putcbyte(OP_TMP);
    putcbyte(n);
}

/* cd_settemporary - compile a set temporary reference */
void cd_settemporary(int n)
{
    putcbyte(OP_TSET);
    putcbyte(n);
}

/* code_variable - compile a variable reference */
void code_variable(int n)
{
    if (n < 32)
	putcbyte(OP_XVAR+n);
    else if (n < 256)
	{ putcbyte(OP_SVAR); putcbyte(n); }
    else
	{ putcbyte(OP_VAR); putcword(n); }
}

/* cd_setvariable - compile a set variable reference */
void cd_setvariable(int n)
{
    if (n < 32)
	putcbyte(OP_XSET+n);
    else if (n < 256)
	{ putcbyte(OP_SSET); putcbyte(n); }
    else
	{ putcbyte(OP_SET); putcword(n); }
}

/* code_literal - compile a literal reference */
void code_literal(int n)
{
    if (n >= 0 && n < 64)
	putcbyte(OP_XPLIT+n);
    else if (n < 0 && n > -64)
	putcbyte(OP_XNLIT-n);
    else if (n >= 64 && n < 256)
	{ putcbyte(OP_SPLIT); putcbyte(n); }
    else if (n <= -64 && n > -256)
	{ putcbyte(OP_SNLIT); putcbyte(-n); }
    else
	{ putcbyte(OP_LIT); putcword(n); }
}

/* do_op - insert an opcode and look for closing paren */
void do_op(int op)
{
    putcbyte(op);
    frequire(T_CLOSE);
}

/* putcbyte - put a code byte into data space */
int putcbyte(int b)
{
    if (cptr < CMAX)
	code[cptr++] = b;
    else
	error("insufficient code space");
    return (cptr-1);
}

/* putcword - put a code word into data space */
int putcword(int w)
{
    putcbyte(w);
    putcbyte(w >> 8);
    return (cptr-2);
}

/* fixup - fixup a reference chain */
void fixup(int chn,int val)
{
    int hval,nxt;

    /* store the value into each location in the chain */
    for (hval = val >> 8; chn != NIL; chn = nxt) {
	if (chn < 0 || chn > CMAX-2)
	    return;
	nxt = (code[chn] & 0xFF) | (code[chn+1] << 8);
	code[chn] = val;
	code[chn+1] = hval;
    }
}

                                                                                                                            
