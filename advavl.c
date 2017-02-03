/* advavl.c - avl tree manipulation routines */
/*
	Copyright (c) 1993, by David Michael Betz
	All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "advavl.h"
#include "advcom.h"
#include "advdbs.h"

#define TRUE	1
#define FALSE	0

/* external variables */
extern char *data;
extern int curwrd;
extern int dptr;

/* local variables */
static TREE *curtree;
static char thiskey[WRDSIZE+1];

/* prototypes */
static int tenter1(TNODE **pnode,int *ph);
static int tfind1(TNODE *node);

/* tnew - allocate a new avl tree */
TREE *tnew(void)
{
    TREE *tree;

    /* allocate the tree structure */
    if ((tree = (TREE *)malloc(sizeof(TREE))) == NULL)
	return (NULL);

    /* initialize the new tree */
    tree->tr_root = NULL;
    tree->tr_cnt = 0;

    /* return the new tree */
    return (tree);
}

/* tenter - add an entry to an avl tree */
int tenter(TREE *tree,char *key)
{
    int h;
    curtree = tree;
    strncpy(thiskey,key,WRDSIZE); thiskey[WRDSIZE] = 0;
    return (tenter1(&tree->tr_root,&h));
}

/* tenter1 - internal insertion routine */
static int tenter1(TNODE **pnode,int *ph)
{
    TNODE *p,*q,*r;
    int val,c;

    /* check for the subtree being empty */
    if ((p = *pnode) == NULL) {
	if ((p = (TNODE *)malloc(sizeof(TNODE))) != NULL) {
	    curtree->tr_cnt++;
	    KEY(p) = save(thiskey);
	    WORD(p) = curwrd;
	    LLINK(p) = RLINK(p) = NULL;
	    B(p) = 0;
	    *pnode = p;
	    *ph = TRUE;
	    return (WORD(p));
	}
	else {
	    *ph = FALSE;
	    return (NIL);
	}
    }

    /* otherwise, check for a match at this node */
    else if ((c = strcmp(thiskey,KEY(p))) == 0) {
	*ph = FALSE;
	return (WORD(p));
    }

    /* otherwise, check the left subtree */
    else if (c < 0) {
	val = tenter1(&LLINK(p),ph);
	if (*ph)
	    switch (B(p)) {
	    case 1:
		B(p) = 0;
		*ph = FALSE;
		break;
	    case 0:
		B(p) = -1;
		break;
	    case -1:
		q = LLINK(p);
		if (B(q) == -1) {
		    LLINK(p) = RLINK(q);
		    RLINK(q) = p;
		    B(p) = 0;
		    p = q;
		}
		else {
		    r = RLINK(q);
		    RLINK(q) = LLINK(r);
		    LLINK(r) = q;
		    LLINK(p) = RLINK(r);
		    RLINK(r) = p;
		    B(p) = (B(r) == -1 ?  1 : 0);
		    B(q) = (B(r) ==  1 ? -1 : 0);
		    p = r;
		}
		B(p) = 0;
		*pnode = p;
		*ph = FALSE;
		break;
	    }
    }

    /* otherwise, check the right subtree */
    else {
	val = tenter1(&RLINK(p),ph);
	if (*ph)
	    switch (B(p)) {
	    case -1:
		B(p) = 0;
		*ph = FALSE;
		break;
	    case 0:
		B(p) = 1;
		break;
	    case 1:
		q = RLINK(p);
		if (B(q) == 1) {
		    RLINK(p) = LLINK(q);
		    LLINK(q) = p;
		    B(p) = 0;
		    p = q;
		}
		else {
		    r = LLINK(q);
		    LLINK(q) = RLINK(r);
		    RLINK(r) = q;
		    RLINK(p) = LLINK(r);
		    LLINK(r) = p;
		    B(p) = (B(r) ==  1 ? -1 : 0);
		    B(q) = (B(r) == -1 ?  1 : 0);
		    p = r;
		}
		B(p) = 0;
		*pnode = p;
		*ph = FALSE;
		break;
	    }
    }

    /* return the node found or inserted */
    return (val);
}

/* tfind - find an entry in an avl tree */
int tfind(TREE *tree,char *key)
{
    strncpy(thiskey,key,WRDSIZE); thiskey[WRDSIZE] = 0;
    return (tfind1(tree->tr_root));
}

/* tfind1 - internal lookup routine */
static int tfind1(TNODE *node)
{
    int c;

    /* check for the subtree being empty */
    if (node == NULL)
	return (NIL);

    /* otherwise, check for a match at this node */
    else if ((c = strcmp(thiskey,KEY(node))) == 0)
	return (WORD(node));

    /* otherwise, check the left subtree */
    else if (c < 0)
	return (tfind1(LLINK(node)));

    /* otherwise, check the right subtree */
    else
	return (tfind1(RLINK(node)));
}
