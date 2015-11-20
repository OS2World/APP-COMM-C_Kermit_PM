/*  C K C F N 3  --  Packet buffer management for C-Kermit  */

/* (plus assorted functions tacked on at the end) */

#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#include "ckcxla.h"
#define NULL 0
extern int deblog;
extern int unkcs;
extern int wmax;
extern CHAR *data, filnam[];
extern char optbuf[];
extern int wslots;

extern int binary, tcharset, fcharset, rprintf, rmailf, spsiz;
extern int pktnum, cxseen, czseen, bsave, bsavef, filcnt;
extern int memstr, n_len, stdouf, ffc, tfc, keep, sndsrc, hcflg;
extern int ntcsets;
extern long fsize;
extern struct csinfo tcsinfo[];

/* Pointers to translation functions */
extern CHAR (*xlr[MAXTCSETS+1][MAXFCSETS+1])();	
extern CHAR (*xls[MAXTCSETS+1][MAXFCSETS+1])();	
extern CHAR (*rx)();	/* Pointer to input character translation function */
extern CHAR (*sx)();    /* Pointer to output character translation function */

/* Variables global to Kermit that are defined in this module */

int winlo;				/* packet number at low window edge  */
int sslotsiz,rslotsiz;			/* # of send & receive window slots  */

int bigsbufsiz;
int bigrbufsiz;

int sbufnum;				/* number of free buffers */
int dum001 = 1234;			/* protection... */
int sbufuse[MAXWS];			/* buffer in-use flag */
int dum003 = 1111;
int rbufnum;				/* number of free buffers */
int dum002 = 4321;			/* more protection */
int rbufuse[MAXWS];			/* buffer in-use flag */
int sseqtbl[64];			/* sequence # to buffer # table */
int rseqtbl[64];			/* sequence # to buffer # table */

struct pktinfo s_pkt[MAXWS];		/* array of pktinfo structures */
struct pktinfo r_pkt[MAXWS];		/* array of pktinfo structures */

char xbuf[1000];			/* For debug logging */

CHAR bigsbuf[BIGSBUFSIZ + 5];		/* Send-packet buffer area */
CHAR bigrbuf[BIGRBUFSIZ + 5];		/* Receive-packet area */

/* FUNCTIONS */

/* For sanity, use "i" for buffer slots, "n" for packet numbers. */


/* M A K E B U F  --  Makes and clears a new buffers.  */

/* Call with: */
/*  slots:  number of buffer slots to make, 1 to 31 */
/*  bufsiz: size of the big buffer */
/*  buf:    address of the big buffer */
/*  xx:     pointer to array of pktinfo structures for these buffers */

/* Subdivides the big buffer into "slots" buffers. */

/* Returns: */
/*  -1 if too many or too few slots requested,     */
/*  -2 if slots would be too small.      */
/*   n (positive) on success = size of one buffer. */
/*   with pktinfo structure initialized for this set of buffers. */

makebuf(slots,bufsiz,buf,xx)
    int slots, bufsiz; CHAR buf[]; struct pktinfo *xx; {

    CHAR *a;
    int i, size;

    debug(F101,"makebuf","",slots);
    debug(F101,"makebuf bufsiz","",bufsiz);
    debug(F101,"makebuf MAXWS","",MAXWS);

    if (slots > MAXWS || slots < 1) return(-1);
    if (bufsiz < slots * 10 ) return(-2);

    size = bufsiz / slots;		/* Divide up the big buffer. */
    a = buf;				/* Address of first piece. */

    for (i = 0; i < slots; i++) {
	struct pktinfo *x = &xx[i];
	x->bf_adr = a;			/* Address of this buffer */
	x->bf_len = size;		/* Length of this buffer */
	x->pk_len = 0;			/* Length of data field */
        x->pk_typ = ' ';		/* packet type */
	x->pk_seq = -1;			/* packet sequence number */
        x->pk_flg = 0;			/* ack'd bit */
        x->pk_rtr = 0;			/* retransmissions */
	*a = '\0';			/* Clear the buffer */
	a += size;			/* Position to next buffer slot */
    }
    return(size);
}

/*  M A K S B U F  --  Makes the send-packet buffer  */

mksbuf(slots) {
    int i, x;
    if ((x = makebuf(slots,BIGSBUFSIZ,bigsbuf,s_pkt)) < 0) {
	debug(F101,"mksbuf makebuf return","",x);
	return(x);
    }
    debug(F101,"mksbuf makebuf return","",x);
    for (i = 0; i < 64; i++) {		/* Initialize sequence-number- */
	sseqtbl[i] = -1;		/* to-buffer-number table. */
    }
    for (i = 0; i < MAXWS; i++)
      sbufuse[i] = 0;			/* Mark each buffer as free */
    sbufnum = slots;
    return(x);
}

/*  M A K R B U F  --  Makes the receive-packet buffer  */

mkrbuf(slots) {
    int i, x;
    if ((x = makebuf(slots,BIGRBUFSIZ,bigrbuf,r_pkt)) < 0) {
	debug(F101,"mkrbuf makebuf return","",x);
	return(x);
    }
    debug(F101,"mkrbuf makebuf return","",x);
    for (i = 0; i < 64; i++) {		/* Initialize sequence-number- */
	rseqtbl[i] = -1;		/* to-buffer-number table. */
    }
    for (i = 0; i < MAXWS; i++)
      rbufuse[i] = 0;			/* Mark each buffer as free */
    rbufnum = slots;
    return(x);
}

/*  W I N D O W  --  Resize the window to n  */

window(n) {
    debug(F101,"window","",n);
    if (n < 1 || n > 31) return(-1);
    if (mksbuf(n) < 0) return(-1);
    if (mkrbuf(n) < 0) return(-1);
    wslots = n;
    if (deblog) dumpsbuf(0,n);
    if (deblog) dumprbuf(0,n);
    return(0);
}

/*  G E T S B U F  --  Allocate a send-buffer.  */

/*  Call with packet sequence number to allocate buffer for. */
/*  Returns: */
/*   -3 if buffers were thought to be available but really weren't (bug!) */
/*   -2 if the number of free buffers is negative (bug!) */
/*   -1 if no free buffers. */
/*   0 or positive, packet sequence number, with buffer allocated for it. */

getsbuf(n) int n; {			/* Allocate a send-buffer */
    int i;
    if (sbufnum == 0) return(-1);	/* No free buffers. */
    if (sbufnum < 0) return(-2);	/* Shouldn't happen. */
    debug(F101,"getsbuf, packet","",n);
    for (i = 0; i < wslots; i++)	/* Find the first one not in use. */
      if (sbufuse[i] == 0) {		/* Got one? */
	  sbufuse[i] = 1;		/* Mark it as in use. */
	  sbufnum--;			/* One less free buffer. */
	  *s_pkt[i].bf_adr = '\0';	/* Zero the buffer data field */
	  s_pkt[i].pk_seq = n;		/* Put in the sequence number */
          sseqtbl[n] = i;		/* Back pointer from sequence number */
	  s_pkt[i].pk_len = 0;		/* Data field length now zero. */
	  s_pkt[i].pk_typ = ' ';	/* Blank the packet type too. */
	  s_pkt[i].pk_flg = 0;		/* Zero the flags */
	  s_pkt[i].pk_rtr = 0;		/* Zero the retransmission count */
	  data = s_pkt[i].bf_adr + 7;	/* Set global "data" address. */
	  if (i+1 > wmax) wmax = i+1;	/* For statistics. */
	  return(n);			/* Return its index. */
      }
    sbufnum = 0;			/* Didn't find one. */
    return(-3);				/* Shouldn't happen! */
}

getrbuf() {				/* Allocate a receive buffer */
    int i;
    debug(F101,"getrbuf rbufnum","",rbufnum);
    debug(F101,"getrbuf wslots","",wslots);
    debug(F101,"getrbuf dum002","",dum002);
    debug(F101,"getrbuf dum003","",dum003);
    if (rbufnum == 0) return(-1);	/* No free buffers. */
    if (rbufnum < 0) return(-2);	/* Shouldn't happen. */
    for (i = 0; i < wslots; i++)	/* Find the first one not in use. */
      if (rbufuse[i] == 0) {		/* Got one? */
	  rbufuse[i] = 1;		/* Mark it as in use. */
	  *r_pkt[i].bf_adr = '\0';	/* Zero the buffer data field */
	  rbufnum--;			/* One less free buffer. */
	  debug(F101,"getrbuf new rbufnum","",rbufnum);
	  if (i+1 > wmax) wmax = i+1;	/* For statistics. */
	  return(i);			/* Return its index. */
      }
    debug(F101,"getrbuf foulup","",i);
    rbufnum = 0;			/* Didn't find one. */
    return(-3);				/* Shouldn't happen! */
}

/*  F R E E S B U F  --  Free send-buffer "i"  */

/*  Returns:  */
/*   1 upon success  */
/*  -1 if specified buffer does not exist */

freesbuf(n) int n; {			/* Release send-buffer for packet n. */
    int i;

    debug(F101,"freesbuf","",n);
    if (n < 0 || n > 63)		/* No such packet. */
      return(-1);
    i = sseqtbl[n];			/* Get the pktinfo struct index */
    if (i > -1) {
	sseqtbl[n] = -1;		/* If valid, remove from seqtbl */
	if (sbufuse[i] != 0) {		/* If really allocated, */
	    sbufuse[i] = 0;		/* mark it as free, */
	    sbufnum++;			/* and count one more free buffer. */
	}
    } else {
	debug(F101," sseqtbl[n]","",sseqtbl[n]);
	return(-1);
    }

/* The following is done only so dumped buffers will look right. */

    if (1) {
	*s_pkt[i].bf_adr = '\0';	/* Zero the buffer data field */
	s_pkt[i].pk_seq = -1;		/* Invalidate the sequence number */
	s_pkt[i].pk_len = 0;		/* Data field length now zero. */
	s_pkt[i].pk_typ = ' ';		/* Blank the packet type too. */
	s_pkt[i].pk_flg = 0;		/* And zero the flag */
	s_pkt[i].pk_rtr = 0;		/* And the retries field. */
    }
    return(1);
}

freerbuf(i) int i; {			/* Release receive-buffer slot "i". */
    int n;

/* NOTE !! Currently, this function frees the indicated buffer, but */
/* does NOT erase the data.  The program counts on this.  Will find a */
/* better way later.... */

    debug(F101,"freerbuf, slot","",i);
    if (i < 0 || i >= wslots) {		/* No such slot. */
	debug(F101,"freerbuf no such slot","",i);
	return(-1);
    }
    n = r_pkt[i].pk_seq;		/* Get the packet sequence number */
    debug(F101,"freerbuf, packet","",n);
    if (n > -1) rseqtbl[n] = -1;	/* If valid, remove from seqtbl */
    if (rbufuse[i] != 0) {		/* If really allocated, */
	rbufuse[i] = 0;			/* mark it as free, */
	rbufnum++;			/* and count one more free buffer. */
	debug(F101,"freerbuf, new rbufnum","",rbufnum);
    }

/* The following is done only so dumped buffers will look right. */

    if (1) {
     /* *r_pkt[i].bf_adr = '\0'; */	/* Zero the buffer data field */
	r_pkt[i].pk_seq = -1;		/* And from packet list */
	r_pkt[i].pk_len = 0;		/* Data field length now zero. */
	r_pkt[i].pk_typ = ' ';		/* Blank the packet type too. */
	r_pkt[i].pk_flg = 0;		/* And zero the flag */
	r_pkt[i].pk_rtr = 0;		/* And the retries field. */
    }
    return(1);
}

/*  C H K W I N  --  Check if packet n is in window. */

/*  Returns: */
/*    0 if it is in the current window,  */
/*   +1 if in would have been in previous window (e.g. if ack was lost), */
/*   -1 if it is outside any window (protocol error),   */
/*   -2 if either of the argument packet numbers is out of range.  */

/* Call with packet number to check (n), lowest packet number in window */
/* (bottom), and number of slots in window (slots).  */

chkwin(n,bottom,slots) int n, bottom, slots; {
    int top, prev;

    debug(F101,"chkwin packet","",n);
    debug(F101,"chkwin winlo","",bottom);
    debug(F101,"chkwin slots","",slots);

/* First do the easy and common cases, where the windows are not split. */

    if (n < 0 || n > 63 || bottom < 0 || bottom > 63)
      return(-2);

    top = bottom + slots;		/* Calculate window top. */
    if (top < 64 && n < top && n >= bottom)
      return(0);			/* In current window. */

    prev = bottom - slots;		/* Bottom of previous window. */
    if (prev > -1 && n < bottom && n > prev)
      return(1);			/* In previous. */

/* Now consider the case where the current window is split. */

    if (top > 63) {			/* Wraparound... */
	top -= 64;			/* Get modulo-64 sequence number */
	if (n < top || n >= bottom) {
	    return(0);			/* In current window. */
	} else {			/* Not in current window. */
	    if (n < bottom && n >= prev) /* Previous window can't be split. */
	      return(1);		/* In previous window. */
	    else
	      return(-1);		/* Not in previous window. */
	}
    }

/* Now the case where current window not split, but previous window is. */ 

    if (prev < 0) {			/* Is previous window split? */
	prev += 64;			/* Yes. */
	if (n < bottom || n >= prev)
	  return(1);			/* In previous window. */
    } else {				/* Previous window not split. */
	if (n < bottom && n >= prev)
	  return(1);			/* In previous window. */
    }
    
/* It's not in the current window, and not in the previous window... */

    return(-1);				/* So it's not in any window. */
}

dumpsbuf() {				/* Dump send-buffers */
    int j;

    if (! deblog) return(0);
    zsoutl(ZDFILE,"SEND BUFFERS:");
    zsoutl(ZDFILE,"buffer inuse address length data type seq flag retries");

    for ( j = 0; j < wslots; j++ ) {
	sprintf(xbuf,"%4d%6d%10d%5d%6d%4c%5d%5d%6d\n",
	       j,
	       sbufuse[j],
	       s_pkt[j].bf_adr, 
	       s_pkt[j].bf_len,
	       s_pkt[j].pk_len,
	       s_pkt[j].pk_typ,
	       s_pkt[j].pk_seq,
	       s_pkt[j].pk_flg,
	       s_pkt[j].pk_rtr
	       );
	zsout(ZDFILE,xbuf);
	sprintf(xbuf,"[%s]\n",s_pkt[j].pk_adr);
	zsout(ZDFILE,xbuf);
    }
    sprintf(xbuf,"free: %d, winlo: %d\n", sbufnum, winlo);
    zsout(ZDFILE,xbuf);
    return(0);
}
dumprbuf() {				/* Dump receive-buffers */
    int j;
    if (! deblog) return(0);
    zsoutl(ZDFILE,"RECEIVE BUFFERS:");
    zsoutl(ZDFILE,"buffer inuse address length data type seq flag retries");
    for ( j = 0; j < wslots; j++ ) {
	sprintf(xbuf,"%4d%6d%10d%5d%6d%4c%5d%5d%6d\n",
	       j,
	       rbufuse[j],
	       r_pkt[j].bf_adr, 
	       r_pkt[j].bf_len,
	       r_pkt[j].pk_len,
	       r_pkt[j].pk_typ,
	       r_pkt[j].pk_seq,
	       r_pkt[j].pk_flg,
	       r_pkt[j].pk_rtr
	       );
	zsout(ZDFILE,xbuf);
	sprintf(xbuf,"[%s]\n",r_pkt[j].bf_adr);
	zsout(ZDFILE,xbuf);
    }
    sprintf(xbuf,"free: %d, winlo: %d\n", rbufnum, winlo);
    zsout(ZDFILE,xbuf);
    return(0);
}

/*** Some misc functions also moved here from the other ckcfn*.c modules ***/
/*** to even out the module sizes. ***/


/* Attribute Packets. */

/*
  Call with xp == 0 if we're sending a real file (F packet),
  or xp != 0 for screen data (X packet).
  Returns 0 on success, -1 on failure.
*/
sattr(xp) int xp; {			/* Send Attributes */
    int i, j, k, aln;
    char *tp;
    struct zattr x;

    if (zsattr(&x) < 0) return(-1);	/* Get attributes or die trying */
    nxtpkt();				/* Next packet number */
    i = 0;				/* i = Data field character number */
    data[i++] = '.';			/* System type */
    data[i++] = tochar(x.systemid.len); /*  Copy from attribute structure */
    for (j = 0; j < x.systemid.len; j++)
      data[i++] = x.systemid.val[j];
    data[i++] = '"';			/* File type */
    if (binary) {			/*  Binary  */
	data[i++] = tochar(2);		/*  Two characters */
	data[i++] = 'B';		/*  B for Binary */
	data[i++] = '8';		/*  8-bit bytes (note assumption...) */
    } else {				/* Text */
	data[i++] = tochar(3);		/*  Three characters */
	data[i++] = 'A';		/*  A = (extended) ASCII with CRLFs */
	data[i++] = 'M';		/*  M for carriage return */
	data[i++] = 'J';		/*  J for linefeed */
	tp = tcsinfo[tcharset].designator; /* Transfer character set */
	if ((tp != NULL) && (aln = strlen(tp)) > 0) {
	    data[i++] = '*';		/* Encoding */
	    data[i++] = tochar(aln+1);	/*  Length of char set designator. */
	    data[i++] = 'C';		/*  Text in specified character set. */
	    for (j = 0; j < aln; j++)	/*  Designator from tcsinfo struct */
	      data[i++] = *tp++;	/*  Example: *&I2/100 */
	}
    }
    if ((xp == 0) && (x.length > -1L)) { /* If it's a real file */

	if ((aln = x.date.len) > 0) {	/* Creation date, if any */
	    data[i++] = '#';
	    data[i++] = tochar(aln);
	    for (j = 0; j < aln; j++)
	      data[i++] = x.date.val[j];
	}
	data[i] = '!';			/* File length in K */
	sprintf(&data[i+2],"%ld",x.lengthk);
	aln = strlen(&data[i+2]);
	data[i+1] = tochar(aln);
	i += aln + 2;

	data[i] = '1';			/* File length in bytes */
	sprintf(&data[i+2],"%ld",fsize);
	aln = strlen(&data[i+2]);
	data[i+1] = tochar(aln);
	i += aln + 2;
    }
    if (rprintf || rmailf) {		/* MAIL, or REMOTE PRINT?  */
	data[i++] = '+';		/* Disposition */
        data[i++] = tochar(strlen(optbuf) + 1); /* Options, if any */
	if (rprintf)
	  data[i++] = 'P';		/* P for Print */
	else
	  data[i++] = 'M';		/* M for Mail */
	for (j = 0; optbuf[j]; j++)	/* Copy any options */
	  data[i++] = optbuf[j];
    }
    data[i] = '\0';			/* Make sure it's null-terminated */
    aln = strlen(data);			/* Get overall length of attributes */
    if (aln > spsiz) {			/* Check length of result */
	spack('A',pktnum,"",0);		/* send an empty attribute packet */
	debug(F101,"sattr pkt too long","",aln); /* if too long */
	debug(F101,"sattr spsiz","",spsiz);
    } else {				/* Otherwise */
	spack('A',pktnum,aln,data);	/* send the real thing. */
	debug(F111,"sattr",data,aln);
    }
    /* Change this code to break attribute data up into multiple packets! */
    return(0);
}

rsattr(s) char *s; {			/* Read response to attribute packet */
    debug(F111,"rsattr: ",s,*s);	/* If it's 'N' followed by anything */
    if (*s == 'N') return(-1);		/* then other Kermit is refusing. */
    return(0);
}

gattr(s, yy) char *s; struct zattr *yy;{ /* Read incoming attribute packet */
    char c;
    int aln, i, x;
    long l;
#define ABUFL 100			/* Temporary buffer for conversions */
    char abuf[ABUFL];
#define FTBUFL 10			/* File type buffer */
    static char ftbuf[FTBUFL];
#define DTBUFL 24			/* File creation date */
    static char dtbuf[DTBUFL];
#define TSBUFL 10			/* Transfer syntax */
    static char tsbuf[TSBUFL];
#define IDBUFL 10			/* System ID */
    static char idbuf[IDBUFL];
#define DSBUFL 100			/* Disposition */
    static char dsbuf[DSBUFL];
#define SPBUFL 512			/* System-dependent parameters */
    static char spbuf[SPBUFL];
#define CSBUFL 10			/* Transfer syntax character set */
    static char csbuf[CSBUFL];
#define RPBUFL 20			/* Attribute reply */
    static char rpbuf[RPBUFL];

    char *rp;				/* Pointer to reply buffer */
    int retcode;			/* Return code */

/* fill in the attributes we have received */

    rp = rpbuf;				/* Initialize reply buffer */
    *rp++ = 'N';			/* for negative reply. */
    retcode = 0;			/* Initialize return code. */

    while (c = *s++) {			/* Get attribute tag */
	aln = xunchar(*s++);		/* Length of attribute string */
	switch (c) {
	  case '!':			/* file length in K */
            /* It would be nice to be able to refuse a file that was */
	    /* bigger than the available disk space, but the method for */
	    /* finding this out in Unix is not obvious... */
	    for (i = 0; (i < aln) && (i < ABUFL); i++) /* Copy it */
	      abuf[i] = *s++;
	    abuf[i] = '\0';		/* Terminate with null */
	    yy->lengthk = atol(abuf);	/* Convert to number */
	    break;

	  case '"':			/* file type */
	    for (i = 0; (i < aln) && (i < FTBUFL); i++)
	      ftbuf[i] = *s++;		/* Copy it into a static string */
	    ftbuf[i] = '\0';
	    yy->type.val = ftbuf;	/* Pointer to string */
	    yy->type.len = i;		/* Length of string */
	    debug(F101,"gattr file type",tsbuf,i);
	    if (*ftbuf != 'A' && *ftbuf != 'B') { /* Reject unknown types */
		retcode = -1;
		*rp++ = c;
	    }		
	    break;

	  case '#':			/* file creation date */
	    for (i = 0; (i < aln) && (i < DTBUFL); i++)
	      dtbuf[i] = *s++;		/* Copy it into a static string */
	    dtbuf[i] = '\0';
	    yy->date.val = dtbuf;	/* Pointer to string */
	    yy->date.len = i;		/* Length of string */
	    break;

	  case '*':			/* encoding (transfer syntax) */
	    for (i = 0; (i < aln) && (i < TSBUFL); i++)
	      tsbuf[i] = *s++;		/* Copy it into a static string */
	    tsbuf[i] = '\0';
	    yy->encoding.val = tsbuf;	/* Pointer to string */
	    yy->encoding.len = i;	/* Length of string */
	    debug(F101,"gattr encoding",tsbuf,i);
	    switch (*tsbuf) {
	      case 'A':			/* Normal (maybe extended) ASCII */
		tcharset = TC_USASCII;  /* Transparent, chars untranslated */
		break;
	      case 'C':			/* Specified character set */
		for (i = 0; i < ntcsets; i++) { 
		    if (!strcmp(tcsinfo[i].designator,tsbuf+1)) break;
		}
		debug(F101,"gattr xfer charset lookup","",i);
		if (i == ntcsets) {	/* If unknown character set, */
		    debug(F110,"gattr: xfer charset unknown",tsbuf+1,0);
		    if (!unkcs) {	/* and SET UNKNOWN DISCARD, */
			retcode = -1;	/* reject the file. */
			*rp++ = c;
		    }
		} else {
		    tcharset = tcsinfo[i].code;   /* if known, use it */
		    rx = xlr[tcharset][fcharset]; /* translation function */
		}
		debug(F101,"gattr tcharset","",tcharset);
		break;
	      default:			/* Something else. */
		debug(F110,"gattr unk encoding attribute",tsbuf,0);
		if (!unkcs) {		/* If SET UNK DISC */
		    retcode = -1;	/* reject the file */
		    *rp++ = c;
		}
		break;
	    }
	    break;

	  case '+':			/* disposition */
	    for (i = 0; (i < aln) && (i < DSBUFL); i++)
	      dsbuf[i] = *s++;		/* Copy it into a static string */
	    dsbuf[i] = '\0';
	    yy->disp.val = dsbuf;	/* Pointer to string */
	    yy->disp.len = i;		/* Length of string */
	    if (*dsbuf != 'M' && *dsbuf != 'P') {
		retcode = -1;
		*rp++ = c;
	    }
	    break;

	  case '.':			/* sender's system ID */
	    for (i = 0; (i < aln) && (i < IDBUFL); i++)
	      idbuf[i] = *s++;		/* Copy it into a static string */
	    idbuf[i] = '\0';
	    yy->systemid.val = idbuf;	/* Pointer to string */
	    yy->systemid.len = i;	/* Length of string */
	    break;

	  case '0':			/* system-dependent parameters */
	    for (i = 0; (i < aln) && (i < SPBUFL); i++)
	      spbuf[i] = *s++;		/* Copy it into a static string */
	    spbuf[i] = '\0';
	    yy->sysparam.val = spbuf;	/* Pointer to string */
	    yy->sysparam.len = i;	/* Length of string */
	    break;

	  case '1':			/* file length in bytes */
            /* It would be nice to be able to refuse a file that was */
	    /* bigger than the available disk space, but the method for */
	    /* finding this out in Unix is not obvious... */
	    for (i = 0; (i < aln) && (i < ABUFL); i++) /* Copy it */
	      abuf[i] = *s++;
	    abuf[i] = '\0';		/* Terminate with null */
	    yy->length = atol(abuf);	/* Convert to number */
	    debug(F101,"gattr length","",(int) yy->length);
	    break;

	  default:			/* Unknown attribute */
	    s += aln;			/* Just skip past it */
	    break;
	}
    }

    /* (PWP) show the info */
    if (yy->length > 0) {		/* Let the user know the size */
	fsize = yy->length;		
	screen(SCR_QE,0,fsize," Size");
    } else if (yy->lengthk > 0) {
	fsize = yy->lengthk * 1024L;
	screen(SCR_QE,0,fsize," Size");
    }
#ifdef CHECK_SIZE			/* PWP code not used (maybe on Mac) */
    if ((l > 0) && ( l > zfree(filnam))) {
	cxseen = 1 ; 	/* Set to true so file will be deleted */
	return(-1);	/* can't accept file */
    }
#endif
    if (retcode == 0) rp = rpbuf;	/* Null reply string if accepted */
    *rp = '\0';				/* End of reply string */
    yy->reply.val = rpbuf;		/* Add it to attribute structure */
    yy->reply.len = strlen(rpbuf);
    debug(F111,"gattr",rpbuf,retcode);
    return(retcode);
}

/*  I N I T A T T R  --  Initialize file attribute structure  */

initattr(yy) struct zattr *yy; {
    yy->lengthk = -1L;
    yy->type.val = "";
    yy->type.len = 0;
    yy->date.val = "";
    yy->date.len = 0;
    yy->encoding.val = "";
    yy->encoding.len = 0;
    yy->disp.val = "";
    yy->disp.len = 0;
    yy->systemid.val = "";
    yy->systemid.len = 0;
    yy->sysparam.val = "";
    yy->sysparam.len = 0;
    yy->creator.val = "";
    yy->creator.len = 0;
    yy->account.val = "";
    yy->account.len = 0;
    yy->area.val = "";
    yy->area.len = 0;
    yy->passwd.val = "";
    yy->passwd.len = 0;
    yy->blksize = -1L;
    yy->access.val = "";
    yy->access.len = 0;
    yy->lprotect.val = "";
    yy->lprotect.len = 0;
    yy->gprotect.val = "";
    yy->gprotect.len = 0;
    yy->recfm.val = "";
    yy->recfm.len = 0;
    yy->reply.val = "";
    yy->reply.len = 0;
    return(0);
}

/*  A D E B U -- Write attribute packet info to debug log  */

adebu(f,zz) char *f; struct zattr *zz; {
#ifdef DEBUG
    if (deblog == 0) return(0);
    debug(F110,"Attributes for incoming file ",f,0);
    debug(F101," length in K","",(int) zz->lengthk);
    debug(F111," file type",zz->type.val,zz->type.len);
    debug(F111," creation date",zz->date.val,zz->date.len);
    debug(F111," creator",zz->creator.val,zz->creator.len);
    debug(F111," account",zz->account.val,zz->account.len);
    debug(F111," area",zz->area.val,zz->area.len);
    debug(F111," password",zz->passwd.val,zz->passwd.len);
    debug(F101," blksize",(int) zz->blksize,0);
    debug(F111," access",zz->access.val,zz->access.len);
    debug(F111," encoding",zz->encoding.val,zz->encoding.len);
    debug(F111," disposition",zz->disp.val,zz->disp.len);
    debug(F111," lprotection",zz->lprotect.val,zz->lprotect.len);
    debug(F111," gprotection",zz->gprotect.val,zz->gprotect.len);
    debug(F111," systemid",zz->systemid.val,zz->systemid.len);
    debug(F111," recfm",zz->recfm.val,zz->recfm.len);
    debug(F111," sysparam",zz->sysparam.val,zz->sysparam.len);
    debug(F101," length","",(int) zz->length);
    debug(F110," reply",zz->reply.val,0);
#endif /* DEBUG */
    return(0);
}

/*  O P E N A -- Open a file, with attributes.  */
/*
  This function tries to open a new file to put the arriving data in.
  The filename is the one in the srvcmd buffer.  If warning is on and
  a file of that name already exists, then a unique name is chosen.
*/
opena(f,zz) char *f; struct zattr *zz; {
    int x;

    adebu(f,zz);			/* Write attributes to debug log */

    ffc = 0;				/* Init file character counter */

    if (bsavef) {			/* If somehow file mode */
	binary = bsave;			/* was saved but not restored, */
	bsavef = 0;			/* restore it. */
	debug(F101,"opena restoring binary","",binary);
    }
    if (x = openo(f,zz)) {		/* Try to open the file. */
	tlog(F110," as",f,0l);		/* OK, open, record name. */
	if (zz->type.val[0] == 'A') {	/* Check attributes */
	    bsave = binary;		/* Save global file type */
	    bsavef = 1;			/* ( restore it in clsof() ) */
	    binary = 0;
	    debug(F100,"opena attribute A = text","",binary);
	} else if (zz->type.val[0] == 'B') {
	    bsave = binary;		/* Save global file type */
	    bsavef = 1;
	    binary = 1;
	    debug(F100,"opena attribute B = binary","",binary);
	}
	if (binary) {			/* Log file mode in transaction log */
	    tlog(F100," mode: binary","",0l);
	} else {			/* If text mode, check character set */
	    tlog(F100," mode: text","",0l);
	    debug(F111," opena charset",zz->encoding.val,zz->encoding.len);
	}
	screen(SCR_AN,0,0l,f);
	intmsg(filcnt);

#ifdef datageneral
/* Need to turn on multi-tasking console interrupt task here, since multiple */
/* files may be received. */
        if ((local) && (!quiet))        /* Only do this if local & not quiet */
            consta_mt();                /* Start the asynch read task */
#endif

    } else {				/* Did not open file OK. */
        tlog(F110,"Failure to open",f,0l);
	screen(SCR_EM,0,0l,"Can't open file");
    }
    return(x);				/* Pass on return code from openo */
}

/*  C A N N E D  --  Check if current file transfer cancelled */

canned(buf) char *buf; {
    if (*buf == 'X') cxseen = 1;
    if (*buf == 'Z') czseen = 1;
    debug(F101,"canned: cxseen","",cxseen);
    debug(F101," czseen","",czseen);
    return((czseen || cxseen) ? 1 : 0);
}


/*  O P E N I  --  Open an existing file for input  */

openi(name) char *name; {
    int x, filno;
    if (memstr) return(1);		/* Just return if file is memory. */

    debug(F110,"openi",name,0);
    debug(F101," sndsrc","",sndsrc);

    filno = (sndsrc == 0) ? ZSTDIO : ZIFILE;    /* ... */

    debug(F101," file number","",filno);

    if (zopeni(filno,name)) {		/* Otherwise, try to open it. */
	debug(F110," ok",name,0);
    	return(1);
    } else {				/* If not found, */
	char xname[100];		/* convert the name */
	zrtol(name,xname);		/* to local form and then */
	x = zopeni(filno,xname);	/* try opening it again. */
	debug(F101," zopeni","",x);
	if (x) {
	    debug(F110," ok",xname,0);
	    return(1);			/* It worked. */
        } else {
	    screen(SCR_EM,0,0l,"Can't open file");  /* It didn't work. */
	    tlog(F110,xname,"could not be opened",0l);
	    debug(F110," openi failed",xname,0);
	    return(0);
        }
    }
}

/*  O P E N O  --  Open a new file for output.  */

/*  Returns actual name under which the file was opened in string 'name2'. */

openo(name,zz) char *name; struct zattr *zz; {

    if (stdouf)				/* Receiving to stdout? */
	return(zopeno(ZSTDIO,"",zz));

    debug(F110,"openo: name",name,0);

    if (cxseen || czseen) {		/* If interrupted, get out before */
	debug(F100," open cancelled","",0); /* destroying existing file. */
	return(1);			/* Pretend to succeed. */
    }
    if (zopeno(ZOFILE,name,zz) == 0) {	/* Try to open the file */
	debug(F110,"openo failed",name,0);
	tlog(F110,"Failure to open",name,0l);
	return(0);
    } else {
	debug(F110,"openo ok, name",name,0);
	return(1);
    }
}

/*  O P E N T  --  Open the terminal for output, in place of a file  */

opent(zz) struct zattr *zz; {
    ffc = tfc = 0;
    return(zopeno(ZCTERM,"",zz));
}

/*  C L S I F  --  Close the current input file. */

clsif() {
#ifdef datageneral
    if ((local) && (!quiet))    /* Only do this if local & not quiet */
        if (nfils < 1)          /* More files to send ... leave it on! */
            connoi_mt();
#endif
    if (memstr) {			/* If input was memory string, */
	memstr = 0;			/* indicate no more. */
    } else zclose(ZIFILE);		/* else close input file. */

    if (czseen || cxseen) 
    	screen(SCR_ST,ST_DISC,0l,"");
    else
    	screen(SCR_ST,ST_OK,0l,"");
    hcflg = 0;				/* Reset flags, */
    *filnam = '\0';			/* and current file name */
    n_len = -1;		   /* (pwp) reinit packet encode-ahead length */
}


/*  C L S O F  --  Close an output file.  */

/*  Call with disp != 0 if file is to be discarded.  */
/*  Returns -1 upon failure to close, 0 or greater on success. */

clsof(disp) int disp; {
    int x;

    if (bsavef) {			/* If we saved global file type */
	debug(F101,"clsof restoring binary","",binary);
	binary = bsave;			/* restore it */
	bsavef = 0;			/* only this once. */
    }
#ifdef datageneral
    if ((local) && (!quiet))		/* Only do this if local & not quiet */
        connoi_mt();
#endif
    if ((x = zclose(ZOFILE)) < 0) {	/* Try to close the file */
	tlog(F100,"Failure to close",filnam,0l);
	screen(SCR_ST,ST_ERR,0l,"");
    } else if (disp && (keep == 0)) {	/* Delete it if interrupted, */
	if (*filnam) zdelet(filnam);	/* and not keeping incomplete files */
	debug(F100,"Discarded","",0);
	tlog(F100,"Discarded","",0l);
	screen(SCR_ST,ST_DISC,0l,"");
    } else {				/* Nothing wrong, just keep it */
	debug(F100,"Closed","",0);	/* and give comforting messages. */
	screen(SCR_ST,ST_OK,0l,"");
    }
    return(x);				/* Send back zclose() return code. */
}

