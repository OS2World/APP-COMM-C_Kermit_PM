/*
  File ckuus4.c -- Functions moved from other ckuus*.c modules to even
  out their sizes.
*/
#include <stdio.h>
#include <ctype.h>
#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#include "ckucmd.h"
#include "ckuusr.h"
#include "ckcxla.h"
#ifndef AMIGA
#include <signal.h>
#include <setjmp.h>
#endif

#ifdef OS2
#define SIGALRM SIGUSR1
void alarm( unsigned );
#endif

extern CHAR mystch, stchr, eol, seol, padch, mypadc, ctlq;
extern CHAR *data, *rdatap, ttname[];
extern char *ckxsys, *ckzsys, *cmarg, *cmarg2, **xargv, **cmlist;
extern char cmdbuf[], line[], debfil[], pktfil[], sesfil[], trafil[];
extern kermrc[];
extern char inpbuf[];           /* Buffer for INPUT and REINPUT */
extern char *inpbp;         /* And pointer to same */
extern struct keytab cmdtab[];
extern int ncmd, network;
extern int rcflag;
extern struct mtab mactab[];
extern int escape;
extern int nmac;
extern int action, cflg, xargc, stdouf, stdinf, displa, cnflg, nfils, cnflg;
extern int nrmt, nprm, dfloc, deblog, seslog, speed, local, parity, duplex;
extern int turn, turnch, pktlog, tralog, mdmtyp, flow, cmask, timef, spsizf;
extern int rtimo, timint, srvtim, npad, mypadn, bctr, delay;
extern int maxtry, spsiz, urpsiz, maxsps, maxrps, ebqflg, ebq;
extern int rptflg, rptq, fncnv, binary, pktlog, warn, quiet, fmask, keep;
extern int tsecs, bctu, len, atcapu, lpcapu, swcapu, sq, rpsiz;
extern int wslots, wslotsn, wslotsr;
extern int capas, atcapr;
extern int spackets, rpackets, timeouts, retrans, crunched, wmax;
extern int fcharset, tcharset, tslevel;
extern int indef, intime, incase, inecho;
extern int rptn, language, nlng, cmask, zincnt;
extern int xxstring();

extern CHAR *zinptr;
extern long filcnt, tfc, tlci, tlco, ffc, flci, flco;
extern char *dftty, *versio, *ckxsys;
extern struct langinfo langs[];
extern struct keytab prmtab[];
extern struct keytab remcmd[];
extern struct keytab lngtab[];
extern struct csinfo fcsinfo[], tcsinfo[];

char *malloc();

/* Macro stuff */
extern int maclvl;
extern char *m_arg[MACLEVEL][10]; /* You have to put in the dimensions */
extern char *g_var[GVARS];    /* for external 2-dimensional arrays. */

/* Pointers to translation functions (ditto!) */
extern CHAR (*xls[MAXTCSETS+1][MAXFCSETS+1])();
CHAR (*sx)();               /* Local translation function */

/*** The following functions moved here from ckuus2.c because that module ***/
/*** got too big... ***/



/*  P R E S C A N -- Quick look thru command-line args for init file name */
prescan() {
    int yargc; char **yargv;
    char x;

    yargc = xargc;
    yargv = xargv;
    strcpy(kermrc,KERMRC);      /* Default init file name */
    while (--yargc > 0) {       /* Look for -y on command line */
    yargv++;
    if (**yargv == '-') {       /* Option starting with dash */
        x = *(*yargv+1);        /* Get option letter */
        if (x == 'y') {     /* Is it y? */
        yargv++, yargc--;   /* Yes, count and check argument */
        if (yargc < 1) fatal("missing name in -y");
        strcpy(kermrc,*yargv);  /* Replace init file name */
        rcflag = 1;     /* Flag that this has been done */
        return;
        } else if (x == 'd') {  /* Do this early as possible! */
        debopn("debug.log");
        return;
        }
    } 
    }
}


/*  C M D L I N  --  Get arguments from command line  */
/*
 Simple Unix-style command line parser, conforming with 'A Proposed Command
 Syntax Standard for Unix Systems', Hemenway & Armitage, Unix/World, Vol.1,
 No.3, 1984.
*/
cmdlin() {
    char x;             /* Local general-purpose int */
    cmarg = "";             /* Initialize globals */
    cmarg2 = "";
    action = cflg = 0;
 
    while (--xargc > 0) {       /* Go through command line words */
    xargv++;
    debug(F111,"xargv",*xargv,xargc);
        if (**xargv == '-') {       /* Got an option (begins with dash) */
        x = *(*xargv+1);        /* Get the option letter */
        if (doarg(x) < 0) doexit(BAD_EXIT); /* Go handle the option */
        } else {            /* No dash where expected */
        usage();
        doexit(BAD_EXIT);
    }
    }
    debug(F101,"action","",action);
    if (!local) {
    if ((action == 'g') || (action == 'r') ||
        (action == 'c') || (cflg != 0))
        fatal("-l and -b required");
    }
    if (*cmarg2 != 0) {
    if ((action != 's') && (action != 'r') &&
        (action != 'v'))
        fatal("-a without -s, -r, or -g");
    }
    if ((action == 'v') && (stdouf) && (!local)) {
        if (isatty(1))
        fatal("unredirected -k can only be used in local mode");
    }
    if ((action == 's') || (action == 'v') ||
        (action == 'r') || (action == 'x')) {
    if (local) displa = 1;
    if (stdouf) { displa = 0; quiet = 1; }
    }
 
    if (quiet) displa = 0;      /* No display if quiet requested */
 
    if (cflg) {
    conect();           /* Connect if requested */
    if (action == 0) {
        if (cnflg) conect();    /* And again if requested */
        doexit(GOOD_EXIT);      /* Then exit indicating success */
    }
    }
    if (displa) concb(escape);      /* (for console "interrupts") */
    return(action);         /* Then do any requested protocol */
}

/*  D O A R G  --  Do a command-line argument.  */
 
doarg(x) char x; {
    int z; char *xp;
 
    xp = *xargv+1;          /* Pointer for bundled args */
    while (x) {
    switch (x) {
 
case 'x':               /* server */
    if (action) fatal("conflicting actions");
    action = 'x';
    break;
 
case 'f':
    if (action) fatal("conflicting actions");
    action = setgen('F',"","","");
    break;
 
case 'r':               /* receive */
    if (action) fatal("conflicting actions");
    action = 'v';
    break;
 
case 'k':               /* receive to stdout */
    if (action) fatal("conflicting actions");
    stdouf = 1;
    action = 'v';
    break;
 
case 's':               /* send */
    if (action) fatal("conflicting actions");
    if (*(xp+1)) fatal("invalid argument bundling after -s");
    z = nfils = 0;          /* Initialize file counter, flag */
    cmlist = xargv+1;           /* Remember this pointer */
    while (--xargc > 0) {       /* Traverse the list */ 
    xargv++;
    if (**xargv == '-') {       /* Check for sending stdin */
        if (strcmp(*xargv,"-") != 0) break;
        z++;
        }
    nfils++;            /* Bump file counter */
    }
    xargc++, xargv--;           /* Adjust argv/argc */
    if (nfils < 1) fatal("missing filename for -s");
    if (z > 1) fatal("-s: too many -'s");
    if (z == 1) {
    if (nfils == 1) nfils = 0;
    else fatal("invalid mixture of filenames and '-' in -s");
    }
    if (nfils == 0) {
    if (isatty(0)) fatal("sending from terminal not allowed");
    else stdinf = 1;
    }
    debug(F101,*xargv,"",nfils);
    action = 's';
    break;
 
case 'g':               /* get */
    if (action) fatal("conflicting actions");
    if (*(xp+1)) fatal("invalid argument bundling after -g");
    xargv++, xargc--;
    if ((xargc == 0) || (**xargv == '-'))
        fatal("missing filename for -g");
    cmarg = *xargv;
    action = 'r';
    break;
 
case 'c':               /* connect before */
    cflg = 1;
    break;
 
case 'n':               /* connect after */
    cnflg = 1;
    break;
 
case 'h':               /* help */
    usage();
    doexit(GOOD_EXIT);
 
case 'a':               /* "as" */
    if (*(xp+1)) fatal("invalid argument bundling after -a");
    xargv++, xargc--;
    if ((xargc < 1) || (**xargv == '-'))
        fatal("missing name in -a");
    cmarg2 = *xargv;
    break;
 
case 'y':               /* alternate init-file name */
    if (*(xp+1)) fatal("invalid argument bundling after -y");
    xargv++, xargc--;
    if (xargc < 1) fatal("missing name in -y");
    /* strcpy(kermrc,*xargv); ...this was already done in prescan()... */
    break;

case 'l':               /* set line */
    if (*(xp+1)) fatal("invalid argument bundling after -l");
    xargv++, xargc--;
    if ((xargc < 1) || (**xargv == '-'))
        fatal("communication line device name missing");
    strcpy(ttname,*xargv);
/*  if (strcmp(ttname,dftty) == 0) local = dfloc; else local = 1;  */
    local = (strcmp(ttname,CTTNAM) != 0); /* (better than old way) */
    debug(F101,"local","",local);
    ttopen(ttname,&local,0);
    break;
 
case 'b':                       /* set baud */
    if (*(xp+1)) fatal("invalid argument bundling");
    xargv++, xargc--;
    if ((xargc < 1) || (**xargv == '-'))
        fatal("missing baud");
    z = atoi(*xargv);           /* Convert to number */
    if (chkspd(z) > -1) speed = z;  /* Check it */
        else fatal("unsupported baud rate");
    break;
 
case 'e':               /* Extended packet length */
    if (*(xp+1)) fatal("invalid argument bundling");
    xargv++, xargc--;
    if ((xargc < 1) || (**xargv == '-'))
        fatal("missing length");
    z = atoi(*xargv);           /* Convert to number */
    if (z > 10 && z < maxrps) {
        rpsiz = urpsiz = z;
    if (z > 94) rpsiz = 94;     /* Fallback if other Kermit can't */
    } else fatal("Unsupported packet length");
    break;

case 'i':               /* Treat files as binary */
    binary = 1;
    break;
 
case 'w':               /* File warning */
    warn = 1;
    break;
 
case 'q':               /* Quiet */
    quiet = 1;
    break;
 
case 'd':               /* debug */
/** debopn("debug.log"); *** already did this in prescan() **/
    break;
 
case 'p':               /* set parity */
    if (*(xp+1)) fatal("invalid argument bundling");
    xargv++, xargc--;
    if ((xargc < 1) || (**xargv == '-'))
        fatal("missing parity");
    switch(x = **xargv) {
    case 'e':
    case 'o':
    case 'm':
    case 's': parity = x; break;
    case 'n': parity = 0; break;
    default:  fatal("invalid parity");
        }
    break;
 
case 't':
    turn = 1;               /* Line turnaround handshake */
    turnch = XON;           /* XON is turnaround character */
    duplex = 1;             /* Half duplex */
    flow = 0;               /* No flow control */
    break;
 
#ifdef OS2
case 'u':
    /* get numeric argument */
    if (*(xp+1)) fatal("invalid argument bundling");
    *xargv++, xargc--;
    if ((xargc < 1) || (**xargv == '-'))
        fatal("missing handle");
    z = atoi(*xargv);           /* Convert to number */
    ttclos();
    if (!ttiscom(z)) fatal("invalid handle");
    speed = ttspeed();
    break;
#endif /* OS2 */

default:
    fatal("invalid argument, type 'kermit -h' for help");
        }
 
    x = *++xp;              /* See if options are bundled */
    }
    return(0);
}


/*  T R A N S M I T  --  Raw upload  */

/*  Obey current line, duplex, parity, flow, text/binary settings. */
/*  Returns 0 upon apparent success, 1 on obvious failure.  */

/***
 Things to add:
 . Make both text and binary mode obey set file bytesize.
 . Maybe allow user to specify terminators other than CR?
 . Maybe allow user to specify prompts other than single characters?
***/

int tr_int;             /* Flag if TRANSMIT interrupted */

SIGTYP
trtrap() {              /* TRANSMIT interrupt trap */
    tr_int = 1;
    return;
}


/*  T R A N S M I T  --  Raw upload  */

transmit(s,t) char *s; char t; {

#ifndef OS2
    SIGTYP (* oldsig)();        /* For saving old interrupt trap. */
#endif
    int z = 1;              /* Return code. 0=fail, 1=succeed. */
    int x, c, i, n;         /* Workers... */
    CHAR tt;

    tt = dopar(t);          /* Turnaround char, with parity */

    if (zopeni(ZIFILE,s) == 0) {    /* Open the file to be transmitted */
    printf("?Can't open %s\n",s);
    return(0);
    }
    x = -1;             /* Open the communication line */
    if (ttopen(ttname,&x,mdmtyp) < 0) { /* (does no harm if already open) */
    printf("Can't open %s\n",ttname);
    return(0);
    }
    x = x ? speed : -1;         /* Put the line in "packet mode" */
    if (ttpkt(x,flow,parity) < 0) {
    printf("Can't condition line\n");
    return(0);
    }
    i = 0;              /* Beginning of buffer. */
#ifndef OS2
#ifndef AMIGA
    oldsig = signal(SIGINT, trtrap);    /* Save current interrupt trap. */
#endif
#endif
    tr_int = 0;             /* Have not been interrupted (yet). */
    z = 1;              /* Return code presumed good. */

    while ((c = zminchar()) != -1) {    /* Loop for all characters in file */
    if (tr_int) {           /* Interrupted? */
        printf("^C...\n");      /* Print message */
        z = 0;
        break;
    }
    if (duplex) conoc(c);       /* Echo character on screen */
    if (binary) {           /* If binary file */
        if (ttoc(dopar(c)) < 0) {   /* just try to send the character */
        printf("?Can't transmit character\n");
        z = 0;
        break;
        }
        if (! duplex) {
        x = ttinc(1);       /* Try to read back echo */
        if (x > -1) conoc(x);
        }
    } else {            /* Line at a time for text files... */
        if (c == '\n') {        /* Got a line */
        if (i == 0 || line[i-1] != dopar('\r'))
          line[i++] = dopar('\r'); /* Terminate it with CR */
        if (ttol(line,i) < 0) { /* try to send it */
            printf("?Can't transmit line\n");
            z = 0;
            break;
        }
        i = 0;          /* Reset the buffer pointer */
        if (t) {        /* If we want a turnaround character */
            x = 0;      /* wait for it */
            while ((x != -1) && (x != t)) {
            x = ttinc(1);
            if (! duplex) conoc(x); /* also echo any echoes */
            }
        }
        } else {            /* Not a newline, regular character */
        line[i++] = dopar(c);   /* Put it in line buffer. */
        if (i == LINBUFSIZ) {   /* If buffer full, */
            if (ttol(line,i) < 0) { /* try to send it. */
            printf("Can't send buffer\n");
            z = 0;
            break;
            }           /* Don't wait for turnaround */
            i = 0;      /* Reset buffer pointer */
        }
        }
    }
    }
#ifndef OS2
#ifndef AMIGA
    signal(SIGINT,oldsig);      /* put old signal action back. */
#endif
#endif
    ttres();                /* Done, restore tty, */
    zclose(ZIFILE);         /* close file, */
    return(z);              /* and return successfully. */
}

/*  D O T Y P E  --  Type a file  */

dotype(s) char *s; {

#ifndef OS2
    SIGTYP (* oldsig)();        /* For saving old interrupt trap. */
#endif
    int z = 1;              /* Return code. */
    int x, c, i, n;         /* Workers... */

    if (zopeni(ZIFILE,s) == 0) {    /* Open the file to be transmitted */
    printf("?Can't open %s\n",s);
    return(0);
    }
    i = 0;              /* Beginning of buffer. */
#ifndef OS2
#ifndef AMIGA
    oldsig = signal(SIGINT, trtrap);    /* Save current interrupt trap. */
#endif
#endif
    tr_int = 0;             /* Have not been interrupted (yet). */
    z = 1;              /* Return code presumed good. */

    while ((c = zminchar()) != -1) {    /* Loop for all characters in file */
    if (tr_int) {           /* Interrupted? */
        printf("^C...\n");      /* Print message */
        z = 0;
        break;
    }
    conoc(c);           /* Echo character on screen */
    }
#ifndef OS2
#ifndef AMIGA
    signal(SIGINT,oldsig);      /* put old signal action back. */
#endif
#endif
    tr_int = 0;
    ttres();                /* Done, restore tty, */
    zclose(ZIFILE);         /* close file, */
    return(z);              /* and return successfully. */
}
/*  X L A T E  --  Translate a local file from one character set to another */

/*
  Translates from current file character set (fcharset) to the current
  transfer character set (tcharset).  For now there's no way to ask it
  to translate in the other direction, e.g. from Latin-1 to German ASCII.
*/

xlate(fin, fout) char *fin, *fout; {    /* Call with names of in & out files */

#ifndef OS2
    SIGTYP (* oldsig)();        /* For saving old interrupt trap. */
#endif
    int z = 1;              /* Return code. */
    int x, c;               /* Workers. */

    if (zopeni(ZIFILE,fin) == 0) {  /* Open the file to be transmitted */
    printf("?Can't open input file %s\n",fin);
    return(0);
    }
    if (zopeno(ZOFILE,fout) == 0) { /* And the output file */
    printf("?Can't open output file %s\n",fout);
    return(0);
    }
#ifndef OS2
#ifndef AMIGA
    oldsig = signal(SIGINT, trtrap);    /* Save current interrupt trap. */
#endif
#endif
    tr_int = 0;             /* Have not been interrupted (yet). */
    z = 1;              /* Return code presumed good. */

    printf("%s (%s) => %s (%s)\n",  /* Say what we're doing. */
       fin, fcsinfo[fcharset].name,
       fout,tcsinfo[tcharset].name
    );
    while ((c = zminchar()) != -1) {    /* Loop for all characters in file */
    if (tr_int) {           /* Interrupted? */
        printf("^C...\n");      /* Print message */
        z = 0;
        break;
    }
    sx = xls[tcharset][fcharset];   /* Get translation function */
    if (zchout(ZOFILE,(*sx)(c)) < 0) { /* Output translated character */
        printf("File output error\n");
        z = 0;
        break;
    }
    }
#ifndef OS2
#ifndef AMIGA
    signal(SIGINT,oldsig);      /* put old signal action back. */
#endif
#endif
    tr_int = 0;
    zclose(ZIFILE);         /* close files, */
    zclose(ZOFILE);
    return(z);              /* and return successfully. */
}

/*  D O L O G  --  Do the log command  */
 
dolog(x) int x; {
    int y; char *s;
 
    switch (x) {
 
    case LOGD:
#ifdef DEBUG
        y = cmofi("Name of debugging log file","debug.log",&s,xxstring);
#else
            y = -2; s = "";
        printf("%s","- Sorry, debug log not available\n");
#endif
        break;
 
    case LOGP:
        y = cmofi("Name of packet log file","packet.log",&s,xxstring);
        break;
 
    case LOGS:
        y = cmofi("Name of session log file","session.log",&s,xxstring);
        break;
 
    case LOGT:
#ifdef TLOG
        y = cmofi("Name of transaction log file","transact.log",&s,
              xxstring);
#else
            y = -2; s = "";
        printf("%s","- Sorry, transaction log not available\n");
#endif
        break;
 
    default:
        printf("\n?Unexpected log designator - %d\n",x);
        return(-2);
    }
    if (y < 0) return(y);
 
    strcpy(line,s);
    s = line;
    if ((y = cmcfm()) < 0) return(y);
 
    switch (x) {
 
    case LOGD:
        return(deblog = debopn(s));
 
    case LOGP:
        return(pktlog = pktopn(s));
 
    case LOGS:
        return(seslog=sesopn(s));

    case LOGT:
        return(tralog=traopn(s));
 
    default:
        return(-2);
    }
}

int pktopn(char * s)
    {
    extern char pktfil[];
    
    int y;

    zclose(ZPFILE);
    if(s[0]='\0')
        return(0);

    y = zopeno(ZPFILE,s);
    if (y > 0) 
        strcpy(pktfil,s); 
    else 
        *pktfil = '\0';
    
    return(y);
    }


int traopn(char * s)
    {
    extern char trafil[];

    int y;

    zclose(ZTFILE);
    if(s[0]=='\0')
        return(0);

    y = zopeno(ZTFILE,s);
    if (y > 0) {
    strcpy(trafil,s);
    tlog(F110,"Transaction Log:",versio,0l);
    tlog(F100,ckxsys,"",0);
    ztime(&s);
    tlog(F100,s,"",0l);
        }
    else *trafil = '\0';
    
    return(y);
    }


int sesopn(s)
char * s;
    {
    extern char sesfil[];
    int y;

    zclose(ZSFILE);
    if(s[0]=='\0')
        return(0);

    y = zopeno(ZSFILE,s);
    if (y > 0) strcpy(sesfil,s); else *sesfil = '\0';

    return(y);
    }
 


/*  D E B O P N  --  Open a debugging file  */
 
debopn(s) char *s; {
#ifdef DEBUG
    char *tp;

    zclose(ZDFILE);
    if(s[0]=='\0')
        {
        return(deblog=0);
        }

    deblog = zopeno(ZDFILE,s);
    if (deblog > 0) {
    strcpy(debfil,s);
    debug(F110,"Debug Log ",versio,0);
    debug(F100,ckxsys,"",0);
    ztime(&tp);
    debug(F100,tp,"",0);
    } else *debfil = '\0';
    return(deblog);
#else
    return(0);
#endif
}

/*  S H O P A R  --  Show Parameters  */
 
shoparc() {
    int i;
    extern struct keytab mdmtab[]; extern int nmdm;

    puts("Communications Parameters:");

    if (network) {
    printf(" Host: %s",ttname);
    } else {
    printf(" Line: %s, speed: ",ttname);
    if (speed < 0) printf("unknown"); else printf("%d",speed);
    }
    printf(", mode: ");
    if (local) printf("local"); else printf("remote");
    if (network == 0) {
    for (i = 0; i < nmdm; i++) {
        if (mdmtab[i].val == mdmtyp) {
        printf(", modem-dialer: %s",mdmtab[i].kwd);
        break;
        }
    }
    }
    printf("\n Bits: %d",(parity) ? 7 : 8);
    printf(", parity: ");
    switch (parity) {
    case 'e': printf("even");  break;
    case 'o': printf("odd");   break;
    case 'm': printf("mark");  break;
    case 's': printf("space"); break;
    case 0:   printf("none");  break;
    default:  printf("invalid - %d",parity); break;
    }       
    printf(", duplex: ");
    if (duplex) printf("half, "); else printf("full, ");
    printf("flow: ");
    if (flow == 1) printf("xon/xoff");
    else if (flow == 0) printf("none");
    else printf("%d",flow);
    printf(", handshake: ");
    if (turn) printf("%d\n",turnch); else printf("none\n");
    printf("Terminal emulation: %d bits\n", (cmask == 0177) ? 7 : 8);
    return(0);
}

shoparf() {
    printf("\nFile parameters:              Attributes:       ");
    if (atcapr) printf("on"); else printf("off");
    printf("\n File Names:   ");
    if (fncnv) printf("%-12s","converted"); else printf("%-12s","literal");
#ifdef DEBUG
    printf("   Debugging Log:    ");
    if (deblog) printf("%s",debfil); else printf("none");
#endif
    printf("\n File Type:    ");
    if (binary) printf("%-12s","binary"); else printf("%-12s","text");
    printf("   Packet Log:       ");
    if (pktlog) printf(pktfil); else printf("none");
    printf("\n File Warning: ");
    if (warn) printf("%-12s","on"); else printf("%-12s","off");
    printf("   Session Log:      ");
    if (seslog) printf(sesfil); else printf("none");
    printf("\n File Display: ");
    if (quiet) printf("%-12s","off"); else printf("%-12s","on");
#ifdef TLOG
    printf("   Transaction Log:  ");
    if (tralog) printf(trafil); else printf("none");
#endif
    if (! binary) {
    shocharset();
    } else printf("\n");
    printf("\nFile Byte Size: %d",(fmask == 0177) ? 7 : 8);
    printf(", Incomplete File Disposition: ");
    if (keep) printf("keep"); else printf("discard");
#ifdef KERMRC    
    printf(", Init file: %s",kermrc);
#endif
    printf("\n");
}

shoparp() {
    printf("\nProtocol Parameters:   Send    Receive");
    if (timef || spsizf) printf("    (* = override)");
    printf("\n Timeout:      %11d%9d", rtimo,  timint);
    if (timef) printf("*"); else printf(" ");
    printf("       Server timeout:%4d\n",srvtim);
    printf("\n Padding:      %11d%9d", npad,   mypadn);
    printf("        Block Check: %6d\n",bctr);
    printf(  " Pad Character:%11d%9d", padch,  mypadc);
    printf("        Delay:       %6d\n",delay);
    printf(  " Packet Start: %11d%9d", mystch, stchr);
    printf("        Max Retries: %6d\n",maxtry);
    printf(  " Packet End:   %11d%9d", seol,   eol);
    if (ebqflg)
      printf("        8th-Bit Prefix: '%c'",ebq);
    printf(  "\n Packet Length:%11d", spsiz);
    printf( spsizf ? "*" : " " ); printf("%8d",  urpsiz);
    printf( (urpsiz > 94) ? " (94)" : "     ");
    if (rptflg)
      printf("   Repeat Prefix:  '%c'",rptq);
    printf(  "\n Length Limit: %11d%9d", maxsps, maxrps);
    printf("        Window:%12d%4d\n",wslotsr,wslotsn);
}

shoparl() {
    int i;
    printf("\nAvailable Languages:\n");
    for (i = 0; i < MAXLANG; i++) {
    printf(" %s\n",langs[i].description);
    }   
    printf("\nCurrent Language: %s\n",langs[language].description);
    shocharset();
    printf("\n\n");
}

shocharset() {
    printf("\nFile Character-Set: %s (",fcsinfo[fcharset].name);
    if (fcsinfo[fcharset].size == 128) printf("7-bit)");
    else printf("8-bit)");
    printf("\nTransfer Character Set");
    if (tslevel == TS_L2)
      printf(": (international)");
    else
      printf(": %s",tcsinfo[tcharset].name);
}

shopar() {
    printf("\n%s,%s, ",versio,ckxsys); 
    shoparc();
    shoparp();
    shoparf();
}

/*  D O S T A T  --  Display file transfer statistics.  */

dostat() {
    printf("\nMost recent transaction --\n");
    printf(" files: %ld\n",filcnt);
    printf(" total file characters  : %ld\n",tfc);
    printf(" communication line in  : %ld\n",tlci);
    printf(" communication line out : %ld\n",tlco);
    printf(" packets sent           : %d\n", spackets);
    printf(" packets received       : %d\n", rpackets);
    printf(" damaged packets rec'd  : %d\n", crunched);
    printf(" timeouts               : %d\n", timeouts);
    printf(" retransmissions        : %d\n", retrans);
    printf(" window slots used      : %d\n", wmax);
    printf(" elapsed time           : %d sec\n",tsecs);
    if (filcnt > 0) {
    if (tsecs > 0) {
        long lx;
        lx = tfc / tsecs;
        printf(" effective data rate    : %ld cps\n",lx);
        if (speed > 0 && network == 0) {
        lx = (lx * 100l) / speed;
        printf(" efficiency             : %ld %%\n",lx * 10L);
        }
    }
    printf(" packet length          : %d (send), %d (receive)\n",
           spsiz,urpsiz);
    printf(" block check type used  : %d\n",bctu);
    printf(" compression            : ");
    if (rptflg) printf("yes [%c] (%d)\n",rptq,rptn); else printf("no\n");
    printf(" 8th bit prefixing      : ");
    if (ebqflg) printf("yes [%c]\n",ebq); else printf("no\n\n");
    } else printf("\n");
    return(1);
}

/*  F S T A T S  --  Record file statistics in transaction log  */

fstats() {
    tfc += ffc;
    tlog(F100," end of file","",0l);
    tlog(F101,"  file characters        ","",ffc);
    tlog(F101,"  communication line in  ","",flci);
    tlog(F101,"  communication line out ","",flco);
}


/*  T S T A T S  --  Record statistics in transaction log  */

tstats() {
    char *tp; int x;

    ztime(&tp);             /* Get time stamp */
    tlog(F110,"End of transaction",tp,0l);  /* Record it */

    if (filcnt < 1) return;     /* If no files, done. */

/* If multiple files, record character totals for all files */

    if (filcnt > 1) {
    tlog(F101," files","",filcnt);
    tlog(F101," total file characters   ","",tfc);
    tlog(F101," communication line in   ","",tlci);
    tlog(F101," communication line out  ","",tlco);
    }

/* Record timing info for one or more files */

    tlog(F101," elapsed time (seconds)  ","",(long) tsecs);
    if (tsecs > 0) {
    long lx;
    lx = (tfc / tsecs) * 10;
    tlog(F101," effective data rate     ","",lx);
    if (speed > 0 && network == 0) {
        lx = (lx * 100L) / speed;
        tlog(F101," efficiency (percent)    ","",lx);
    }
    }
    tlog(F100,"","",0L);        /* Leave a blank line */
}

/*  S D E B U  -- Record spar results in debugging log  */

sdebu(len) int len; {
    debug(F111,"spar: data",rdatap,len);
    debug(F101," spsiz ","", spsiz);
    debug(F101," timint","",timint);
    debug(F101," npad  ","",  npad);
    debug(F101," padch ","", padch);
    debug(F101," seol  ","",  seol);
    debug(F101," ctlq  ","",  ctlq);
    debug(F101," ebq   ","",   ebq);
    debug(F101," ebqflg","",ebqflg);
    debug(F101," bctr  ","",  bctr);
    debug(F101," rptq  ","",  rptq);
    debug(F101," rptflg","",rptflg);
    debug(F101," atcapu","",atcapu);
    debug(F101," lpcapu","",lpcapu);
    debug(F101," swcapu","",swcapu);
    debug(F101," wslotsn","", wslotsn);
}
/*  R D E B U -- Debugging display of rpar() values  */

rdebu(len) int len; {
    debug(F111,"rpar: data",data,len); /*** was rdatap ***/
    debug(F101," rpsiz ","",xunchar(data[0]));
    debug(F101," rtimo ","", rtimo);
    debug(F101," mypadn","",mypadn);
    debug(F101," mypadc","",mypadc);
    debug(F101," eol   ","",   eol);
    debug(F101," ctlq  ","",  ctlq);
    debug(F101," sq    ","",    sq);
    debug(F101," ebq   ","",   ebq);
    debug(F101," ebqflg","",ebqflg);
    debug(F101," bctr  ","",  bctr);
    debug(F101," rptq  ","",data[9]);
    debug(F101," rptflg","",rptflg);
    debug(F101," capas ","",capas);
    debug(F101," bits  ","",data[capas]);
    debug(F101," atcapu","",atcapu);
    debug(F101," lpcapu","",lpcapu);
    debug(F101," swcapu","",swcapu);
    debug(F101," wslotsr","", wslotsr);
    debug(F101," rpsiz(extended)","",rpsiz);
}

/*  D O C O N E C T  --  Do the connect command  */
 
/*  Note, we don't call this directly from dial, because we need to give */
/*  the user a chance to change parameters (e.g. parity) after the */
/*  connection is made. */
 
doconect() {
    int x;
    conres();               /* Put console back to normal */
    x = conect();           /* Connect */
    concb(escape);          /* Put console into cbreak mode, */
    return(x);              /* for more command parsing. */
}

/* The INPUT command */

doinput(timo,s) int timo; char *s; {
    int x, y, i, icn;
    char *xp, *xq;
    CHAR c;

    y = strlen(s);
    debug(F111,"doinput",s,y);
    if (timo <= 0) timo = 1;        /* Give at least 1 second timeout */
    x = 0;              /* Return code, assume failure */
    i = 0;              /* String pattern match position */

    xp = malloc(y+2);           /* Make a separate copy of the */
    if (!xp) {              /* input string for editing. */
    printf("?malloc error 5\n");
    return(x);
    }
    xq = xp;                /* Save pointer to beginning */
    if (!incase) {          /* INPUT CASE = IGNORE?  */
    while (*s) {            /* Yes, convert to lowercase */
        *xp = *s;
        if (isupper(*xp)) *xp = tolower(*xp);
        *xp++; *s++;
    }
    *xp = NUL;          /* Terminate the search string. */
    s = xq;             /* Point back to beginning. */
    }
    while (1) {             /* Character-getting loop */
    if (timo) {
        debug(F100,"input calling ttinc(0)","",0);
        if (local) {        /* One case for local */
        y = ttinc(1);       /* Get character from comm line. */
        debug(F101,"input ttinc(1) returns","",y);
        if (icn = conchk()) {   /* Interrupted from keyboard? */
            debug(F101,"input interrupted from keyboard","",icn);
            while (icn--) coninc(0); /* Yes, read what was typed. */
            x = 0;      /* And fail. */
            break;
        }
        } else {            /* Another for remote */
        y = coninc(1);
        debug(F101,"input coninc(1) returns","",y);
        }
        if (y < 0) {
        if (--timo == 0) break; /* Failed. */
        debug(F101,"input timo","",timo);
        }
    }
    if (y < 1) continue;        /* No character arrived, keep trying */
    c = cmask & (CHAR) y;       /* Mask off parity */
    *inpbp++ = c;           /* Store result in circular buffer */
    if (inpbp >= inpbuf + INPBUFSIZ) inpbp = inpbuf;
    if (inecho) conoc(c);       /* Echo and log the input character */
    if (seslog)
      if (zchout(ZSFILE,c) < 0) seslog = 0;
    if (!incase) {          /* Ignore alphabetic case? */
        if (isupper(c)) c = tolower(c); /* Yes */
    }
    debug(F000,"doinput char","",c);
    debug(F000,"compare char","",s[i]);
    if (c == s[i]) i++; else i = 0; /* Check for match */
    if (s[i] == '\0') {     /* Matched all the way to end? */
        x = 1;          /* Yes, */
        break;          /* done. */
    }
    }
    if (xq) free(xq);           /* Free dynamic memory */
    return(x);
}

/* REINPUT Command */

/* Note, the timeout parameter is required, but ignored. */
/* Syntax is compatible with MS-DOS Kermit except timeout can't be omitted. */
/* This function only looks at the characters already received */
/* and does not read any new characters from the communication line. */

doreinp(timo,s) int timo; char *s; {
    int x, y, i;
    char *xx, *xp, *xq;
    CHAR c;

    y = strlen(s);
    debug(F111,"doinput",s,y);
    if (timo <= 0) timo = 1;        /* Give at least 1 second timeout */
    x = 0;              /* Return code, assume failure */
    i = 0;              /* String pattern match position */

    xp = malloc(y+2);           /* Make a separate copy of the */
    if (!xp) {              /* search string. */
    printf("?malloc error 6\n");
    return(x);
    }
    xq = xp;                /* Keep pointer to beginning. */
    if (!incase) {          /* INPUT CASE = IGNORE?  */
    while (*s) {            /* Yes, convert to lowercase */
        *xp = *s;
        if (isupper(*xp)) *xp = tolower(*xp);
        *xp++; *s++;
    }
    s = xq;
    }
    xx = inpbp;
    do {
    c = *xx++;
    if (xx >= inpbuf + INPBUFSIZ) xx = inpbuf;
    if (!incase) {          /* Ignore alphabetic case? */
        if (isupper(c)) c = tolower(c); /* Yes */
    }
    debug(F000,"doreinp char","",c);
    debug(F000,"compare char","",s[i]);
    if (c == s[i]) i++; else i = 0; /* Check for match */
    if (s[i] == '\0') {     /* Matched all the way to end? */
        x = 1;          /* Yes, */
        break;          /* done. */
    }
    } while (xx != inpbp);
    return(x);
}


/*  X X S T R I N G  --  Interpret strings containing backslash escapes  */

/*
 Copies result to new string.
  strips enclosing braces or doublequotes.
  interprets backslash escapes.
  returns 0 on success, nonzero on failure.
  tries to be compatible with MS-DOS Kermit.

 Syntax of input string:
  string = chars | "chars" | {chars}
  chars = (c* e*)*
  where c = any printable character, ascii 32-126
  and e = a backslash escape
  and * means 0 or more repetitions of preceding quantity
  backslash escape = \operand
  operand = {number} or number
  number = [r]n[n[n]]], i.e. an optional radix code followed by 1-3 digits
  radix code is oO (octal), hHxX (hex), dD or none (decimal).
*/

yystring(s,s2) char *s; char **s2; {    /* Reverse a string */
    int x;
    static char *new;
    new = *s2;
    if ((x = strlen(s)) == 0) {
    *new = '\0';
    return(0);
    }
    x--;
    *new++ = s[x];
    s[x] = 0;
    return(xxstring(s,&new));
}

/*
  X X S T R I N G  --  Expand variables and backslash codes.

    int xxtstring(s,&s2);

  Expands \%x variables via recursive descent.
  Argument s is a pointer to string to expand (source).
  Argument s2 is the address of where to put result (destination).
  Returns -1 on failure, 0 on success,
    with destination string null-terminated and s2 pointing to the
    terminating null, so that subsequent characters can added.
*/

xxstring(s,s2) char *s; char **s2; {
    int x,              /* Current character */
        y,              /* Worker */
        z;              /* Flag for enclosing braces, quotes */
    static int depth = 0;       /* Call depth, avoid overflow */
    char *new;              /* Where to build expanded string */

    depth++;                /* Sink to a new depth */
    if (depth > 20) {           /* Too deep? */
    printf("?definition is circular or too deep\n");
    depth = 0;
    **s2 = NUL;
    return(-1);
    }
    y = strlen(s);          /* Get length */
    z = 0;              /* Flag for stripping last char */
#ifdef COMMENT
    if (*s == 34) {         /* Strip enclosing quotes, if any */
    if (s[y-1] == 34) {
        s++;
        z = 1;
    }
    } else if (*s == '{') {     /* or else enclosing braces */
    if (s[y-1] == '}') {
        s++;
        z = 1;
    }
    }
#endif /* COMMENT */
    while ( x = *s ) {          /* Loop for all characters */
        if (x != CMDQ) {        /* Convert backslash escapes */
        *(*s2)++ = *s++;        /* Normal char, copy it */
        continue;
    } else {
        if ((x = *(s+1)) != '%') {  /* If it's a backslash code */
        y = xxesc(&s);      /* Go interpret it */
        if (y < 0) {        /* Upon failure */
            *(*s2)++ = CMDQ;    /* Just copy the characters */
            *(*s2)++ = x;
            s++;        /* move source pointer past them */
            continue;       /* and go back for more */
        } else *(*s2)++ = y;    /* else deposit interpreted value */
        } else {            /* Otherwise it's a variable */
        char vb, *vp, *ss; int j;
        s += 2;         /* Get the letter or digit */
        vb = *s++;      /* and move source pointer past it */
        if (vb >= '0' && vb <= '9') { /* Digit for macro arg */
            vb -= '0';      /* convert character to integer */
            if (maclvl < 0) /* Digit variables are global */
              vp = g_var[vb];   /* if no macro is active */
            else        /* otherwise */
              vp = m_arg[maclvl][vb]; /* they're on the stack */
        } else vp = g_var[vb];  /* Letter for global variable */
        if (vp) {       /* If definition not empty */
            new = *s2;      /* Recurse... */
            if (xxstring(vp,&new) < 0) return(-1);
            *s2 = new;
        }
        }
    }
    }
#ifdef COMMENT
    if (z) (*s2)--;         /* Strip trailing quote or brace */
#endif /* COMMENT */
    *(*s2) = NUL;           /* Terminate the new string */
    depth--;                /* Adjust stack depth gauge */
    return(0);              /* and return. */
}

xxstrcmp(s1,s2) char *s1; char *s2; {   /* Caseless string comparison. */
    int x;              /* Returns 0 if equal, 1 if not. */
    char t1, t2;
    x = strlen(s1);
    if (x != strlen(s2)) return(1); /* Unequal lengths, so unequal. */
    while (x--) {
    t1 = *s1++;
    t2 = *s2++;
    if (isupper(t1)) t1 = tolower(t1);
    if (isupper(t2)) t2 = tolower(t2);
    if (t1 != t2) break;
    }
    return(!(x == -1));
}
