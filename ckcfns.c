char *fnsv = "C-Kermit functions, 4F(058) 14 Jul 89";

/*  C K C F N S  --  System-independent Kermit protocol support functions.  */

/*  ...Part 1 (others moved to ckcfn2,3 to make this module small enough) */

/*
 Author: Frank da Cruz (fdc@columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 First released January 1985.
 Copyright (C) 1985, 1989, Trustees of Columbia University in the City of New 
 York.  Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained. 
*/
/*
 System-dependent primitives defined in:

   ck?tio.c -- terminal i/o
   cx?fio.c -- file i/o, directory structure
*/
#include "ckcsym.h"			/* Once needed this for Mac... */
#include "ckcasc.h"			/* ASCII symbols */
#include "ckcdeb.h"			/* Debug formats, typedefs, etc. */
#include "ckcker.h"			/* Symbol definitions for Kermit */
#include "ckcxla.h"

#ifndef NULL
#define NULL 0
#endif

/* Externals from ckcmai.c */
extern int spsiz, spmax, rpsiz, timint, srvtim, rtimo, npad, ebq, ebqflg,
 rpt, rptq, rptflg, capas, keep;
extern int pktnum, prvpkt, sndtyp, bctr, bctu, fmask,
 size, osize, maxsize, spktl, nfils, stdouf, warn, timef, spsizf;
extern int parity, speed, turn, turnch, network,
 delay, displa, pktlog, tralog, seslog, xflg, mypadn;
extern long filcnt, ffc, flci, flco, tlci, tlco, tfc, fsize;
extern int tsecs;
extern int spackets, rpackets, timeouts, retrans, crunched, wmax;
extern int deblog, hcflg, binary, savmod, fncnv, local, server, cxseen, czseen;
extern int nakstate;
extern int rq, rqf, sq, wslots, wslotsn, wslotsr, winlo, urpsiz, rln;
extern int atcapr, atcapb, atcapu;
extern int lpcapr, lpcapb, lpcapu;
extern int swcapr, swcapb, swcapu;
extern int bsave, bsavef;
extern int sseqtbl[];
extern int numerrs;
extern int rptn;
extern int tcharset, fcharset;
extern int maxtry, ntcsets;
extern struct csinfo tcsinfo[];
extern CHAR padch, mypadc, eol, seol, ctlq, myctlq, sstate;
extern CHAR filnam[], *recpkt, *data, srvcmd[], padbuf[], stchr, mystch;
extern CHAR encbuf[];
extern CHAR *srvptr;
extern CHAR *rdatap;
extern char *cmarg, *cmarg2, *hlptxt, **cmlist;
char *strcpy();
CHAR *rpar();
long zchki();

/* International character sets */

/* Pointers to translation functions */
extern CHAR (*xlr[MAXTCSETS+1][MAXFCSETS+1])();	
extern CHAR (*xls[MAXTCSETS+1][MAXFCSETS+1])();	
CHAR (*rx)();				/* Input translation function */
CHAR (*sx)();				/* Output translation function */

/* Windowing things */

extern int rseqtbl[];			/* Rec'd-packet sequence # table */

/* (PWP) external def. of things used in buffered file input and output */

extern CHAR zinbuffer[], zoutbuffer[];
extern CHAR *zinptr, *zoutptr;
extern int zincnt, zoutcnt;

/* Variables defined in this module, but shared by ckcfn3, to which */
/* several functions have been moved... */

int sndsrc;				/* Flag for where to send from: */
					/* -1: name in cmdata */
					/*  0: stdin          */
					/* >0: list in cmlist */

int n_len;			/* (PWP) packet encode-ahead length (& flag) */
				/* if < 0, no pre-encoded data */

int  memstr;				/* Flag for input from memory string */

/* Variables local to this module */

static char *memptr;			/* Pointer for memory strings */

static char cmdstr[100];		/* Unix system command string */

static int drain;			/* For draining stacked-up ACKs. */

static int first;			/* Flag for first char from input */
static CHAR t,				/* Current character */
    next;				/* Next character */

#ifdef datageneral
extern int quiet;
#endif

/*  E N C S T R  --  Encode a string from memory. */

/*  Call this instead of getpkt() if source is a string, rather than a file. */

#define ENCBUFL 200
CHAR encbuf[ENCBUFL];			/* Because getpkt always writes */
					/* into "data", but when this */
					/* function is called, "data" might */
					/* might not be allocated. */
encstr(s) char* s; {
    int m; char *p;
    CHAR *dsave;

    if ((m = strlen(s)) > ENCBUFL) {
	debug(F111,"encstr string too long for buffer",s,ENCBUFL);
	s[ENCBUFL] = '\0';
    }
    if (m > spsiz-bctu-3) {
	debug(F111,"encstr string too long for packet",s,spsiz-bctu-3);
	s[spsiz-bctu-3] = '\0';
    }
    m = memstr; p = memptr;		/* Save these. */

    memptr = s;				/* Point to the string. */
    memstr = 1;				/* Flag memory string as source. */
    first = 1;				/* Initialize character lookahead. */
    dsave = data;			/* Boy is this ugly... */
    data = encbuf;			/* Boy is this ugly... */
    getpkt(spsiz-bctu-3);		/* Fill a packet from the string. */
    data = dsave;			/* (sorry...) */
    memstr = m;				/* Restore memory string flag */
    memptr = p;				/* and pointer */
    first = 1;				/* Put this back as we found it. */
}

/* E N C O D E - Kermit packet encoding procedure */

encode(a) CHAR a; {			/* The current character */
    int a7;				/* Low order 7 bits of character */
    int b8;				/* 8th bit of character */

    if (!binary) a = (*sx)(a);		/* Translate. */

    if (rptflg)	{   	    		/* Repeat processing? */
        if (a == next && (first == 0)) { /* Got a run... */
	    if (++rpt < 94)		/* Below max, just count */
                return;
	    else if (rpt == 94) {	/* Reached max, must dump */
                data[size++] = rptq;
                data[size++] = tochar(rpt);
		rptn += rpt;		/* Count, for stats */
                rpt = 0;
	    }
        } else if (rpt == 1) {		/* Run broken, only 2? */
            rpt = 0;			/* Yes, reset repeat flag & count. */
	    encode(a);			/* Do the character twice. */
	    if (size <= maxsize) osize = size;
	    rpt = 0;
	    encode(a);
	    return;
	} else if (rpt > 1) {		/* More than two */
            data[size++] = rptq;	/* Insert the repeat prefix */
            data[size++] = tochar(++rpt); /* and count. */
	    rptn += rpt;
            rpt = 0;			/* Reset repeat counter. */
        }
    }
    a7 = a & 0177;			/* Isolate ASCII part */
    b8 = a & 0200;			/* and 8th (parity) bit. */

    if (ebqflg && b8) {			/* Do 8th bit prefix if necessary. */
        data[size++] = ebq;
        a = a7;
    }
    if ((a7 < SP) || (a7==DEL))	{	/* Do control prefix if necessary */
        data[size++] = myctlq;
	a = ctl(a);
    }
    if (a7 == myctlq)			/* Prefix the control prefix */
        data[size++] = myctlq;

    if ((rptflg) && (a7 == rptq))	/* If it's the repeat prefix, */
        data[size++] = myctlq;		/* quote it if doing repeat counts. */

    if ((ebqflg) && (a7 == ebq))	/* Prefix the 8th bit prefix */
        data[size++] = myctlq;		/* if doing 8th-bit prefixes */

    data[size++] = a;			/* Finally, insert the character */
    data[size] = '\0';			/* itself, and mark the end. */
}

/*  Output functions passed to 'decode':  */

putsrv(c) register char c; { 	/*  Put character in server command buffer  */
    *srvptr++ = c;
    *srvptr = '\0';	/* Make sure buffer is null-terminated */
    return(0);
}

puttrm(c) register char c; {     /*  Output character to console.  */
    conoc(c);
    return(0);
}

putfil(c) register char c; {			/*  Output char to file. */
    if (zchout(ZOFILE, c & fmask) < 0) {
	czseen = 1;   			/* If write error... */
	debug(F101,"putfil zchout write error, setting czseen","",1);
	return(-1);
    }
    return(0);
}

/* D E C O D E  --  Kermit packet decoding procedure */

/* Call with string to be decoded and an output function. */
/* Returns 0 on success, -1 on failure (e.g. disk full).  */

decode(buf,fn) register CHAR *buf; register int (*fn)(); {
    register unsigned int a, a7, b8;	/* Low order 7 bits, and the 8th bit */
    int x;

    rpt = 0;				/* Initialize repeat count. */

    while ((a = *buf++) != '\0') {
	if (rptflg) {			/* Repeat processing? */
	    if (a == rptq) {		/* Yes, got a repeat prefix? */
		rpt = xunchar(*buf++);	/* Yes, get the repeat count, */
		rptn += rpt;
		a = *buf++;		/* and get the prefixed character. */
	    }
	}
	b8 = 0;				/* Check high order "8th" bit */
	if (ebqflg) {			/* 8th-bit prefixing? */
	    if (a == ebq) {		/* Yes, got an 8th-bit prefix? */
		b8 = 0200;		/* Yes, remember this, */
		a = *buf++;		/* and get the prefixed character. */
	    }
	}
	if (a == ctlq) {		/* If control prefix, */
	    a  = *buf++;		/* get its operand. */
	    a7 = a & 0177;		/* Only look at low 7 bits. */
	    if ((a7 >= 0100 && a7 <= 0137) || a7 == '?') /* Uncontrollify */
	      a = ctl(a);		/* if in control range. */
	}
	a |= b8;			/* OR in the 8th bit */
	if (rpt == 0) rpt = 1;		/* If no repeats, then one */
#ifdef NLCHAR
	if (!binary) {			/* If in text mode, */
	    if (a == CR) continue;	/* discard carriage returns, */
    	    if (a == LF) a = NLCHAR; 	/* convert LF to system's newline. */
    	}
#endif
	if (!binary) a = (*rx)(a);	/* Translate */

	if (fn == putfil) { /* (PWP) speedup via buffered output and a macro */
	    for (; rpt > 0; rpt--) {	/* Output the char RPT times */
		if ((x = zmchout(a)) < 0) { /* zmchout is a macro */
		    debug(F101,"decode zmchout","",x);
		    return(-1);
		}
		ffc++;			/* Count the character */
	    }
	} else {			/* Not to the file */
	    for (; rpt > 0; rpt--) {	/* Output the char RPT times */
		if ((*fn)(a) < 0) return(-1); /* Send to output function. */
	    }
	}
    }
    return(0);
}

/*  G E T P K T -- Fill a packet data field  */

/*
 Gets characters from the current source -- file or memory string.
 Encodes the data into the packet, filling the packet optimally.
 Set first = 1 when calling for the first time on a given input stream
 (string or file).

 Uses global variables:
 t     -- current character.
 first -- flag: 1 to start up, 0 for input in progress, -1 for EOF.
 next  -- next character.
 data  -- pointer to the packet data buffer.
 size  -- number of characters in the data buffer.
 memstr - flag that input is coming from a memory string instead of a file.
 memptr - pointer to string in memory.
 (*sx)()  character set translation function

Returns the size as value of the function, and also sets global "size",
and fills (and null-terminates) the global data array.  Returns 0 upon eof.

Rewritten by Paul W. Placeway (PWP) of Ohio State University, March 1989.
Incorporates old getchx() and encode() inline to eliminate function calls,
uses buffered input for much-improved efficiency, and clears up some
confusion with line termination (CRLF vs LF vs CR).
*/

getpkt(bufmax) int bufmax; {		/* Fill one packet buffer */
    register CHAR rt = t, rnext = next; /* register shadows of the globals */
    register CHAR *dp, *odp, *p1, *p2;	/* pointers... */
    register int x;			/* Loop index. */
    register int a7;			/* Low 7 bits of character */
    static CHAR leftover[6] = { '\0', '\0', '\0', '\0', '\0', '\0' };

    if (first == 1) {		/* If first time thru...  */
	first = 0;		/* remember, */
	*leftover = '\0';   	/* discard any interrupted leftovers, */
	/* get first character of file into t, watching out for null file */
	if (memstr) {
	    if ((rt = *memptr++) == '\0') { /* end of string ==> EOF */
		first = -1;
	        size = 0;
		debug(F100,"getpkt: empty string","",0);
		return (0);
	    }
	} else {
	    if ((x = zminchar()) == -1) { /* End of file */
		first = -1;
		debug(F100,"getpkt: empty file","",0);
	        size = 0;
		return(0);
	    }
	    ffc++;			/* Count a file character */
	    rt = x;
	    debug(F101,"getpkt zminchar","",rt);
	}
	debug(F101,"getpkt about to call translate function","",rt);
	if (!binary) rt = (*sx)(rt);	/* Translate */
	debug(F101," translate function returns","",rt);
	rt &= fmask;			/* bytesize mask */

	/* PWP: handling of NLCHAR is done later (in the while loop)... */

    } else if ((first == -1) && (*leftover == '\0')) /* EOF from last time? */
        return(size = 0);

    /* Do any leftovers */

    dp = data;
    for (p1 = leftover; (*dp = *p1) != '\0'; p1++, dp++) /* copy leftovers */
    	;
    *leftover = '\0';
    if (first == -1) 
      return(size = (dp - data));	/* Handle final leftovers */
  
    /* Now fill up the rest of the packet. */
    rpt = 0;				/* Clear out any old repeat count. */
    while (first > -1) {		/* Until EOF... */
	if (memstr) {			/* get next character */
	    if ((rnext = *memptr++) == '\0') { /* end of string ==> EOF */
		first = -1;		/* Flag eof for next time. */
	    } else {
		if (!binary) rnext = (*sx)(rnext); /* Translate */
		rnext &= fmask;		/* Bytesize mask. */
	    }
	} else {
	    if ((x = zminchar()) == -1) { /* End of file */
		first = -1;		/* Flag eof for next time. */
	    } else {
		if (!binary) x = (*sx)(x); /* Translate */
		rnext = x & fmask;	/* Bytesize mask. */
		ffc++;			/* Count it */
	    }
	}

	/* PWP: handling of NLCHAR is done in a bit...  */

	odp = dp;			/* Remember current position. */

	/* PWP: the encode() procedure, in-line (for speed) */
	if (rptflg) {			/* Repeat processing? */
	    if (
#ifdef NLCHAR
		/*
		 * PWP: this is a bit esoteric, so bear with me.
		 * If the next char is really CRLF, then we cannot
		 * be doing a repeat (unless CR,CR,LF which becomes
		 * "~ <n-1> CR CR LF", which is OK but not most efficient).
		 * I just plain don't worry about this case.  The actual
		 * conversion from NL to CRLF is done after the rptflg if...
		 */
	    (binary || (rnext != NLCHAR)) &&
#endif /* NLCHAR */
	    rt == rnext && (first == 0)) { /* Got a run... */
		if (++rpt < 94) {	/* Below max, just count */
		    continue;		/* go back and get another */
		}
		else if (rpt == 94) {	/* Reached max, must dump */
		    *dp++ = rptq;
		    *dp++ = tochar(rpt);
		    rptn += rpt;
		    rpt = 0;
		}
	    } else if (rpt == 1) {	/* Run broken, only 2? */
		/* 
		 * PWP: Must encode two characters.  This is handled
		 * later, with a bit of blue smoke and mirrors, after
		 * the first character is encoded.
		 */
	    } else if (rpt > 1) {	/* More than two */
		*dp++ = rptq;		/* Insert the repeat prefix */
		*dp++ = tochar(++rpt);	/* and count. */
		rptn += rpt;
		rpt = 0;		/* Reset repeat counter. */
	    }
	}

#ifdef NLCHAR
	/*
	 * PWP: even more esoteric NLCHAR handling.  Remember, at
	 * this point t may still be the _system_ NLCHAR.  If so,
	 * we do it here.
	 */
	if (!binary && (rt == NLCHAR)) {
	    *dp++ = myctlq;		/* just put in the encoding directly */
	    *dp++ = 'M';		/* == ctl(CR) */
	    if ((dp-data) <= maxsize) odp = dp;	/* check packet bounds */
	    rt = LF;
	}
#endif

	/* meta control stuff fixed by fdc */
	a7 = rt & 0177;			/* Low 7 bits of character */
	if (ebqflg && (rt & 0200)) {	/* Do 8th bit prefix if necessary. */
	    *dp++ = ebq;
	    rt = a7;
	}
	if ((a7 < SP) || (a7 == DEL)) { /* Do control prefix if necessary */
	    *dp++ = myctlq;
	    rt = ctl(rt);
	}
	if (a7 == myctlq)		/* Prefix the control prefix */
	    *dp++ = myctlq;

	if ((rptflg) && (a7 == rptq))	/* If it's the repeat prefix, */
	    *dp++ = myctlq;		/* quote it if doing repeat counts. */

	if ((ebqflg) && (a7 == ebq))	/* Prefix the 8th bit prefix */
	    *dp++ = myctlq;		/* if doing 8th-bit prefixes */

	*dp++ = rt;			/* Finally, insert the character */
	
	if (rpt == 1) {			/* Exactly two copies? */
	    rpt = 0;
	    p2 = dp;			/* save current size temporarily */
	    for (p1 = odp; p1 < p2; p1++) /* copy the old chars over again */
		*dp++ = *p1;
	    if ((p2-data) <= maxsize) odp = p2; /* check packet bounds */
	}
	rt = rnext;			/* Next is now current. */
	if ((dp-data) >= bufmax) {	/* If too big, save some for next. */
	    size = (dp-data);
	    *dp = '\0';			/* mark (current) the end. */
	    if ((dp-data) > bufmax) {	/* if packet is overfull */
		for (p1 = leftover, p2=odp; (*p1 = *p2) != '\0'; p1++,p2++)
		    ;
		debug(F111,"getpkt leftover",leftover,size);
		debug(F101," osize","",(odp-data));
		size = (odp-data);	/* Return truncated packet. */
		*odp = '\0';		/* mark real end */
	    } else {			/* If the packet is exactly full, */
		debug(F101,"getpkt exact fit","",size);
	    }
	    t = rt; next = rnext;	/* save for next time */
	    return(size);
	}
    }					/* Otherwise, keep filling. */
    size = (dp-data);
    *dp = '\0';				/* mark (current) the end. */
    debug(F111,"getpkt eof/eot",data,size); /* Fell thru before packet full, */
    return(size);		     /* return partially filled last packet. */
}

/*  T I N I T  --  Initialize a transaction  */

extern struct pktinfo s_pkt[];		/* arrays of pktinfo structures */
extern struct pktinfo r_pkt[];		/* for sent and received packets */

tinit() {
    int x; CHAR *y;

    rx = xlr[tcharset][fcharset];	/* Input translation function */
    sx = xls[tcharset][fcharset];	/* Output translation function */

    xflg = 0;				/* Reset x-packet flag */
    rqf = -1;				/* Reset 8th-bit-quote request flag */
    memstr = 0;				/* Reset memory-string flag */
    memptr = NULL;			/*  and pointer */
    n_len = -1;				/* No encoded-ahead data */
    bctu = 1;				/* Reset block check type to 1 */
    ebq = ebqflg = 0;			/* Reset 8th-bit quoting stuff */
    if (savmod) {			/* If global file mode was saved, */
    	binary = 1;			/*  restore it, */
	savmod = 0;			/*  unsave it. */
    }
    pktnum = 0;				/* Initial packet number */
    cxseen = czseen = 0;		/* Reset interrupt flags */
    *filnam = '\0';			/* Clear file name */
    spktl = 0;				/* And its length */
    nakstate = 0;			/* Assume not in a NAK'ing state */
    numerrs = 0;			/* Transmission error counter */
    timeouts = retrans = 0;		/* Statistics counters */
    spackets = rpackets = 0;		/*  ... */
    crunched = 0;			/*  ... */
    wmax = 1;				/*  ... */
    if (server) 			/* If acting as server, */
      timint = srvtim;			/* Use server timeout interval. */
    wslots = 1;				/* One window slot */
    winlo = 0;				/* Packet 0 is at window-low */
    x = mksbuf(wslots);			/* Make a 1-slot send-packet buffer */
    if (x < 0) return(x);
    x = getsbuf(0);			/* Allocate first send-buffer. */
    if (x < 0) return(x);
    debug(F100,"tinit getsbuf","",0);
    dumpsbuf();
    x = mkrbuf(1);			/* & a 1-slot receive-packet buffer. */
    if (x < 0) return(x);
    return(0);
}

pktinit() {				/* Initialize packet sequence */
    pktnum = 0;				/* number & window low. */
    winlo = 0;
}

/*  R I N I T  --  Respond to S or I packet  */

rinit(d) char *d; {
    char *tp;
    ztime(&tp);
    tlog(F110,"Transaction begins",tp,0l); /* Make transaction log entry */
    if (binary)
      tlog(F100,"Global file mode = binary","",0l);
    else
      tlog(F100,"Global file mode = text","",0l);
    filcnt = 0;				/* Init file counter */
    spar(d);
    ack1(rpar());
#ifdef datageneral
    if ((local) && (!quiet))            /* Only do this if local & not quiet */
        consta_mt();                    /* Start the asynch read task */
#endif
}


/*  R E S E T C  --  Reset per-transaction character counters */

resetc() {
    rptn = 0;				/* Repeat counts */
    flci = flco = 0;
    tfc = tlci = tlco = 0;	/* Total file chars, line chars in & out */
}

/*  S I N I T  --  Make sure file exists, then send Send-Init packet */

sinit() {
    int x; char *tp;

    filcnt = 0;
    sndsrc = nfils;			/* Where to look for files to send */

    ztime(&tp);
    tlog(F110,"Transaction begins",tp,0l); /* Make transaction log entry */
    debug(F101,"sinit: sndsrc","",sndsrc);
    if (sndsrc < 0) {			/* Must expand from 'send' command */
#ifdef DTILDE
	char *tnam, *tilde_expand();	/* May have to expand tildes */
	tnam = tilde_expand(cmarg);	/* Try to expand tilde. */
	if (*tnam != '\0') cmarg = tnam;
#endif
	nfils = zxpand(cmarg);		/* Look up literal name. */
	if (nfils < 0) {
	    if (server)
	      errpkt("Too many files");
	    else
	      screen(SCR_EM,0,0l,"Too many files");
	    return(0);
        } else if (nfils == 0) {	/* If none found, */
	    char xname[100];		/* convert the name. */
	    zrtol(cmarg,xname);
	    nfils = zxpand(xname); 	/* Look it up again. */
	}
	if (nfils < 1) {		/* If no match, report error. */
	    if (server) 
	    	errpkt("File not found");
	    else
		screen(SCR_EM,0,0l,"File not found");
	    return(0);
	}
	x = gnfile();			/* Position to first file. */
	if (x < 1) {
	    if (!server) 
	    	screen(SCR_EM,0,0l,"No readable file to send");
            else
	    	errpkt("No readable file to send");
	    return(0);
    	} 
    } else if (sndsrc > 0) {		/* Command line arglist -- */
	x = gnfile();			/* Get the first file from it. */
	if (x < 1) return(0);		/* (if any) */
    } else if (sndsrc == 0) {		/* stdin or memory always exist... */
	if ((cmarg2 != NULL) && (*cmarg2)) {
	    strcpy(filnam,cmarg2);	/* If F packet, "as" name is used */
	    cmarg2 = "";		/* if provided, */
        } else				/* otherwise */
	    strcpy(filnam,"stdin");	/* just use this. */
    }
    debug(F101,"sinit: nfils","",nfils);
    debug(F110," filnam",filnam,0);
    debug(F110," cmdstr",cmdstr,0);
    ttflui();				/* Flush input buffer. */
    if (!local && !server) sleep(delay);
#ifdef datageneral
    if ((local) && (!quiet))            /* Only do this if local & not quiet */
        consta_mt();                    /* Start the asynch read task */
#endif
    freerbuf(rseqtbl[0]);		/* Free the buffer the GET came in. */
    sipkt('S');				/* Send the Send-Init packet. */
    return(1);
}

sipkt(c) char c; {			/* Send S or I packet. */
    CHAR *rp;
    ttflui();				/* Flush pending input. */
    rp = rpar();			/* Get parameters. */
    spack(c,pktnum,strlen(rp),rp);
}

/*  R C V F I L -- Receive a file  */

/*
  Incoming filename is in data field of F packet.
  This function decodes it into the srvcmd buffer, substituting an
  alternate "as-name", if one was given.
  Finally, it does any requested transformations (like converting to
  lowercase) then if a file of the same name already exists, makes
  a new unique name.
*/
rcvfil(n) char *n; {
    char xname[100], *xp;		/* Buffer for constructing name */
#ifdef DTILDE
    char *dirp, *tilde_expand();
#endif

    srvptr = srvcmd;			/* Decode file name from packet. */
    decode(rdatap,putsrv);
    if (*srvcmd == '\0')		/* Watch out for null F packet. */
    	strcpy(srvcmd,"NONAME");
#ifdef DTILDE
    dirp = tilde_expand(srvcmd);	/* Expand tilde, if any. */
    if (*dirp != '\0') strcpy(srvcmd,dirp);
#endif
    screen(SCR_FN,0,0l,srvcmd);		/* Put it on screen */
    tlog(F110,"Receiving",srvcmd,0l);	/* Transaction log entry */
    if (cmarg2 != NULL) {               /* Check for alternate name */
        if (*cmarg2 != '\0') {
            strcpy(srvcmd,cmarg2);	/* Got one, use it. */
	    *cmarg2 = '\0';
        }
    }
    xp = xname;				/* OK to proceed. */
    if (fncnv)				/* If desired, */
    	zrtol(srvcmd,xp);		/* convert name to local form */
    else				/* otherwise, */
    	strcpy(xname,srvcmd);		/* use it literally */

    if (warn) {				/* File collision avoidance? */
	if (zchki(xname) != -1) {	/* Yes, file exists? */
	    znewn(xname,&xp);		/* Yes, make new name. */
	    strcpy(xname,xp);
	    debug(F110," exists, new name ",xname,0);
        }
    }
    debug(F110,"rcvfil: xname",xname,0);
    strcpy(n,xname);			/* Return pointer to actual name. */
    debug(F110,"rcvfil: n",n,0);
    ffc = 0;				/* Init per-file counters */
    filcnt++;
    return(1);				/* Always succeeds */
}

/*  R E O F  --  Receive End Of File  */

reof(f,yy) char *f; struct zattr *yy; {
    int x;
    char *p;
    char c;

    if (cxseen == 0) cxseen = (*rdatap == 'D');	/* Got discard directive? */
    x = clsof(cxseen | czseen);
    if (atcapu) zstime(f,yy);		/* Set file creation date */
    if (cxseen || czseen) {
	tlog(F100," *** Discarding","",0l);
	cxseen = 0;
    } else {
	fstats();

/* Handle dispositions from attribute packet... */

	if (yy->disp.len != 0) {
	    p = yy->disp.val;
	    c = *p++;
	    if (c == 'M') {		/* Mail to user. */
		zmail(p,filnam);	/* Do the system's mail command */
		tlog(F110,"mailed",filnam,0l);
		tlog(F110," to",p,0l);
		zdelet(filnam);		/* Delete the file */
	    } else if (c == 'P') {	/* Print the file. */
		zprint(p,filnam);	/* Do the system's print command */
		tlog(F110,"printed",filnam,0l);
		tlog(F110," with options",p,0l);
		zdelet(filnam);		/* Delete the file */
	    }
	}
    }
    *filnam = '\0';
    return(x);
}

/*  R E O T  --  Receive End Of Transaction  */

reot() {
    cxseen = czseen = 0;		/* Reset interruption flags */
    tstats();
}

/*  S F I L E -- Send File header or teXt header packet  */

/*  Call with x nonzero for X packet, zero for F packet  */
/*  Returns 1 on success, 0 on failure                   */

sfile(x) int x; {
    char pktnam[100];			/* Local copy of name */
    char *s;

    if (nxtpkt() < 0) return(0);	/* Bump packet number, get buffer */
    if (x == 0) {			/* F-Packet setup */

    	if (*cmarg2 != '\0') {		/* If we have a send-as name, */
	    strcpy(pktnam,cmarg2);	/* copy it literally, */
	    cmarg2 = "";		/* and blank it out for next time. */
    	} else {			/* Otherwise use actual file name: */
	    if (fncnv) {		/* If converting names, */
	    	zltor(filnam,pktnam);	/* convert it to common form, */
	    } else {			/* otherwise, */
	    	strcpy(pktnam,filnam);	/* copy it literally. */
            }
    	}
    	debug(F110,"sfile",filnam,0);	/* Log debugging info */
    	debug(F110," pktnam",pktnam,0);
    	if (openi(filnam) == 0) 	/* Try to open the file */
	    return(0); 		
    	s = pktnam;			/* Name for packet data field */

    } else {				/* X-packet setup */

    	debug(F110,"sxpack",cmdstr,0);	/* Log debugging info */
    	s = cmdstr;			/* Name for data field */
    }

    encstr(s);				/* Encode the name into encbuf[]. */
    spack(x ? 'X' : 'F', pktnum, size, encbuf); /* Send the F or X packet */

    if (x == 0) {			/* Display for F packet */
    	if (displa) {			/* Screen */
	    screen(SCR_FN,'F',(long)pktnum,filnam);
	    screen(SCR_AN,0,0l,pktnam);
	    screen(SCR_FS,0,(long)fsize,"");
    	}
    	tlog(F110,"Sending",filnam,0l);	/* Transaction log entry */
    	tlog(F110," as",pktnam,0l);

    } else {				/* Display for X-packet */

    	screen(SCR_XD,'X',(long)pktnum,cmdstr);	/* Screen */
    	tlog(F110,"Sending from:",cmdstr,0l);	/* Transaction log */
    }
    intmsg(++filcnt);			/* Count file, give interrupt msg */
    first = 1;				/* Init file character lookahead. */
    n_len = -1;			/* (PWP) Init the packet encode-ahead length */
    ffc = 0;				/* Init file character counter. */
    return(1);
}

/*  S D A H E A D -- (PWP) Encode the next data packet to send */

sdahead() {    
    if (spsiz > MAXPACK)	     /* see the logic in ckcfn2.c, spack() */
	n_len = getpkt(spsiz-bctu-5);	/* long packet size */
    else
	n_len = getpkt(spsiz-bctu-2);	/* short packet size */
}

/*  S D A T A -- Send a data packet */

/*  Return -1 if no data to send, else send packet and return length  */

sdata() {
    int i,p,len;
    int xx, yy;
    
    debug(F101,"sdata entry, first","",first);
    debug(F101," drain","",drain);

    if (first == 1) drain = 0;		/* Try to explain this... */
    if (drain) {
	debug(F101,"sdata draining, winlo","",winlo);
	debug(F101," winlo","",winlo);
	if (winlo == pktnum) return(-1); else return(0);
    }

    for (i = 0; i < wslots; i++) {	/* Send as many as possible. */
	int k,x;
	CHAR *y;

	x = nxtpkt();			/* Get next pkt number and buffer */
	debug(F101,"sdata packet","",pktnum);
	if (x < 0) {
#ifdef COMMENT
	    debug(F101,"sdata nxtpkt failed, resending","",winlo);
	    x = resend(winlo);		/* Try to resend window-low. */
	    if (x < 0) doexit(BAD_EXIT); /* Fatal if can't. */
#endif
	    return(0);
	}
	dumpsbuf();

	if (cxseen || czseen) {		/* If interrupted, done. */
	    return(-1);
	}	
	if (spsiz > 94)			/* Fill a packet data buffer */
	  len = getpkt(spsiz-bctu-6);	/* long packet */
	else				/*  or */
	  len = getpkt(spsiz-bctu-3);	/* short packet */
	if (len == 0) {			/* Done if no data. */
	    if (pktnum == winlo) return(-1);
	    drain = 1;			/* But can't return -1 until all */
	    debug(F101,"sdata eof, drain","",drain);
	    return(0);			/* ACKs are drained. */
	}
	spack('D',pktnum,len,data);	/* Send the packet */
	x = ttchk();
	debug(F101,"sdata ttchk","",x);	/* Any ACKs waiting? */
	if (x) return(0);		/* ??? */
    }    

/* comment out next statement if it causes problems... */
#ifdef COMMENT
    if (len > 0)			/* if we got data last time */
	sdahead();			/* encode more now */
#endif
    return(len);
}


/*  S E O F -- Send an End-Of-File packet */

/*  Call with a string pointer to character to put in the data field, */
/*  or else a null pointer or "" for no data.  */

seof(s) char *s; {

    /* NOTE, we don't call nextpkt() here because it was already called */
    /* by sdata() when it got the end of file condition, and so did not */
    /* use the packet number it had just allocated. */

    if ((s != NULL) && (*s != '\0')) {
	spack('Z',pktnum,1,s);
	tlog(F100," *** interrupted, sending discard request","",0l);
    } else {
	spack('Z',pktnum,0,"");
	fstats();
    }
}

/*
  Special version of seof() to be called when sdata() has not been called
  before.  The difference is that this version calls nxpkt().  For example,
  when a file that is about to be sent has been refused by the attribute
  mechanism.
*/

sxeof(s) char *s; {
    int x;
    x = nxtpkt();			/* Get next pkt number and buffer */
    if (x < 0)
      debug(F101,"sxeof nxtpkt fails","",pktnum);
    else
      debug(F101,"sxeof packet","",pktnum);
    seof(s);
}

/*  S E O T -- Send an End-Of-Transaction packet */

seot() {
    if (nxtpkt() < 0) return(-1);	/* Bump packet number, get buffer */
    spack('B',pktnum,0,"");		/* Send the EOT packet */
    cxseen = czseen = 0;		/* Reset interruption flags */
    tstats();				/* Log timing info */
    return(0);
}

/*   R P A R -- Fill the data array with my send-init parameters  */


static CHAR dada[20];			/* Use this instead of data[]. */
					/* To avoid some kind of wierd */
					/* addressing foulup in spack()... */

CHAR *
rpar() {
    int cps;

/* The following bit is in case user's timeout is shorter than the amount */
/* of time it would take to receive a packet of the negotiated size. */

    if (speed > 0 && !network) {	/* First recompute timeout */
	cps = speed / 10;		/* Characters per second */
	if (cps * rtimo < spsiz) {
	    rtimo = (spsiz / cps) + 1;
	    debug(F101,"rpar new rtimo","",rtimo);
	}
    }
    if (rpsiz > MAXPACK)		/* Biggest normal packet I want. */
      dada[0] = tochar(MAXPACK);	/* If > 94, use 94, but specify */
    else				/* extended packet length below... */
      dada[0] = tochar(rpsiz);		/* else use what the user said. */
    dada[1] = tochar(rtimo);		/* When I want to be timed out */
    dada[2] = tochar(mypadn);		/* How much padding I need (none) */
    dada[3] = ctl(mypadc);		/* Padding character I want */
    dada[4] = tochar(eol);		/* End-Of-Line character I want */
    dada[5] = '#';			/* Control-Quote character I send */
    switch (rqf) {			/* 8th-bit prefix */
	case -1:
	case  1: if (parity) ebq = sq = '&'; break;
	case  0:
	case  2: break;
    }
    dada[6] = sq;
    dada[7] = bctr + '0';		/* Block check type */
    if (rptflg)				/* Run length encoding */
    	dada[8] = rptq;			/* If receiving, agree. */
    else
    	dada[8] = '~'; 		

    dada[9] = tochar((atcapr?atcapb:0)|(lpcapr?lpcapb:0)|(swcapr?swcapb:0));
    dada[10] = tochar(swcapr ? wslotsr : 0); /* Window size */
    rpsiz = urpsiz;			/* Long packets ... */
    dada[11] = tochar(rpsiz / 95);	/* Long packet size, big part */
    dada[12] = tochar(rpsiz % 95);	/* Long packet size, little part */
    dada[13] = '\0';			/* Terminate the init string */
    if (deblog) {
	debug(F110,"rpar",dada,0);
	rdebu(strlen(dada));
    }
    return(dada);			/* Return pointer to string. */
}

spar(s) char *s; {			/* Set parameters */
    int x, y, cps, lpsiz;

debug(F110,"entering spar",s,0);

    s--;				/* Line up with field numbers. */

/* Limit on size of outbound packets */
    x = (rln >= 1) ? xunchar(s[1]) : 80;
    lpsiz = spsiz;			/* Remember what they SET. */
    if (spsizf) {			/* SET-command override? */
	if (x < spsiz) spsiz = x;	/* Ignore LEN unless smaller */
    } else {				/* otherwise */
	spsiz = (x < 10) ? 80 : x;	/* believe them if reasonable */
    }

/* Timeout on inbound packets */
    if (!timef) {			/* Only if not SET-cmd override */
	x = (rln >= 2) ? xunchar(s[2]) : 5;
	timint = (x < 0) ? 5 : x;
    }
    if (speed > 0 && !network) {	/* If comm line speed known, */
	cps = speed / 10;		/* adjust for packet length */
	if (cps * timint < urpsiz) {	/* if timeout to short for packet. */
	    timint = (urpsiz / cps) + 1;
	    debug(F101,"spar new timint","",timint);
	}
    }

/* Outbound Padding */
    npad = 0; padch = '\0';
    if (rln >= 3) {
	npad = xunchar(s[3]);
	if (rln >= 4) padch = ctl(s[4]); else padch = 0;
    }
    if (npad) {
	int i;
	for (i = 0; i < npad; i++) padbuf[i] = dopar(padch);
    }

/* Outbound Packet Terminator */
    seol = (rln >= 5) ? xunchar(s[5]) : '\r';
    if ((seol < 2) || (seol > 31)) seol = '\r';

/* Control prefix */
    x = (rln >= 6) ? s[6] : '#';
    myctlq = ((x > 32 && x < 63) || (x > 95 && x < 127)) ? x : '#';

/* 8th-bit prefix */
    rq = (rln >= 7) ? s[7] : 0;
    if (rq == 'Y') rqf = 1;
      else if ((rq > 32 && rq < 63) || (rq > 95 && rq < 127)) rqf = 2;
        else rqf = 0;
    switch (rqf) {
	case 0: ebqflg = 0; break;
	case 1: if (parity) { ebqflg = 1; ebq = '&'; } break;
	case 2: if (ebqflg = (ebq == sq || sq == 'Y')) ebq = rq;
    }

/* Block check */
    x = 1;
    if (rln >= 8) {
	x = s[8] - '0';
	if ((x < 1) || (x > 3)) x = 1;
    }
    bctr = x;

/* Repeat prefix */
    if (rln >= 9) {
	rptq = s[9]; 
	rptflg = ((rptq > 32 && rptq < 63) || (rptq > 95 && rptq < 127));
    } else rptflg = 0;

/* Capabilities */
    atcapu = lpcapu = swcapu = 0;
    if (rln >= 10) {
        x = xunchar(s[10]);
	debug(F101,"spar capas","",x);
        atcapu = (x & atcapb) && atcapr;
	lpcapu = (x & lpcapb) && lpcapr;
	swcapu = (x & swcapb) && swcapr;
	debug(F101,"spar swcapr","",swcapr);
	debug(F101,"spar swcapu","",swcapu);
	for (y = 10; (xunchar(s[y]) & 1) && (rln >= y); y++) ;
	debug(F101,"spar y","",y);
    }

/* Long Packets */
    if (lpcapu) {
        if (rln > y+1) {
	    x = xunchar(s[y+2]) * 95 + xunchar(s[y+3]);
	    if (spsizf) {		/* If overriding negotiations */
		spsiz = (x < lpsiz) ? x : lpsiz; /* do this, */
	    } else {			         /* otherwise */
		spsiz = (x > MAXSP) ? MAXSP : x; /* do this. */
	    }
	    if (spsiz < 10) spsiz = 80;	/* Be defensive... */
	}
    }
    /* (PWP) save current send packet size for optimal packet size calcs */
    spmax = spsiz;
    
/* Sliding Windows... */
/* This should be fixed to settle on the lower of what the user asked for */
/* in the 'set window' command and how many slots the other Kermit said.  */

    if (swcapr) {			/* Only if requested... */
        if (rln > y) {			/* See what other Kermit says */
	    x = xunchar(s[y+1]);
	    debug(F101,"spar window","",x);
	    wslotsn = x > MAXWS ? MAXWS : x;
	    if (wslotsn > wslotsr) wslotsn = wslotsr;
	    if (wslotsn > 1) swcapu = 1; /* We do windows... */
	}
	else {
	    wslotsn = 1;		/* We don't do windows... */
	    swcapu = 0;
	    debug(F101,"spar no windows","",wslotsn);
	}
    }

/* Now recalculate packet length based on number of windows.   */
/* The nogotiated number of window slots will be allocated,    */
/* and the maximum packet length will be reduced if necessary, */
/* so that a windowful of packets can fit in the big buffer.   */

    if (wslotsn > 1) {			/* Shrink to fit... */
	x = ( BIGSBUFSIZ / wslotsn ) - 1;
	if (x < spsiz) {
	    spsiz = spmax = x;
	    debug(F101,"spar sending, redefine spsiz","",spsiz);
	}
    }

/* Record parameters in debug log */
    if (deblog) sdebu(rln);
    numerrs = 0;			/* Start counting errors here. */
    return(0);
}

/*  G N F I L E  --  Get the next file name from a file group.  */

/*  Returns 1 if there's a next file, 0 otherwise  */

gnfile() {
    int x; long y;
#ifdef datageneral
    int dgfiles = 0;     		/* Number of files expanded */
    static int dgnfils = -1;		/* Saved nfils value */
#endif

/* If file group interruption (C-Z) occured, fail.  */

    debug(F101,"gnfile: czseen","",czseen);

    if (czseen) {
	tlog(F100,"Transaction cancelled","",0l);
	return(0);
    }

/* If input was stdin or memory string, there is no next file.  */

    if (sndsrc == 0) return(0);

/* If file list comes from command line args, get the next list element. */

    y = -1;
    while (y < 0) {			/* Keep trying till we get one... */

	if (sndsrc > 0) {
	    if (nfils-- > 0) {

#ifdef datageneral
/* The DG does not internally expand the file names when a string of */
/* filenames is given. So we must in this case. */
                if (dgnfils == -1) {   /* This is executed first time only! */
                    strcpy(filnam,*cmlist++);
	            dgfiles = zxpand(filnam); /* Expand the string */
                    debug(F111,"gnfile:cmlist filnam (x-init)",filnam,dgfiles);
                    dgnfils = nfils + 1;
                    debug(F111,"gnfile:cmlist filnam (x-init)",filnam,dgnfils);
	         } 
	         x = znext(filnam);
	         if (x > 0) {      /* Get the next file in the expanded list */
	            debug(F111,"gnfile znext: filnam (exp-x=)",filnam,x);
                    nfils = dgnfils;     /* The virtual number of files left */
                 }
	         if (x == 0) {     /* Expand the next command argument */
	            if (dgnfils == 1) {
	                 dgnfils = -1;   /* The last argument resets things */
	                 *filnam = '\0';
	                 debug(F101,"gnfile cmlist: nfils","",nfils);
	                 return(0);
	            }
	          
                    strcpy(filnam,*cmlist++);  /* Finish expanding last arg */
	            dgfiles = zxpand(filnam); /* Expand the string */
                    debug(F111,"gnfile: cmlist filnam (exp)",filnam,dgnfils);
	            x = znext(filnam);
	            debug(F111,"gnfile znext: filnam (exp)",filnam,x);
	            nfils = dgnfils--;
		} 
#else
		strcpy(filnam,*cmlist++);
		debug(F111,"gnfile: cmlist filnam",filnam,nfils);
#endif
	    } else {
		*filnam = '\0';
		debug(F101,"gnfile cmlist: nfils","",nfils);
		return(0);
	    }
	}

/* Otherwise, step to next element of internal wildcard expansion list. */

	if (sndsrc < 0) {
	    x = znext(filnam);
	    debug(F111,"gnfile znext: filnam",filnam,x);
	    if (x == 0) return(0);
	}

/* Get here with a filename. */

	y = zchki(filnam);		/* Check if file readable */
	if (y < 0) {
	    debug(F110,"gnfile skipping:",filnam,0);
	    tlog(F111,filnam,"not sent, reason",(long)y);
	    screen(SCR_ST,ST_SKIP,0l,filnam);
	} else fsize = y;
    }    	
    return(1);
}

/*  S N D H L P  --  Routine to send builtin help  */

sndhlp() {
    nfils = 0;				/* No files, no lists. */
    xflg = 1;				/* Flag we must send X packet. */
    strcpy(cmdstr,"help text");		/* Data for X packet. */
    first = 1;				/* Init getchx lookahead */
    memstr = 1;				/* Just set the flag. */
    memptr = hlptxt;			/* And the pointer. */
    if (binary) {			/* If file mode is binary, */
	binary = 0;			/*  turn it back to text for this, */
	savmod = 1;			/*  remember to restore it later. */
    }
    return(sinit());
}


/*  C W D  --  Change current working directory  */

/*
 String passed has first byte as length of directory name, rest of string
 is name.  Fails if can't connect, else ACKs (with name) and succeeds. 
*/

cwd(vdir) char *vdir; {
    char *cdd, *zgtdir();
    char *dirp;
#ifdef DTILDE
    char *tilde_expand();
#endif

    vdir[xunchar(*vdir) + 1] = '\0';	/* Terminate string with a null */

    dirp = vdir+1;
#ifdef DTILDE
    dirp = tilde_expand(vdir+1);	/* Attempt to expand tilde */
    if (*dirp == '\0') dirp = vdir+1;	/* in directory name. */
#endif
    tlog(F110,"Directory requested: ",dirp,01);
    if (zchdir(dirp)) {
	cdd = zgtdir();			/* Get new working directory. */
	tlog(F110,"Changed directory to ",cdd,01);
	encstr(cdd);
	ack1(encbuf);
	tlog(F110,"Changed directory to",cdd,0l);
	return(1); 
    } else {
	tlog(F110,"Failed to change directory to",dirp,0l);
	return(0);
    }
}


/*  S Y S C M D  --  Do a system command  */

/*  Command string is formed by concatenating the two arguments.  */

syscmd(prefix,suffix) char *prefix, *suffix; {
    char *cp;

    if (prefix == NULL || *prefix == '\0') return(0);

#ifdef datageneral
/* A kludge for now -- the real change needs to be done elsewhere... */
    {
	extern char *WHOCMD;
	if ((strcmp(prefix,WHOCMD) == 0) && (*suffix == 0))
	  strcpy(suffix,"[!pids]");
    }
#endif
    for (cp = cmdstr; *prefix != '\0'; *cp++ = *prefix++) ;
    while (*cp++ = *suffix++) ;

    debug(F110,"syscmd",cmdstr,0);
    if (zopeni(ZSYSFN,cmdstr) > 0) {
	debug(F100,"syscmd zopeni ok",cmdstr,0);
	nfils = sndsrc = 0;		/* Flag that input is from stdin */
	xflg = hcflg = 1;		/* And special flags for pipe */
	if (binary) {			/* If file mode is binary, */
	    binary = 0;			/*  turn it back to text for this, */
	    savmod = 1;			/*  remember to restore it later. */
	}
	return (sinit());		/* Send S packet */
    } else {
	debug(F100,"syscmd zopeni failed",cmdstr,0);
	return(0);
    }
}

/*  R E M S E T  --  Remote Set  */
/*  Called by server to set variables as commanded in REMOTE SET packets.  */
/*  Returns 1 on success, 0 on failure.  */

remset(s) char *s; {
    int len, i, x, y;
    char *p;

    len = xunchar(*s++);		/* Length of first field */
    p = s + len;			/* Pointer to second length field */
    len = xunchar(*p);			/* Length of second field */
    *p++ = '\0';			/* Zero out second length field */
    x = atoi(s);			/* Value of first field */
    debug(F111,"remset",s,x);
    debug(F110,"remset",p,0);
    switch (x) {			/* Do the right thing */
      case 132:				/* Attributes (all, in) */
	atcapr = atoi(p);
	return(1);
      case 232:				/* Attributes (all, out) */
	atcapr = atoi(p);
	return(1);
      case 300:				/* File type (text, binary) */
	binary = atoi(p);
	return(1);
      case 301:				/* File name conversion */
	fncnv = 1 - atoi(p);		/* (oops) */
	return(1);
      case 302:				/* File name collision */
	switch (atoi(p)) {
	  case 0: warn = 1; return(1);	/* Rename */
	  case 1: warn = 0; return(1);	/* Replace */
	  default: return(0);
	}
      case 310:				/* Incomplete File Disposition */
	keep = atoi(p);			/* Keep, Discard */
	return(0);
      case 400:				/* Block check */
	y = atoi(p);
	if (y < 4 && y > 0) {
	    bctr = y;
	    return(1);
	} else return(0);
      case 401:				/* Receive packet-length */
	urpsiz = atoi(p);
	if ((urpsiz * wslotsr) > BIGRBUFSIZ) urpsiz = BIGRBUFSIZ / wslotsr - 1;
	return(1);
      case 402:				/* Receive timeout */
	y = atoi(p);
	if (y > -1 && y < 95) {
	    timef = 1;
	    timint = y;
	    return(1);
	} else return(0);
      case 403:				/* Retry limit */
	y = atoi(p);
	if (y > -1 && y < 95) {
	    maxtry = y;
	    return(1);
	} else return(0);
      case 404:				/* Server timeout */
	y = atoi(p);
	if (y < 0) return(0);
	srvtim = y;
	return(1);
      case 405:				/* Transfer character set */
	for (i = 0; i < ntcsets; i++) { 
	    if (!strcmp(tcsinfo[i].designator,p)) break;
	}
	debug(F101,"remset xfer charset lookup","",i);
	if (i == ntcsets) return(0);
	tcharset = tcsinfo[i].code;	/* if known, use it */
	rx = xlr[tcharset][fcharset];	/* translation function */
	return(1);
      case 406:				/* Window slots */
	y = atoi(p);
	if (y == 0) y = 1;
	if (y < 1 && y > 31) return(0);
	wslotsr = y;
	swcapr = 1;
	if ((urpsiz * wslotsr) > BIGRBUFSIZ) urpsiz = BIGRBUFSIZ / wslotsr - 1;
	return(1);
      default:				/* Anything else... */
	return(0);
    }
}

