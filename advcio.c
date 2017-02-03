/* advcio.c - adventure compiler i/o routines */

#include <stdio.h>
#include <string.h>
#include "advcom.h"

#define BSIZE	8192

/* global variables */
long ad_foff;

/* local variables */
static char buf[BSIZE];
static int boff;
static FILE *fp;

void macinit(char *iname,char *oname)
{
    printf("File name? "); fflush(stdout);
    gets(iname); strcpy(oname,iname);
    strcat(iname,".adv");
    strcat(oname,".dat");
}

void macpause(void)
{
}

void ad_create(char *name)
{
    /* open the output file */
    if (!(fp = fopen(name,"wb")))
	fail("can't create data file");
	
    /* initialize the buffer and file offset */
    ad_foff = 0L;
    boff = 0;
}

void ad_close(void)
{
    ad_flush();
    fclose(fp);
}

void ad_putc(int ch)
{
    buf[boff++] = ch; ad_foff++;
    if (boff >= BSIZE)
	ad_flush();
}

void ad_seek(long pos)
{
    ad_flush();
    if (fseek(fp,pos,SEEK_SET) != pos)
	fail("error positioning output file");
    ad_foff = pos;
}

void ad_flush(void)
{
    if (boff) {
	if (fwrite(buf,1, boff, fp) != boff)
	    fail("error writing to output file");
	boff = 0;
    }
}
