/* ckcker.h -- Symbol and macro definitions for C-Kermit */

/*
 Author: Frank da Cruz (fdc@columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 First released January 1985.
 Copyright (C) 1985, 1990, Trustees of Columbia University in the City of New 
 York.  Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained. 
*/

/* Terminal modes */
#define VT100     0         /* Also for VT52 mode */
#define TEKTRONIX 1

#ifdef OS2          /* Keytable return values */
#define ODSR 2
#define IDSR 3
#define OCTS 4
#endif

/* Packet buffer and window sizes, will probably need to be #ifdef'd for */
/* each system. */

#define MAXPACK 94          /* Maximum unextended packet size */
#define MAXSP 2048          /* Send packet buffer size  */
#ifdef vms
#define MAXRP 1920          /* Receive packet buffer size  */
#else
#define MAXRP 2048          /* Receive packet buffer size  */
#endif
#define MAXWS 31            /* Maximum window size */

#define BIGSBUFSIZ (MAXPACK * (MAXWS + 1))
#define BIGRBUFSIZ (MAXPACK * (MAXWS + 1))

/* Kermit parameters and defaults */

#define CTLQ       '#'          /* Control char prefix I will use */
#define MYEBQ      '&'          /* 8th-Bit prefix char I will use */
#define MYRPTQ     '~'          /* Repeat count prefix I will use */

#define MAXTRY      10          /* Times to retry a packet */
#define MYPADN      0           /* How many padding chars I need */
#define MYPADC      '\0'        /* Which padding character I need */

#define DMYTIM      7           /* Default timeout interval to use. */
#define URTIME      10          /* Timeout interval to be used on me. */
#define DSRVTIM     30          /* Default server command wait timeout. */

#define DEFTRN      0           /* Default line turnaround handshake */
#define DEFPAR      0           /* Default parity */
#define MYEOL       CR          /* End-Of-Line character I need on packets. */

#define DRPSIZ      90          /* Default incoming packet size. */
#define DSPSIZ      90          /* Default outbound packet size. */

#define DDELAY      5           /* Default delay. */
#define DSPEED      9600        /* Default line speed. */

/* Files */

#define ZCTERM      0           /* Console terminal */
#define ZSTDIO      1       /* Standard input/output */
#define ZIFILE      2       /* Current input file */
#define ZOFILE      3           /* Current output file */
#define ZDFILE      4           /* Current debugging log file */
#define ZTFILE      5           /* Current transaction log file */
#define ZPFILE      6           /* Current packet log file */
#define ZSFILE      7       /* Current session log file */
#define ZSYSFN      8       /* Input from a system function */
#define ZNFILS      9           /* How many defined file numbers */

/*
 * (PWP) this is used to avoid gratuitous function calls while encoding
 * a packet.  The previous way involved 2 nested function calls for
 * EACH character of the file.  This way, we only do 2 calls per K of
 * data.  This reduces packet encoding time to 1% of its former cost.
 */
#define INBUFSIZE 1024  /* size of the buffered file input and output buffer */

/* get the next char; sorta like a getc() macro */
#define zminchar() (((--zincnt)>=0) ? ((int)(*zinptr++) & 0377) : zinfill())

/* stuff a character into the input buffer */
#define zmstuff(c) zinptr--, *zinptr = c, zincnt++

/* put a character to a file, like putchar() macro */
#define zmchout(c) \
((*zoutptr++=(CHAR)(c)),((++zoutcnt)>=INBUFSIZE)?zoutdump():0)


/* Screen functions */

#define SCR_FN 1        /* filename */
#define SCR_AN 2        /* as-name */
#define SCR_FS 3    /* file-size */
#define SCR_XD 4        /* x-packet data */
#define SCR_ST 5        /* File status: */
#define   ST_OK   0     /*  Transferred OK */
#define   ST_DISC 1     /*  Discarded */
#define   ST_INT  2     /*  Interrupted */
#define   ST_SKIP 3     /*  Skipped */
#define   ST_ERR  4     /*  Fatal Error */
#define SCR_PN 6        /* packet number */
#define SCR_PT 7        /* packet type or pseudotype */
#define SCR_TC 8        /* transaction complete */
#define SCR_EM 9        /* error message */
#define SCR_WM 10       /* warning message */
#define SCR_TU 11   /* arbitrary undelimited text */
#define SCR_TN 12       /* arbitrary new text, delimited at beginning */
#define SCR_TZ 13       /* arbitrary text, delimited at end */
#define SCR_QE 14   /* quantity equals (e.g. "foo: 7") */

/* Macros */

#define tochar(ch)  ((ch) + SP )    /* Number to character */
#define xunchar(ch) ((ch) - SP )    /* Character to number */
#define ctl(ch)     ((ch) ^ 64 )    /* Controllify/Uncontrollify */
#define unpar(ch)   ((ch) & 127)    /* Clear parity bit */

/* Structure definitions for Kermit file attributes */
/* All strings come as pointer and length combinations */
/* Empty string (or for numeric variables, -1) = unused attribute. */

struct zstr {             /* string format */
    int len;              /* length */
    char *val;            /* value */
};
struct zattr {            /* Kermit File Attribute structure */
    long lengthk;         /* (!) file length in K */
    struct zstr type;     /* (") file type (text or binary) */
    struct zstr date;     /* (#) file creation date [yy]yymmdd[ hh:mm[:ss]] */
    struct zstr creator;  /* ($) file creator id */
    struct zstr account;  /* (%) file account */
    struct zstr area;     /* (&) area (e.g. directory) for file */
    struct zstr passwd;   /* (') password for area */
    long blksize;         /* (() file blocksize */
    struct zstr access;   /* ()) file access: new, supersede, append, warn */
    struct zstr encoding; /* (*) encoding (transfer syntax) */
    struct zstr disp;     /* (+) disposition (mail, message, print, etc) */
    struct zstr lprotect; /* (,) protection (local syntax) */
    struct zstr gprotect; /* (-) protection (generic syntax) */
    struct zstr systemid; /* (.) ID for system of origin */
    struct zstr recfm;    /* (/) record format */
    struct zstr sysparam; /* (0) system-dependent parameter string */
    long length;          /* (1) exact length on system of origin */
    struct zstr charset;  /* (2) transfer syntax character set */
    struct zstr reply;    /* This goes last, used for attribute reply */
};

/* Kermit packet information structure */

struct pktinfo {            /* Packet information structure */
    CHAR *bf_adr;           /*  buffer address */
    int   bf_len;           /*  buffer length */
    CHAR *pk_adr;           /* Packet address within buffer */
    int   pk_len;           /*  length of data within buffer */
    int   pk_typ;           /*  packet type */
    int   pk_seq;           /*  packet sequence number */
    int   pk_flg;           /*  ack'd bit */
    int   pk_rtr;           /*  retransmission count */
};
