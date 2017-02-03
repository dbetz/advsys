/* advfcn.c - functions for the adventure compiler */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "advavl.h"
#include "advcom.h"
#include "advdbs.h"

/* external variables */
extern char aname[];		/* adventure name */
extern int aversion;		/* adventure version number */
extern int cptr;		/* code space pointer */
extern int objbuf[];		/* object staging buffer */
extern int nprops;		/* number of properties in current object */
extern int t_value;		/* token value */
extern char t_token[];		/* token string */
extern char *t_names[];		/* token names */
extern int otable[];		/* object table */
extern int curobj;		/* current object number */
extern int curact;		/* current action offset */
extern int atable[],acnt;	/* action table and count */
extern SYMBOL *exfunctions;	/* extended function list */
extern ARGUMENT *arguments;	/* function argument list */
extern ARGUMENT *temporaries;	/* function temporary variable list */
extern int def_flag;		/* default action flag value */
extern int def_mask;		/* default action mask value */

/* do_adventure - handle the <ADVENTURE name version-number> statement */
void do_adventure(void)
{
    /* get the adventure name */
    frequire(T_IDENTIFIER);
    strncpy(aname,t_token,18);
    aname[18] = 0;

    /* get the adventure version number */
    frequire(T_NUMBER);
    aversion = t_value;

    /* check for the closing paren */
    frequire(T_CLOSE);
}

/* do_word - enter words of a particular type */
void do_word(int type)
{
    int tkn;

    while ((tkn = token()) == T_IDENTIFIER)
	add_word(t_token,type);
    require(tkn,T_CLOSE);
}

/* do_synonym - handle the <SYNONYMS ... > statement */
void do_synonym(void)
{
    int tkn,wrd;

    frequire(T_IDENTIFIER);
    wrd = add_word(t_token,WT_UNKNOWN);
    while ((tkn = token()) == T_IDENTIFIER)
	add_synonym(t_token,wrd);
    require(tkn,T_CLOSE);
}

/* do_define - handle the <DEFINE ... > statement */
void do_define(void)
{
    char name[TKNSIZE+1];
    int tkn;

    if ((tkn = token()) == T_OPEN)
	do_function();
    else {
	stoken(tkn);
	while ((tkn = token()) == T_IDENTIFIER) {
	    strcpy(name,t_token);
	    center(name,getvalue());
	}
	require(tkn,T_CLOSE);
    }
}

/* do_extended - handle the <EXTENDED ... > statement */
void do_extended(void)
{
    char name[TKNSIZE+1];

    frequire(T_IDENTIFIER);
    strcpy(name,t_token);
    frequire(T_NUMBER);
    senter(&exfunctions,name,0,t_value);
    frequire(T_CLOSE);
}

/* do_variable - handle the <VARIABLE ... > statement */
void do_variable(void)
{
    int tkn;

    while ((tkn = token()) == T_IDENTIFIER)
	venter(t_token);
    require(tkn,T_CLOSE);
}

/* do_defproperty - handle the <PROPERTY ... > statement */
void do_defproperty(void)
{
    int tkn;

    while ((tkn = token()) == T_IDENTIFIER)
	penter(t_token);
    require(tkn,T_CLOSE);
}

/* do_default - handle the <DEFAULT ... > statement */
void do_default(void)
{
    int tkn;

    /* process statements until end of file */
    while ((tkn = token()) == T_OPEN) {
	frequire(T_IDENTIFIER);
	if (match("actor"))
	    do_dflag(A_ACTOR);
	else if (match("direct-object"))
	    do_dflag(A_DOBJECT);
	else if (match("indirect-object"))
	    do_dflag(A_IOBJECT);
	else
	    error("Unknown default definition statement type");
    }
    require(tkn,T_CLOSE);
}

/* do_dflag - handle ACTOR, DIRECT-OBJECT, and INDIRECT-OBJECT statements */
void do_dflag(int flag)
{
    int tkn;

    if ((tkn = token()) == T_IDENTIFIER) {
	if (match("required")) {
	    def_flag |= flag;
	    def_mask &= ~flag;
	}
	else if (match("forbidden")) {
	    def_flag &= ~flag;
	    def_mask &= ~flag;
	}
	else if (match("optional"))
	    def_mask |= flag;
	else
	    error("Expecting: REQUIRED, FORBIDDEN or OPTIONAL");
	tkn = token();
    }
    else {
	def_flag |= flag;
	def_mask &= ~flag;
    }
    require(tkn,T_CLOSE);
}

/* do_object - handle object (LOCATION,OBJECT,ACTOR) definitions */
int do_object(char *cname,int class)
{
    int tkn,obj,obase,osize,i,p;

printf("[ %s: ",cname);
    frequire(T_IDENTIFIER);
printf("%s ]\n",t_token);
    obj = curobj = oenter(t_token);

    /* initialize the object */
    objbuf[O_CLASS/2] = class;
    objbuf[O_NOUNS/2] = NIL;
    objbuf[O_ADJECTIVES/2] = NIL;
    objbuf[O_NPROPERTIES/2] = nprops = 0;

    /* copy the property list of the class object */
    if (class) {
	obase = otable[class];
	osize = getword(obase+O_NPROPERTIES);
	for (i = p = 0; i < osize; i++, p += 4)
	    if ((getword(obase+O_PROPERTIES+p) & P_CLASS) == 0)
		addprop(getword(obase+O_PROPERTIES+p),0,
                        getword(obase+O_PROPERTIES+p+2));
    }

    /* process statements until end of file */
    while ((tkn = token()) == T_OPEN) {
	frequire(T_IDENTIFIER);
	if (match("noun"))
	    do_noun();
	else if (match("adjective"))
	    do_adjective();
	else if (match("property"))
	    do_property(0);
	else if (match("class-property"))
	    do_property(P_CLASS);
	else if (match("method"))
	    do_method();
	else
	    error("Unknown object definition statement type");
    }
    require(tkn,T_CLOSE);

    /* copy the object to data memory */
    osize = O_SIZE/2 + nprops*2;
    obase = dalloc(osize*2);
    for (i = p = 0; i < osize; i++, p += 2)
	putword(obase+p,objbuf[i]);
    otable[obj] = obase;
    curobj = NIL;

    /* return the object number */
    return (obj);
}

/* do_noun - handle the <NOUN ... > statement */
void do_noun(void)
{
    int tkn,new;

    while ((tkn = token()) == T_IDENTIFIER) {
	new = dalloc(L_SIZE);
	putword(new+L_DATA,add_word(t_token,WT_NOUN));
	putword(new+L_NEXT,objbuf[O_NOUNS/2]);
	objbuf[O_NOUNS/2] = new;
    }
    require(tkn,T_CLOSE);
}

/* do_adjective - handle the <ADJECTIVE ... > statement */
void do_adjective(void)
{
    int tkn,new;

    while ((tkn = token()) == T_IDENTIFIER) {
	new = dalloc(L_SIZE);
	putword(new+L_DATA,add_word(t_token,WT_ADJECTIVE));
	putword(new+L_NEXT,objbuf[O_ADJECTIVES/2]);
	objbuf[O_ADJECTIVES/2] = new;
    }
    require(tkn,T_CLOSE);
}

/* do_property - handle the <PROPERTY ... > statement */
void do_property(int flags)
{
    int tkn,name,value;

    while ((tkn = token()) == T_IDENTIFIER || tkn == T_NUMBER) {
	name = (tkn == T_IDENTIFIER ? penter(t_token) : t_value);
	value = getvalue();
	setprop(name,flags,value);
    }
    require(tkn,T_CLOSE);
}

/* do_method - handle <METHOD (FUN ...) ... > statement */
void do_method(void)
{
    int tkn,name,tcnt;

    /* get the property name */
    frequire(T_OPEN);
    frequire(T_IDENTIFIER);
printf("[ method: %s ]\n",t_token);

    /* create a new property */
    name = penter(t_token);

    /* allocate a new (anonymous) action */
    if (acnt < AMAX)
	++acnt;
    else
	error("too many actions");

    /* store the action as the value of the property */
    setprop(name,P_CLASS,acnt);

    /* initialize the action */
    curact = atable[acnt] = dalloc(A_SIZE);
    putword(curact+A_VERBS,NIL);
    putword(curact+A_PREPOSITIONS,NIL);
    arguments = temporaries = NULL;
    tcnt = 0;

    /* enter the "self" argument */
    addargument(&arguments,"self");
    addargument(&arguments,"(dummy)");

    /* get the argument list */
    while ((tkn = token()) != T_CLOSE) {
	require(tkn,T_IDENTIFIER);
	if (match("&aux"))
	    break;
	addargument(&arguments,t_token);
    }
    
    /* check for temporary variable definitions */
    if (tkn == T_IDENTIFIER)
	while ((tkn = token()) != T_CLOSE) {
	    require(tkn,T_IDENTIFIER);
	    addargument(&temporaries,t_token);
	    tcnt++;
	}

    /* store the code address */
    putword(curact+A_CODE,cptr);

    /* allocate space for temporaries */
    if (temporaries) {
	putcbyte(OP_TSPACE);
	putcbyte(tcnt);
    }

    /* compile the code */
    do_code(NULL);

    /* free the argument and temporary variable symbol tables */
    freelist(arguments);
    freelist(temporaries);
    arguments = temporaries = NULL;
}

/* setprop - set the value of a property */
void setprop(int prop,int flags,int value)
{
    int i;

    /* look for the property */
    for (i = 0; i < nprops; i++)
	if ((objbuf[O_PROPERTIES/2 + i*2] & ~P_CLASS) == prop) {
	    objbuf[O_PROPERTIES/2 + i*2 + 1] = value;
	    return;
	}
    addprop(prop,flags,value);
}

/* addprop - add a property to the current object's property list */
void addprop(int prop,int flags,int value)
{
    if (nprops >= OPMAX) {
	printf("too many properties for this object\n");
	return;
    }
    objbuf[O_PROPERTIES/2 + nprops*2] = prop|flags;
    objbuf[O_PROPERTIES/2 + nprops*2 + 1] = value;
    objbuf[O_NPROPERTIES/2] = ++nprops;
}

/* do_code - compile code for an expression */
int do_code(char *type)
{
    int adr,tkn;

    if (type) printf("[ compiling %s code ]\n",type);
    adr = putcbyte(OP_PUSH);
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
    }
    putcbyte(OP_RETURN);
    return (adr);
}

/* do_action - handle <ACTION ... > statement */
void do_action(void)
{
    int tkn,act;

    /* get the action name */
    frequire(T_IDENTIFIER);
printf("[ action: %s ]\n",t_token);

    /* create a new action */
    act = aenter(t_token);
    if ((curact = atable[act]) == NIL) {
	curact = atable[act] = dalloc(A_SIZE);
	putword(curact+A_VERBS,NIL);
	putword(curact+A_PREPOSITIONS,NIL);
	putbyte(curact+A_FLAG,def_flag);
	putbyte(curact+A_MASK,def_mask);
	putword(curact+A_CODE,NIL);
    }

    /* process statements until end of file */
    while ((tkn = token()) == T_OPEN) {
	frequire(T_IDENTIFIER);
	if (match("actor"))
	    do_flag(A_ACTOR);
	else if (match("verb"))
	    do_verb();
	else if (match("direct-object"))
	    do_flag(A_DOBJECT);
	else if (match("preposition"))
	    do_preposition();
	else if (match("indirect-object"))
	    do_flag(A_IOBJECT);
	else if (match("code"))
	    putword(curact+A_CODE,do_code(NULL));
	else
	    error("Unknown action definition statement type");
    }
    require(tkn,T_CLOSE);
}

/* do_flag - handle ACTOR, DIRECT-OBJECT, and INDIRECT-OBJECT statements */
void do_flag(int flag)
{
    int tkn;

    if ((tkn = token()) == T_IDENTIFIER) {
	if (match("required")) {
	    putbyte(curact+A_FLAG,getbyte(curact+A_FLAG) | flag);
	    putbyte(curact+A_MASK,getbyte(curact+A_MASK) & ~flag);
	}
	else if (match("forbidden")) {
	    putbyte(curact+A_FLAG,getbyte(curact+A_FLAG) & ~flag);
	    putbyte(curact+A_MASK,getbyte(curact+A_MASK) & ~flag);
	}
	else if (match("optional"))
	    putbyte(curact+A_MASK,getbyte(curact+A_MASK) | flag);
	else
	    error("Expecting: REQUIRED, FORBIDDEN or OPTIONAL");
	tkn = token();
    }
    else {
	putbyte(curact+A_FLAG,getbyte(curact+A_FLAG) | flag);
	putbyte(curact+A_MASK,getbyte(curact+A_MASK) & ~flag);
    }
    require(tkn,T_CLOSE);
}

/* do_verb - handle the <VERB ... > statement */
void do_verb(void)
{
    int tkn,new,lst;

    while ((tkn = token()) == T_IDENTIFIER || tkn == T_OPEN) {
	new = dalloc(L_SIZE);
	putword(new+L_NEXT,getword(curact+A_VERBS));
	putword(curact+A_VERBS,new);
	lst = dalloc(L_SIZE);
	putword(lst+L_NEXT,NIL);
	putword(new+L_DATA,lst);
	if (tkn == T_IDENTIFIER)
	    putword(lst+L_DATA,add_word(t_token,WT_VERB));
	else {
	    if ((tkn = token()) == T_IDENTIFIER)
		putword(lst+L_DATA,add_word(t_token,WT_VERB));
	    else
		error("Expecting verb");
	    while ((tkn = token()) == T_IDENTIFIER) {
		new = dalloc(L_SIZE);
		putword(new+L_DATA,add_word(t_token,WT_UNKNOWN));
		putword(new+L_NEXT,NIL);
		putword(lst+L_NEXT,new);
		lst = new;
	    }
	    require(tkn,T_CLOSE);
	}
    }
    require(tkn,T_CLOSE);
}

/* do_preposition - handle the <PREPOSITION ... > statement */
void do_preposition(void)
{
    int tkn,new;

    while ((tkn = token()) == T_IDENTIFIER) {
	new = dalloc(L_SIZE);
	putword(new+L_DATA,add_word(t_token,WT_PREPOSITION));
	putword(new+L_NEXT,getword(curact+A_PREPOSITIONS));
	putword(curact+A_PREPOSITIONS,new);
    }
    require(tkn,T_CLOSE);
}

/* do_function - handle <DEFINE (FUN ...) ... > statement */
void do_function(void)
{
    int tkn,act,tcnt;

    /* get the function name */
    frequire(T_IDENTIFIER);
printf("[ function: %s ]\n",t_token);

    /* create a new action */
    act = aenter(t_token);

    /* initialize the action */
    if ((curact = atable[act]) == NIL) {
	curact = atable[act] = dalloc(A_SIZE);
	putword(curact+A_VERBS,NIL);
	putword(curact+A_PREPOSITIONS,NIL);
	putbyte(curact+A_FLAG,def_flag);
	putbyte(curact+A_MASK,def_mask);
	putword(curact+A_CODE,NIL);
    }
    
    /* setup the symbol tables */
    arguments = temporaries = NULL;
    tcnt = 0;

    /* get the argument list */
    while ((tkn = token()) != T_CLOSE) {
	require(tkn,T_IDENTIFIER);
	if (match("&aux"))
	    break;
	addargument(&arguments,t_token);
    }
    
    /* check for temporary variable definitions */
    if (tkn == T_IDENTIFIER)
	while ((tkn = token()) != T_CLOSE) {
	    require(tkn,T_IDENTIFIER);
	    addargument(&temporaries,t_token);
	    tcnt++;
	}

    /* store the code address */
    putword(curact+A_CODE,cptr);

    /* allocate space for temporaries */
    if (temporaries) {
	putcbyte(OP_TSPACE);
	putcbyte(tcnt);
    }

    /* compile the code */
    do_code(NULL);

    /* free the argument and temporary variable symbol tables */
    freelist(arguments);
    freelist(temporaries);
    arguments = temporaries = NULL;
}

/* addargument - add a formal argument */
void addargument(ARGUMENT **list,char *name)
{
    ARGUMENT *arg;
    
    if ((arg = (ARGUMENT *)malloc(sizeof(ARGUMENT))) == NULL)
	fail("out of memory");
    arg->arg_name = save(name);
    arg->arg_next = *list;
    *list = arg;
}

/* freelist - free a list of arguments or temporaries */
void freelist(ARGUMENT *arg)
{
    ARGUMENT *nxt;

    while (arg) {
	nxt = arg->arg_next;
	free(arg->arg_name);
	free(arg);
	arg = nxt;
    }
}

/* findarg - find an argument offset */
int findarg(char *name)
{
    ARGUMENT *arg;
    int n;
    
    for (n = 0, arg = arguments; arg; n++, arg = arg->arg_next)
	if (strcmp(name,arg->arg_name) == 0)
	    return (n);
    return (-1);
}

/* findtmp - find a temporary variable offset */
int findtmp(char *name)
{
    ARGUMENT *tmp;
    int n;

    for (n = 0, tmp = temporaries; tmp; n++, tmp = tmp->arg_next)
	if (strcmp(name,tmp->arg_name) == 0)
	    return (n);
    return (-1);
}

          
