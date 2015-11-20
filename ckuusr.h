/*  C K U U S R . H  --  Symbol definitions for C-Kermit ckuus*.c modules  */
 
/*
 Author: Frank da Cruz (fdc@columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 First released January 1985.
 Copyright (C) 1985, 1989, Trustees of Columbia University in the City of New 
 York.  Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained.
*/
 
/* Name of C-Kermit program initialization file. */
#ifdef vms
#define KERMRC "ckermit.ini"
#else
#ifdef OS2
#define KERMRC "ckermit.ini"
#else
#define KERMRC ".kermrc"
#endif
#endif

#ifndef OS2
#ifndef AMIGA
#ifndef vms
#ifndef datageneral
#ifndef OSK
#include <pwd.h>
#endif
#endif
#endif
#endif
#endif 

/* Symbols for command source */

#define CMD_KB 0			/* Command source = Keboard or stdin */
#define CMD_TF 1			/* Command source = TAKE File */
#define CMD_MD 2			/* Command source = Macro Definition */

#define VNAML 20			/* Max length for variable name */

/* Values associated with top-level commands, must be 0 or greater. */
 
#define XXBYE   0	/* BYE */
#define XXCLE   1	/* CLEAR */
#define XXCLO   2	/* CLOSE */
#define XXCON   3	/* CONNECT */
#define XXCPY   4	/* COPY */
#define XXCWD   5	/* CWD (Change Working Directory) */
#define XXDEF	6	/* DEFINE (a command macro) */
#define XXDEL   7	/* (Local) DELETE */
#define XXDIR   8	/* (Local) DIRECTORY */
#define XXDIS   9	/* DISABLE <-- changed from DISCONNECT! */
#define XXECH  10	/* ECHO */
#define XXEXI  11	/* EXIT */
#define XXFIN  12	/* FINISH */
#define XXGET  13	/* GET */
#define XXHLP  14	/* HELP */
#define XXINP  15	/* INPUT */
#define XXLOC  16	/* LOCAL */
#define XXLOG  17	/* LOG */
#define XXMAI  18	/* MAIL */
#define XXMOU  19	/* (Local) MOUNT */
#define XXMSG  20	/* (Local) MESSAGE */
#define XXOUT  21	/* OUTPUT */
#define XXPAU  22	/* PAUSE */
#define XXPRI  23	/* (Local) PRINT */
#define XXQUI  24	/* QUIT */
#define XXREC  25	/* RECEIVE */
#define XXREM  26	/* REMOTE */
#define XXREN  27	/* (Local) RENAME */
#define XXSEN  28	/* SEND */
#define XXSER  29   	/* SERVER */
#define XXSET  30	/* SET */
#define XXSHE  31	/* Command for SHELL */
#define XXSHO  32	/* SHOW */
#define XXSPA  33	/* (Local) SPACE */
#define XXSTA  34	/* STATISTICS */
#define XXSUB  35	/* (Local) SUBMIT */
#define XXTAK  36	/* TAKE */
#define XXTRA  37	/* TRANSMIT */
#define XXTYP  38	/* (Local) TYPE */
#define XXWHO  39	/* (Local) WHO */
#define XXDIAL 40	/* (Local) DIAL */
#define XXLOGI 41	/* (Local) SCRIPT */
#define XXCOM  42	/* Comment */
#define XXHAN  43       /* HANGUP */
#define XXXLA  44	/* TRANSLATE */
#define XXIF   45	/* IF */
#define  XXIFCO 0       /* IF COUNT */
#define  XXIFER 1       /* IF ERRORLEVEL */
#define  XXIFEX 2       /* IF EXIST */
#define  XXIFFA 3       /* IF FAILURE */
#define  XXIFSU 4       /* IF SUCCESS */
#define  XXIFNO 5       /* IF NOT */
#define  XXIFDE 6       /* IF DEFINED */
#define  XXIFEQ 7	/* IF EQUAL (strings) */
#define  XXIFAE 8       /* IF = (numbers) */
#define  XXIFLT 9       /* IF < (numbers) */
#define  XXIFGT 10      /* IF > (numbers) */
#define XXLBL  46       /* label */
#define XXGOTO 47	/* GOTO */
#define XXEND  48       /* END */
#define XXSTO  49       /* STOP */
#define XXDO   50       /* DO */
#define XXPWD  51       /* PWD */
#define XXTES  52       /* TEST */
#define XXASK  53       /* ASK */
#define XXASKQ 54       /* ASKQ */
#define XXASS  55       /* ASSIGN */
#define XXREI  56       /* REINPUT */
#define XXINC  57       /* INCREMENT */
#define XXDEC  59       /* DECREMENT */
#define XXELS  60       /* ELSE */
#define XXEXE  61	/* EXECUTE */
#define XXWAI  62	/* WAIT */
#define XXVER  63       /* VERSION */
#define XXENA  64       /* ENABLE */
 
/* SET parameters */
 
#define XYBREA  0	/* BREAK simulation */
#define XYCHKT  1	/* Block check type */
#define XYDEBU  2	/* Debugging */
#define XYDELA  3	/* Delay */
#define XYDUPL  4	/* Duplex */
#define XYEOL   5	/* End-Of-Line (packet terminator) */
#define XYESC   6	/* Escape character */
#define XYFILE  7	/* File Parameters */
#define   XYFILN 0  	/*  Naming  */
#define   XYFILT 1  	/*  Type    */
#define   XYFILW 2      /*  Warning */
#define   XYFILD 3      /*  Display */
#define   XYFILC 4      /*  Character set */
/* empty space to add something */
#define XYFLOW  9	/* Flow Control */
#define XYHAND 10	/* Handshake */
#define XYIFD  11	/* Incomplete File Disposition */
#define XYIMAG 12	/* "Image Mode" */
#define XYINPU 13	/* INPUT command parameters */
#define XYLEN  14	/* Maximum packet length to send */
#define XYLINE 15	/* Communication line to use */
#define XYLOG  16	/* Log file */
#define XYMARK 17	/* Start of Packet mark */
#define XYNPAD 18	/* Amount of padding */
#define XYPADC 19	/* Pad character */
#define XYPARI 20	/* Parity */
#define XYPAUS 21	/* Interpacket pause */
#define XYPROM 22	/* Program prompt string */
#define XYQBIN 23	/* 8th-bit prefix */
#define XYQCTL 24	/* Control character prefix */
#define XYREPT 25	/* Repeat count prefix */
#define XYRETR 26	/* Retry limit */
#define XYSPEE 27	/* Line speed (baud rate) */
#define XYTACH 28	/* Character to be doubled */
#define XYTIMO 29	/* Timeout interval */
#define XYMODM 30	/* Modem type */
#define XYSEND 31	/* SEND parameters, used with some of the above */
#define XYRECV 32   	/* RECEIVE parameters, ditto */
#define XYTERM 33	/* Terminal parameters */
#define XYATTR 34       /* Attribute packets */
#define XYSERV 35	/* Server parameters */
#define   XYSERT 0      /*  Server timeout   */
#define XYWIND 36       /* Window size */
#define XYXFER 37       /* Transfer */
#define XYLANG 38       /* Language */
#define XYCOUN 39       /* Count */
#define XYTAKE 40       /* Take */ 
#define XYUNCS 41       /* Unknown-character-set */
#define XYKEY  42       /* Key */
#define XYMACR 43       /* Macro */
#define XYNETW 44       /* Hostname on network */
#define XYNET  45       /* Name of Network */

/* NETWORK designators */

#define NW_DEC = 0			/* DECnet */
#define NW_TCPA = 1			/* TCP/IP with AT&T Streams */
#define NW_TCPB = 2			/* TCP/IP with Berkeley Sockets */

/* SHOW command symbols */

#define SHPAR 0				/* Parameters */
#define SHVER 1				/* Versions */
#define SHCOM 2				/* Communications */
#define SHPRO 3				/* Protocol */
#define SHFIL 4				/* File */
#define SHLNG 5				/* Language */
#define SHCOU 6				/* Count */
#define SHMAC 7				/* Macros */
#define SHKEY 8				/* Key */
#define SHSCR 9				/* Scripts */
#define SHSPD 10			/* Speed */
#define SHSTA 11			/* Status */
#define SHSER 12			/* Server */

/* REMOTE command symbols */
 
#define XZCPY  0	/* Copy */
#define XZCWD  1	/* Change Working Directory */
#define XZDEL  2	/* Delete */
#define XZDIR  3	/* Directory */
#define XZHLP  4	/* Help */
#define XZHOS  5	/* Host */
#define XZKER  6	/* Kermit */
#define XZLGI  7	/* Login */
#define XZLGO  8	/* Logout */
#define XZMAI  9	/* Mail <-- wrong, this should be top-level */
#define XZMOU 10	/* Mount */
#define XZMSG 11	/* Message */
#define XZPRI 12	/* Print */
#define XZREN 13	/* Rename */
#define XZSET 14	/* Set */
#define XZSPA 15	/* Space */
#define XZSUB 16	/* Submit */
#define XZTYP 17	/* Type */
#define XZWHO 18	/* Who */
 
/* SET INPUT command parameters */

#define IN_DEF  0			/* Default timeout */
#define IN_TIM  1			/* Timeout action */
#define IN_CAS  2			/* Case (matching) */
#define IN_ECH  3			/* Echo */

/* ENABLE/DISABLE command parameters */

#define EN_ALL  0			/* All */
#define EN_CWD  1			/* CD/CWD */
#define EN_DIR  2			/* Directory */
#define EN_FIN  3			/* FINISH */
#define EN_GET  4			/* Get */
#define EN_HOS  5			/* Host command */
#define EN_KER  6			/* Kermit command */
#define EN_LOG  7			/* Login */
#define EN_SEN  8			/* Send */
#define EN_SET  9			/* Set */
#define EN_SPA 10			/* Space */
#define EN_TYP 11			/* Type */
#define EN_WHO 12			/* Who/Finger */

/* Symbols for logs */
 
#define LOGD 0	    	/* Debugging */
#define LOGP 1          /* Packets */
#define LOGS 2          /* Session */
#define LOGT 3          /* Transaction */

/* Sizes of things */

#define GVARS 126			/* Highest global var allowed */
#define MAXTAKE 20			/* Maximum nesting of TAKE files */
#define MACLEVEL 20			/* Maximum nesting for macros */
#define NARGS 10			/* Max number of macro arguments */
#define LINBUFSIZ CMDBL+10		/* Size of line[] buffer */
#define INPBUFSIZ 256			/* Size of INPUT buffer */
#define CMDSTKL ( MACLEVEL + MAXTAKE + 1) /* Command stack depth */

struct cmdptr {				/* Command stack structure */
    int src;				/* Command Source */
    int val;				/* Current TAKE or DO level */
};
