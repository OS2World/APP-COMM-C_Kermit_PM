char *userv = "User Interface 5A(067), 1 Mar 90";
 
/*  C K U U S R --  "User Interface" for Unix Kermit (Part 1)  */

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
  NOTE: Because of the massive additions in functionality, and therefore
  the increase in the number of commands, much code was moved from here to
  the two new modules, ckuus4.c and ckuus5.c.  This module now contains only
  the top-level command keyword table, the SET command keyword table, and
  the top-level interactive command parser/dispatcher.  ckuus3.c contains the
  rest of the SET and REMOTE command parsers; ckuus2.c contains the help
  command parser and help text strings, and ckuus4.c and ckuus5.c contain
  miscellaneous pieces that logically belong in the ckuusr.c file but had to
  be moved because of size problems with some C compilers / linkers.
*/

/*
 The ckuus*.c modules depend on the existence of C library features like fopen,
 fgets, feof, (f)printf, argv/argc, etc.  Other functions that are likely to
 vary among Unix implementations -- like setting terminal modes or interrupts
 -- are invoked via calls to functions that are defined in the system-
 dependent modules, ck?[ft]io.c.  The command line parser processes any
 arguments found on the command line, as passed to main() via argv/argc.  The
 interactive parser uses the facilities of the cmd package (developed for this
 program, but usable by any program).  Any command parser may be substituted
 for this one.  The only requirements for the Kermit command parser are these:

1. Set parameters via global variables like duplex, speed, ttname, etc.  See
   ckmain.c for the declarations and descriptions of these variables.

2. If a command can be executed without the use of Kermit protocol, then
   execute the command directly and set the variable sstate to 0. Examples
   include 'set' commands, local directory listings, the 'connect' command.

3. If a command requires the Kermit protocol, set the following variables:

    sstate                             string data
      'x' (enter server mode)            (none)
      'r' (send a 'get' command)         cmarg, cmarg2
      'v' (enter receive mode)           cmarg2
      'g' (send a generic command)       cmarg
      's' (send files)                   nfils, cmarg & cmarg2 OR cmlist
      'c' (send a remote host command)   cmarg

    cmlist is an array of pointers to strings.
    cmarg, cmarg2 are pointers to strings.
    nfils is an integer.

    cmarg can be a filename string (possibly wild), or
       a pointer to a prefabricated generic command string, or
       a pointer to a host command string.
    cmarg2 is the name to send a single file under, or
       the name under which to store an incoming file; must not be wild.
       If it's the name for receiving, a null value means to store the
       file under the name it arrives with.
    cmlist is a list of nonwild filenames, such as passed via argv.
    nfils is an integer, interpreted as follows:
      -1: filespec (possibly wild) in cmarg, must be expanded internally.
       0: send from stdin (standard input).
      >0: number of files to send, from cmlist.

 The screen() function is used to update the screen during file transfer.
 The tlog() function writes to a transaction log.
 The debug() function writes to a debugging log.
 The intmsg() and chkint() functions provide the user i/o for interrupting
   file transfers.
*/

/* Includes */
 
#include <stdio.h>
#include <ctype.h>
#ifndef AMIGA
/* Apparently these should be included for OS/2 C-Kermit after all... */
/* #ifndef OS2 */
#include <signal.h>
#include <setjmp.h>
/* #endif */
#endif

#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#include "ckucmd.h"
#include "ckuusr.h"
#include "ckcxla.h"
 
#ifdef datageneral
#define fgets(stringbuf,max,fd) dg_fgets(stringbuf,max,fd)
#define fork() vfork()
/* DG version 3.21 of C has bugs in the following routines, since they
 * depend on /etc/passwd.  In the context where the routines are used,
 * we don't need them anyway.
 */
#define getgid() -1
#define getuid() -1
#define geteuid() -1
#endif

/* External Kermit Variables, see ckmain.c for description. */
 
extern int size, rpsiz, urpsiz, speed, local, rmailf,
  server, displa, binary, parity, deblog, escape, xargc, flow,
  turn, duplex, nfils, ckxech, pktlog, seslog, tralog, stdouf,
  turnch, dfloc, keep, maxrps, warn, quiet, cnflg, tlevel,
  mdmtyp, zincnt, cmaskm;
 
extern int xxstring();			/* Variable expander */

extern long vernum;
extern char *versio, *protv, *ckxv, *ckzv, *fnsv, *connv, *dftty, *cmdv;
extern char *dialv, *loginv;
extern char *ckxsys, *ckzsys, *cmarg, *cmarg2, **xargv, **cmlist;
extern char *DIRCMD, *PWDCMD, *DELCMD, cmerrp[], optbuf[];
extern CHAR sstate, ttname[], filnam[];
extern CHAR *zinptr;

extern int tcharset, fcharset;
extern struct csinfo fcsinfo[], tcsinfo[];
char *strcpy(), *getenv(), *varlook();
#ifdef AMIGA
char *getcwd();
#endif
#ifdef OS2
char *getcwd();
#endif
 
/* Declarations from cmd package */
 
extern char cmdbuf[], atmbuf[], savbuf[]; /* Command buffers */
 
/* Declarations from ck?fio.c module */
 
extern char *SPACMD, *zhome();		/* Space command, home directory. */
extern int backgrd;			/* Kermit executing in background */
#ifdef OS2
extern char *zfindfile();
#endif
 
/* The background flag is set by ckutio.c (via conint() ) to note whether */
/* this kermit is executing in background ('&' on shell command line).    */
 
 
/* Variables and symbols local to this module */
 
extern char line[];			/* Character buffer for anything */
char *lp;				/* Pointer to line buffer */
extern char inpbuf[];			/* Buffer for INPUT and REINPUT */
char *inpbp = inpbuf;			/* And pointer to same */
extern char vnambuf[];			/* Buffer for variable names */
char *vnp;				/* Pointer to same */
extern char lblbuf[];			/* Buffer for labels */
extern char debfil[];			/* Debugging log file name */
extern char pktfil[];			/* Packet log file name */
extern char sesfil[];			/* Session log file name */
extern char trafil[];			/* Transaction log file name */
extern char tmpbuf[];			/* Temporary buffer */
char *tp;				/* Temporary pointer */
 
int n,					/* General purpose int */
    cflg,				/* Command-line connect cmd given */
    action,				/* Action selected on command line*/
    repars,				/* Reparse needed */
    ifc,				/* IF case */
    not = 0,				/* Flag for IF NOT */
    techo = 0,				/* Take echo */
    terror = 1,				/* Take error action, 1 = quit */
    tvtflg = 0,				/* tty device put in tvt mode */
    cwdf = 0;				/* CWD has been done */

extern int en_cwd, en_del, en_dir, en_fin, /* Flags for ENABLE/DISABLE */
   en_get, en_hos, en_sen, en_set, en_spa, en_typ, en_who;

int					/* SET INPUT parameters. */
  indef = 5,				/* 5 seconds default timeout */
  intime = 0,				/* 0 = proceed */
  incase = 0,				/* 0 = ignore */
  inecho = 1;				/* 1 = echo on */
 
int cmdsrc = 0;				/* Where commands are coming from: */
					/*  0 = kb, 1 = take, 2 = macro */
int maclvl = -1;			/* Macro to execute */
extern int macx[];			/* Index of current macro */
int mecho = 0;				/* Macro echo */
int merror = 0;				/* Macro error action */
extern char *macp[];			/* Pointer to macro */
char varnam[6];				/* For variable names */
extern int macargc[];			/* ARGC from macro invocation */
int success;				/* "Input" success flag */

extern FILE *tfile[];			/* File pointers for TAKE command */

extern char *m_arg[MACLEVEL][NARGS];	/* Stack of macro arguments */
extern char *g_var[];			/* Global variables %a, %b, etc */
 
extern struct cmdptr cmdstk[];		/* The command stack itself */
int cmdlvl = 0;				/* Current position in command stack */
int count[];				/* For IF COUNT, one for each cmdlvl */
int ifcmd[];				/* Last command was IF */
int iftest[];				/* Last IF was true */

char *homdir;				/* Pointer to home directory string */
extern char kermrc[];			/* Name of initialization file */
int rcflag = 0;				/* Flag for alternate init file name */
extern char cmdstr[];			/* Place to build generic command */

/* Top-Level Interactive Command Keyword Table */
 
struct keytab cmdtab[] = {
    "!",	   XXSHE, 0,		/* shell escape */
    "#",    	   XXCOM, CM_INV,	/* comment */
    ":",           XXLBL, CM_INV,	/* label */
    "ask",         XXASK, 0,		/* ask */
    "askq",        XXASKQ,0,            /* ask quietly */
    "assign",      XXASS, 0,            /* assign */
    "bye",         XXBYE, 0,		/* bye to remote server */
    "c",           XXCON, CM_INV,	/* invisible synonym for connect */
    "cd",          XXCWD, 0,		/* change directory */
    "clear",       XXCLE, 0,		/* clear input buffer */
    "close",	   XXCLO, 0,		/* close a log file */
    "comment",     XXCOM, 0,		/* comment */
    "connect",     XXCON, 0,		/* connect to remote system */
    "cwd",	   XXCWD, CM_INV,	/* invisisble synonym for cd */
    "decrement",   XXDEC, 0,		/* decrement a numeric variable */
    "define",      XXDEF, 0,		/* define a macro */
    "delete",      XXDEL, 0,		/* delete a file */
    "dial",	   XXDIAL,0,		/* dial a phone number */
    "directory",   XXDIR, 0,		/* directory of files */
    "disable",     XXDIS, 0,		/* disable server function */
    "do",          XXDO,  0,		/* execute a macro */
    "echo",        XXECH, 0,		/* echo argument */
    "else",        XXELS, CM_INV,	/* ELSE part of IF statement */
    "enable",      XXENA, 0,		/* enable a server function */
    "end",         XXEND, 0,		/* end a take file or macro */
    "exit",	   XXEXI, 0,		/* exit the program */
    "finish",	   XXFIN, 0,		/* shut down a remote server */
    "get",	   XXGET, 0,		/* get a file from a server */
    "goto",        XXGOTO,0,		/* goto label in take file or macro */
    "hangup",      XXHAN, 0,		/* hangup dialed phone connection */
    "help",	   XXHLP, 0,		/* display help text */
    "if",          XXIF,  0,		/* if (condition) command */
    "increment",   XXINC, 0,		/* increment a numeric variable */
    "input",       XXINP, 0,		/* input string from comm line */
    "l",           XXLOG, CM_INV,	/* invisible synonym for log */
    "log",  	   XXLOG, 0,		/* open a log file */
    "ls",          XXDIR, CM_INV,	/* invisible synonym for directory */
    "mail",        XXMAI, 0,		/* mail file to user */
    "output",      XXOUT, 0,		/* output string to comm line */
    "pause",       XXPAU, 0,		/* sleep for specified interval */
    "pop",         XXEND, CM_INV,	/* allow POP as synonym for END */
    "push",        XXSHE, 0,		/* PUSH command (like RUN, !) */
    "pwd",         XXPWD, 0,            /* print working directory */
    "quit",	   XXQUI, 0,		/* quit from program = exit */
    "r",           XXREC, CM_INV,	/* invisible synonym for receive */
    "reinput",     XXREI, 0,            /* reinput */
    "receive",	   XXREC, 0,		/* receive files */
    "remote",	   XXREM, 0,		/* send generic command to server */
    "replay",      XXTYP, 0,		/* replay (for now, just type) */
    "rm",          XXDEL, CM_INV,	/* invisible synonym for delete */
    "run",         XXSHE, 0,		/* run a program or command */
    "s",           XXSEN, CM_INV,	/* invisible synonym for send */
    "script",	   XXLOGI,0,		/* execute a uucp-style script */
    "send",	   XXSEN, 0,		/* send files */
    "server",	   XXSER, 0,		/* be a server */
    "set",	   XXSET, 0,		/* set parameters */
    "show", 	   XXSHO, 0,		/* show parameters */
    "space",       XXSPA, 0,		/* show available disk space */
    "statistics",  XXSTA, 0,		/* display file transfer stats */
    "stop",        XXSTO, 0,		/* stop all take files */
    "take",	   XXTAK, 0,		/* take commands from file */
    "test",        XXTES, CM_INV,	/* (for testing) */
    "translate",   XXXLA, 0,		/* translate local file char sets */
    "transmit",    XXTRA, 0,		/* raw upload file */
    "type",        XXTYP, 0,		/* display a local file */
    "version",     XXVER, 0,		/* version number display */
    "wait",        XXWAI, CM_INV	/* wait (like pause) */
};
int ncmd = (sizeof(cmdtab) / sizeof(struct keytab));

char toktab[] = {
    '!',				/* Shell escape */
    '#',				/* Comment */
    ';',				/* Comment */
    '&',				/* Echo */
    ':',				/* Label */
    '\0'				/* End of this string */
};

/* Parameter keyword table */
 
struct keytab prmtab[] = {
    "attributes",       XYATTR,  0,
    "baud",	        XYSPEE,  CM_INV,
    "block-check",  	XYCHKT,  0,
    "count",            XYCOUN,  0,
    "debug",            XYDEBU,  CM_INV,
    "delay",	    	XYDELA,  0,
    "duplex",	    	XYDUPL,  0,
    "end-of-packet",    XYEOL,   CM_INV,    /* moved to send/receive */
    "escape-character", XYESC,   0,
    "file", 	  	XYFILE,  0,
    "flow-control", 	XYFLOW,  0,
    "handshake",    	XYHAND,  0,
#ifdef NETCONN
    "host",             XYNETW,  0,
#endif /* NETCONN */
    "incomplete",   	XYIFD,   0,
    "input",            XYINPU,  0,
    "l",                XYLINE,  CM_INV,
    "language",         XYLANG,  0,
    "line",             XYLINE,  0,
    "macro",            XYMACR,  0,
    "modem-dialer",	XYMODM,	 0,
#ifdef NETCONN
    "network",          XYNET,   CM_INV,
#endif /* NETCONN */
    "packet-length",    XYLEN,   CM_INV,    /* moved to send/receive */
    "pad-character",    XYPADC,  CM_INV,    /* moved to send/receive */
    "padding",          XYNPAD,  CM_INV,    /* moved to send/receive */
    "parity",	    	XYPARI,  0,
    "port",             XYLINE,  CM_INV,
    "prompt",	    	XYPROM,  0,
    "receive",          XYRECV,  0,
    "retry",            XYRETR,  0,
    "send",             XYSEND,  0,
    "server",           XYSERV,  0,
    "speed",	        XYSPEE,  0,
    "start-of-packet",  XYMARK,  CM_INV,    /* moved to send/receive */
    "take",             XYTAKE,  0,
    "terminal",         XYTERM,  0,
    "timeout",	        XYTIMO,  CM_INV,     /* moved to send/receive */
    "transfer",         XYXFER,  0,
    "unknown-character-set", XYUNCS, 0,
    "window",           XYWIND,  0
};
int nprm = (sizeof(prmtab) / sizeof(struct keytab)); /* How many parameters */
 
/* Table of networks (not used yet) */
#ifdef NETCONN
struct keytab netcmd[] = {
    "tcp-ip-socket", 0, 0
};
int nnets = 1;				/* How many networks */
#endif /* NETCONN */

/* Remote Command Table */
 
struct keytab remcmd[] = {
    "cd",        XZCWD, CM_INV,
    "cwd",       XZCWD, 0,
    "delete",    XZDEL, 0,
    "directory", XZDIR, 0,
    "help",      XZHLP, 0,
    "host",      XZHOS, 0,
    "print",     XZPRI, 0,
    "set",       XZSET, 0,
    "space",	 XZSPA, 0,
    "type", 	 XZTYP, 0,
    "who",  	 XZWHO, 0
};
int nrmt = (sizeof(remcmd) / sizeof(struct keytab));
 
struct keytab logtab[] = {
    "debugging",    LOGD, 0,
    "packets",	    LOGP, 0,
    "session",      LOGS, 0,
    "transactions", LOGT, 0
};
int nlog = (sizeof(logtab) / sizeof(struct keytab));
 
/* Show command arguments */
 
struct keytab shotab[] = {
    "communications", SHCOM, 0,
    "count", SHCOU, 0,
    "file", SHFIL, 0,
    "key", SHKEY, CM_INV,
    "languages", SHLNG, 0,
    "macros", SHMAC, 0,
    "parameters", SHPAR, CM_INV,
    "protocol", SHPRO, 0,
    "scripts", SHSCR, 0,
    "server", SHSER, 0,
    "speed", SHSPD, CM_INV,
    "status", SHSTA, 0,
    "versions", SHVER, 0
};
int nsho = (sizeof(shotab) / sizeof(struct keytab));

struct keytab iftab[] = {		/* IF commands */
    "<",          XXIFLT, 0,
    "=",          XXIFAE, 0,
    ">",          XXIFGT, 0,
    "count",      XXIFCO, 0,
    "defined",    XXIFDE, 0,
    "equal",      XXIFEQ, 0,
    "exist",      XXIFEX, 0,
    "failure",    XXIFFA, 0,
    "not",        XXIFNO, 0,
    "success",    XXIFSU, 0
};
int nif = (sizeof(iftab) / sizeof(struct keytab));

struct keytab enatab[] = {		/* ENABLE commands */
    "all",        EN_ALL,  0,
    "bye",        EN_FIN,  0,
    "cd",         EN_CWD,  0,
    "cwd",        EN_CWD,  CM_INV,
    "directory",  EN_DIR,  0,
    "finish",     EN_FIN,  0,
    "get",        EN_GET,  0,
    "host",       EN_HOS,  0,
    "send",       EN_SEN,  0,
    "set",        EN_SET,  0,
    "space",      EN_SPA,  0,
    "type",       EN_TYP,  0,
    "who",        EN_WHO,  0
};
int nena = (sizeof(enatab) / sizeof(struct keytab));

struct mtab mactab[MAC_MAX] = {		/* Preinitialized macro table */
    NULL, NULL, 0
};
int nmac = 0;

struct keytab mackey[MAC_MAX];		/* Macro names as command keywords */

/*  D O C M D  --  Do a command  */
 
/*
 Returns:
   -2: user typed an illegal command
   -1: reparse needed
    0: parse was successful (even tho command may have failed).
*/ 
 
docmd(cx) int cx; {
    int b, x, y, z;
    char *s, *p, *q;
    char a;
#ifdef DTILDE
    char *tnam, *tilde_expand();	/* May have to expand tildes */
#endif
 
    debug(F101,"docmd entry, cx","",cx);

    switch (cx) {
 
case -4:				/* EOF */
#ifdef OSK
    if (!quiet && !backgrd) printf("\n");
#else
    if (!quiet && !backgrd) printf("\r\n");
#endif /* OSK */
    doexit(GOOD_EXIT);
case -3:				/* Null command */
    return(0);
case -6:				/* Special */
case -2:				/* Error, maybe */
case -1:				/* Reparse needed */
    return(cx);
 
case XXASK:				/* ask */
case XXASKQ:
    if ((y = cmfld("Variable name","",&s,NULL)) < 0) return(y);
    strcpy(line,s);
    lp = line;
    if (line[0] == CMDQ && line[1] == '%') lp++;
    if (*lp == '%') {
        if (*(lp+2) != '\0') {
	    printf("?Only one character after %% in variable name, please\n");
	    return(-2);
	}
        if (isdigit(*lp) && maclvl < 0) {
	    printf("?Macro parameters not allowed at top level\n");
	    return(-2);
	}
    } else {				/* Actually, we could allow */
	printf("?Variables only, please"); /* them to type in macro */
	return(-2);			/* definitions, but... */
    }
    if ((y = cmtxt("Prompt","",&p,xxstring)) < 0) return(y);
    {					/* Input must be from terminal */
	char psave[40];			/* Save old prompt */
	cmsavp(psave,40);
	cmsetp(p);			/* Make new one */
	if (cx == XXASK)		/* ASK echoes what the user types */
	  cmini(ckxech);
	else				/* ASKQ does not echo */
	  cmini(0);
	x = -1;				/* This means to reparse. */
	if (!backgrd) prompt();		/* Issue prompt. */
	while (x == -1) {		/* Prompt till they answer */
	    x = cmtxt("definition for variable","",&s,NULL);
	    debug(F111," cmtxt",s,x);
	}
	if (cx == XXASKQ) printf("\r\n"); /* If ASKQ must echo CRLF here */
	if (x < 0) {
	    cmsetp(psave);
	    return(x);
	}
	success = 1;			/* Here we know command succeeded */
	if (*s == NUL) {		/* If user types a bare CR, */
	    cmsetp(psave);		/* Restore old prompt, */
	    delmac(lp);			/* delete variable if it exists, */
	    return(0);			/* and return. */
	}
	y = *(lp + 1);			/* Fold case */
	if (isupper(y)) *(lp + 1) = tolower(y);
	y = addmac(lp,s);		/* Add it to the macro table. */
	cmsetp(psave);			/* Restore old prompt. */
    }
    return(0);

case XXBYE:				/* bye */
    if ((x = cmcfm()) < 0) return(x);
    sstate = setgen('L',"","","");
    return(0);
 
case XXCLE:				/* clear */
    if ((x = cmcfm()) < 0) return(x);
    if (!local) {
	printf("You have to 'set line' first\n");
	return(0);
    }
    y = ttflui();			/* flush input buffer */
    for (x = 0; x < INPBUFSIZ; x++)	/* and our local copy too */
      inpbuf[x] = 0;
    inpbp = inpbuf;
    success = (y == 0);			/* Set SUCCESS/FAILURE */
    return(0);

case XXCOM:				/* comment */
    if ((x = cmtxt("Text of comment line","",&s,NULL)) < 0) return(x);
    /* Don't change SUCCESS flag for this one */
    return(0);
 
case XXCON:                     	/* Connect */
    if ((x = cmcfm()) < 0)
      return(x);
    return(success = doconect());

case XXCWD:
#ifdef AMIGA
    if ((x = cmtxt("Name of local directory, or carriage return","",&s,
		   xxstring)) < 0)
    	return(x);
    /* if no name, just print directory name */
    if (*s) {
	if (chdir(s)) {
	    cwdf = success = 0;
	    perror(s);
	}
	success = cwdf = 1;
    }
    if (getcwd(line, sizeof(line)) == NULL)
	printf("Current directory name not available.\n");
    else
	if (!backgrd && cmdlvl == 0) printf("%s\n", line);
#else
    if ((x = cmdir("Name of local directory, or carriage return",homdir,&s,
		   xxstring)) < 0 )
      return(x);
    if (x == 2) {
	printf("\n?Wildcards not allowed in directory name\n");
	return(-2);
    }
#ifdef OS2
    if ( s!=NUL ) {
	if (strlen(s)>=2 && s[1]==':') {	/* Disk specifier */
	    if (zchdsk(*s)) {			/* Change disk successful */
	    	if ( strlen(s)>=3 & ( s[2]==CMDQ || isalnum(s[2]) ) ) {
	    	    if (chdir(s)) perror(s);
	    	}
	    } else {
		cwdf = success = 0;
		perror(s);
	    }
	} else if (chdir(s)) {
	    cwdf = success = 0;
	    perror(s);
	}
    }
    success = cwdf = 1;
    concooked();
    system(PWDCMD);
    conraw();
#else
    if (! zchdir(s)) {
	cwdf = success = 0;
	perror(s);
    }
    system(PWDCMD);			/* assume this works... */
    success = cwdf = 1;			/* system() doesn't tell */
#endif
#endif
    return(0);

case XXCLO:
    x = cmkey(logtab,nlog,"Which log to close","",xxstring);
    if (x == -3) {
	printf("?You must tell which log\n");
	return(-2);
    }
    if (x < 0) return(x);
    if ((y = cmcfm()) < 0) return(y);
    y = doclslog(x);
    success = (y == 1);
    return(success);

case XXDEC: 				/* DECREMENT */
case XXINC: {				/* INCREMENT */
    char c;
    int n;
    
    if ((y = cmfld("Variable name","",&s,NULL)) < 0)
      return(y);
    strncpy(vnambuf,s,VNAML);
    vnp = vnambuf;
    if (vnambuf[0] == CMDQ && vnambuf[1] == '%') vnp++;
    if (*vnp != '%') {
	printf("?Not a variable name - %s\n",s);
	return(-2);
    }
    if (*vnp == '%' && *(vnp+2) != '\0') {
	printf("?Only one character after %% in variable name, please\n");
	return(-2);
    }
    if ((x = cmcfm()) < 0) return(x);
    p = varlook(vnp);			/* Look up variable */
    if (p == NULL) {
	printf("?Not defined - %s\n",s);      
	return(success = 0);		/* If not found, fail. */
    }
    if (!chknum(p)) {			/* If not numeric, fail. */
	printf("?Not numeric - %s = [%s]\n",s,p);
	return(success = 0);
    }
    n = atoi(p);			/* Otherwise convert to integer */
    if (cx == XXINC)			/* And now increment */
      n++;
    else if (cx == XXDEC)		/* or decrement as requested. */
      n--;
    sprintf(tmpbuf,"%d",n);		/* Convert back to numeric string */
    addmac(vnp,tmpbuf);			/* Replace old variable */
    return(success = 1); }

case XXDEF:				/* DEFINE */
case XXASS:				/* ASSIGN */
    if ((y = cmfld("Macro or variable name","",&s,NULL)) < 0) return(y);
    strcpy(vnambuf,s);
    vnp = vnambuf;
    if (vnambuf[0] == CMDQ && vnambuf[1] == '%') vnp++;
    if (*vnp == '%') {
#ifdef COMMENT
	if (isdigit(*(vnp+1)) && maclvl < 0) {
	    printf("?Macro parameters not allowed at top level\n");
	    return(-2);
	}
#endif
	if ((y = cmtxt("Definition of variable","",&s,NULL)) < 0) return(y);
	debug(F110,"xxdef var name",vnp,0);
	debug(F110,"xxdef var def",s,0);
    } else {
	if ((y = cmtxt("Definition of macro","",&s,NULL)) < 0) return(y);
	debug(F110,"xxdef macro name",vnp,0);
	debug(F110,"xxdef macro def",s,0);
    }
    if (*s == NUL) {			/* No arg given, undefine this macro */
	delmac(vnp);			/* silently... */
	return(success = 1);		/* even if it doesn't exist... */
    } 

    /* Defining a new macro or variable */

    if (*vnp == '%' && *(vnp+2) != '\0') {
	printf("?Only one character after %% in variable name, please\n");
	return(-2);
    }
    y = *(vnp + 1);			/* Fold case */
    if (isupper(y)) *(vnp + 1) = tolower(y);

    if (cx == XXASS) {			/* ASSIGN rather than DEFINE? */
	lp = line;			/* If so, expand its value now */
	xxstring(s,&lp);
	s = line;
    }
    debug(F111,"calling addmac",s,strlen(s));

    y = addmac(vnp,s);			/* Add it to the appropriate table. */
    if (y < 0) {
	printf("?No more space for macros\n");
	return(success = 0);
    }
    return(0);
    
case XXDIAL:				/* dial phone number */
    if ((x = cmtxt("Number to be dialed","",&s,xxstring)) < 0)
      return(x);
    return(success = ckdial(s));
 
case XXDEL:				/* delete */
    if ((x = cmifi("File(s) to delete","",&s,&y,xxstring)) < 0) {
	if (x == -3) {
	    printf("?A file specification is required\n");
	    return(-2);
	}
	return(x);
    }
    strncpy(tmpbuf,s,50);		/* Make a safe copy of the name. */
    sprintf(line,"%s %s",DELCMD,s);	/* Construct the system command. */
    if ((y = cmcfm()) < 0) return(y);	/* Confirm the user's command. */
    system(line);			/* Let the system do it. */
    z = zchki(tmpbuf);
    success = (z == -1);
    if (cmdlvl == 0 && !backgrd) {
	if (success)
	  printf("%s - deleted\n",tmpbuf);
	else
	  printf("%s - not deleted\n",tmpbuf);
    }
    return(success);

case XXDIR:				/* directory */
#ifdef vms
    if ((x = cmtxt("Directory/file specification","",&s,xxstring)) < 0)
      return(x);
    /* now do this the same as a shell command - helps with LAT  */
    conres();				/* make console normal */
    lp = line;
    sprintf(lp,"%s %s",DIRCMD,s);
    debug(F110,"Directory string: ", line, 0);
    concb(escape);
/*** So where does it do the command??? ***/
    return(success = 0);
#else
#ifdef AMIGA
    if ((x = cmtxt("Directory/file specification","",&s,xxstring)) < 0)
      return(x);
#else
#ifdef datageneral
    if ((x = cmtxt("Directory/file specification","+",&s,xxstring)) < 0)
      return(x);
#else
    if ((x = cmdir("Directory/file specification","*",&s,xxstring)) < 0)
      return(x);
    strcpy(tmpbuf,s);
    if ((y = cmcfm()) < 0) return(y);
    s = tmpbuf;
#endif
#endif
    lp = line;
    sprintf(lp,"%s %s",DIRCMD,s);
#ifdef OS2
    concooked();
    system(line);
    conraw();
#else
    system(line);
#endif
    return(success = 1);		/* who cares... */
#endif 
 
case XXELS:				/* else */
    if (!ifcmd[cmdlvl]) {
	printf("?ELSE doesn't follow IF\n");
	return(-2);
    }
    ifcmd[cmdlvl] = 0;
    if (!iftest[cmdlvl]) {		/* If IF was false do ELSE part */
	if (maclvl > -1) {		/* In macro, */
	    pushcmd();			/* save rest of command. */
	} else if (tlevel > -1) {	/* In take file, */
	    pushcmd();			/* save rest of command. */
	} else {			/* If interactive, */
	    cmini(ckxech);		/* just start a new command */
	    printf("\n");		/* (like in MS-DOS Kermit) */
	    prompt();
	}
    } else {				/* Condition is false */
	if ((y = cmtxt("command to be ignored","",&s,NULL)) < 0)
	  return(y);			/* Gobble up rest of line */
    }
    return(0);

case XXENA:				/* enable */
case XXDIS:				/* disable */
    if ((x = cmkey(enatab,nena,"Server function to enable","",xxstring)) < 0)
      return(x);
    if ((y = cmcfm()) < 0)
      return(y);
    y = ((cx == XXENA) ? 1 : 0);
    switch (x) {
      case EN_ALL:
	en_cwd = en_del = en_dir = en_fin = en_get = y;
	en_hos = en_sen = en_set = en_spa = en_typ = en_who = y;
	break;
      case EN_CWD:
	en_cwd = y;
	break;
      case EN_DIR:
	en_dir = y;
	break;
      case EN_FIN:
	en_fin = y;
	break;
      case EN_GET:
	en_get = y;
	break;
      case EN_HOS:
	en_hos = y;
	break;
      case EN_SEN:
	en_sen = y;
	break;
      case EN_SET:
	en_set = y;
	break;
      case EN_SPA:
	en_spa = y;
	break;
      case EN_TYP:
	en_typ = y;
	break;
      case EN_WHO:
	en_who = y;
	break;
      default:
	return(-2);
    }
    return(0);

case XXEND:				/* end */
    if ((x = cmcfm()) < 0)
      return(x);
    popclvl();				/* pop command level */
    return(success = 1);		/* always succeeds */

case XXDO:				/* do (a macro) */
    if (nmac == 0) {
	printf("\?No macros defined\n");
	return(-2);
    }
    for (y = 0; y < nmac; y++) {	/* copy the macro table */
	mackey[y].kwd = mactab[y].kwd;	/* into a regular keyword table */
	mackey[y].val = y;		/* with value = pointer to macro tbl */
	mackey[y].flgs = mactab[y].flgs;
    }
    /* parse name as keyword */
    if ((x = cmkey(mackey,nmac,"macro","",xxstring)) < 0)
      return(x);
    if ((y = cmtxt("optional arguments","",&s,xxstring)) < 0) /* get args */
      return(y);
    if (++maclvl > MACLEVEL) {		/* Make sure we have storage */
	--maclvl;
	printf("Macros nested too deeply\n");
	return(success = 0);
    }
    macp[maclvl] = mactab[x].val;	/* Point to the macro body */
    debug(F111,"do macro",macp[maclvl],maclvl);
    macx[maclvl] = x;			/* Remember the macro table index */

    cmdlvl++;				/* Entering a new command level */
    if (cmdlvl > CMDSTKL) {		/* Too many macros + TAKE files? */
	cmdlvl--;
	printf("?TAKE files and DO commands nested too deeply\n");
	return(success = 0);
    }
    ifcmd[cmdlvl] = 0;
    iftest[cmdlvl] = 0;
    count[cmdlvl] = 0;
    cmdstk[cmdlvl].src = CMD_MD;	/* Say we're in a macro */
    cmdstk[cmdlvl].val = maclvl;	/* and remember the macro level */

    debug(F111,"do macro",mactab[macx[maclvl]].kwd,macx[maclvl]);

/* Clear old %0..%9 arguments */

    addmac("%0",mactab[macx[maclvl]].kwd); /* Define %0 = name of macro */
    varnam[0] = '%';
    varnam[2] = '\0';
    for (y = 1; y < 10; y++) {		/* Clear args %1..%9 */
	varnam[1] = y + '0';
	delmac(varnam);
    }	

/* Assign the new args one word per arg, allowing braces to group words */

    p = s;				/* Pointer to beginning of arg */
    b = 0;				/* Flag for outer brace removal */
    x = 0;				/* Flag for in-word */
    y = 0;				/* Brace nesting level */
    z = 0;				/* Argument counter, 0 thru 9 */

    while (1) {				/* Go thru argument list */
	if (*s == '\0') {		/* No more characters? */
	    if (x != 0) {
		if (z == 9) break;	/* Only go up to 9. */
		z++;			/* Count it. */
		varnam[1] = z + '0';	/* Assign last argument */
		addmac(varnam,p);
		break;			/* And get out. */
	    } else break;
	} 
	if (x == 0 && *s == ' ') {	/* Eat leading blanks */
	    s++;
	    continue;
	} else if (*s == '{') {		/* An opening brace */
	    if (x == 0 && y == 0) {	/* If leading brace */
		p = s+1;		/* point past it */
		b = 1;			/* and flag that we did this */
	    }
	    x = 1;			/* Flag that we're in a word */
	    y++;			/* Count the brace. */
	} else if (*s == '}') {		/* A closing brace. */
	    y--;			/* Count it. */
	    if (y == 0 && b != 0) {	/* If it matches the leading brace */
		*s = ' ';		/* change it to a space */
		b = 0;			/* and we're not in braces any more */
	    } else if (y < 0) x = 1;	/* otherwise just start a new word. */
	} else if (*s != ' ') {		/* Nonspace means we're in a word */
	    if (x == 0) p = s;		/* Mark the beginning */
	    x = 1;			/* Set in-word flag */
	}
	/* If we're not inside a braced quantity, and we are in a word, and */
	/* we have hit a space, then we have an argument to assign. */
	if ((y < 1) && (x != 0) && (*s == ' ')) { 
	    *s = '\0';			/* terminate the arg with null */
	    x = 0;			/* say we're not in a word any more */
	    y = 0;			/* start braces off clean again */
	    if (z == 9) break;		/* Only go up to 9. */
	    z++;			/* count this arg */
	    varnam[1] = z + '0';	/* compute its name */
	    addmac(varnam,p);		/* add it to the macro table */
	    p = s+1;
	}
	s++;				/* Point past this character */
    }
    if ((z == 0) && (y > 1)) {		/* Extra closing brace(s) at end */
	z++;
	varnam[1] = z + '0';		/* compute its name */
	addmac(varnam,p);		/* Add rest of line to last arg */
    }
    macargc[maclvl] = z;		/* For ARGC variable */
    return(success = 1);		/* DO command succeeded */

case XXECH: 				/* echo */
    if ((x = cmtxt("Material to be echoed","",&s,xxstring)) < 0) return(x);
    printf("%s\n",s);
    return(1);				/* Don't bother with success? */

case XXOUT:				/* Output */
    if ((x = cmtxt("Material to be output","",&s,xxstring)) < 0) return(x);
    return(success = dooutput(s));

case XXPAU:				/* Pause */
case XXWAI:				/* Wait */
    /* For now, WAIT is just a synonym for PAUSE. */
    /* Both should take not only secs but also hh:mm:ss as argument. */
    /* WAIT should wait for modem signals like MS-DOS Kermit */
    /* Presently there's no mechanism in ckutio.c for this. */
    if (cx == XXWAI)
      y = cmnum("seconds to wait","1",10,&x,xxstring);
    else y = cmnum("seconds to pause","1",10,&x,xxstring);
    if (y < 0) return(y);
    if (x < 0) x = 0;
    switch (cx) {
      case XXPAU:
	if ((y = cmcfm()) < 0) return(y);
	break;
      case XXWAI:
	if ((y = cmtxt("Modem Signals","",&s,xxstring)) < 0) return(y);
	break;
      default:
	return(-2);
    }

/* Command is entered, now do it.  For now, WAIT is just like PAUSE */

    while (x--) {			/* Sleep loop */
	if (y = conchk()) {		/* Did they type something? */
	    while (y--) coninc(0);	/* Yes, gobble it up */
	    break;			/* And quit PAUSing or WAITing */
	}
	sleep(1);			/* No interrupt, sleep one second */
    }
    if (cx == XXWAI) success = 0;	/* For now, WAIT always fails. */
    else success = (x == -1);		/* Set SUCCESS/FAILURE for PAUSE. */
    return(0);

case XXPWD:				/* PWD */
    if ((x = cmcfm()) < 0) return(x);
    system(PWDCMD);
    return(success = 1);		/* blind faith */

case XXQUI:				/* quit, exit */
case XXEXI:
    if ((x = cmcfm()) > -1) doexit(GOOD_EXIT);
    else return(x);
 
case XXFIN:				/* finish */
    if ((x = cmcfm()) < 0) return(x);
    sstate = setgen('F',"","","");
    return(0);

case XXGET:				/* get */
    x = cmtxt("Name of remote file(s), or carriage return","",&cmarg,xxstring);
    if ((x == -2) || (x == -1)) return(x);
 
/* If foreign file name omitted, get foreign and local names separately */
 
    x = 0;				/* For some reason cmtxt returns 1 */
    if (*cmarg == NUL) {
 
	if (tlevel > -1) {		/* Input is from take file */
 
	    if (fgets(line,100,tfile[tlevel]) == NULL)
	    	fatal("take file ends prematurely in 'get'");
	    debug(F110,"take-get 2nd line",line,0);
/****	    stripq(line); ***/
	    for (x = strlen(line);
	     	 x > 0 && (line[x-1] == LF || line[x-1] == CR);
		 x--)
		line[x-1] = '\0';
	    cmarg = line;
	    if (fgets(cmdbuf,CMDBL,tfile[tlevel]) == NULL)
	    	fatal("take file ends prematurely in 'get'");
/****	    stripq(cmdbuf); ***/
	    for (x = strlen(cmdbuf);
	     	 x > 0 && (cmdbuf[x-1] == LF || cmdbuf[x-1] == CR);
		 x--)
		cmdbuf[x-1] = '\0';
	    if (*cmdbuf == NUL) cmarg2 = line; else cmarg2 = cmdbuf;
            x = 0;			/* Return code */
	    printf("%s",cmarg2);
        } else {			/* Input is from terminal */
 
	    char psave[40];		/* Save old prompt */
	    cmsavp(psave,40);
	    cmsetp(" Remote file specification: "); /* Make new one */
	    cmini(ckxech);
	    x = -1;
	    if (!backgrd) prompt();
	    while (x == -1) {		/* Prompt till they answer */
	    	x = cmtxt("Name of remote file(s)","",&cmarg,xxstring);
		debug(F111," cmtxt",cmarg,x);
	    }
	    if (x < 0) {
		cmsetp(psave);
		return(x);
	    }
	    if (*cmarg == NUL) { 	/* If user types a bare CR, */
		printf("(cancelled)\n"); /* Forget about this. */
	    	cmsetp(psave);		/* Restore old prompt, */
		return(0);		/* and return. */
	    }
	    strcpy(line,cmarg);		/* Make a safe copy */
	    cmarg = line;
	    cmsetp(" Local name to store it under: "); /* New prompt */
	    cmini(ckxech);
	    x = -1;
	    if (!backgrd) prompt();
	    while (x == -1) {		/* Again, parse till answered */
	    	x = cmofi("Local file name","",&cmarg2,xxstring);
	    }
	    if (x == -3) {	    	    	/* If bare CR, */
		printf("(cancelled)\n");	/* escape from this... */
	    	cmsetp(psave);		        /* Restore old prompt, */
		return(0);		    	/* and return. */
	    } else if (x < 0) return(x);        /* Handle parse errors. */
	    
	    x = -1;			/* Get confirmation. */
	    while (x == -1) x = cmcfm();
	    cmsetp(psave);		/* Restore old prompt. */
        }
    }
    if (x == 0) {			/* Good return from cmtxt or cmcfm, */
	sstate = 'r';			/* set start state. */
	if (local) displa = 1;
    }
    return(x);

case XXHLP:				/* Help */
    x = cmkey2(cmdtab,ncmd,"C-Kermit command","help",toktab,xxstring);
    if (x == -5) {
	y = chktok(toktab);
	debug(F101,"top-level cmkey token","",y);
	ungword();
	switch (y) {
	  case '!': x = XXSHE; break;
	  case '%': x = XXCOM; break;
	  case '#': x = XXCOM; break;
	  case ';': x = XXCOM; break;
	  case ':': x = XXLBL; break;
	  case '&': x = XXECH; break;
	  default:
	    printf("\n?Invalid - %s\n",cmdbuf);
	    x = -2;
	}
    }
    return(success = (dohlp(x) > -1));
 
case XXHAN:				/* Hangup */
    if ((x = cmcfm()) > -1)
      return(success = (tthang() > -1));

case XXGOTO:				/* Goto */
/* Note, here we don't set SUCCESS/FAILURE flag */
    if ((y = cmfld("label","",&s,xxstring)) < 0) return(y);
    strcpy(lblbuf,s);
    if ((x = cmcfm()) < 0) return(x);
    s = lblbuf;
    debug(F110,"goto before conversion",s,0);
    if (*s != ':') {			/* If the label mentioned */
	int i;				/* does not begin with a colon, */
	y = strlen(s);			/* then insert one. */
	for (i = y; i > 0; i--)
	    s[i] = s[i-1];
	s[0] = ':';
	s[++y] = '\0';
    }
    debug(F111,"goto after conversion",s,y);

    while (cmdlvl > 0) {		/* Only works inside macros & files */
	if (cmdstk[cmdlvl].src == CMD_MD) { /* GOTO inside macro */
	    int i, m;
	    lp = mactab[macx[maclvl]].val; /* point to beginning of macro */
	    debug(F111,"goto in macro",lp,macx[maclvl]);
	    m = strlen(lp) - y;
	    debug(F111,"goto in macro",lp,m);
	    for (i = 0; i < m; i++,lp++) /* search for label in macro body */
	      if (!strncmp(s,lp,y))
		break;
	    if (i == m) {		/* didn't find the label */
		debug(F101,"goto failed at cmdlvl","",cmdlvl);
		if (!popclvl()) {	/* pop up to next higher level */
		    printf("?Label '%s' not found\n",s); /* if none */
		    return(0);		/* quit */
		} else continue;	/* otherwise look again */
	    }
	    debug(F110,"goto found macro label",lp,0);
	    macp[maclvl] = lp;		/* set macro buffer pointer */
	    return(1);
	} else if (cmdstk[cmdlvl].src == CMD_TF) {
	    x = 0;			/* GOTO issued in take file */
	    rewind(tfile[tlevel]);	/* Search file from beginning */
	    while (! feof(tfile[tlevel])) {
		if (fgets(line,LINBUFSIZ,tfile[tlevel]) == NULL) /* Get line */
		  break;		/* If no more, done, label not found */
		lp = line;		/* Got line */
		while (*lp == ' ' || *lp == '\t')
		  lp++;			/* Strip leading whitespace */
		if (!strncmp(lp,s,y)) {	/* Compare result with label */
		    x = 1;		/* Got it */
		    break;		/* done. */
		}
	    }
	    if (x == 0) {		/* If not found, print message */
		debug(F101,"goto failed at cmdlvl","",cmdlvl);
		if (!popclvl()) {	/* pop up to next higher level */
		    printf("?Label '%s' not found\n",s);	/* if none */
		    return(0);		/* quit */
		} else continue;	/* otherwise look again */
	    }
	    return(x);			/* Send back return code */
	}
    }
    printf("?Stack problem in GOTO\n",s); /* Shouldn't see this */
    return(0);

case XXIF:				/* IF command */
    not = 0;				/* Flag for whether "NOT" was seen */

ifagain:
    if ((ifc = cmkey(iftab,nif,"","",xxstring)) < 0) /* If what?... */
      return(ifc);
    switch (ifc) {			/* z = 1 for true, 0 for false */
      case XXIFNO:			/* IF NOT */
	not ^= 1;
	goto ifagain;
      case XXIFSU:			/* IF SUCCESS */
	z = ( success != 0 );
	debug(F101,"if success","",z);
	break;
      case XXIFFA:			/* IF FAILURE */
	z = ( success == 0 );
	debug(F101,"if failure","",z);
	break;
      case XXIFDE:			/* IF DEFINED */
	if ((x = cmfld("Macro or variable name","",&s,NULL)) < 0)
	  return(x);
	strcpy(line,s);			/* Make a copy */
	lp = line;
	if (line[0] == CMDQ && line[1] == '%')	/* Change "\%x" to just "%x" */
	  lp++;				/* Both forms are allowed. */
	if (*lp == '%')	{		/* Is it a variable? */
	    x = *(lp + 1);		/* Fold case */
	    if (isupper(x)) *(lp + 1) = tolower(x);
	    if (x >= '0' && x <= '9' && maclvl > -1)
	      z = ( m_arg[maclvl][x - '0'] != 0 ); /* Digit is macro arg */
	    else			/* Otherwise its a global variable */
	      z = ( g_var[x] != 0 );
	} else {			/* Otherwise its a macro name */
	    z = ( mxlook(mactab,lp,nmac) > -1 ); /* Look for exact match */
	}
	debug(F111,"if defined",s,z);
	break;
      case XXIFCO:			/* IF COUNT */
	z = ( --count[cmdlvl] > 0 );
	debug(F101,"if count","",z);
	break;
      case XXIFEX:			/* IF EXIST */
	if ((x = cmfld("File","",&s,xxstring)) < 0) return(x);
	z = ( zchki(s) > -1L );
	debug(F101,"if exist","",z);
	break;
      case XXIFEQ: 			/* IF EQUAL (string comparison) */
	if ((x = cmfld("first word or variable name","",&s,xxstring)) < 0)
	  return(x);
	lp = line;
	strcpy(lp,s);
	if ((y = cmfld("second word or variable name","",&s,xxstring)) < 0)
	  return(y);
	x = strlen(lp);
	tp = line + x + 2;
	strcpy(tp,s);
	y = strlen(tp);
	if (x + y + 2 > LINBUFSIZ) {	/* Have to do something better */
	    fatal("if equal: strings too long"); /* than this... */
	    z = 0;
	    break;
	}
	if (x != y) {			/* Different lengths */
	    z = 0;			/* so not equal */
	    break;
	}
	if (!incase)			/* Ignoring alphabet case? */
	  z = !xxstrcmp(tp,lp);		/* Yes, special string compare */
	else				/* No, */
	  z = !strcmp(tp,lp);		/* Exact match required. */
	debug(F101,"if equal","",z);
	break;

      case XXIFAE:			/* IF (arithmetically) = */
      case XXIFLT:			/* IF (arithmetically) < */
      case XXIFGT: {			/* IF (arithmetically) > */
	int n1, n2;
	if ((x = cmfld("first number or variable name","",&s,xxstring)) < 0)
	  return(x);
	debug(F101,"xxifgt cmfld","",x);
	lp = line;
	strcpy(lp,s);
	debug(F110,"xxifgt exp1",lp,0);
	if (!xxstrcmp(lp,"count")) {
	    n1 = count[cmdlvl];
	} else if (!xxstrcmp(lp,"version")) {
	    n1 = (int) vernum;
	} else if (!xxstrcmp(lp,"argc")) {
	    n1 = (int) macargc[maclvl];
	} else {
	    if (!chknum(lp)) return(-2);
	    n1 = atoi(lp);
	}
	if ((y = cmfld("second number or variable name","",&s,xxstring)) < 0)
	  return(y);
	x = strlen(lp);
	tp = line + x + 2;
	strcpy(tp,s);
	debug(F110,"xxifgt exp2",tp,0);
	if (!xxstrcmp(tp,"count")) {
	    n2 = count[cmdlvl];
	} else if (!xxstrcmp(tp,"version")) {
	    n2 = (int) vernum;
	} else if (!xxstrcmp(tp,"argc")) {
	    n2 = (int) macargc[maclvl];
	} else {
	    if (!chknum(tp)) return(-2);
	    n2 = atoi(tp);
	}
	z = ((n1 <  n2 && ifc == XXIFLT)
	  || (n1 == n2 && ifc == XXIFAE)
	  || (n1 >  n2 && ifc == XXIFGT));
	debug(F101,"xxifft n1","",n1);
	debug(F101,"xxifft n2","",n2);
	debug(F101,"xxifft z","",z);
	break; }

      default:				/* Shouldn't happen */
	return(-2);
    }
        ifcmd[cmdlvl] = 1;		/* We just completed an IF command */
	if (not) z = !z;		/* Handle NOT here */
        if (z) {			/* Condition is true */
	    iftest[cmdlvl] = 1;		/* Remember that IF succeeded */
	    if (maclvl > -1) {		/* In macro, */
		pushcmd();		/* save rest of command. */
	    } else if (tlevel > -1) {	/* In take file, */
		pushcmd();		/* save rest of command. */
	    } else {			/* If interactive, */
		cmini(ckxech);		/* just start a new command */
		printf("\n");		/* (like in MS-DOS Kermit) */
		prompt();
	    }
	} else {			/* Condition is false */
	    iftest[cmdlvl] = 0;		/* Remember command failed. */
	    if ((y = cmtxt("command to be ignored","",&s,NULL)) < 0)
	      return(y);		/* Gobble up rest of line */
	}
    return(0);

case XXINP:				/* INPUT and */
case XXREI:				/* REINPUT */
    y = cmnum("seconds to wait for input","1",10,&x,xxstring);
    if (y < 0) return(y);
    if (x <= 0) x = 1;
    if ((y = cmtxt("Material to be input","",&s,xxstring)) < 0) return(y);
/*
    if (! local) {
	printf("You must 'set line' first\n");
	return(1);
    }
*/
    if (cx == XXINP) {			/* INPUT */
	debug(F110,"xxinp line",s,0);
	if (local != 0 && tvtflg == 0) { /* Put line in "ttvt" mode */
	    y = ttvt(speed,flow);	/* if not already. */
	    if (y < 0) {
		printf("?Can't condition line for INPUT\n");
		return(0);		/* Watch out for failure. */
	    }
	    tvtflg = 1;			/* On success set this flag */
	}
	success = doinput(x,s);		/* Go try to input the search string */
    } else {				/* REINPUT */
	debug(F110,"xxrei line",s,0);
	success = doreinp(x,s);
    }
    if (intime && !success) {		/* TIMEOUT-ACTION = QUIT? */
	popclvl();			/* If so, pop command level. */
	if (!backgrd && cmdlvl == 0) {
	    if (cx == XXINP) printf("Input timed out\n");
	    if (cx == XXREI) printf("Reinput failed\n");
        }
    }
    return(success);			/* Return (re)doinput's return code */

case XXLBL:				/* Label */
    if ((x = cmtxt("Command-file label","",&s,xxstring)) < 0) return(x);
    /* should be cmfld, then cmcfm, to prevent multiple words in label? */
    return(0);

case XXLOG:				/* Log */
    x = cmkey(logtab,nlog,"What to log","",xxstring);
    if (x == -3) {
	printf("?You must specify what is to be logged\n");
	return(-2);
    }
    if (x < 0) return(x);
    return(success = (dolog(x) > 0));
 
case XXLOGI:				/* Send script remote system */
    if ((x = cmtxt("Text of login script","",&s,xxstring)) < 0) return(x);
    return(success = login(s));		/* Return 1=completed, 0=failed */
 
case XXREC:				/* Receive */
    cmarg2 = "";
    x = cmofi("Name under which to store the file, or CR","",&cmarg2,xxstring);
    if ((x == -1) || (x == -2)) return(x);
    debug(F111,"cmofi cmarg2",cmarg2,x);
    if ((x = cmcfm()) < 0) return(x);
    sstate = 'v';
    if (local) displa = 1;
    return(0);
 
case XXREM:				/* Remote */
    x = cmkey(remcmd,nrmt,"Remote Kermit server command","",xxstring);
    if (x == -3) {
	printf("?You must specify a command for the remote server\n");
	return(-2);
    }
    return(dormt(x));


case XXSEN:				/* SEND command and... */
case XXMAI:				/* MAIL command */
    cmarg = cmarg2 = "";
    if ((x = cmifi("File(s) to send","",&s,&y,xxstring)) < 0) {
	if (x == -3) {
	    printf("?A file specification is required\n");
	    return(-2);
	}
	return(x);
    }
    nfils = -1;				/* Files come from internal list. */
    strcpy(line,s);			/* Save copy of string just parsed. */
    if (cx == XXSEN) {			/* SEND command */
	debug(F101,"Send: wild","",y);
	if (y == 0) {
	    if ((x = cmtxt("Name to send it with","",&cmarg2,xxstring)) < 0)
	      return(x);
	} else {
	    if ((x = cmcfm()) < 0) return(x);
	}
	cmarg = line;			/* File to send */
	debug(F110,"Sending:",cmarg,0);
	if (*cmarg2 != '\0') debug(F110," as:",cmarg2,0);
    } else {				/* MAIL */
	debug(F101,"Mail: wild","",y);
	*optbuf = NUL;			/* Wipe out any old options */
	if ((x = cmtxt("Address to mail to","",&s,xxstring)) < 0) return(x);
	strcpy(optbuf,s);
	if (strlen(optbuf) > 94) {	/* Ensure legal size */
	    printf("?Option string too long\n");
	    return(-2);
	}
	cmarg = line;			/* File to send */
	debug(F110,"Mailing:",cmarg,0);
	debug(F110,"To:",optbuf,0);
	rmailf = 1;			/* MAIL modifier flag for SEND */
    }
    sstate = 's';			/* Set start state to SEND */
    if (local) displa = 1;
    return(0);
 
case XXSER:				/* Server */
    if ((x = cmcfm()) < 0) return(x);
    sstate = 'x';
    if (local) displa = 1;
#ifdef AMIGA
    reqoff();				/* No DOS requestors while server */
#endif
    return(0);
 
case XXSET:				/* SET command */
    x = cmkey(prmtab,nprm,"Parameter","",xxstring);
    if (x == -3) {
	printf("?You must specify a parameter to set\n");
	return(-2);
    }
    if (x < 0) return(x);
    /* have to set success separately for each item in doprm()... */
    /* actually not really, could have just had doprm return 0 or 1 */
    /* and set success here... */
    return(doprm(x,0));
    
/* XXSHE code by H. Fischer; copyright rights assigned to Columbia Univ */
/*
 Adapted to use getpwuid to find login shell because many systems do not
 have SHELL in environment, and to use direct calling of shell rather
 than intermediate system() call. -- H. Fischer
*/
case XXSHE:				/* Local shell command */
    {
    int pid;
#ifdef AMIGA
    if (cmtxt("Command to execute","",&s,xxstring) < 0) return(-1);
#else
#ifdef OS2
    if (cmtxt("OS2 command to execute","",&s,xxstring) < 0) return(-1);
#else
    if (cmtxt("Unix shell command to execute","",&s,xxstring) < 0) return(-1);
#endif /* Amiga */
#endif /* OS2 */

    conres();				/* Make console normal  */

#ifdef OS2
    if (*s == '\0') sprintf(s,"%s","CMD"); /* Command processor */
    concooked();
    system(s);
    conraw();
#else
#ifdef OSK
    system(s);
#else
#ifdef AMIGA
    system(s);
#else
#ifdef MSDOS
    zxcmd(s);
#else
#ifdef vms
    system(s);				/* Best we can do for VMS? */
#else
#ifdef datageneral
    if (*s == NUL)			/* Interactive shell requested? */
#ifdef mvux
	system("/bin/sh ");
#else
        system("x :cli prefix Kermit_Baby:");
#endif
    else				/* Otherwise, */
        system(s);			/* Best for aos/vs?? */
 
#else
#ifdef aegis
    if ((pid = vfork()) == 0) {		/* Make child quickly */
	char *shpath, *shname, *shptr;	/* For finding desired shell */

        if ((shpath = getenv("SHELL")) == NULL) shpath = "/com/sh";

#else					/* All Unix systems */
    if ((pid = fork()) == 0) {		/* Make child */
	char *shpath, *shname, *shptr;	/* For finding desired shell */
	struct passwd *p;
	extern struct passwd * getpwuid();
	extern int getuid();
	char *defShel = "/bin/sh";	/* Default */
 
	p = getpwuid( getuid() );	/* Get login data */
	if ( p == (struct passwd *) NULL || !*(p->pw_shell) )
	    shpath = defShel;
	else
	    shpath = p->pw_shell;
#endif
	shptr = shname = shpath;
	while (*shptr != '\0')
	    if (*shptr++ == '/') shname = shptr;

/* Remove following uid calls if they cause trouble */
#ifdef BSD4
#ifndef BSD41
	setegid(getgid());		/* Override 4.3BSD csh security */
	seteuid(getuid());		/*  checks. */
#endif
#endif
	if (*s == NUL)			/* Interactive shell requested? */
	    execl(shpath,shname,"-i",NULL);    /* Yes, do that */
	else				/* Otherwise, */
	    execl(shpath,shname,"-c",s,NULL); /* exec the given command */
	exit(BAD_EXIT); }		/* Just punt if it didn't work */
 
    else {				/* Parent */
 
    	int wstat;			/* Kermit must wait for child */
	SIGTYP (*istat)(), (*qstat)();
 
	istat = signal(SIGINT,SIG_IGN);	/* Let the fork handle keyboard */
	qstat = signal(SIGQUIT,SIG_IGN); /* interrupts itself... */
 
    	while (((wstat = wait((int *)0)) != pid) && (wstat != -1)) ;
	                                /* Wait for fork */
	signal(SIGINT,istat);		/* Restore interrupts */
	signal(SIGQUIT,qstat);
    }
#endif
#endif
#endif
#endif
#endif
#endif
    concb(escape);			/* Console back in cbreak mode */
    return(success = 1);		/* Just pretend it worked. */
}

case XXSHO:				/* Show */
    x = cmkey(shotab,nsho,"","parameters",xxstring);
    if (x < 0) return(x);
    return(success = doshow(x));
 
case XXSPA:				/* space */
#ifdef datageneral
    /* The DG can take an argument after its "space" command. */
    if ((x = cmtxt("Confirm, or local directory name","",&s,xxstring)) < 0)
      return(x);
    if (*s == NUL) system(SPACMD);
    else {
    	char *cp;
    	cp = alloc(strlen(s) + 7);      /* For "space *s" */
    	strcpy(cp,"space "), strcat(cp,s);
    	system(cp);
    	if (cp) free(cp);
    }
#else
    if ((x = cmcfm()) < 0) return(x);
#ifdef OS2
    concooked();
    system(SPACMD);
    conraw();
#else
    system(SPACMD);
#endif
#endif
    return(success = 1);		/* pretend it worked */
 
case XXSTA:				/* statistics */
    if ((x = cmcfm()) < 0) return(x);
    return(success = dostat());

case XXSTO:				/* stop */
    if ((x = cmcfm()) < 0) return(x);
    dostop();	
    success = 1;			/* always succeeds */
    return(0);

case XXTAK:				/* take */
    if (tlevel > MAXTAKE-1) {
	printf("?Take files nested too deeply\n");
	return(-2);
    }
    if ((y = cmifi("C-Kermit command file","",&s,&x,xxstring)) < 0) { 
	if (y == -3) {
	    printf("?A file specification is required\n");
	    return(-2);
	} else return(y);
    }
    if (x != 0) {
	printf("?Wildcards not allowed in command file name\n");
	return(-2);
    }
    strcpy(line,s);			/* Make a safe copy of the string */
    if ((y = cmcfm()) < 0) return(y);
    if ((tfile[++tlevel] = fopen(line,"r")) == NULL) {
	perror(line);
	debug(F110,"Failure to open",line,0);
	success = 0;
	tlevel--;
    } else {
	cmdlvl++;			/* Entering a new command level */
	if (cmdlvl > CMDSTKL) {
	    cmdlvl--;
	    printf("?TAKE files and DO commands nested too deeply\n");
	    return(success = 0);
	}
	ifcmd[cmdlvl] = 0;
	iftest[cmdlvl] = 0;
	count[cmdlvl] = 0;
	cmdstk[cmdlvl].src = CMD_TF;	/* Say we're in a TAKE file */
	cmdstk[cmdlvl].val = tlevel;	/* nested at this level */
	success = 1;
    }
    return(success);
 
case XXTRA:				/* transmit */
    if ((x = cmifi("File to transmit","",&s,&y,xxstring)) < 0) {
	if (x == -3) {
	    printf("?Name of an existing file\n");
	    return(-2);
	}
	return(x);
    }
    if (y != 0) {
	printf("?Only a single file may be transmitted\n");
	return(-2);
    }
    strcpy(line,s);			/* Save copy of string just parsed. */
    y = cmnum("Decimal ASCII value of line turnaround character","10",10,&x,
	      xxstring);
    if (y < 0) return(y);
    if (x < 0 || x > 127) {
	printf("?Decimal number between 0 and 127\n");
	return(-2);
    }
    if ((y = cmcfm()) < 0) return(y);	/* Confirm the command */
    debug(F110,"calling transmit",line,0);
    return(success = transmit(line,x));	/* Do the command */

case XXTYP:				/* TYPE */
    if ((x = cmifi("File to type","",&s,&y,xxstring)) < 0) {
	if (x == -3) {
	    printf("?Name of an existing file\n");
	    return(-2);
	}
	return(x);
    }
    if (y != 0) {
	printf("?A single file please\n");
	return(-2);
    }
    strcpy(line,s);			/* Save copy of string just parsed. */
    if ((y = cmcfm()) < 0) return(y);	/* Confirm the command */
    debug(F110,"calling transmit",line,0);
    return(success = dotype(line));	/* Do the TYPE command */

case XXTES:				/* TEST */
    /* Fill this in with whatever is being tested... */
    if ((y = cmcfm()) < 0) return(y);	/* Confirm the command */
    printf("cmdlvl = %d, tlevel = %d, maclvl = %d\n",cmdlvl,tlevel,maclvl);
    if (maclvl < 0) {
	printf("%s\n",
	       "Call me from inside a macro and I'll dump the argument stack");
	return(0);
    }
    printf("Macro level: %d, ARGC = %d\n     ",maclvl,macargc[maclvl]);
    for (y = 0; y < 10; y++) printf("%7d",y);
    for (x = 0; x <= maclvl; x++) {
	printf("\n%2d:  ",x);
	for (y = 0; y < 10; y++) {
	    s = m_arg[x][y];
	    printf("%7s",s ? s : "(none)");
	}
    }
    printf("\n");
    return(0);

case XXXLA:				/* translate */
    if ((x = cmifi("File to translate","",&s,&y,xxstring)) < 0) {
	if (x == -3) {
	    printf("?Name of an existing file\n");
	    return(-2);
	}
	return(x);
    }
    if (y != 0) {
	printf("?A single file please\n");
	return(-2);
    }
    strcpy(line,s);			/* Save copy of string just parsed. */
    if ((x = cmofi("Output file","",&s,&y,xxstring)) < 0) {
	if (x != -3) return(x);		/* don't ask... */
	else s = CTTNAM;
    }
    if (y != 0) {
	printf("?A single file please\n");
	return(-2);
    }
    if ((y = cmcfm()) < 0) return(y);	/* Confirm the command */
    return(success = xlate(line,s));	/* Do the TRANSLATE command */

case XXVER:
    if ((y = cmcfm()) < 0) return(y);
    printf("%s,%s\n Numeric: %d\n",versio,ckxsys,vernum);
    return(success = 1);

default:
    printf("Not available - %s\n",cmdbuf);
    return(-2);
    }
}
