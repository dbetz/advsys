#include <stdio.h>
#include "advint.h"

int advgetc(void)
{
    return getchar();
}

void advwaitc(void)
{
    getchar();
}

int advputc(int ch,FILE *fp)
{
    return putc(ch,fp);
}

int advsave(char *hdr,int hlen,char *save,int slen)
{
    char fname[50],*p;
    FILE *fp;

    trm_str("File name? ");
    if (!(p = trm_get()))
        return 0;

    /* add the extension */
    sprintf(fname,"%s.sav",p);

    /* create the data file */
    if ((fp = fopen(fname,"wb")) == NULL)
        return 0;

    /* write the header */
    if (fwrite(hdr,hlen,1,fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* write the data */
    if (fwrite(save,slen,1,fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* close the file and return successfully */
    fclose(fp);
    return 1;
}

int advrestore(char *hdr,int hlen,char *save,int slen)
{
    char fname[50],hbuf[50],*p;
    FILE *fp;

    if (hlen > 50)
        error("save file header buffer too small");

    trm_str("File name? ");
    if (!(p = trm_get()))
        return 0;

    /* add the extension */
    sprintf(fname,"%s.sav",p);

    /* open the data file */
    if ((fp = fopen(fname,"rb")) == NULL)
        return 0;

    /* read the header */
    if (fread(hbuf,hlen,1,fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* compare the headers */
    for (p = hbuf; hlen--; )
        if (*hdr++ != *p++) {
            trm_str("This save file does not match the adventure!\n");
            return 0;
        }

    /* read the data */
    if (fread(save,slen,1,fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* close the file and return successfully */
    fclose(fp);
    return 1;
}
