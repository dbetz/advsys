/* advprs.c - adventure parser */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include "advint.h"
#include "advdbs.h"

/* parser result variables */
int nouns[20];
int *adjectives[20];
static int actor,action,dobject,ndobjects,iobject;

/* local variables */
static char *lptr;	/* line pointer */
static int words[100];	/* word table */
static char *wtext[100];/* word text table */
static int *wptr;	/* word pointer */
static int wcnt;	/* word count */

static int verbs[3]; 	/* words in the verb phrase */
static int nnums[20];	/* noun word numbers */
static int nptr;	/* noun pointer (actually, an index) */
static int adjs[100]; 	/* adjective lists */
static int anums[100];	/* adjective word numbers */
static int aptr;	/* adjective pointer (actually, an index) */

/* prototypes */
static int getverb(void);
static int getnoun(void);
static int get_line(void);
static int skip_spaces(void);
static int get_word(void);
static int spacep(int ch);
static void parse_error(void);

/* parse - read and parse an input line */
int parse(void)
{
    int noun1,cnt1,noun2,cnt2;
    int preposition,flag;

    /* initialize */
    noun1 = noun2 = NIL; cnt1 = cnt2 = 0;
    nptr = aptr = 0;
    preposition = 0;
    flag = 0;

    /* initialize the parser result variables */
    actor = action = dobject = iobject = NIL;
    ndobjects = 0;

    /* get an input line */
    if (!get_line())
	return (NIL);

    /* check for actor */
    if (wtype(*wptr) == WT_ADJECTIVE || wtype(*wptr) == WT_NOUN) {
	if ((actor = getnoun()) == NIL)
	    return (NIL);
	flag |= A_ACTOR;
    }

    /* get verb phrase */
    if (!getverb())
	return (NIL);

    /* direct object, preposition and indirect object */
    if (*wptr) {

	/* get the first set of noun phrases (direct objects) */
	noun1 = nptr+1;
	for (;;) {

            /* get the next direct object */
            if (getnoun() == NIL)
    		return (NIL);
	    ++cnt1;

	    /* check for more direct objects */
	    if (*wptr == NIL || wtype(*wptr) != WT_CONJUNCTION)
		break;
	    wptr++;
	}

	/* get the preposition and indirect object */
	if (*wptr) {

	    /* get the preposition */
	    if (wtype(*wptr) == WT_PREPOSITION)
		preposition = *wptr++;

	    /* get the second set of noun phrases (indirect object) */
	    noun2 = nptr+1;
	    for (;;) {

        	/* get the next direct object */
        	if (getnoun() == NIL)
    		    return (NIL);
		++cnt2;

		/* check for more direct objects */
		if (*wptr == NIL || wtype(*wptr) != WT_CONJUNCTION)
		    break;
		wptr++;
	    }
	}

	/* make sure this is the end of the sentence */
	if (*wptr) {
	    parse_error();
	    return (NIL);
	}
    }

    /* setup the direct and indirect objects */
    if (preposition) {
	if (cnt2 > 1) {
	    parse_error();
	    return (NIL);
	}
	dobject = noun1;
	ndobjects = cnt1;
	iobject = noun2;
    }
    else if (noun2) {
	if (cnt1 > 1) {
	    parse_error();
	    return (NIL);
	}
	preposition = findword("to");
	dobject = noun2;
	ndobjects = cnt2;
	iobject = noun1;
    }
    else {
	dobject = noun1;
	ndobjects = cnt1;
    }

    /* setup the flags for the action lookup */
    if (dobject) flag |= A_DOBJECT;
    if (iobject) flag |= A_IOBJECT;

    /* find the action */
    if ((action = findaction(verbs,preposition,flag)) == NIL) {
	parse_error();
	return (NIL);
    }

    /* setup the interpreter variables */
    setvalue(V_ACTOR,actor);
    setvalue(V_ACTION,action);
    setvalue(V_DOBJECT,dobject);
    setvalue(V_NDOBJECTS,ndobjects);
    setvalue(V_IOBJECT,iobject);
    
    /* return successfully */
    return (T);
}

/* next - get the next command (next direct object) */
int next(void)
{
    if (getvalue(V_NDOBJECTS) > 1) {
	setvalue(V_ACTOR,actor);
	setvalue(V_ACTION,action);
	setvalue(V_DOBJECT,getvalue(V_DOBJECT) + 1);
	setvalue(V_NDOBJECTS,getvalue(V_NDOBJECTS) - 1);
	setvalue(V_IOBJECT,iobject);
	return (T);
    }
    else
	return (NIL);
}

/* show_noun - show a noun phrase */
void show_noun(int n)
{
    int adj,*p;

    /* print the adjectives */
    for (p = adjectives[n-1], adj = FALSE; *p; p++, adj = TRUE) {
	if (adj) trm_chr(' ');
	trm_str(wtext[anums[p-adjs]]);
    }

    /* print the noun */
    if (adj) trm_chr(' ');
    trm_str(wtext[nnums[n-1]]);
}

/* getverb - get a verb phrase and return the action it refers to */
static int getverb(void)
{
    /* get the verb */
    if (*wptr == NIL || wtype(*wptr) != WT_VERB) {
	parse_error();
	return (NIL);
    }
    verbs[0] = *wptr++;
    verbs[1] = NIL;

    /* check for a word following the verb */
    if (*wptr) {
	verbs[1] = *wptr;
	verbs[2] = NIL;
	if (checkverb(verbs))
	    wptr++;
	else {
	    verbs[1] = words[wcnt-1];
	    if (checkverb(verbs))
		words[--wcnt] = NIL;
	    else {
		verbs[1] = NIL;
		if (!checkverb(verbs)) {
       		    parse_error();
		    return (NIL);
		}
	    }
	}
    }
    return (T);
}

/* getnoun - get a noun phrase and return the object it refers to */
static int getnoun(void)
{
    /* initialize the adjective list pointer */
    adjectives[nptr] = adjs + aptr;

    /* get the optional article */
    if (*wptr != NIL && wtype(*wptr) == WT_ARTICLE)
	wptr++;

    /* get optional adjectives */
    while (*wptr != NIL && wtype(*wptr) == WT_ADJECTIVE) {
	adjs[aptr] = *wptr++;
	anums[aptr] = wptr - words - 1;
	aptr++;
    }
    adjs[aptr++] = NIL;

    /* get the noun itself */
    if (*wptr == NIL || wtype(*wptr) != WT_NOUN) {
	parse_error();
	return (NIL);
    }

    /* save the noun */
    nouns[nptr] = *wptr++;
    nnums[nptr] = wptr - words - 1;
    return (++nptr);
}

/* get_line - get the input line and lookup each word */
static int get_line(void)
{
    /* read an input line */
    trm_chr(':');
    if ((lptr = trm_get()) == NULL) {
	trm_str("Speak up!  I can't hear you!\n");
	return (FALSE);
    }

    /* get each word on the line */
    for (wcnt = 0; skip_spaces(); wcnt++)
	if (get_word() == NIL)
	    return (FALSE);
    words[wcnt] = NIL;

    /* check for a blank line */
    if (wcnt == 0) {
	trm_str("Speak up!  I can't hear you!\n");
	return (FALSE);
    }

    /* point to the first word and return successfully */
    wptr = words;
    return (TRUE);
}

/* skip_spaces - skip leading spaces */
static int skip_spaces(void)
{
    while (spacep(*lptr))
	lptr++;
    return (*lptr != EOS);
}

/* get_word - get the next word */
static int get_word(void)
{
    int ch;

    /* get the next word */
    for (wtext[wcnt] = lptr; (ch = *lptr) != EOS && !spacep(ch); )
	*lptr++ = (isupper(ch) ? tolower(ch) : ch);
    if (*lptr != EOS) *lptr++ = EOS;

    /* look up the word */
    if ((words[wcnt] = findword(wtext[wcnt])) != 0)
	return (words[wcnt]);
    else {
	trm_str("I don't know the word \"");
	trm_str(wtext[wcnt]);
	trm_str("\".\n");
	return (NIL);
    }
}

/* spacep - is this character a space? */
static int spacep(int ch)
{
    return (ch == ' ' || ch == ',' || ch == '.');
}

/* parse_error - announce a parsing error */
static void parse_error(void)
{
    trm_str("I don't understand.\n");
}

