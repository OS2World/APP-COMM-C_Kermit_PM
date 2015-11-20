/* File ckuus5.c - Spillover, created to even out sizes of ckuus*.c modules */

/* Includes */

#include <stdio.h>
#include <ctype.h>
#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#include "ckucmd.h"
#include "ckuusr.h"

/* External variables */

extern int size, rpsiz, urpsiz, speed, local, backgrd, tvtflg,
  server, displa, binary, parity, deblog, escape, xargc, flow,
  turn, duplex, nfils, ckxech, pktlog, seslog, tralog, stdouf, stdinf,
  turnch, dfloc, keep, maxrps, warn, quiet, cnflg, tlevel,
  mdmtyp, zincnt, cmask, rcflag, success;
extern char *DIRCMD, *PWDCMD, *DELCMD, cmerrp[];
extern char *zhome(), *malloc();
extern char *versio, *ckxsys, *dftty, *cmarg, *homdir, *lp;
extern char debfil[], pktfil[], sesfil[], trafil[], cmdbuf[], cmdstr[];
extern char cmdbuf[], atmbuf[], savbuf[], toktab[];
extern CHAR sstate, ttname[], filnam[];
extern struct mtab mactab[];
extern struct keytab cmdtab[];
extern int cmdlvl, maclvl, tlevel, rmailf, rprintf;
extern int nmac, mecho, merror, techo, terror, repars, ncmd;
extern int xxstring();
#ifdef OS2
extern char far * zfindfile(char far *);
#endif

/* Variables declared here for use by other ckuus*.c modules */
/* Space is allocated here to save room in ckuusr.c */

FILE *tfile[MAXTAKE];
struct cmdptr cmdstk[CMDSTKL];
int ifcmd[CMDSTKL], count[CMDSTKL], iftest[CMDSTKL];
char *m_arg[MACLEVEL][NARGS];
char *g_var[GVARS], *macp[MACLEVEL];
int macargc[MACLEVEL];
int macx[MACLEVEL];

char line[LINBUFSIZ];			/* Character buffer for anything */
char inpbuf[INPBUFSIZ];			/* Buffer for INPUT and REINPUT */
char vnambuf[VNAML];			/* Buffer for variable names */
char lblbuf[50];			/* Buffer for labels */
char debfil[50];			/* Debugging log file name */
char pktfil[50];			/* Packet log file name */
char sesfil[50];			/* Session log file name */
char trafil[50];			/* Transaction log file name */
char tmpbuf[50];			/* Temporary buffer */
char optbuf[50];			/* Options for MAIL or REMOTE PRINT */
char kermrc[100];			/* Name of initialization file */
char cmdstr[256];			/* Place to build generic command */

/*  T R A P  --  Terminal interrupt handler */
 
trap(sig,code) int sig, code; {
    printf("^C...\n");
    debug(F101,"trap() caught signal","",sig);
    debug(F101," code","",code);
    doexit(GOOD_EXIT);			/* Exit indicating success */
}

/*  S T P T R A P -- Handle SIGTSTP signals */

#ifdef RTU
    extern int rtu_bug;
#endif

stptrap(sig,code) int sig, code; {
    debug(F101,"stptrap() caught signal","",sig);
    debug(F101," code","",code);
    conres();				/* Reset the console */
#ifdef SIGTSTP
#ifdef RTU
    rtu_bug = 1;
#endif
    kill(0, SIGSTOP);			/* If job control, suspend the job */
#else
    doexit(GOOD_EXIT);			/* Probably won't happen otherwise */
#endif
    concb(escape);			/* Put console back in Kermit mode */
    if (!backgrd) prompt();		/* Reissue prompt when fg'd */
}

/*  C M D I N I  --  Initialize the interactive command parser  */
 
cmdini() {

#ifdef AMIGA
    congm();
    concb(escape);
#endif
    cmdlvl = 0;				/* Start at command level 0 */
    cmdstk[cmdlvl].src = CMD_KB;
    cmdstk[cmdlvl].val = 0;
    tlevel = -1;			/* Take file level = keyboard */
    cmsetp("C-Kermit>");		/* Set default prompt */
    initmac();				/* Initialize macro table */
    addmac("ibm-linemode",
	   "set parity mark, set dupl half, set handsh xon, set flow none");

/* Look for init file in home or current directory. */
/* (NOTE - should really use zkermini for this!)    */
#ifdef OS2
    if (rcflag)
      lp = kermrc;
    else
      lp = zfindfile(kermrc);
    strcpy(line,lp);
    if ((tfile[0] = fopen(line,"r")) != NULL) {
        tlevel = 0;
	cmdlvl++;
	cmdstk[cmdlvl].src = CMD_TF;
	cmdstk[cmdlvl].val = tlevel;
	ifcmd[cmdlvl] = 0;
	iftest[cmdlvl] = 0;
	count[cmdlvl] = 0;
        debug(F110,"init file",line,0);
    } else {
        debug(F100,"no init file","",0);
    }
#else 
    homdir = zhome();
    lp = line;
    lp[0] = '\0';
#ifdef vms
    zkermini(line,sizeof(line),kermrc);
#else
    if (rcflag) {			/* If init file name from cmd line */
	strcpy(lp,kermrc);		/* use it */
    } else {				/* otherwise */
	if (homdir) {			/* look in home directory for it */
	    strcpy(lp,homdir);
	    if (lp[0] == '/') strcat(lp,"/");
	}
	strcat(lp,kermrc);		/* and use the default name */
    }
#endif
#ifdef AMIGA
    reqoff();				/* disable requestors */
#endif
    debug(F110,"ini file is",line,0);
    if ((tfile[0] = fopen(line,"r")) != NULL) {
	tlevel = 0;
	cmdlvl++;
	ifcmd[cmdlvl] = 0;
	iftest[cmdlvl] = 0;
	count[cmdlvl] = 0;
	debug(F101,"open ok","",cmdlvl);
	cmdstk[cmdlvl].src = CMD_TF;
	cmdstk[cmdlvl].val = tlevel;
	debug(F110,"init file",line,0);
    }
    if (homdir && (tlevel < 0)) {
    	strcpy(lp,kermrc);
	if ((tfile[0] = fopen(line,"r")) != NULL) {
	    tlevel = 0;
	    cmdlvl++;
	    cmdstk[cmdlvl].src = CMD_TF;
	    cmdstk[cmdlvl].val = tlevel;
	    ifcmd[cmdlvl] = 0;
	    iftest[cmdlvl] = 0;
	    count[cmdlvl] = 0;
	}
    }
#endif
#ifdef AMIGA
    reqpop();				/* restore requestors */
#else
    congm();				/* Get console tty modes */
#endif
}

/*  P A R S E R  --  Top-level interactive command parser.  */
 
parser() {
    int y, xx, yy, zz, cbn;
    int icn;
    char ic; 
    char *cbp;

#ifdef AMIGA
    reqres();			/* restore AmigaDOS requestors */
#endif
    concb(escape);		/* Put console in cbreak mode. */
    conint(trap);		/* Turn on console terminal interrupts. */
    ifcmd[0] = 0;		/* Command-level related variables */
    iftest[0] = 0;		/* initialize variables at top level */
    count[0] = 0;		/* of stack... */
/*
 sstate becomes nonzero when a command has been parsed that requires some
 action from the protocol module.  Any non-protocol actions, such as local
 directory listing or terminal emulation, are invoked directly from below.
*/
    if (local && !backgrd) printf("\n"); /* Just returned from connect? */
    sstate = 0;				/* Start with no start state. */
    rmailf = rprintf = 0;		/* MAIL and PRINT modifiers for SEND */
    *optbuf = NUL;			/* MAIL and PRINT options */
    while (sstate == 0) {		/* Parse cmds until action requested */
	while ((cmdstk[cmdlvl].src == CMD_TF)  /* If end of take file */
	       && (tlevel > -1)
	       && feof(tfile[tlevel])) {
	    popclvl();			/* pop command level */
	    cmini(ckxech);		/* and clear the cmd buffer. */
	    if (cmdlvl == 0) {		/* Just popped out of all cmd files? */
		conint(trap);		/* Check background stuff again. */
		return(0);		/* End of init file or whatever. */
	    }
 	}
        if (cmdstk[cmdlvl].src == CMD_MD) { /* Executing a macro? */
	    if (icn = conchk()) {	/* Yes */
		while (icn--) {		/* User typed something */
		    ic = coninc(0);
		    if (ic == CR) {	/* Carriage return? */
			printf(" Interrupted...\n");
			dostop();
		    } else {
			putchar(BEL);
		    }
		}		
	    }
	    maclvl = cmdstk[cmdlvl].val; /* No interrupt, get current level */
	    cbp = cmdbuf;		/* Copy next cmd to macro buffer. */
	    *cbp = NUL;
	    if (*savbuf) {		/* In case then-part of 'if' command */
		strcpy(cbp,savbuf);	/* was saved, restore it. */
		*savbuf = '\0';
	    } else {			/* Else get next cmd from macro def */
		debug(F111,"macro",macp,maclvl);
		for (y = 0;		/* copy next macro part */
		     *macp[maclvl] && y < CMDBL;
		     y++,cbp++,macp[maclvl]++) {
		    *cbp = *macp[maclvl];
		    if (*cbp == ',') {	/* up to a comma */
			macp[maclvl]++;
			break;
		    }
		}			/* or up to end, whichever is first */
		if (*cmdbuf == NUL) {	/* If nothing was copied */
		    popclvl();		/* pop command level. */
		    debug(F101,"macro level popped","",maclvl);
		    continue;
		} else {		/* otherwise, tack CR onto end */
		    *cbp++ = '\r';
		    *cbp = '\0';
		    if (mecho) printf("%s\n",cmdbuf);
		    debug(F110,"cmdbuf",cmdbuf,0);
		}
	    }
	} else if (cmdstk[cmdlvl].src == CMD_TF) { /* Reading TAKE file? */
	    debug(F101,"tlevel","",tlevel);
	    cbp = cmdbuf;		/* Get the next line. */
	    cbn = CMDBL;
 
/* Loop to get next command line and all continuation lines from take file. */
 
again:	    if (*savbuf) {		/* In case then-part of 'if' command */
		strcpy(line,savbuf);	/* was saved, restore it. */
		*savbuf = '\0';
	    } else {
		if (fgets(line,cbn,tfile[tlevel]) == NULL) continue;
	    }
	    if (icn = conchk()) {
		while (icn--) {		/* User typed something... */
		    ic = coninc(0);	/* Look for carriage return */
		    if (ic == CR) {
			printf(" Interrupted...\n");
			dostop();
		    } else putchar(BEL); /* Ignore anything else */
		}		
	    }
	    lp = line;			/* Got a line, copy it. */
	    debug(F110,"from TAKE file",lp,0);
	    while (*cbp++ = *lp++)
	      if (--cbn < 1) fatal("Command too long for internal buffer");
	    if (*(cbp - 3) == CMDQ || *(cbp - 3) == '-') { /* Continued? */
		cbp -= 3;		/* If so, back up pointer, */
		goto again;		/* go back, get next line. */
	    }
/***	    stripq(cmdbuf);		/* Strip any quotes from cmd buffer. */
	    untab(cmdbuf);		/* Convert tabs to spaces */
	    if (techo) printf("%s",cmdbuf); /* Echo if "take echo on" */
	} else {			/* No take file, get typein. */
	    if (!backgrd) prompt();	/* Issue interactive prompt. */
	    cmini(ckxech);
    	}
	repars = 1;			/* Now we know where command is */
	displa = 0;			/* coming from, so read it. */
	while (repars) {
	    cmres();			/* Reset buffer pointers. */
	    xx = cmkey2(cmdtab,ncmd,"Command","",toktab,xxstring);
	    debug(F101,"top-level cmkey2","",xx);
	    if (xx == -5) {
		yy = chktok(toktab);
		debug(F101,"top-level cmkey token","",yy);
		ungword();
		switch (yy) {
		  case '!': xx = XXSHE; break;
		  case '#': xx = XXCOM; break;
		  case ';': xx = XXCOM; break;
		  case ':': xx = XXLBL; break;
		  case '&': xx = XXECH; break;
		  default:
		    printf("\nInvalid - %s\n",cmdbuf);
		    xx = -2;
		}
	    }
	    if (ifcmd[cmdlvl])		/* Count stmts after IF */
	      ifcmd[cmdlvl]++;
	    if (ifcmd[cmdlvl] > 2 && xx != XXELS && xx != XXCOM)
	      ifcmd[cmdlvl] = 0;
	    switch (zz = docmd(xx)) {
		case -4:		/* EOF */
		    doexit(GOOD_EXIT);	/* ...exit successfully */
	        case -1:		/* Reparse needed */
		    repars = 1;
		    continue;
		case -6:		/* Invalid command given w/no args */
	    	case -2:		/* Invalid command given w/args */
		    /* This is going to be really ugly... */
		    yy = mlook(mactab,atmbuf,nmac); /* Look in macro table */
		    if (yy > -1) {	            /* If it's there */
			if (zz == -2) {	            /* insert "do" */
			    char *mp;
			    mp = malloc(strlen(cmdbuf) + 5);
			    if (!mp) {
				printf("?malloc error 1\n");
				return(-2);
			    }
			    sprintf(mp,"do %s ",cmdbuf);
			    strcpy(cmdbuf,mp);
			    free(mp);
			} else sprintf(cmdbuf,"do %s \r",atmbuf);
			if (ifcmd[cmdlvl] == 2)	/* This one doesn't count! */
			  ifcmd[cmdlvl]--;
			debug(F111,"stuff cmdbuf",cmdbuf,zz);
			repars = 1;	/* go for reparse */
			continue;
		    } else printf("?Invalid - %s\n",cmdbuf);

		    success = 0;
		    debug(F110,"top-level cmkey failed",cmdbuf,0);
		    if (backgrd) 	/* if in background, terminate */
			fatal("Kermit command error in background execution");
		    if (cmdstk[cmdlvl].src == CMD_TF && terror) {
			ermsg("Kermit command error: take file terminated.");
			popclvl();
		    }
		    if (cmdstk[cmdlvl].src == CMD_MD && merror) {
			ermsg("Kermit command error: macro terminated.");
			popclvl();
		    }
		    cmini(ckxech);	/* (fall thru) */
 	    	case -3:		/* Empty command OK at top level */
		default:		/* Anything else (fall thru) */
		    repars = 0;		/* No reparse, get new command. */
		    continue;
            }
        }
    }

/* Got an action command; disable terminal interrupts and return start state */
 
    if (!local) connoi();		/* Interrupts off only if remote */
    return(sstate);
}

/* OUTPUT command */

xxout(c) char c; {			/* Function to output a character. */
    if (local)				/* If in local mode */
      return(ttoc(c));			/* then to the external line */
    else return(conoc(c));		/* otherwise to the console. */
}

/* Returns 0 on failure, 1 on success */

dooutput(s) char *s; {
    int x, y, quote;

    if (local && (tvtflg == 0)) {	/* If external line not conditioned */
	y = ttvt(speed,flow);		/* do it now. */
	if (y < 0) return(0);
	tvtflg = 1;
    }
    quote = 0;				/* Initialize backslash (\) quote */
    while (x = *s++) {			/* Loop through the string */
	y = 0;				/* Error code, 0 = no error. */
	if (x == CMDQ) {		/* Look for \b or \B in string */
            quote = 1;			/* Got \ */
	    continue;			/* Get next character */
	} else if (quote) {		/* This character is quoted */
	    quote = 0;			/* Turn off quote flag */
	    if (x == 'b' || x == 'B') {	/* If \b or \B */
		debug(F101,"output BREAK","",0);
		ttsndb();		/* send BREAK signal */
		continue;		/* and not the b or B */
	    } else {			/* if \ not followed by b or B */
		y = xxout(dopar(CMDQ));	/* output the backslash. */
	    }
	}
	y = xxout(dopar(x));	/* Output this character */
	if (y < 0) {
	    printf("output error.\n");
	    return(0);
	}
	if (seslog)
	  if (zchout(ZSFILE,x) < 0) seslog = 0;
    }
    return(1);
}

/* Display version herald and initial prompt */

herald() {
    if (!backgrd)
      printf("%s,%s\nType ? or 'help' for help\n",versio,ckxsys);
}

fatal(msg) char *msg; {			/* Fatal error message */
#ifdef OSK
    printf("\nFatal: %s\n",msg);
#else
    printf("\r\nFatal: %s\n",msg);
#endif /* OSK */
    tlog(F110,"Fatal:",msg,0l);
    doexit(BAD_EXIT);			/* Exit indicating failure */
}
 
 
ermsg(msg) char *msg; {			/* Print error message */
#ifdef OSK
    if (!quiet) printf("\n%s - %s\n",cmerrp,msg);
#else
    if (!quiet) printf("\r\n%s - %s\n",cmerrp,msg);
#endif /* OSK */
    tlog(F110,"Error -",msg,0l);
}

/*  D O E X I T  --  Exit from the program.  */
 
doexit(exitstat) int exitstat; {
    
    ttclos();				/* Close external line, if any */
    if (local) {
	strcpy(ttname,dftty);		/* Restore default tty */
	local = dfloc;			/* And default remote/local status */
    }
    if (!quiet) conres();		/* Restore console terminal. */
    if (!quiet) connoi();		/* Turn off console interrupt traps. */
 
    if (deblog) {			/* Close any open logs. */
	debug(F100,"Debug Log Closed","",0);
	*debfil = '\0';
	deblog = 0;
	zclose(ZDFILE);
    }
    if (pktlog) {
	*pktfil = '\0';
	pktlog = 0;
	zclose(ZPFILE);
    }
    if (seslog) {
    	*sesfil = '\0';
	seslog = 0;
	zclose(ZSFILE);
    }
    if (tralog) {
	tlog(F100,"Transaction Log Closed","",0l);
	*trafil = '\0';
	tralog = 0;
	zclose(ZTFILE);
    }
    syscleanup();
    exit(exitstat);				/* Exit from the program. */
}


/*  B L D L E N  --  Make length-encoded copy of string  */
 
char *
bldlen(str,dest) char *str, *dest; {
    int len;
    len = strlen(str);
    *dest = tochar(len);
    strcpy(dest+1,str);
    return(dest+len+1);
}
 
 
/*  S E T G E N  --  Construct a generic command  */
 
setgen(type,arg1,arg2,arg3) char type, *arg1, *arg2, *arg3; {
    char *upstr, *cp;
 
    cp = cmdstr;
    *cp++ = type;
    *cp = NUL;
    if (*arg1 != NUL) {
	upstr = bldlen(arg1,cp);
	if (*arg2 != NUL) {
	    upstr = bldlen(arg2,upstr);
	    if (*arg3 != NUL) bldlen(arg3,upstr);
	}
    }
    cmarg = cmdstr;
    debug(F110,"setgen",cmarg,0);
 
    return('g');
}

/*  M L O O K  --  Lookup the macro name in the macro table  */
 
/*
 Call this way:  v = mlook(table,word,n,&x);
 
   table - a 'struct mtab' table.
   word  - the target string to look up in the table.
   n     - the number of elements in the table.
   x     - address of an integer for returning the table array index.
 
 The keyword table must be arranged in ascending alphabetical order, and
 all letters must be lowercase.
 
 Returns the table index, 0 or greater, if the name was found, or:
 
  -3 if nothing to look up (target was null),
  -2 if ambiguous,
  -1 if not found.
 
 A match is successful if the target matches a keyword exactly, or if
 the target is a prefix of exactly one keyword.  It is ambiguous if the
 target matches two or more keywords from the table.
*/
mlook(table,cmd,n) char *cmd; struct mtab table[]; int n; {
 
    int i, v, cmdlen;
 
/* Lowercase & get length of target, if it's null return code -3. */
 
    if ((((cmdlen = lower(cmd))) == 0) || (n < 1)) return(-3);
 
/* Not null, look it up */
 
    for (i = 0; i < n-1; i++) {
        if (!strcmp(table[i].kwd,cmd) ||
           ((v = !strncmp(table[i].kwd,cmd,cmdlen)) &&
             strncmp(table[i+1].kwd,cmd,cmdlen))) {
                return(i);
             }
        if (v) return(-2);
    }   
 
/* Last (or only) element */
 
    if (!strncmp(table[n-1].kwd,cmd,cmdlen)) {
        return(n-1);
    } else return(-1);
}

/* mxlook is like mlook, but an exact full-length match is required */

mxlook(table,cmd,n) char *cmd; struct mtab table[]; int n; {
    int i, j, cmdlen;
    if ((((cmdlen = lower(cmd))) == 0) || (n < 1)) return(-3);
    for (i = 0; i < n; i++)
      if ((strlen(table[i].kwd) == cmdlen) &&
	  (!strncmp(table[i].kwd,cmd,cmdlen))) return(i);
    return(-1);
}

addmac(nam,def) char *nam, *def; {	/* Add a macro to the macro table */
    int i, y;
    char *p, c;

    debug(F110,"addmac",nam,0);
    debug(F110,"addmac",def,0);

    if (*nam == '%') {			/* If it's a variable name */
	delmac(nam);			/* Delete any old value */
	p = malloc(strlen(def) + 1);	/* Allocate space for its definition */
	if (!p) {
	    printf("?malloc error 2\n");
	    return(-1);
	}
	strcpy(p,def);			/* Copy definition into new space */
	c = *(nam + 1);			/* Variable name letter or digit */
	if (c >= '0' && c <= '9') {	/* Digit? */
	    if (maclvl < 0) {		/* Yes, calling or in a macro? */
		g_var[c] = p;		/* No, so make it global. */
		debug(F111,"addmac numeric glob",p,maclvl);
	    } else {
		m_arg[maclvl][c - '0'] = p; /* Put ptr in macro-arg table */
		debug(F111,"addmac arg",p,maclvl);
	    }
	} else {			/* It's a global variable */
	    if (c < 33 || c > GVARS) return(-1);
	    g_var[c] = p;		/* Put pointer in global-var table */
	    debug(F110,"addmac glob",p,0);
	}
	return(0);
    }
	    
/* Not a macro argument or a variable, so it's a macro definition */

    if (mxlook(mactab,nam,nmac) >= 0)	/* Look it up */
      delmac(nam);			/* if it's already there, delete it. */

    lower(nam);
    p = malloc(strlen(nam) + 1);	/* Allocate space for name */
    if (!p) fatal("?malloc error 3");
    for (y = 0;				/* Find the slot */
	 y < MAC_MAX && mactab[y].kwd != NULL && strcmp(nam,mactab[y].kwd) > 0;
	 y++) ;
    if (y == MAC_MAX)			/* No more room. */
      return(-1);
    if (mactab[y].kwd != NULL) {	/* Must insert */
	for (i = nmac; i > y; i--) {	/* Move the rest down one slot */
	    mactab[i].kwd = mactab[i-1].kwd;
	    mactab[i].val = mactab[i-1].val;
	    mactab[i].flgs = mactab[i-1].flgs;
	}
    }
    p = malloc(strlen(nam) + 1);	/* Allocate space for name */
    if (!p) {
	printf("?malloc error 4\n");
	return(-1);
    }
    strcpy(p,nam);			/* Copy name into new space */
    mactab[y].kwd = p;			/* Add pointer to table */

    p = malloc(strlen(def) + 1);	/* Same deal for definition */
    if (p == NULL) {
	printf("?malloc error 5\n");
	free(mactab[y].kwd);
	mactab[y].kwd = NULL;
	return(-1);
    }
    strcpy(p,def);
    mactab[y].val = p;
    mactab[y].flgs = 0;
    nmac++;				/* Count this macro */
    return(y);
}

delmac(nam) char *nam; {		/* Delete the named macro */
    int i, x;
    char *p, c;

    if (*nam == '%') {			/* If it's a variable name */
	c = *(nam + 1);			/* Variable name letter or digit */
	if (maclvl > -1 && c >= '0' && c <= '9') { /* Digit? */
	    p = m_arg[maclvl][c - '0'];	/* Get pointer from macro-arg table */
	    m_arg[maclvl][c - '0'] = NULL; /* Zero the table entry */
	} else {			/* It's a global variable */
	    if (c < 33 || c > GVARS) return(0);
	    p = g_var[c];		/* Get pointer from global-var table */
	    g_var[c] = NULL;		/* Zero the table entry */
	}
	if (p) free(p);			/* Free the storage */
	return(0);
    }
	    
    if ((x = mlook(mactab,nam,nmac)) < 0) return(x); /* Look it up */

    for (i = x; i < nmac; i++) {
	mactab[i].kwd = mactab[i+1].kwd;
	mactab[i].val = mactab[i+1].val;
	mactab[i].flgs = mactab[i+1].flgs;
    }
    nmac--;				/* One less macro */
    if (mactab[nmac].kwd)		/* Free the storage for the name */
      free(mactab[nmac].kwd);
    mactab[nmac].kwd = NULL;		/* Delete it from the table */
    if (mactab[nmac].val)		/* Ditto for the definition */
      free(mactab[nmac].val);
    mactab[nmac].val = NULL;
    mactab[nmac].flgs = 0;
    return(0);
}

initmac() {				/* Init macro & variable tables */
    int i, j;

    nmac = 0;				/* No macros */
    for (i = 0; i < MAC_MAX; i++) {	/* Initialize the macro table */
	mactab[i].kwd = NULL;
	mactab[i].val = NULL;
	mactab[i].flgs = 0;
    }
    for (i = 0; i < MACLEVEL; i++) {	/* Init the macro argument tables */
	for (j = 0; j < 10; j++) {
	    m_arg[i][j] = NULL;
	}
    }
    for (i = 0; i < GVARS; i++) {	/* And the global variables table */
	g_var[i] = NULL;
    }
}

/* Look up a % variable, return pointer to its value, else NULL */

char *
varlook(vnp) char *vnp; {
    char c;
    char *p;
    c = *(vnp + 1);			/* Fold case */
    if (isupper(c)) *(vnp + 1) = tolower(c);
    c = *(vnp + 1);			/* Variable name letter or digit */
    if (maclvl > -1 && c >= '0' && c <= '9') { /* Digit? */
	p = m_arg[maclvl][c - '0'];	/* Get pointer from macro-arg table */
    } else {				/* It's a global variable */
	if (c > 31 && c <= GVARS)
	  p = g_var[c];			/* Get pointer from global-var table */
    }
    return(p);
}

popclvl() {				/* Pop command level, return cmdlvl */
    if (cmdlvl < 1) {			/* If we're already at top level */
	cmdlvl = 0;			/* just make sure all the */
	tlevel = -1;			/* stack pointers are set right */
	maclvl = -1;			/* and return */
	return(0);
    } else if (cmdstk[cmdlvl].src == CMD_TF) { /* Reading from TAKE file? */
	if (tlevel > -1) {		/* Yes, */
	    fclose(tfile[tlevel--]);	/* close it and pop take level */
	    cmdlvl--;			/* pop command level */
	} else tlevel = -1;
    } else if (cmdstk[cmdlvl].src == CMD_MD) { /* In a macro? */
	if (maclvl > -1) {		/* Yes, */
	    macp[maclvl] = "";		/* set macro pointer to null string */
	    *cmdbuf = '\0';		/* clear the command buffer */
	    maclvl--;			/* pop macro level */
	    cmdlvl--;			/* and command level */
	} else maclvl = -1;
    }
    return(cmdlvl < 1 ? 0 : cmdlvl);	/* Return command level */
}

/* STOP - get back to C-Kermit prompt, no matter where from. */

dostop() {
    while (popclvl()) ;			/* Pop all command levels */
    cmini(ckxech);			/* Clear the command buffer. */
    conint(trap);			/* Check background stuff again. */
    return(0);
}

/* Close the given log */

doclslog(x) int x; {
    switch (x) {
	case LOGD:
	    if (deblog == 0) {
		printf("?Debugging log wasn't open\n");
		return(0);
	    }
	    *debfil = '\0';
	    deblog = 0;
	    return(zclose(ZDFILE));
 
	case LOGP:
	    if (pktlog == 0) {
		printf("?Packet log wasn't open\n");
		return(0);
	    }
	    *pktfil = '\0';
	    pktlog = 0;
	    return(zclose(ZPFILE));
 
	case LOGS:
	    if (seslog == 0) {
		printf("?Session log wasn't open\n");
		return(0);
	    }
	    *sesfil = '\0';
	    seslog = 0;
	    return(zclose(ZSFILE));
 
    	case LOGT:
	    if (tralog == 0) {
		printf("?Transaction log wasn't open\n");
		return(0);
	    }
	    *trafil = '\0';
	    tralog = 0;
	    return(zclose(ZTFILE));
 
	default:
	    printf("\n?Unexpected log designator - %ld\n", x);
	    return(0);
	}
}

static char *nm[] = { "disabled", "enabled" };

doshow(x) int x; {
    int y;
    char *s;
    extern int en_cwd, en_del, en_dir, en_fin,
      en_get, en_hos, en_sen, en_set, en_spa, en_typ, en_who;
    extern int vernum, srvtim, incase, inecho, intime;
    extern char *protv, *fnsv, *cmdv, *userv, *ckxv, *ckzv, *ckzsys;
    extern char *connv, *dialv, *loginv;

    switch (x) {
	case SHPAR:
	    if ((y = cmcfm()) < 0) return(y);
	    shopar();
	    break;
 
	case SHCOU:
	    if ((y = cmcfm()) < 0) return(y);
	    printf(" %d\n",count[cmdlvl]);
	    break;

        case SHSER:			/* Show Server */
	    if ((y = cmcfm()) < 0) return(y);
	    printf("Function           Status:\n");
	    printf(" GET                %s\n",nm[en_get]);
	    printf(" SEND               %s\n",nm[en_sen]);	    
	    printf(" REMOTE CD/CWD      %s\n",nm[en_cwd]);
	    printf(" REMOTE DIRECTORY   %s\n",nm[en_dir]);
	    printf(" REMOTE HOST        %s\n",nm[en_hos]);	    
	    printf(" REMOTE SET         %s\n",nm[en_set]);	    
	    printf(" REMOTE SPACE       %s\n",nm[en_spa]);	    
	    printf(" REMOTE TYPE        %s\n",nm[en_typ]);	    
	    printf(" REMOTE WHO         %s\n",nm[en_who]);	    
	    printf("Server Timeout: %d\n\n",srvtim);
	    break;

        case SHSTA:			/* Status of last command */
	    if ((y = cmcfm()) < 0) return(y);
	    if (success) printf(" SUCCESS\n"); else printf(" FAILURE\n");
	    break;

	case SHVER:
	    if ((y = cmcfm()) < 0) return(y);
	    printf("\nVersions:\n %s\n Numeric: %d\n",versio,vernum);
	    printf(" %s\n",protv);
	    printf(" %s\n",fnsv);
	    printf(" %s\n %s\n",cmdv,userv);
	    printf(" %s for%s\n",ckxv,ckxsys);
	    printf(" %s for%s\n",ckzv,ckzsys);
	    printf(" %s\n",connv);
	    printf(" %s\n %s\n\n",dialv,loginv);
	    break;
 
	case SHMAC:			/* Macro definitions */
	    x = cmfld("Macro name, variable name, or carriage return","",&s,
		      NULL);
	    if (x == -3)
	      *line = '\0';
	    else if (x < 0)
	      return(x);
	    else
	      strcpy(line,s);
	    if ((y = cmcfm()) < 0) return(y);	    
	    if (*line != '\0' && *line != CMDQ && *line != '%') {
		x = mlook(mactab,s,nmac);
		switch (x) {
		  case -3:
		    return(0);
		  case -1:
		    printf("%s - not found\n",s);
		    return(0);
		  case -2:
		    y = strlen(line);
		    for (x = 0; x < nmac; x++)
		    if (!strncmp(mactab[x].kwd,line,y))
		      printf(" %s = %s\n",mactab[x].kwd,mactab[x].val);
		    return(0);
		  default:
		    printf(" %s = %s\n",mactab[x].kwd,mactab[x].val);
		    return(0);
		}
	    }
	    printf("Macros:\n");
	    for (y = 0; y < nmac; y++) {
		printf(" %s = %s\n",mactab[y].kwd,mactab[y].val);
	    }
	    x = 0;
	    for (y = 33; y < GVARS; y++)
	      if (g_var[y]) {
		  if (x == 0) printf("Global variables:\n");
		  x = 1;
		  printf(" %%%c = %s\n",y,g_var[y]);
	      }
	    if (maclvl > -1) {
		printf("Macro arguments at level %d\n",maclvl);
		for (y = 0; y < 10; y++)
		  if (m_arg[maclvl][y])
		    printf("%%%d = %s\n",y,m_arg[maclvl][y]);
	    }
	    break;

	case SHPRO:			/* Protocol parameters */
	    if ((y = cmcfm()) < 0) return(y);
	    shoparp();
	    printf("\n");
	    break;

	case SHCOM:			/* Communication parameters */
	    if ((y = cmcfm()) < 0) return(y);
	    printf("\n");
	    shoparc();
	    printf("\n");
	    break;

	case SHFIL:			/* File parameters */
	    if ((y = cmcfm()) < 0) return(y);
	    shoparf();
	    printf("\n");
	    break;

	case SHKEY: {			/* Key */
	    CHAR c;
	    if ((y = cmcfm()) < 0) return(y);	    
	    printf(" Press key: ");
	    c = coninc(0);		/* For some reason this doesn't */
	    printf("\\%d\n",c);		/* return the 8th bit... */
	    break;
	    }

	case SHLNG:			/* Languages */
	    if ((y = cmcfm()) < 0) return(y);
	    shoparl();
	    break;

	case SHSCR:			/* Scripts */
	    if ((y = cmcfm()) < 0) return(y);	    
	    printf(" Take  Echo:     %s\n", techo  ? "On" : "Off");
	    printf(" Take  Error:    %s\n", terror ? "On" : "Off");
	    printf(" Macro Echo:     %s\n", mecho  ? "On" : "Off");
	    printf(" Macro Error:    %s\n", merror ? "On" : "Off");
	    printf(" Input Case:     %s\n", incase ? "Observe" : "Ignore");
	    printf(" Input Echo:     %s\n", inecho ? "On" : "Off");
	    printf(" Input Timeout:  %s\n", intime ? "Quit" : "Proceed");
	    break;

	  case SHSPD:
	    if ((y = cmcfm()) < 0) return(y);
	    y = ttgspd();
	    if (y > -1)
	      printf("%s, estimated speed: %d\n",ttname,y);
	    else
	      printf("%s, speed unknown\n",ttname);
	    break;

	default:
	    printf("\nNothing to show...\n");
	    return(-2);
    }
    return(success = 1);
}
