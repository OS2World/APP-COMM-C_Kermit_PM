char *cmdv = "Unix cmd package V3(031), 20 Feb 90";
 
/*  C K U C M D  --  Interactive command package for Unix  */

/*
 Author: Frank da Cruz (fdc@columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 First released January 1985.
 Copyright (C) 1985, 1990, Trustees of Columbia University in the City of New 
 York.  Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained. 
*/

/*
 Modelled after the DECSYSTEM-20 command parser (the COMND JSYS)
 
 Features:
 . parses and verifies keywords, filenames, text strings, numbers, other data
 . displays appropriate menu or help message when user types "?"
 . does keyword and filename completion when user types ESC or TAB
 . accepts any unique abbreviation for a keyword
 . allows keywords to have attributes, like "invisible"
 . can supply defaults for fields omitted by user
 . provides command line editing (character, word, and line deletion)
 . accepts input from keyboard, command files, or redirected stdin
 . allows for full or half duplex operation, character or line input
 . settable prompt, protected from deletion
 
 Functions:
  cmsetp - Set prompt (cmprom is prompt string, cmerrp is error msg prefix)
  cmsavp - Save current prompt
  prompt - Issue prompt 
  cmini  - Clear the command buffer (before parsing a new command)
  cmres  - Reset command buffer pointers (before reparsing)
  cmkey  - Parse a keyword
  cmnum  - Parse a number
  cmifi  - Parse an input file name
  cmofi  - Parse an output file name
  cmdir  - Parse a directory name (UNIX only)
  cmfld  - Parse an arbitrary field
  cmtxt  - Parse a text string
  cmcfm  - Parse command confirmation (end of line)
  stripq - Strip out backslash quotes from a string (no longer used)
 
 Return codes:
  -3: no input provided when required
  -2: input was invalid
  -1: reparse required (user deleted into a preceding field)
   0 or greater: success
  See individual functions for greater detail.
 
 Before using these routines, the caller should #include ckucmd.h, and
 set the program's prompt by calling cmsetp().  If the file parsing
 functions cmifi, cmofi, or cmdir are to be used, this module must be linked
 with a ck?fio file system support module for the appropriate system,
 e.g. ckufio for Unix.  If the caller puts the terminal in
 character wakeup ("cbreak") mode with no echo, then these functions will
 provide line editing -- character, word, and line deletion, as well as
 keyword and filename completion upon ESC and help strings, keyword, or
 file menus upon '?'.  If the caller puts the terminal into character
 wakeup/noecho mode, care should be taken to restore it before exit from
 or interruption of the program.  If the character wakeup mode is not
 set, the system's own line editor may be used.

 NOTE: Contrary to expectations, many #ifdef's have been added to this
 module.  Any operation requiring an #ifdef (like clear screen, get character
 from keyboard, erase character from screen, etc) should eventually be turned 
 into a call to a function that is defined in ck?tio.c, but then all the
 ck?tio.c modules would have to be changed...
*/

/* Includes */
 
#include <stdio.h>                      /* Standard C I/O package */
#include <ctype.h>                      /* Character types */
#include "ckcasc.h"         /* ASCII character symbols */
#include "ckucmd.h"                     /* Command parsing definitions */
#include "ckcdeb.h"                     /* Formats for debug(), etc. */
#ifdef OS2
#define INCL_SUB
#include <os2.h>
#endif /* OS2 */

#ifdef OSK
#define cc ccount           /* OS-9/68K compiler bug */
#endif

/* Local variables */
 
int psetf = 0,                          /* Flag that prompt has been set */
    cc = 0,                             /* Character count */
    dpx = 0;                            /* Duplex (0 = full) */
 
int hw = HLPLW,                         /* Help line width */
    hc = HLPCW,                         /* Help line column width */
    hh,                                 /* Current help column number */
    hx;                                 /* Current help line position */
 
#define PROML 60                        /* Maximum length for prompt */
 
char cmprom[PROML+1];                   /* Program's prompt */
char *dfprom = "Command? ";             /* Default prompt */
 
char cmerrp[PROML+1];                   /* Program's error message prefix */
 
int cmflgs;                             /* Command flags */
int cmflgsav;               /* A saved version of them */
 
char cmdbuf[CMDBL+4];                   /* Command buffer */
char savbuf[CMDBL+4];           /* Buffer to save copy of command */
char hlpbuf[HLPBL+4];                   /* Help string buffer */
char atmbuf[ATMBL+4];                   /* Atom buffer */
char atxbuf[CMDBL+4];           /* For expanding the atom buffer */
char atybuf[ATMBL+4];                   /* For copying atom buffer */
char filbuf[ATMBL+4];                   /* File name buffer */
 
/* Command buffer pointers */
 
static char *bp,                        /* Current command buffer position */
    *pp,                                /* Start of current field */
    *np;                                /* Start of next field */
 
static int ungw;            /* For ungetting words */

long zchki();                           /* From ck?fio.c. */

 
/*  C M S E T P  --  Set the program prompt.  */
 
cmsetp(s) char *s; {
    char *sx, *sy, *strncpy();
    psetf = 1;                          /* Flag that prompt has been set. */
    strncpy(cmprom,s,PROML - 1);    /* Copy the string. */
    cmprom[PROML] = NUL;                /* Ensure null terminator. */
    sx = cmprom; sy = cmerrp;           /* Also use as error message prefix. */
    while (*sy++ = *sx++) ;             /* Copy. */
    sy -= 2; if (*sy == '>') *sy = NUL; /* Delete any final '>'. */
}
/*  C M S A V P  --  Save a copy of the current prompt.  */
 
cmsavp(s,n) int n; char s[]; {
    extern char *strncpy();                                     /* +1   */
    strncpy(s,cmprom,n-1);
    s[n] = NUL;
}
 
/*  P R O M P T  --  Issue the program prompt.  */
 
prompt() {
    if (psetf == 0) cmsetp(dfprom);     /* If no prompt set, set default. */
#ifdef OSK
    fputs(cmprom, stdout);
#else
    printf("\r%s",cmprom);              /* Print the prompt. */
#endif
}
 
 
pushcmd() {             /* For use with IF command. */
    strcpy(savbuf,np);          /* Save the dependent clause,  */
    cmres();                /* and clear the command buffer. */
}

popcmd() {
    strcpy(cmdbuf,savbuf);      /* Put back the saved material */
    *savbuf = '\0';         /* and clear the save buffer */
    cmres();
}

/*  C M R E S  --  Reset pointers to beginning of command buffer.  */
 
cmres() {  
    cc = 0;                             /* Reset character counter. */
    pp = np = bp = cmdbuf;              /* Point to command buffer. */
    cmflgs = -5;                        /* Parse not yet started. */
    ungw = 0;               /* Haven't ungotten a word. */
}
 
 
/*  C M I N I  --  Clear the command and atom buffers, reset pointers.  */
 
/*
The argument specifies who is to echo the user's typein --
  1 means the cmd package echoes
  0 somebody else (system, front end, terminal) echoes
*/
cmini(d) int d; {
    for (bp = cmdbuf; bp < cmdbuf+CMDBL; bp++) *bp = NUL;
    *atmbuf = NUL;
    dpx = d;
    cmres();
}
 
stripq(s) char *s; {                    /* Function to strip '\' quotes */
    char *t;
    while (*s) {
        if (*s == CMDQ) {
            for (t = s; *t != '\0'; t++) *t = *(t+1);
        }
        s++;
    }
}

/* Convert tabs to spaces */
untab(s) char *s; {
    while (*s++) 
      if (*s == HT) *s = SP;
}

/*  C M N U M  --  Parse a number in the indicated radix  */
 
/* 
 For now, the only radix allows in unquoted numbers is 10.
 Parses unquoted numeric strings in base 10.
 Parses backslash-quoted numbers in the radix indicated by the quote:
   \nnn = \dnnn = decimal, \onnn = octal, \xnn = hexidecimal.
 If these fail, then if a preprocessing function is supplied, that is applied 
 and then a second attempt is made to parse an unquoted decimal string. 

 Returns
   -3 if no input present when required,
   -2 if user typed an illegal number,
   -1 if reparse needed,
    0 otherwise, with argument n set to the number that was parsed
*/
cmnum(xhlp,xdef,radix,n,f) char *xhlp, *xdef; int radix, *n; int (*f)(); {
    int x; char *s, *zp, *zq;
 
    if (radix != 10) {                  /* Just do base 10 for now */
        printf("cmnum: illegal radix - %d\n",radix);
        return(-1);
    }
    x = cmfld(xhlp,xdef,&s,NULL);
    debug(F101,"cmnum: cmfld","",x);
    if (x < 0) return(x);       /* Parse a field */
    zp = atmbuf;

    if (chknum(zp)) {           /* Check for decimal number */
        *n = atoi(zp);          /* Got one, we're done. */
        return(0);
    } else if ((x = xxesc(&zp)) > -1) { /* Check for backslash escape */
    *n = x;
    return(*zp ? -2 : 0);
    } else if (*f) {            /* If conversion function given */
        zp = atmbuf;            /* Try that */
    zq = atxbuf;
    x = (*f)(zp,&zq);       /* Convert */
    zp = atxbuf;
    }
    if (chknum(zp)) {           /* Check again for decimal number */
        *n = atoi(zp);          /* Got one, we're done. */
        return(0);
    } else {                /* Not numeric */
    printf("\n?not a number - %s\n",s);
    return(-2);     
    }
}

/*  C M O F I  --  Parse the name of an output file  */
 
/*
 Depends on the external function zchko(); if zchko() not available, use
 cmfld() to parse output file names.
 
 Returns
   -3 if no input present when required,
   -2 if permission would be denied to create the file,
   -1 if reparse needed,
    0 or 1 otherwise, with xp pointing to name.
*/
cmofi(xhlp,xdef,xp,f) char *xhlp, *xdef, **xp; int (*f)(); {
    int x; char *s, *zq;
#ifdef DTILDE
    char *tilde_expand(), *dirp;
#endif 

    if (*xhlp == NUL) xhlp = "Output file";
    *xp = "";
 
    if ((x = cmfld(xhlp,xdef,&s,NULL)) < 0) return(x);

    if (*f) {               /* If a conversion function is given */
    zq = atxbuf;
    x = (*f)(s,&zq);
    s = atxbuf;
    }

#ifdef DTILDE
    dirp = tilde_expand(s);     /* Expand tilde, if any, */
    if (*dirp != '\0') setatm(dirp);    /* right in the atom buffer. */
#endif

    if (chkwld(s)) {
        printf("\n?Wildcards not allowed - %s\n",s);
        return(-2);
    }
    if (zchko(s) < 0) {
        printf("\n?Write permission denied - %s\n",s);
        return(-2);
    } else {
        *xp = s;
        return(x);
    }
}

 
/*  C M I F I  --  Parse the name of an existing file  */
 
/*
 This function depends on the external functions:
   zchki()  - Check if input file exists and is readable.
   zxpand() - Expand a wild file specification into a list.
   znext()  - Return next file name from list.
 If these functions aren't available, then use cmfld() to parse filenames.
*/
/*
 Returns
   -4 EOF
   -3 if no input present when required,
   -2 if file does not exist or is not readable,
   -1 if reparse needed,
    0 or 1 otherwise, with:
        xp pointing to name,
        wild = 1 if name contains '*' or '?', 0 otherwise.
*/
cmifi(xhlp,xdef,xp,wild,f) char *xhlp, *xdef, **xp; int *wild; int (*f)(); {
    int i, x, xc; long y; char *sp, *zq;
#ifdef DTILDE
    char *tilde_expand(), *dirp;
#endif
 
    cc = xc = 0;                        /* Initialize counts & pointers */
    *xp = "";
    if ((x = cmflgs) != 1) {            /* Already confirmed? */
        x = gtword();                   /* No, get a word */
    } else {
        cc = setatm(xdef);              /* If so, use default, if any. */
    }

    *xp = atmbuf;                       /* Point to result. */
    *wild = chkwld(*xp);
 
    while (1) {
        xc += cc;                       /* Count the characters. */
        debug(F111,"cmifi: gtword",atmbuf,xc);
        switch (x) {
            case -4:                    /* EOF */
            case -2:                    /* Out of space. */
            case -1:                    /* Reparse needed */
                return(x);
            case 0:                     /* SP or NL */
            case 1:
                if (xc == 0) *xp = xdef;     /* If no input, return default. */
                else *xp = atmbuf;
                if (**xp == NUL) return(-3); /* If field empty, return -3. */

        if (*f) {       /* If a conversion function is given */
            zq = atxbuf;    /* ... */
            x = (*f)(*xp,&zq);  /* run it. */
            *xp = atxbuf;
        }
#ifdef DTILDE
        dirp = tilde_expand(*xp);    /* Expand tilde, if any, */
        if (*dirp != '\0') setatm(dirp); /* right in atom buffer. */
#endif
                /* If filespec is wild, see if there are any matches */
 
                *wild = chkwld(*xp);
                debug(F101," *wild","",*wild);
                if (*wild != 0) {
                    y = zxpand(*xp);
                    if (y == 0) {
                        printf("\n?No files match - %s\n",*xp);
                        return(-2);
                    } else if (y < 0) {
                        printf("\n?Too many files match - %s\n",*xp);
                        return(-2);
                    } else return(x);
                }
 
                /* If not wild, see if it exists and is readable. */
 
                y = zchki(*xp);

                if (y == -3) {
                    printf("\n?Read permission denied - %s\n",*xp);
                    return(-2);
                } else if (y == -2) {
                    printf("\n?File not readable - %s\n",*xp);
                    return(-2);
                } else if (y < 0) {
                    printf("\n?File not found - %s\n",*xp);
                    return(-2);
                }
                return(x);

            case 2:                     /* ESC */
                if (xc == 0) {
                    if (*xdef != '\0') {
                        printf("%s ",xdef); /* If at beginning of field, */
                        addbuf(xdef);   /* supply default. */
                        cc = setatm(xdef);
                    } else {            /* No default */
                        putchar(BEL);
                    }
                    break;
                } 
        if (*f) {       /* If a conversion function is given */
            zq = atxbuf;    /* ... */
            x = (*f)(*xp,&zq);  /* run it. */
            *xp = atxbuf;
        }
#ifdef DTILDE
        dirp = tilde_expand(*xp);    /* Expand tilde, if any, */
        if (*dirp != '\0') setatm(dirp); /* in the atom buffer. */
#endif
                if (*wild = chkwld(*xp)) {  /* No completion if wild */
                    putchar(BEL);
                    break;
                }
                sp = atmbuf + cc;
                *sp++ = '*';
                *sp-- = '\0';
                y = zxpand(atmbuf);     /* Add * and expand list. */
                *sp = '\0';             /* Remove *. */
 
                if (y == 0) {
                    printf("\n?No files match - %s\n",atmbuf);
                    return(-2);
                } else if (y < 0) {
                    printf("\n?Too many files match - %s\n",atmbuf);
                    return(-2);
                } else if (y > 1) {     /* Not unique, just beep. */
                    putchar(BEL);
                } else {                /* Unique, complete it.  */
                    znext(filbuf);      /* Get whole name of file. */
                    sp = filbuf + cc;   /* Point past what user typed. */
                    printf("%s ",sp);   /* Complete the name. */
                    addbuf(sp);         /* Add the characters to cmdbuf. */
                    setatm(pp);         /* And to atmbuf. */
                    *xp = atmbuf;       /* Return pointer to atmbuf. */
                    return(cmflgs = 0);
                }
                break;
 
            case 3:                     /* Question mark */
                if (*xhlp == NUL)
                    printf(" Input file specification");
                else
                    printf(" %s",xhlp);
                if (xc > 0) {
            if (*f) {       /* If a conversion function is given */
            zq = atxbuf;    /* ... */
            x = (*f)(*xp,&zq); /* run it. */
            *xp = atxbuf;
            }
#ifdef DTILDE
            dirp = tilde_expand(*xp);    /* Expand tilde, if any */
            if (*dirp != '\0') setatm(dirp);
#endif
                    sp = atmbuf + cc;   /* Insert "*" at end */
#ifdef datageneral
                    *sp++ = '+';        /* Insert +, the DG wild card */
#else
                    *sp++ = '*';
#endif
                    *sp-- = '\0';
                    y = zxpand(atmbuf);
                    *sp = '\0';
                    if (y == 0) {                   
                        printf("\n?No files match - %s\n",atmbuf);
                        return(-2);
                    } else if (y < 0) {
                        printf("\n?Too many file match - %s\n",atmbuf);
                        return(-2);
                    } else {
                        printf(", one of the following:\n");
                        clrhlp();
                        for (i = 0; i < y; i++) {
                            znext(filbuf);
                            addhlp(filbuf);
                        }
                        dmphlp();
                    }
                } else printf("\n");
                printf("%s%s",cmprom,cmdbuf);
                break;
        }
    x = gtword();
    }
}

/*  C M D I R  --  Parse a directory specification  */
 
/*
 This function depends on the external functions:
   zchki()  - Check if input file exists and is readable.
 If these functions aren't available, then use cmfld() to parse dir names.
 Note: this function quickly cobbled together, mainly by deleting lots of
 lines from cmifi().  It seems to work, but various services are missing,
 like completion, lists of matching directories on "?", etc.
*/
/*
 Returns
   -4 EOF
   -3 if no input present when required,
   -2 if out of space or other internal error,
   -1 if reparse needed,
    0 or 1, with xp pointing to name, if directory specified,
    2 if a wildcard was included.
*/
cmdir(xhlp,xdef,xp,f) char *xhlp, *xdef, **xp; int (*f)(); {
    int x, xc; long y; char *zq;
#ifdef DTILDE
    char *tilde_expand(), *dirp;
#endif 

    cc = xc = 0;                        /* Initialize counts & pointers */
    *xp = "";
    if ((x = cmflgs) != 1) {            /* Already confirmed? */
        x = gtword();                   /* No, get a word */
    } else {
        cc = setatm(xdef);              /* If so, use default, if any. */
    }
    *xp = atmbuf;                       /* Point to result. */
    while (1) {
        xc += cc;                       /* Count the characters. */
        debug(F111,"cmifi: gtword",atmbuf,xc);
        switch (x) {
            case -4:                    /* EOF */
            case -2:                    /* Out of space. */
            case -1:                    /* Reparse needed */
                return(x);
            case 0:                     /* SP or NL */
            case 1:
                if (xc == 0) *xp = xdef;     /* If no input, return default. */
                if (**xp == NUL) return(-3); /* If field empty, return -3. */
        if (*f) {       /* If a conversion function is given */
            zq = atxbuf;    /* ... */
            x = (*f)(*xp,&zq);  /* run it. */
            *xp = atxbuf;
            cc = strlen(atxbuf);
        }
#ifdef DTILDE
        dirp = tilde_expand(*xp);    /* Expand tilde, if any, */
        if (*dirp != '\0') setatm(dirp); /* in the atom buffer. */
        *xp = atmbuf;
#endif
        if (chkwld(*xp) != 0)   /* If wildcard included... */
          return(2);

                /* If not wild, see if it exists and is readable. */
 
                y = zchki(*xp);

                if (y == -3) {
                    printf("\n?Read permission denied - %s\n",*xp);
                    return(-2);
        } else if (y == -2) {   /* Probably a directory... */
            return(x);
                } else if (y < 0) {
                    printf("\n?Not found - %s\n",*xp);
                    return(-2);
                }
                return(x);
            case 2:                     /* ESC */
        putchar(BEL);
        break;

            case 3:                     /* Question mark */
                if (*xhlp == NUL)
                    printf(" Directory name");
                else
                    printf(" %s",xhlp);
                printf("\n%s%s",cmprom,cmdbuf);
                break;
        }
    x = gtword();
    }
}
 
/*  C H K W L D  --  Check for wildcard characters '*' or '?'  */
 
chkwld(s) char *s; {
 
    for ( ; *s != '\0'; s++) {
#ifdef datageneral
        /* Valid DG wild cards are '-', '+', '#', or '*' */
        if ( (*s <= '-') && (*s >= '#') &&
            ((*s == '-') || (*s == '+') || (*s == '#') || (*s == '*')) )
#else
        if ((*s == '*') || (*s == '?'))
#endif
            return(1);
    }
    return(0);
}

 
/*  C M F L D  --  Parse an arbitrary field  */
/*
 Returns
   -3 if no input present when required,
   -2 if field too big for buffer,
   -1 if reparse needed,
    0 otherwise, xp pointing to string result.
*/
cmfld(xhlp,xdef,xp,f) char *xhlp, *xdef, **xp; int (*f)(); {
    int x, xc;
    char *zq;

#ifdef OS2  /* Needed for COMMENT but below */
   char * xx;
   int i;
#endif

    cc = xc = 0;                        /* Initialize counts & pointers */
    *xp = "";
    if ((x = cmflgs) != 1) {            /* Already confirmed? */
        x = gtword();                   /* No, get a word */
    } else {
        cc = setatm(xdef);              /* If so, use default, if any. */
    }
    *xp = atmbuf;                       /* Point to result. */
 
    while (1) {
        xc += cc;                       /* Count the characters. */
        debug(F111,"cmfld: gtword",atmbuf,xc);
        debug(F101,"cmfld x","",x);
        switch (x) {
            case -4:                    /* EOF */
            case -2:                    /* Out of space. */
            case -1:                    /* Reparse needed */
                return(x);
            case 0:                     /* SP or NL */
            case 1:
                if (xc == 0)        /* If no input, return default. */
          cc = setatm(xdef);
        *xp = atmbuf;
        if (*f) {       /* If a conversion function is given */
            zq = atxbuf;    /* ... */
            (*f)(*xp,&zq);  /* run it. */
            cc = setatm(atxbuf);
            *xp = atmbuf;
        }
                if (**xp == NUL) x = -3;    /* If field empty, return -3. */
#ifdef COMMENT
/* The following is apparently not necessary. */
/* Remove it if nothing is broken, esp. TAKE file with trailing comments */
        xx = *xp;
        debug(F111,"cmfld before trim",*xp,x);
        for (i = strlen(xx) - 1; i > 0; i--)
          if (xx[i] != SP)  /* Trim trailing blanks */
            break;
          else
            xx[i] = NUL;
        debug(F111,"cmfld returns",*xp,x); 
#endif /* COMMENT */
        debug(F101,"cmfld: returns","",x);
                return(x);
            case 2:                     /* ESC */
                if (xc == 0) {
                    printf("%s ",xdef); /* If at beginning of field, */
                    addbuf(xdef);       /* supply default. */
                    cc = setatm(xdef);  /* Return as if whole field */
                    return(0);          /* typed, followed by space. */
                } else {
                    putchar(BEL);       /* Beep if already into field. */
                }                   
                break;
            case 3:                     /* Question mark */
                if (*xhlp == NUL)
                    printf(" Please complete this field");
                else
                    printf(" %s",xhlp);
                printf("\n%s%s",cmprom,cmdbuf);
                break;
        }
    x = gtword();
    }
}

 
/*  C M T X T  --  Get a text string, including confirmation  */
 
/*
  Print help message 'xhlp' if ? typed, supply default 'xdef' if null
  string typed.  Returns
 
   -1 if reparse needed or buffer overflows.
    1 otherwise.
 
  with cmflgs set to return code, and xp pointing to result string.
*/
 
cmtxt(xhlp,xdef,xp,f) char *xhlp; char *xdef; char **xp; int (*f)(); {
 
    int x, i;
    char *xx, *zq;
    static int xc;
 
    debug(F101,"cmtxt, cmflgs","",cmflgs);
    cc = 0;                             /* Start atmbuf counter off at 0 */
    if (cmflgs == -1) {                 /* If reparsing, */
        xc = strlen(*xp);               /* get back the total text length, */
    } else {                            /* otherwise, */
        *xp = "";                       /* start fresh. */
        xc = 0;
    }
    *atmbuf = NUL;                      /* And empty the atom buffer. */
    if ((x = cmflgs) != 1) {
        x = gtword();                   /* Get first word. */
        *xp = pp;                       /* Save pointer to it. */
    }
    while (1) {
        xc += cc;                       /* Char count for all words. */
        debug(F111,"cmtxt: gtword",atmbuf,xc);
        debug(F101," x","",x);
        switch (x) {
            case -4:                    /* EOF */
            case -2:                    /* Overflow */
            case -1:                    /* Deletion */
                return(x);
            case 0:                     /* Space */
                xc++;                   /* Just count it */
                break;
            case 1:                     /* CR or LF */
                if (xc == 0) *xp = xdef;
        if (*f) {       /* If a conversion function is given */
            zq = atxbuf;    /* ... */
            x = (*f)(*xp,&zq);  /* run it. */
            *xp = atxbuf;
            cc = strlen(atxbuf);
        }
        xx = *xp;
        for (i = strlen(xx) - 1; i > 0; i--)
          if (xx[i] != SP)  /* Trim trailing blanks */
            break;
          else
            xx[i] = NUL;
                return(x);
            case 2:                     /* ESC */
                if (xc == 0) {
                    printf("%s ",xdef);
                    cc = addbuf(xdef);
                } else {
                    putchar(BEL);
                }
                break;
            case 3:                     /* Question Mark */
                if (*xhlp == NUL)
                    printf(" Text string");
                else
                    printf(" %s",xhlp);
                printf("\n%s%s",cmprom,cmdbuf);
                break;
            default:
                printf("\n?Unexpected return code from gtword() - %d\n",x);
                return(-2);
        }
        x = gtword();
    }
}

 
/*  C M K E Y  --  Parse a keyword  */
 
/*
 Call with:
   table    --  keyword table, in 'struct keytab' format;
   n        --  number of entries in table;
   xhlp     --  pointer to help string;
   xdef     --  pointer to default keyword;
 
 Returns:
   -3       --  no input supplied and no default available
   -2       --  input doesn't uniquely match a keyword in the table
   -1       --  user deleted too much, command reparse required
    n >= 0  --  value associated with keyword
*/
 
cmkey(table,n,xhlp,xdef,f)
/* cmkey */  struct keytab table[]; int n; char *xhlp, *xdef; int (*f)(); {
    return(cmkey2(table,n,xhlp,xdef,"",f));
}

cmkey2(table,n,xhlp,xdef,tok,f)
    struct keytab table[]; int n; char *xhlp, *xdef; char *tok; int (*f)(); {

    int i, tl, y, z, zz, xc;
    char *xp, *zq;
 
    tl = strlen(tok);
    xc = cc = 0;                        /* Clear character counters. */
 
    if ((zz = cmflgs) == 1)             /* Command already entered? */
        setatm(xdef);           /* Yes, copy default into atom buf */
    else zz = gtword();         /* Otherwise get a command word */
 
debug(F101,"cmkey: table length","",n);
debug(F101," cmflgs","",cmflgs);
debug(F101," zz","",zz);
while (1) {
    xc += cc;
    debug(F111,"cmkey: gtword",atmbuf,xc);
 
    switch(zz) {
        case -4:                        /* EOF */
        case -2:                        /* Buffer overflow */
        case -1:                        /* Or user did some deleting. */
            return(zz);
 
        case 0:                         /* User terminated word with space */
        case 1:                         /* or newline */
            if (cc == 0) setatm(xdef);  /* Supply default if user typed nada */
        if (*f) {           /* If a conversion function is given */
        zq = atxbuf;        /* apply it */
        (*f)(atmbuf,&zq);
        cc = setatm(atxbuf);
        }
            y = lookup(table,atmbuf,n,&z); /* Look up the word in the table */
            switch (y) {
                case -2:        /* Ambiguous */
                    printf("\n?Ambiguous - %s\n",atmbuf);
                    return(cmflgs = -2);
                case -1:        /* Not found at all */
            if (tl) {
            for (i = 0; i < tl; i++) /* Check for token */
              if (tok[i] == *atmbuf) { /* Got one */
                  ungword();  /* Put back the following word */
                  return(-5); /* Special return code for token */
              }
            }
            /* Kludge alert... only print error if */
            /* we were called as cmkey2, but not cmkey... */
            /* This doesn't seem to always work. */
            if (tl == 0) {
            /* printf("\n?Invalid - %s\n",atmbuf); /* cmkey */
            return(cmflgs = -2);
            } else { 
            if (cmflgs == 1) return(cmflgs = -6); /* cmkey2 */
            else return(cmflgs = -2);
            /* The -6 code is to let caller try another table */
            }
                default:
                    break;
        }
            return(y);
 
        case 2:                         /* User terminated word with ESC */
            if (cc == 0) {
                if (*xdef != NUL) {     /* Nothing in atmbuf */
                    printf("%s ",xdef); /* Supply default if any */
                    addbuf(xdef);
                    cc = setatm(xdef);
                    debug(F111,"cmkey: default",atmbuf,cc);
                } else {
                    putchar(BEL);       /* No default, just beep */
                    break;
                }
            }
        if (*f) {           /* If a conversion function is given */
        zq = atxbuf;        /* apply it */
        (*f)(atmbuf,&zq);
        cc = setatm(atxbuf);
        }
            y = lookup(table,atmbuf,n,&z); /* Something in atmbuf */
            debug(F111,"cmkey: esc",atmbuf,y);
            if (y == -2) {
                putchar(BEL);
                break;
            }
            if (y == -1) {
                if (tl == 0) printf("\n?Invalid - %s\n",atmbuf);
                return(cmflgs = -2);
            }
            xp = table[z].kwd + cc;
            printf("%s ",xp);
            addbuf(xp);
            debug(F110,"cmkey: addbuf",cmdbuf,0);
            return(y);
 
        case 3:                         /* User terminated word with "?" */
        if (*f) {           /* If a conversion function is given */
        zq = atxbuf;
        (*f)(atmbuf,&zq);
        cc = setatm(atxbuf);
        }
            y = lookup(table,atmbuf,n,&z);
            if (y > -1) {
                printf(" %s\n%s%s",table[z].kwd,cmprom,cmdbuf);
                break;
            } else if (y == -1) {
                if (tl == 0) printf("\n?Invalid\n");
                return(cmflgs = -2);
            }
            if (*xhlp == NUL)
                printf(" One of the following:\n");
            else
                printf(" %s, one of the following:\n",xhlp);
 
            clrhlp();
            for (i = 0; i < n; i++) {   
                if (!strncmp(table[i].kwd,atmbuf,cc)
                        && !test(table[i].flgs,CM_INV))
                    addhlp(table[i].kwd);
            }
            dmphlp();
        if (*atmbuf == NUL) {
        if (tl == 1)
          printf("or the token '%c'\n",*tok);
        else if (tl > 1) printf("or one of the tokens '%s'\n",tok);
        }
            printf("%s%s", cmprom, cmdbuf);
            break;
 
        default:            
            printf("\n%d - Unexpected return code from gtword\n",zz);
            return(cmflgs = -2);
        }
        zz = gtword();
    }
}

chktok(tlist) char *tlist; {
    return(*atmbuf);
}

/*  C M C F M  --  Parse command confirmation (end of line)  */
 
/*
 Returns
   -2: User typed anything but whitespace or newline
   -1: Reparse needed
    0: Confirmation was received
*/
 
cmcfm() {
    int x, xc;
 
    debug(F101,"cmcfm: cmflgs","",cmflgs);
 
    xc = cc = 0;
    if (cmflgs == 1) return(0);
 
    while (1) {
        x = gtword();
        xc += cc;
        debug(F111,"cmcfm: gtword",atmbuf,xc);
        switch (x) {
            case -4:                    /* EOF */
            case -2:
            case -1:
                return(x);
 
            case 0:                     /* Space */
                continue;
            case 1:                     /* End of line */
                if (xc > 0) {
                    printf("?Not confirmed - %s\n",atmbuf);
                    return(-2);
                } else return(0);                   
            case 2:         /* ESC */
                putchar(BEL);
                continue;
 
            case 3:         /* Question mark */
                if (xc > 0) {
                    printf("\n?Not confirmed - %s\n",atmbuf);
                    return(-2);
                }
                printf("\n Type a carriage return to confirm the command\n");
                printf("%s%s",cmprom,cmdbuf);
                continue;
        }
    }
}

/* Keyword help routines */
 
 
/*  C L R H L P -- Initialize/Clear the help line buffer  */
 
clrhlp() {                              /* Clear the help buffer */
    hlpbuf[0] = NUL;
    hh = hx = 0;
}
 
 
/*  A D D H L P  --  Add a string to the help line buffer  */
 
addhlp(s) char *s; {                    /* Add a word to the help buffer */
    int j;
 
    hh++;                               /* Count this column */
 
    for (j = 0; (j < hc) && (*s != NUL); j++) { /* Fill the column */
        hlpbuf[hx++] = *s++;
    }
    if (*s != NUL)                      /* Still some chars left in string? */
        hlpbuf[hx-1] = '+';             /* Mark as too long for column. */
 
    if (hh < (hw / hc)) {               /* Pad col with spaces if necessary */
        for (; j < hc; j++) {
            hlpbuf[hx++] = SP;
        }
    } else {                            /* If last column, */
        hlpbuf[hx++] = NUL;             /* no spaces. */
        dmphlp();                       /* Print it. */
        return;
    }
}
 
 
/*  D M P H L P  --  Dump the help line buffer  */
 
dmphlp() {                              /* Print the help buffer */
    hlpbuf[hx++] = NUL;
    printf(" %s\n",hlpbuf);
    clrhlp();
}

 
/*  L O O K U P  --  Lookup the string in the given array of strings  */
 
/*
 Call this way:  v = lookup(table,word,n,&x);
 
   table - a 'struct keytab' table.
   word  - the target string to look up in the table.
   n     - the number of elements in the table.
   x     - address of an integer for returning the table array index.
 
 The keyword table must be arranged in ascending alphabetical order, and
 all letters must be lowercase.
 
 Returns the keyword's associated value ( zero or greater ) if found,
 with the variable x set to the array index, or:
 
  -3 if nothing to look up (target was null),
  -2 if ambiguous,
  -1 if not found.
 
 A match is successful if the target matches a keyword exactly, or if
 the target is a prefix of exactly one keyword.  It is ambiguous if the
 target matches two or more keywords from the table.
*/
 
lookup(table,cmd,n,x) char *cmd; struct keytab table[]; int n, *x; {
 
    int i, v, cmdlen;
 
/* Lowercase & get length of target, if it's null return code -3. */
 
    if ((((cmdlen = lower(cmd))) == 0) || (n < 1)) return(-3);
 
/* Not null, look it up */
 
    for (i = 0; i < n-1; i++) {
        if (!strcmp(table[i].kwd,cmd) ||
           ((v = !strncmp(table[i].kwd,cmd,cmdlen)) &&
             strncmp(table[i+1].kwd,cmd,cmdlen))) {
                *x = i;
                return(table[i].val);
             }
        if (v) return(-2);
    }   
 
/* Last (or only) element */
 
    if (!strncmp(table[n-1].kwd,cmd,cmdlen)) {
        *x = n-1;
        return(table[n-1].val);
    } else return(-1);
}

 
/*  G T W O R D  --  Gets a "word" from the command input stream  */
 
/*
Usage: retcode = gtword();
 
Returns:
 -4 if end of file (e.g. pipe broken)
 -2 if command buffer overflows
 -1 if user did some deleting
  0 if word terminates with SP or tab
  1 if ... CR
  2 if ... ESC
  3 if ... ?
 
With:
  pp pointing to beginning of word in buffer
  bp pointing to after current position
  atmbuf containing a copy of the word
  cc containing the number of characters in the word copied to atmbuf
*/

ungword() {             /* unget a word */
    if (ungw) return(0);
    cmflgsav = cmflgs;
    debug(F101,"ungword cmflgs","",cmflgs);
    ungw = 1;
    cmflgs = 0;
    return(0);
}

gtword() {
    int c;                              /* Current char */
    static int inword = 0;              /* Flag for start of word found */
    int quote = 0;                      /* Flag for quote character */
    int echof = 0;                      /* Flag for whether to echo */
    int chsrc = 0;          /* Source of character, 1 = tty */
    int comment = 0;            /* Flag for in comment */
    char *cp;               /* Comment pointer */

#ifdef RTU
    extern int rtu_bug;
#endif

#ifdef datageneral
    extern int termtype;                /* DG terminal type flag */
    extern int con_reads_mt;            /* Console read asynch is active */
    if (con_reads_mt) connoi_mt();      /* Task would interfere w/cons read */
#endif 
 
    if (ungw) {             /* Have a word saved? */
    debug(F110,"gtword ungetting from pp",pp,0);
    while (*pp++ == SP) ;
    setatm(pp);
    ungw = 0;
    cmflgs = cmflgsav;
    debug(F111,"gtword returning atmbuf",atmbuf,cmflgs);
    return(cmflgs);
    }
    pp = np;                            /* Start of current field */
    debug(F111,"gtword: cmdbuf",cmdbuf,(int) cmdbuf);
    debug(F111," bp",bp,(int) bp);
    debug(F111," pp",pp,(int) pp);
 
    while (bp < cmdbuf+CMDBL) {         /* Big get-a-character loop */
    echof = 0;          /* Assume we don't echo because */
    chsrc = 0;          /* character came from reparse buf. */
        if ((c = *bp) == NUL) {         /* If no char waiting in reparse buf */
            if (dpx) echof = 1;         /* must get from tty, set echo flag. */
        c = cmdgetc();      /* Read a character from the tty. */
        chsrc = 1;          /* Remember character source is tty. */
            if (c == EOF) {     /* This can happen if stdin not tty. */
        return(-4);
        }
        c &= 127;           /* Strip any parity bit. */
    }               /* Change this for 8-bit UNIX! */
 
/* Now we have the next character */

        if (quote == 0) {       /* If this is not a quoted character */
            if (c == CMDQ) {        /* Got the quote character itself */
        if (!comment) quote = 1; /* Flag it if not in a comment */
            }
        if (c == FF) {      /* Formfeed. */
                c = NL;                 /* Replace with newline */
        cmdclrscn();        /* Clear the screen */
            }

        if (c == HT) {      /* Tab */
        if (comment)        /* If in comment, */
          c = SP;       /* substitute space */
        else            /* otherwise */
          c = ESC;      /* substitute ESC (for completion) */
        }
        if (c == ';' || c == '#') { /* Trailing comment */
        if (inword == 0) {  /* If we're not in a word */
            comment = 1;    /* start a comment. */
            cp = bp;        /* remember where it starts. */
        }
        }
        if (!comment && c == SP) {  /* Space */
                *bp++ = c;      /* deposit in buffer if not already */
                if (echof) putchar(c);  /* echo it. */
                if (inword == 0) {      /* If leading, gobble it. */
                    pp++;
                    continue;
                } else {                /* If terminating, return. */
                    np = bp;
                    setatm(pp);
                    inword = 0;
            return(cmflgs = 0);
                }
            }
            if (c == NL || c == CR) {   /* CR or LF. */
        if (echof) cmdnewl(c);  /* Echo it. */
        if (*(bp-1) == '-') {   /* Is this line continued? */
            if (chsrc) {    /* If reading from tty, */
            bp--;       /* back up the buffer pointer, */
            *bp = NUL;  /* erase the dash, */
            continue;   /* and go back for next char now. */
            }
        } else {        /* No, a command has been entered. */
            *bp = NUL;      /* Terminate the command string. */
            if (comment) {  /* If we're in a comment, */
            comment = 0;    /* Say we're not any more, */
            *cp = NUL;  /* cut it off. */
            }
            np = bp;        /* Where to start next field. */
            setatm(pp);     /* Copy this field to atom buffer. */
            inword = 0;     /* Not in a word any more. */
            return(cmflgs = 1);
        }
            }
            if (!comment && echof && (c == '?')) { /* Question mark */
                putchar(c);
                *bp = NUL;
                setatm(pp);
                return(cmflgs = 3);
            }
            if (c == ESC) {     /* ESC */
        if (!comment) {
            *bp = NUL;
            setatm(pp);
            return(cmflgs = 2);
        } else {
            putchar(BEL);
            continue;
        }
            }
            if (c == BS || c == RUB) {  /* Character deletion */
                if (bp > cmdbuf) {      /* If still in buffer... */
            cmdchardel();   /* erase it. */
                    bp--;               /* point behind it, */
                    if (*bp == SP) inword = 0; /* Flag if current field gone */
                    *bp = NUL;          /* Erase character from buffer. */
                } else {                /* Otherwise, */
                    putchar(BEL);       /* beep, */
                    cmres();            /* and start parsing a new command. */
                }
                if (pp < bp) continue;
                else return(cmflgs = -1);
            }
            if (c == LDEL) {            /* ^U, line deletion */
                while ((bp--) > cmdbuf) {
                    cmdchardel();
                    *bp = NUL;
                }
                cmres();                /* Restart the command. */
                inword = 0;
                return(cmflgs = -1);
            }
            if (c == WDEL) {            /* ^W, word deletion */
                if (bp <= cmdbuf) {     /* Beep if nothing to delete */
                    putchar(BEL);
                    cmres();
                    return(cmflgs = -1);
                }
                bp--;
                for ( ; (bp >= cmdbuf) && (*bp == SP) ; bp--) {
                    cmdchardel();
                    *bp = NUL;
                }
                for ( ; (bp >= cmdbuf) && (*bp != SP) ; bp--) {
                    cmdchardel();
                    *bp = NUL;
                }
                bp++;
                inword = 0;
                return(cmflgs = -1);
            }
            if (c == RDIS) {            /* ^R, redisplay */
                *bp = NUL;
                printf("\n%s%s",cmprom,cmdbuf);
                continue;
            }
        if (c < SP && quote == 0) { /* Any other unquoted control */
        putchar(BEL);       /* character -- just beep and */
        continue;       /* continue, don't put in buffer */
        }
        } else {            /* This character was quoted. */
        quote = 0;          /* Unset the quote flag. */
        /* Quote character at this level is only for SP, ?, and controls */
            /* If anything else was quoted, leave quote in, and let */
        /* the command-specific parsing routines handle it, e.g. \007 */
        if (c > 32 && c != '?' && c != RUB && chsrc != 0)
          *bp++ = CMDQ;     /* Deposit \ if it came from tty */
        debug(F110,"gtword quote",cmdbuf,0);
    }
        if (echof) cmdecho(c,quote);    /* Echo what was typed. */
        if (!comment) inword = 1;   /* Flag we're in a word. */
    if (quote) continue;        /* Don't deposit quote character. */
        if (c != NL) *bp++ = c;     /* Deposit command character. */
    debug(F110,"gtword deposit",cmdbuf,0);
    }                                   /* End of big while */
    putchar(BEL);                       /* Get here if... */
    printf("\n?Buffer full\n");
    return(cmflgs = -2);
}

/* Utility functions */
 
/* A D D B U F  -- Add the string pointed to by cp to the command buffer  */
 
addbuf(cp) char *cp; {
    int len = 0;
    while ((*cp != NUL) && (bp < cmdbuf+CMDBL)) {
        *bp++ = *cp++;                  /* Copy and */
        len++;                          /* count the characters. */
    }   
    *bp++ = SP;                         /* Put a space at the end */
    *bp = NUL;                          /* Terminate with a null */
    np = bp;                            /* Update the next-field pointer */
    return(len);                        /* Return the length */
}
 
/*  S E T A T M  --  Deposit a token in the atom buffer.  */
/*  Break on space, newline, carriage return, or null. */
/*  Null-terminate the result. */
/*  If the source pointer is the atom buffer itself, do nothing. */
/*  Return length of token, and also set global "cc" to this length. */
 
setatm(cp) char *cp; {
    char *ap, *xp;

    cc = 0;             /* Character couner */
    ap = atmbuf;            /* Address of atom buffer */

    if (cp == ap) {         /* In case source is atom buffer */
    xp = atybuf;            /* make a copy */
    strcpy(xp,ap);          /* so we can copy it back, edited. */
    cp = xp;
    }
    *ap = NUL;              /* Zero the atom buffer */
    while (*cp == SP) cp++;     /* Trim leading spaces */
    while ((*cp != SP) && (*cp != NL) && (*cp != NUL) && (*cp != CR)) {
        *ap++ = *cp++;          /* Copy up to SP, NL, CR, or end */
        cc++;               /* and count */
    }
    *ap++ = NUL;            /* Terminate the string. */
    return(cc);                         /* Return length. */
}
 
/*  R D I G I T S  -- Verify that all the characters in line ARE DIGITS  */
 
rdigits(s) char *s; {
    while (*s) {
        if (!isdigit(*s)) return(0);
        s++;
    }
    return(1);
}
 
/*  L O W E R  --  Lowercase a string  */
 
lower(s) char *s; {
    int n = 0;
    while (*s) {
        if (isupper(*s)) *s = tolower(*s);
        s++, n++;
    }
    return(n);
}
 
/*  T E S T  --  Bit test  */
 
test(x,m) int x, m; { /*  Returns 1 if any bits from m are on in x, else 0  */
    return((x & m) ? 1 : 0);
}

/* These functions attempt to hide system dependencies from the mainline */
/* code in gtword().  Ultimately they should be moved to ck*tio.c, where */
/* * = each and every system supported by C-Kermit. */

cmdgetc() {             /* Get a character from the tty. */
    int c;

#ifdef datageneral
    {
    char ch;
    c = dgncinb(0,&ch,1);       /* -1 is EOF, -2 TO, 
                                         * -c is AOS/VS error */
    if (c == -2) {          /* timeout was enabled? */
        resto(channel(0));      /* reset timeouts */
        c = dgncinb(0,&ch,1);   /* retry this now! */
    }
    if (c < 0) return(-4);      /* EOF or some error */
    else c = (int) ch & 0177;   /* Get char without parity */
/*  echof = 1; */
    }
#else
#ifdef OS2
    c = isatty(0) ? coninc(0) : getchar();
    if (c<0) return(-4);
#else
    c = getchar();          /* or from tty. */
#ifdef RTU
    if (rtu_bug) {
    c = getchar();          /* RTU doesn't discard the ^Z */
    rtu_bug = 0;
    }
#endif /* RTU */
#endif
#endif
    return(c);              /* Return what we got */
}


cmdclrscn() {               /* Clear the screen */

#ifdef aegis
    putchar(FF);
#else
#ifdef AMIGA
    putchar(FF);
#else
#ifdef OSK
    putchar(FF);
#else
#ifdef datageneral
    putchar(FF);
#else
#ifdef OS2
    { char cell[2];
      cell[0] = ' ';
      cell[1] = 7;
/*      VioScrollUp(0,0,-1,-1,-1,cell,0); */
/*      VioSetCurPos(0,0,0); */
      AVIOScrollUp(0,0,-1,-1,-1,cell);
      AVIOSetCurPos(0,0);
//      VioScrollUp(0,0,-1,-1,-1,cell,0);
//      VioSetCurPos(0,0,0);
  }
#else
    system("clear");
#endif
#endif
#endif
#endif
#endif
}

cmdnewl(c) char c; {            /* What to echo at end of command */
    putchar(c);             /* c is the terminating character */
#ifdef OS2
    if (c == CR) putchar(NL);
#endif
#ifdef aegis
    if (c == CR) putchar(NL);
#endif
#ifdef AMIGA
    if (c == CR) putchar(NL);
#endif
#ifdef datageneral
    if (c == CR) putchar(NL);
#endif
}

cmdchardel() {              /* Erase a character from the screen */
#ifdef datageneral
    /* DG '\b' is EM (^y or \031) */
    if (termtype == 1)
      /* Erase a character from non-DG screen, */
      dgncoub(1,"\010 \010",3);
    else
#endif
      printf("\b \b");
}

cmdecho(c,quote) char c; int quote; {   /* Echo tty input character c */
    putchar(c);
#ifdef OS2
    if (quote==1 && c==CR) putchar(NL);
#endif
}

/*  X X E S C  --  Interprets backslash codes  */
/*  Returns the int value of the backslash code if it is > -1 and < 256 */
/*  Otherwise returns -1 */
/*  Updates the string pointer to first character after backslash code */ 
/*  If the backslash code is invalid, leaves pointer pointing at the */
/*  second character after the backslash. */

xxesc(s) char **s; {            /* Expand backslash escapes */
    int x, y, brace, radix;     /* Returns the int value */
    char hd, *p;

    p = *s;             /* pointer to beginning */
    x = *p++;               /* character at beginning */
    if (x != CMDQ) return(-1);      /* make sure it's a backslash code */

    x = *p;             /* it is, get the next character */
    if (x == '{') {         /* bracketed quantity? */
    x = *(++p);         /* begin past bracket */
    brace = 1;
    } else brace = 0;
    switch (x) {            /* Start interpreting */
      case 'd':             /* Decimal radix indicator */
      case 'D':
    p++;                /* Just point past it and fall thru */
      case '0':             /* Starts with digit */
      case '1':
      case '2':  case '3':  case '4':  case '5':
      case '6':  case '7':  case '8':  case '9':
    radix = 10;         /* Decimal */
    hd = '9';           /* highest valid digit */
    break;
      case 'o':             /* Starts with o or O */
      case 'O':
    radix = 8;          /* Octal */
    hd = '7';           /* highest valid digit */
    p++;                /* point past radix indicator */
    break;
      case 'x':             /* Starts with x, X, h, or H */
      case 'X':
      case 'h':
      case 'H':
    radix = 16;         /* Hexidecimal */
    p++;                /* point past radix indicator */
    break;
      case CMDQ:            /* A second backslash */
    *s = p+1;           /* point past it (\\ becomes \) */
        return(CMDQ);
      default:              /* All others */
    *s = p;             /* Return current pointer */
    return(-1);         /* with failure indication. */
    }
    if (radix <= 10) {          /* Number in radix 8 or 10 */
    for ( x = y = 0;
          (*p) && (*p >= '0') && (*p <= hd) && (y < 3) && (x*radix < 256);
          p++,y++) {
        x = x * radix + (int) *p - 48;
    }
    if (y == 0 || x > 255) {    /* No valid digits? */
        *s = p;         /* point after it */
        return(-1);         /* return failure. */
    }
    } else if (radix == 16) {       /* Special case for hex */
    if ((x = unhex(*p++)) < 0) { *s = p - 1; return(-1); }
    if ((y = unhex(*p++)) < 0) { *s = p - 2; return(-1); }
    x = ((x << 4) & 0xF0) | (y & 0x0F);
    } else x = -1;
    if (brace && *p == '}' && x > -1)   /* Point past closing brace, if any */
      p++;
    *s = p;             /* Point to next char after sequence */
    return(x);              /* Return value of sequence */
}

unhex(x) char x; {
    if (x >= '0' && x <= '9')       /* 0-9 is offset by hex 30 */
      return(x - 0x30);
    else if (x >= 'A' && x <= 'F')  /* A-F offset by hex 37 */
      return(x - 0x37);
    else if (x >= 'a' && x <= 'f')  /* a-f offset by hex 57 */
      return(x - 0x57);         /* (obviously ASCII dependent) */
    else return(-1);
}

/* See if argument string is numeric */
/* Returns 1 if OK, zero if not OK */
/* If OK, string should be acceptable to atoi() */
/* Allows leading space, sign */

chknum(s) char *s; {            /* Check Numeric String */
    int x = 0;              /* Flag for past leading space */
    int y = 0;              /* Flag for digit seen */
    char c;
    debug(F110,"chknum",s,0);
    while (c = *s++) {          /* For each character in the string */
    switch (c) {
      case SP:          /* Allow leading spaces */
      case HT:
        if (x == 0) continue;
        else return(0);
      case '+':         /* Allow leading sign */
      case '-':
        if (x == 0) x = 1;
        else return(0);
        break;
      default:          /* After that, only decimal digits */
        if (c >= '0' && c <= '9') {
        x = y = 1;
        continue;
        } else return(0);
    }
    }
    return(y);
}
