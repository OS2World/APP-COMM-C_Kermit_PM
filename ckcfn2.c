/*  C K C F N 2  --  System-independent Kermit protocol support functions... */

/*** NOTE: there are some sprintf's in here which should probably not be. ***/

/*  ...Part 2 (continued from ckcfns.c)  */

/*
 Author: Frank da Cruz (fdc@cunixc.cc.columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 First released January 1985.
 Copyright (C) 1985, 1989, Trustees of Columbia University in the City of New 
 York.  Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained. 
*/
/*
 Note -- if you change this file, please amend the version number and date at
 the top of ckcfns.c accordingly.
*/

#include "ckcsym.h"		/* Conditional compilation (for Macintosh) */
#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#include "ckcxla.h"

extern struct pktinfo s_pkt[];		/* array of pktinfo structures */
extern struct pktinfo r_pkt[];		/* array of pktinfo structures */
extern int sseqtbl[], rseqtbl[], sbufuse[], wslots, winlo, sbufnum;

extern int spsiz, spmax, rpsiz, timint, npad, ebq, ebqflg,
 rpt, rptq, rptflg, capas, spsizf;
extern int pktnum, prvpkt, sndtyp, bctr, bctu, rsn, rln, maxtry, size;
extern int osize, maxsize, spktl, nfils, stdouf, warn, timef, parity, speed;
extern int turn, turnch,  delay, displa, pktlog, tralog, seslog, xflg, mypadn;
extern int deblog, hcflg, binary, fncnv, local, server, cxseen, czseen;
extern int nakstate, quiet, success;
extern int spackets, rpackets, timeouts, retrans, crunched, wmax;
extern int tcharset, fcharset;
extern struct csinfo fcsinfo[], tcsinfo[];
extern long filcnt, ffc, flci, flco, tlci, tlco, tfc, fsize;
extern char *cmarg, *cmarg2, **cmlist;
extern CHAR padch, mypadc, eol, seol, ctlq, myctlq, sstate, *hlptxt;
extern CHAR filnam[], *recpkt, *data, srvcmd[], encbuf[];
extern CHAR *srvptr, stchr, mystch, *rdatap;
extern CHAR padbuf[];

int numerrs = 0;		/* (PWP) total number packet errors so far */

char *strcpy();				/* Forward declarations */
unsigned int chk2();			/* of non-int functions */
unsigned int chk3();
CHAR dopar();				/* ... */

char plog[20];				/* For packet logging */

static CHAR partab[] = {		/* Even parity table for dopar() */

    '\000', '\201', '\202', '\003', '\204', '\005', '\006', '\207',
    '\210', '\011', '\012', '\213', '\014', '\215', '\216', '\017',
    '\220', '\021', '\022', '\223', '\024', '\225', '\226', '\027',
    '\030', '\231', '\232', '\033', '\234', '\035', '\036', '\237',
    '\240', '\041', '\042', '\243', '\044', '\245', '\246', '\047',
    '\050', '\251', '\252', '\053', '\254', '\055', '\056', '\257',
    '\060', '\261', '\262', '\063', '\264', '\065', '\066', '\267',
    '\270', '\071', '\072', '\273', '\074', '\275', '\276', '\077',
    '\300', '\101', '\102', '\303', '\104', '\305', '\306', '\107',
    '\110', '\311', '\312', '\113', '\314', '\115', '\116', '\317',
    '\120', '\321', '\322', '\123', '\324', '\125', '\126', '\327',
    '\330', '\131', '\132', '\333', '\134', '\335', '\336', '\137',
    '\140', '\341', '\342', '\143', '\344', '\145', '\146', '\347',
    '\350', '\151', '\152', '\353', '\154', '\355', '\356', '\157',
    '\360', '\161', '\162', '\363', '\164', '\365', '\366', '\167',
    '\170', '\371', '\372', '\173', '\374', '\175', '\176', '\377'
};

/* CRC generation tables */

static unsigned int crcta[16] = {0, 010201, 020402, 030603, 041004,
  051205, 061406, 071607, 0102010, 0112211, 0122412, 0132613, 0143014,
  0153215, 0163416, 0173617};

static unsigned int crctb[16] = {0, 010611, 021422, 031233, 043044,
  053655, 062466, 072277, 0106110, 0116701, 0127532, 0137323, 0145154,
  0155745, 0164576, 0174367};

/*  I N P U T  --  Attempt to read packet number 'pktnum'.  */

/*
 This is the function that feeds input to Kermit's finite state machine.

 If a special start state is in effect, that state is returned as if it were
 the type of an incoming packet.  Otherwise:

 (fill in...)
*/

input() {
    int numtry, type;
    int i, x, y, k;

    debug(F101,"input sstate","",sstate);
    debug(F101," nakstate","",nakstate);

    while (1) {				/* Big loop... */

	if (sstate != 0) {		/* If a start state is in effect, */
	    type = sstate;		/* return it like a packet type, */
	    sstate = 0;			/* and then nullify it. */
	    numerrs = 0;		/* (PWP) no errors so far */
	    return(type);
	}
	
	if (nakstate) {			/* This section for file receiver. */

	    x = rseqtbl[winlo];		/* See if desired packet already in. */
	    debug(F101," winlo","",winlo);
	    debug(F101," rseqtbl[winlo]","",rseqtbl[winlo]);
	    if (x > -1) {		/* Already there? */
		if (r_pkt[x].pk_seq == winlo) {	/* (double check) */
		    rsn = winlo;	        /* Yes, return its info */
		    debug(F101,"input return pre-stashed packet","",rsn);
		    dumprbuf();
		    rdatap = r_pkt[x].pk_adr;   /* like rpack would do. */
		    rln = strlen(rdatap);
		    type = r_pkt[x].pk_typ;
		    break;
		}
	    }
	    type = rpack();	        /* Try to read a packet. */
	    debug(F111,"input",rdatap,type);
	    if (type == sndtyp) type = rpack(); /* Handle echoes */
	    if (type < -1) return('q'); /* Ctrl-C */
	    if (type < 0) {		/* Receive window full */
		/* Another thing to do here would be to delete */
		/* the highest packet and NAK winlo.  But that */
		/* shouldn't be necessary since the other Kermit */
		/* should not have sent a packet outside the window. */
		debug(F101,"rpack receive window full","",0);
		dumprbuf();
		errpkt("Receive window full.");
		strcpy(recpkt,"Receive window full.");
		return(type = 'E');
	    }
	    dumprbuf();

	    chkint();			/* Check for console interrupts. */
	    if (type == 'E') break;	/* Error packet */
	    if (type == 'Q') {		/* Crunched packet. */
		crunched++;
		numerrs++;
		if (nack(winlo) < 0) {	/* Request resend of window-low.. */
		    debug(F101,"input sent too many naks","",winlo);
		    errpkt("Too many retries.");
		    strcpy(recpkt,"Sent too many NAKs.");
		    return(type = 'E');
		} else continue;
	    }		
	    if (type == 'T') {		/* Timeout */
		int z;			/* NAK all unreceived packets. */
		timeouts++;
		z = winlo + wslots;
		if (z > 63) z -= 64;
		debug(F101,"input sending bulk NAKs, winlo","",winlo);
		for (x = winlo; (x != z) && ttchk() == 0; x++) {
		    if (rseqtbl[x] < 0) {
			if (nack(x) < 0) {
			    debug(F101,"input sent too many naks","",winlo);
			    errpkt("Too many retries.");
			    strcpy(recpkt,"Sent too many NAKs.");
			    return(type = 'E');
			}
		    }
		}
		continue;
	    }
	    if (rsn == winlo) {
		debug(F101,"input rsn=winlo","",rsn);
		break;
	    }

	    /* Got a packet out of order. */

	    debug(F101,"input got data packet out of order","",rsn);
	    k = rseqtbl[rsn];		/* Get window index of this packet. */
	    debug(F101,"input rseqtbl[rsn]","",k);
	    if (k < 0) {
		debug(F101,"input can't find index for rcvd pkt","",rsn);
		errpkt("internal error number 21");
		strcpy(recpkt,"S/W Protocol Error.");
		type = 'E';
		break;
	    }		
	    y = chkwin(rsn,winlo,wslots); /* See what window it's in. */
	    debug(F101,"input chkwin","",y);
	    if (y == 0) {		/* This window. */
		debug(F101,"input pkt in window","",rsn);
		r_pkt[k].pk_flg++;
		debug(F101," ack'd bit for this pkt","",r_pkt[k].pk_flg);
#ifdef COMMENT
/* Note: no, we don't want to ACK a packet that's in the current window */
/* but is not at winlo.  Should only ACK packets AFTER they have been */
/* processed! */
		if (r_pkt[k].pk_flg) {	/* If it has been ack'd already */
		    if (resend(rsn) < 0) { /* resend the ACK. */
			debug(F101,"input, too many re-acks","",rsn);
			type = 'E';
			errpkt(recpkt);
			break;
		    }
		} else {		/* Otherwise */
		    ackn(rsn);		/* ack it. */
		}
		continue;
#endif		
	    } else if (y == 1) {	/* Previous window. */
		ackn(rsn);		/* *** dangerous!? *** */
		freerbuf(rseqtbl[rsn]);	/* Get rid of it */
		continue;		/* (what if this was an S-Packet?) */
	    } else {			/* If not in this or previous window */
		freesbuf(rsn);
		if (nack(winlo) < 0) {	/* NAK winlo. */
		    errpkt("Too many retries.");
		    strcpy(recpkt,"Timed out."); /* Give up if too many. */
		    type = 'E';
		    break;
		} else continue;
	    }
/*!!!*/
	} else {			/* Otherwise file sender... */

	    type = rpack();		/* Try to read an ack. */
	    debug(F111,"input",rdatap,type);
	    if (type == sndtyp) type = rpack(); /* Handle echoes */
	    if (type == -2) return('q');
	    if (type == -1) {
		errpkt("Internal error number 18");
		strcpy(recpkt,"Can't allocate receive buffer");
		type = 'E';
		break;		
	    }
	    dumprbuf();			/* debugging */

	    chkint();			/* Check for console interrupts. */

	    /* got a packet */

	    if (type == 'E') break;	/* Error packet */
	    if (type == 'Q') {		/* Crunched packet */ 
		crunched++;		/* For statistics */
		numerrs++;		/* For packet resizing */
		x = resend(winlo);	/* Resend window-low */
		if (x < 0) {
		    type = 'E';
		    errpkt(recpkt);
		    break;
		} else continue;	    
	    }
	    if (type == 'T') {		/* Timeout waiting for ACKs. */
		int z;			/* Resend all un-ACK'd packets. */
		timeouts++;
		z = (pktnum+1)%64;
		debug(F101,"input resending unack'd packets, winlo","",winlo);
		debug(F101," pktnum","",pktnum);
		for (x = winlo; (x != z) && (ttchk() == 0); (x = (x+1)%64)) {
		    if (x < 0 || x > 63) {
			debug(F101,"input resend invalid packet","",x);
			continue;
		    }
		    if ((k = sseqtbl[x]) > -1) {
			if (k > 31) {
			    debug(F101,"input resend invalid slot","",k);
			    continue;
			}
			if (s_pkt[k].pk_flg == 0) { /* If unack'd, resend */
			    if (resend(x) < 0) {    /* Check retries */
				debug(F101,"input too many resends","",maxtry);
				errpkt(recpkt);
				return(type = 'E');
			    }
			} else {	/* Already ACK'd, don't retransmit */
			    debug(F101,"input resend pkt already ack'd","",x);
			}
		    } else {		/* Shouldn't happen */
			debug(F101,"input resend can't find pkt","",x);
		    }			      
		}
		continue;
	    }

	    /* Got an actual normal packet */

	    y = chkwin(rsn,winlo,wslots); /* Is it in the window? */
	    debug(F101,"input rsn","",rsn);
	    debug(F101,"input winlo","",winlo);
	    debug(F101,"input chkwin","",y);
	    if (type == 'Y') {		/* Got an ACK */
		if (y == 0) {		/* In current window */
		    debug(F101,"input ACK in window, freeing buffers","",rsn);
	            freesbuf(rsn);	/* Free the sent packet's buffer */
/*
  NOTE: The following statement frees the buffer of the ACK we just got.
  But the upper layers still need the data, like if it's the ACK to an I,
  S, F, D, Z, or just about any kind of packet.  So for now, freerbuf()
  deallocates the buffer, but does not erase the data or destroy the pointer
  to it.  There's no other single place where these receive buffers can be
  correctly freed (???) ...
*/
		    freerbuf(rseqtbl[rsn]); /* Free the ACK's buffer */
		    if (rsn == winlo) {	/* Got the one we want */
			winlo = (winlo + 1) % 64; /* Rotate window */
			debug(F101,"input/ACK rotated window","",winlo);
			break;		/* Return the ACK */
		    }
		} else {		/* ACK not in window, ignore */
		    debug(F101,"input ACK out of window","",rsn);
		    freerbuf(rseqtbl[rsn]);
		    continue;
		}
	    }
	    if (type == 'N') {		/* NAK */
		debug(F101,"input NAK","",rsn);
		if (y == 0) {		/* In current window */		
		    debug(F100," in window","",0);
		    freerbuf(rseqtbl[rsn]);
		    k = sseqtbl[rsn];
		    if (k > -1 && s_pkt[k].pk_typ == ' ') /* Packet */
		      x = resend(winlo); /* alloc'd but not yet built. */
		    else
		      x = resend(rsn);	/* Resend requested packet. */
		    if (x < 0) {
			type = 'E';
			errpkt(recpkt);
			break;
		    }
		} else if ((rsn == (pktnum + 1) % 64)) {
		    if (wslots > 1) {
			debug( F101,"NAK for next packet, windowing","",rsn);
			x = resend(winlo);
			if (x < 0) {
			    type = 'E';
			    errpkt(recpkt);
			    break;
			}
			freerbuf(rseqtbl[rsn]);
			continue;	/* Just ignore it */
		    }
		    debug(F101," NAK for next packet, no windowing","",rsn);
		    freerbuf(rseqtbl[rsn]);
		    x = (rsn - 1) % 64;
		    if ((x = sseqtbl[x]) > -1) s_pkt[x].pk_flg++;
		    type = 'Y';		/* Treat it as ACK for current pkt */
		    break;
		} else {
		    debug(F101," NAK out of window","",rsn); /* bad... */
		    type = 'E';
		    errpkt("NAK out of window");
		    strcpy(recpkt,"NAK out of window.");
		    break;
		}
	    }
	}
    }
    debug(F101,"input returning type","",type);

    if (wslots == 1)
      ttflui();			/* Got what we want, clear input buffer. */
    if (spktl && !spsizf && !(pktnum & 007))  /* should we recalc pack len? */
      rcalcpsz();		/* (PWP) recalc every 8 packets */
    return(type);		/* Success, return packet type. */
}

/*  D O P A R  --  Add an appropriate parity bit to a character  */

/*
  (PWP) this is still used in the Mac terminal emulator, so we have to keep it
*/
CHAR
dopar(ch)
    register CHAR ch; {
    register unsigned int a;
    if (!parity) return(ch & 255); else a = ch & 127;
    switch (parity) {
	case 'e':  return(partab[a]);	    /* Even */
	case 'm':  return(a | 128);         /* Mark */
	case 'o':  return(partab[a] ^ 128); /* Odd */
	case 's':  return(a);		    /* Space */
	default:   return(a);
    }
}

/*  S P A C K  --  Construct and send a packet  */

/*
  spack() sends a packet of the given type, sequence number n, with len data
  characters pointed to by d, in either a regular or extended- length packet,
  depending on length.  Returns the number of bytes actually sent, or else -1
  upon failure.  Uses global npad, padch, mystch, bctu.  Leaves packet fully
  built and null-terminated for later retransmission by resend().  Updates
  global sndpktl (send-packet length).
*/

spack(type,n,len,d) char type; int n, len; CHAR *d; {
    register int i;
    int j, k, lp, longpkt, copy;
    register CHAR *cp;
    unsigned crc;

    debug(F101,"spack n","",n);
    debug(F111," data",data,data);
    debug(F111," d",d,d);

    copy = (d != data);			/* Flag whether data must be copied  */
    longpkt = (len + bctu + 2) > 94;	/* Decide whether it's a long packet */
    data = data - 7 + (longpkt ? 0 : 3); /* Starting position of header */

    k = sseqtbl[n];			/* Packet structure info for pkt n */ 
    debug(F101," sseqtbl[n]","",k);
    if (k < 0)
      debug(F101,"spack sending packet out of window","",n);
    else
      s_pkt[k].pk_adr = data;		/* Remember address of data field. */

    spktl = 0;
    i = 0;

/* Now fill the packet */

    data[i++] = mystch;			/* MARK */
    lp = i++;				/* Position of LEN, fill in later */

    data[i++] = tochar(n);		/* SEQ field */
    data[i++] = sndtyp = type;		/* TYPE field */
    j = len + bctu;			/* Length of data + block check */
    if (longpkt) {			/* Long packet? */
        data[lp] = tochar(0);		/* Yes, set LEN to zero */
        data[i++] = tochar(j / 95);	/* High part */
        data[i++] = tochar(j % 95);	/* Low part */
        data[i] = '\0';			/* Header checksum */
        data[i++] = tochar(chk1(data+lp));
    } else data[lp] = tochar(j+2);	/* Normal LEN */

    if (copy)				/* Data field pre-built? */
      for ( ; len--; i++) data[i] = *d++; /* No, must copy. */
    else				/* Otherwise, */
      i += len;				/* Just skip past data field. */
    data[i] = '\0';			/* Null-terminate for checksum calc. */

    switch (bctu) {			/* Block check */
	case 1:				/* 1 = 6-bit chksum */
	    data[i++] = tochar(chk1(data+lp));
	    break;
	case 2:				/* 2 = 12-bit chksum */
	    j = chk2(data+lp);
	    data[i++] = (unsigned)tochar((j >> 6) & 077);
	    data[i++] = (unsigned)tochar(j & 077);
	    break;
        case 3:				/* 3 = 16-bit CRC */
	    crc = chk3(data+lp);
	    data[i++] = (unsigned)tochar(((crc & 0170000)) >> 12);
	    data[i++] = (unsigned)tochar((crc >> 6) & 077);
	    data[i++] = (unsigned)tochar(crc & 077);
	    break;
    }
    data[i++] = seol;			/* End of line (packet terminator) */
    data[i] = '\0';			/* Terminate string */

    if (pktlog) {			/* If logging packets, log this one */
	sprintf(plog,"s-%02d-%02d-",n,(gtimer()%60));
	zsout(ZPFILE,plog);
	if (*data) zsoutl(ZPFILE,data);
    }	
    /* (PWP) add the parity quickly at the end */
    switch (parity) {
	case 'e':			/* Even */
	    for (cp = &data[i-1]; cp >= data; cp--)
		*cp = partab[*cp];
	    break;
	case 'm':			/* Mark */
	    for (cp = &data[i-1]; cp >= data; cp--)
		*cp = *cp | 128;
	    break;
	case 'o':			/* Odd */
	    for (cp = &data[i-1]; cp >= data; cp--)
		*cp = partab[*cp] ^ 128;
	    break;
	  case 's':
	    for (cp = &data[i-1]; cp >= data; cp--)
		*cp = partab[*cp] & 127;
	    break;
    }

    if (npad) ttol(padbuf,npad);	/* Send any padding */
    spktl = i;				/* Remember packet length */
    if (ttol(data,spktl) < 0) return(-1); /* Send the packet */
    spackets++;
    if (k > -1) {			/* If packet is in window... */
	s_pkt[k].pk_seq = n;		/* Record sequence number */
	s_pkt[k].pk_typ = type;		/* Record packet type */
	s_pkt[k].pk_len = spktl;	/* and length */
	dumpsbuf();
    }
    flco += spktl;			/* Count the characters */
    tlco += spktl;			/* for statistics... */

    screen(SCR_PT,type,(long)n,data);	/* Update screen */
    return(i);				/* Return length */
}

/*  C H K 1  --  Compute a type-1 Kermit 6-bit checksum.  */

chk1(pkt) register CHAR *pkt; {
    register unsigned int chk;
    chk = chk2(pkt);
    chk = (((chk & 0300) >> 6) + chk) & 077;
    return(chk);
}

/*  C H K 2  --  Compute the numeric sum of all the bytes in the packet.  */

unsigned int
chk2(pkt) register CHAR *pkt; {
    register long chk; register unsigned int m;
    m = (parity) ? 0177 : 0377;
    for (chk = 0; *pkt != '\0'; pkt++)
      chk += *pkt & m;
    return(chk & 07777);
}


/*  C H K 3  --  Compute a type-3 Kermit block check.  */
/*
 Calculate the 16-bit CRC-CCITT of a null-terminated string using a lookup 
 table.  Assumes the argument string contains no embedded nulls.
*/
unsigned int
chk3(pkt) register CHAR *pkt; {
    register LONG c, crc;
    register unsigned int m;
    m = (parity) ? 0177 : 0377;
    for (crc = 0; *pkt != '\0'; pkt++) {
	c = (*pkt & m) ^ crc;
	crc = (crc >> 8) ^ (crcta[(c & 0xF0) >> 4] ^ crctb[c & 0x0F]);
    }
    return(crc & 0xFFFF);
}

nxtpkt() {				/* Called by file sender */
    int i,j,n;

    debug(F101,"nxtpkt pktnum","",pktnum);
    debug(F101,"nxtpkt winlo ","",winlo);
    n = (pktnum + 1) % 64;		/* Increment packet number mod 64 */
    j = getsbuf(n);			/* Get a buffer for packet n */
    if (j < 0) {
	debug(F101,"nxtpkt can't get s-buffer","",0);
	return(-1);
    }
    pktnum = n;	      
    debug(F101,"nxtpkt bumped pktnum to","",pktnum);
    return(0);
}

/* Functions for sending ACKs and NAKs */

ack() {					/* Acknowledge the current packet. */
    return(ackns(winlo,""));
}

ackns(n,s) int n; char *s; {		/* Acknowledge packet n */
    int j,k;
    debug(F111,"ackns",s,n);

    k = rseqtbl[n];			/* First find received packet n. */
    debug(F101,"ackns k","",k);
    if (k > -1)				/* If in window */
      s_pkt[k].pk_flg++;		/* mark the ack'd bit. */
    else
      debug(F101,"ackns can't set ack'd bit","",k);

    /* Now get a buffer for this ack. */
    /* There already might be a NAK sitting in the same buffer, */
    /* In which case we can write over it. */
    if (sseqtbl[n] < 0)	{
	if ((j = getsbuf(n)) < 0) 	 
	  debug(F101,"ackns can't getsbuf","",n);
    }
    spack('Y',n,strlen(s),s);		/* Now send it. */
    debug(F101,"ackns winlo","",winlo);
    debug(F101,"ackns n","",n);
    if (n == winlo) {			/* If we're acking winlo */
	freerbuf(k);			/* don't need it any more */
	freesbuf(j);			/* and don't need to keep ACK either */
	winlo = (winlo + 1) % 64;
    }
    return(0);
}

ackn(n) int n; {			/* Send ACK for packet number n */
    return(ackns(n,""));
}

ack1(s) char *s; {			/* Send an ACK with data. */
    debug(F110,"ack1",s,0);
    return(ackns(winlo,s));
}

nack(n) int n; {			/* Negative acknowledgment. */
    int j,k;
    debug(F101,"nack","",n);

    if ((j = sseqtbl[n]) < 0) {		/* If necessary */
	if ((j = getsbuf(n)) < 0) {	/* get a buffer for this NAK */
	    debug(F101,"nack can't getsbuf","",n);
	    return(0);
	}
    }
    if (r_pkt[j].pk_rtr++ > maxtry)	/* How many times have we done this? */
      return(-1);			/* Too many... */

/* Note, don't free this buffer.  Eventually an ACK will come, and that */
/* will set it free.  If not, well, it's back to ground zero anyway...  */

    spack('N',n,0,"");			/* NAK's never have data. */
    return(0);
}

/*
 * (PWP) recalculate the optimal packet length in the face of errors.
 * This is a modified version of the algorithm by John Chandler in Kermit/370, 
 * see "Dynamic Packet Size Control", Kermit News, V2 #1, June 1988.
 *
 * My implementation minimizes the total overhead equation, which is
 *
 *  Total chars = file_chars + (header_len * num_packs)
 *                           + (errors * (header_len + packet_len))
 *
 * Differentiate with respect to number of chars, solve for packet_len, get:
 *
 *  packet_len = sqrt (file_chars * header_len / errors)
 */

rcalcpsz()
{
    register long x, q;

    if (numerrs == 0) return;	/* bounds check just in case */

    /* overhead on a data packet is npad+5+bctr, plus 3 if extended packet */
    /* an ACK is 5+bctr */

    /* first set x = per packet overhead */
#ifdef COMMENT
    /* (PWP) hook for doing windowing code */
    if (window)
	x = (long) (npad+5+bctr);    /* only the packet, don't count the ack */
    else
#endif /* COMMENT */
	x = (long) (npad+5+3+bctr+5+bctr);

    /* then set x = packet length ** 2 */
    x = x * ((long) ffc / (long) numerrs);	/* careful of overflow */
    
    /* calculate the long integer sqrt(x) quickly */
    q = 500;
    q = (q + x/q) >> 1;
    q = (q + x/q) >> 1;
    q = (q + x/q) >> 1;
    q = (q + x/q) >> 1;		/* should converge in about 4 steps */
    if ((q > 94) && (q < 130))	/* break-even point for long packets */
	q = 94;
    if (q > spmax) q = spmax;	/* maximum bounds */
    if (q < 10) q = 10;		/* minimum bounds */
    spsiz = q;			/* set new send packet size */
}

/*  R E S E N D  --  Retransmit packet n.  */

/* Returns 0 or positive on success. */
/* On failure, returns a negative number, and an error message is placed */
/* in recpkt.  All errors are considered fatal.  */

resend(n) int n; {			/* Send the old packet again. */
    int k;
    debug(F101,"resend seq","",n);
    if ((k = chkwin(n,winlo,wslots)) != 0) {
	debug(F101,"resend","",n);
	debug(F101,"resend packet not in window","",k);
	if (nakstate && k == 1) {	/* Take a chance... */
	    spack('Y',n,0,"");		/* Send an ACK... */
	    retrans++;
	    return(0);
	} else {
	    debug(F100,"resend can't recover","",0);
	    strcpy(recpkt,"resend error number 13.");
	    return(-2);
	}
    }
    k = sseqtbl[n];			/* OK, it's in the window. */
    debug(F101,"resend pktinfo index","",k);
    if (k < 0) {
	debug(F101,"resend sseqtbl failure for pkt","",n);
	strcpy(recpkt,"resend error number 12.");
	return(-2);
    }
    if (s_pkt[k].pk_rtr++ > maxtry) {
	strcpy(recpkt,"Too many retries.");
	return(-1);
    }
    debug(F101," retry","",s_pkt[k].pk_rtr);
    dumpsbuf();
    if (s_pkt[k].pk_typ == ' ') {
	if (nakstate) {
	    nack(n);
	    retrans++;
	    return(s_pkt[k].pk_rtr);
	} else {			/* No packet to resend! */
	    strcpy(recpkt,"resend error number 19.");
	    return(-2);
	}
    }
    debug(F111,"resend",s_pkt[k].pk_adr,s_pkt[k].pk_len);
    if (ttol(s_pkt[k].pk_adr,s_pkt[k].pk_len) < 1) {
	debug(F100,"resend ttol failed","",0);
	strcpy(recpkt,"resend error number 52.");
	return(-2);
    }
    retrans++;
    screen(SCR_PT,'%',(long)pktnum,"(resend)");	/* Say resend occurred */
    if (pktlog) {			/* If logging packets, log this one */
	sprintf(plog,"x-%02d-%02d-",n,(gtimer()%60));
	zsout(ZPFILE,plog);
	if (*s_pkt[k].pk_adr) zsoutl(ZPFILE,s_pkt[k].pk_adr);
    }	
    return(0);				/* Return retries. */
}

errpkt(reason) char *reason; {		/* Send an error packet. */
    int x;
    encstr(reason);
    spack('E',pktnum,size,encbuf);
    x = quiet; quiet = 1; 		/* Close files silently. */
    clsif(); clsof(1);
    quiet = x;
    screen(SCR_TC,0,0l,"");
    success = 0;
}

scmd(t,dat) char t, *dat; {		/* Send a packet of the given type */
    encstr(dat);			/* Encode the command string */
    spack(t,pktnum,size,encbuf);
}

srinit() {				/* Send R (GET) packet */
    encstr(cmarg);			/* Encode the filename. */
    spack('R',pktnum,size,encbuf);	/* Send the packet. */
}

sigint(sig,code) int sig, code; {	/* Terminal interrupt handler */
    if (local) errpkt("User typed ^C");
    debug(F101,"sigint() caught signal","",sig);
    debug(F101," code","",code);
    doexit(GOOD_EXIT);			/* Exit program */
}

/* R P A C K  --  Read a Packet */

/*
 rpack reads a packet and returns the packet type, or else Q if the
 packet was invalid, or T if a timeout occurred.  Upon successful return, sets
 the values of global rsn (received sequence number),  rln (received
 data length), and rdatap (pointer to null-terminated data field).
*/
rpack() {
    register int i, j, x, try, lp;	/* Local variables */
    int k, type;
    unsigned crc;
    CHAR pbc[4];			/* Packet block check */
    CHAR *sohp;				/* Pointer to SOH */
    CHAR e;				/* Packet end character */

    debug(F101,"entering rpack, pktnum","",pktnum);
    k = getrbuf();			/* Get a new packet input buffer. */
    debug(F101,"rpack getrbuf","",k);
    if (k < 0) return(-1);		/* Return like this if none free. */
    recpkt = r_pkt[k].bf_adr;
    *recpkt = '\0';			/* Clear receive buffer. */
    sohp = recpkt;			/* Initialize pointers to it. */
    rdatap = recpkt;
    rsn = rln = -1;			/* In case of failure. */
    e = (turn) ? turnch : eol;		/* Use any handshake char for eol */

/* Try to get a "line". */

/* MUST CHANGE TTINL() to discard everything up to stchr! */

    j = ttinl(recpkt,r_pkt[k].bf_len - 1,timint,e);
    if (j < 0) {
	freerbuf(k);			/* Free this buffer */
	if (j < -1) return(j);		/* Bail out if ^C^C typed. */
	debug(F101,"rpack: ttinl fails","",j);
	if (nakstate)
	  screen(SCR_PT,'T',(long)winlo,"");
	else
	  screen(SCR_PT,'T',(long)pktnum,"");
	if (pktlog) zsoutl(ZPFILE,"<timeout>");
	return('T');			/* Otherwise, call it a timeout. */
    }
    tlci += j;				/* All OK, Count the characters. */
    flci += j;

/* THEN eliminate this loop... */

    for (i = 0; (recpkt[i] != stchr) && (i < j); i++)
      sohp++;				/* Find mark */
    if (i++ >= j) {			/* Didn't find it. */
	if (pktlog) zsoutl(ZPFILE,"<timeout>");
	freerbuf(k);
	return('T');
    }    
    rpackets++;
    lp = i;				/* Remember LEN position. */
    if ((j = xunchar(recpkt[i++])) == 0) {
        if ((j = lp+5) > MAXRP) return('Q'); /* Long packet */
	x = recpkt[j];			/* Header checksum. */
	recpkt[j] = '\0';		/* Calculate & compare. */
	if (xunchar(x) != chk1(recpkt+lp)) {
	    freerbuf(k);
	    if (pktlog) zsoutl(ZPFILE,"<crunched:hdr>");
	    return('Q');
	}
	recpkt[j] = x;			/* Checksum ok, put it back. */
	rln = xunchar(recpkt[j-2]) * 95 + xunchar(recpkt[j-1]) - bctu;
	j = 3;				/* Data offset. */
    } else if (j < 3) {
	debug(F101,"rpack packet length less than 3","",j);
	freerbuf(k);
	if (pktlog) zsoutl(ZPFILE,"<crunched:len>");
	return('Q');
    } else {
	rln = j - bctu - 2;		/* Regular packet */
	j = 0;				/* No extended header */
    }
    rsn = xunchar(recpkt[i++]);		/* Sequence number */
    if (pktlog) {			/* Log what we got */
	sprintf(plog,"r-%02d-%02d-",rsn,(gtimer()%60));
	zsout(ZPFILE,plog);
	if (*sohp) zsoutl(ZPFILE,sohp); else zsoutl(ZPFILE,"");
    }	
    if (rsn < 0 || rsn > 63) {
	debug(F101,"rpack bad sequence number","",rsn);
	freerbuf(k);
	if (pktlog) zsoutl(ZPFILE,"<crunched:seq>");
	return('Q');
    }
    type = recpkt[i++];			/* Packet type */
    i += j;				/* Where data begins */
    rdatap = recpkt+i;			/* The data itself */
    if ((j = rln + i) > r_pkt[k].bf_len ) {
	debug(F101,"packet sticks out too far","",j);
	freerbuf(k);
	if (pktlog) zsoutl(ZPFILE,"<overflow>");
	return('Q');
    }
    for (x = 0; x < bctu; x++)		/* Copy out the block check */
      pbc[x] = recpkt[j+x];
    pbc[x] = '\0';			/* Null-terminate block check string */
    recpkt[j] = '\0';			/*  and the packet data. */
    debug(F101,"rpack bctu","",bctu);

    switch (bctu) {			/* Check the block check */
	case 1:
	    if (xunchar(*pbc) != chk1(recpkt+lp)) {
		debug(F110,"checked chars",recpkt+lp,0);
	        debug(F101,"block check","",xunchar(*pbc));
		debug(F101,"should be","",chk1(recpkt+lp));
		freerbuf(k);
		if (pktlog) zsoutl(ZPFILE,"<crunched:chk1>");
		return('Q');
 	    }
	    break;
	case 2:
	    x = xunchar(*pbc) << 6 | xunchar(pbc[1]);
	    if (x != chk2(recpkt+lp)) {
		debug(F110,"checked chars",recpkt+lp,0);
	        debug(F101,"block check","", x);
		debug(F101,"should be","", chk2(recpkt+lp));
		freerbuf(k);
		if (pktlog) zsoutl(ZPFILE,"<crunched:chk2>");
		return('Q');
	    }
	    break;
	case 3:
	    crc = (xunchar(pbc[0]) << 12)
	        | (xunchar(pbc[1]) << 6)
		| (xunchar(pbc[2]));
	    if (crc != chk3(recpkt+lp)) {
		debug(F110,"checked chars",recpkt+lp,0);
	        debug(F101,"block check","",xunchar(*pbc));
		debug(F101,"should be","",chk3(recpkt+lp));
		freerbuf(k);
		if (pktlog) zsoutl(ZPFILE,"<crunched:chk3>");
		return('Q');
	    }
	    break;
	default:			/* Shouldn't happen... */
	    freerbuf(k);
	    if (pktlog) zsoutl(ZPFILE,"<crunched:chkx>");
	    return('Q');
    }
    debug(F101,"rpack block check OK","",rsn);

/* Now we can believe the sequence number, etc. */
/* Here we violate strict principles of layering, etc, and look at the  */
/* packet sequence number.  If there's already a packet with the same   */
/* number in the window, we remove this one so that the window will not */
/* fill up. */

    if ((x = rseqtbl[rsn]) != -1) {	/* Already a packet with this number */
	retrans++;			/* Count it for statistics */
	debug(F101,"rpack got dup","",rsn);
	if (pktlog) zsoutl(ZPFILE,"<duplicate>");
	freerbuf(x);			/* Free old buffer, keep new packet. */
	r_pkt[k].pk_rtr++;		/* Count this as a retransmission. */
    }

/* New packet, not seen before, enter it into the "database". */

    rseqtbl[rsn] = k;			/* Make back pointer */
    r_pkt[k].pk_seq = rsn;		/* Record in packet info structure */
    r_pkt[k].pk_typ = type;		/* Sequence, type,... */
    r_pkt[k].pk_adr = rdatap;		/* pointer to data buffer */
    screen(SCR_PT,type,(long)rsn,sohp);	/* Update screen */
    return(type);			/* Return packet type */
}
