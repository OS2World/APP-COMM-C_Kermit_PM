char *ckxv = "OS/2 console i/o, 13-Apr-90";
char *ckxsys = "\nA PM 1.2 version\nWith some Tektronix support\n27-May-90\n";
 
/* C K O T I O  --  Kermit console i/o support for OS/2 systems */

/* Also contains code to emulate the Unix 'alarm' function under OS/2 */
 
/*
 Author: Chris Adie (C.Adie@uk.ac.edinburgh)
 Copyright (C) 1988 Edinburgh University Computing Service
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained.
*/

/*
Modifications and additions for Presentation Manager by C.P.Armstrong
Copyright (C) 1990 C.P.Armstrong.
*/

/*
ttinc() modified wrt timeouts.  The "untimed" timeout setting assumed rdch()
returned immediately if no char waiting.  Not so. The DosRead waits forever
and is impervious to the alarm signals for some reason.  So ttins() for 
"untimed"
reads sets a timeout of 0.1s then goes ahead and repeatedly checks the rdch()
return value for a character.

Note that timo means centiseconds plus 0.01 according to MS Prog.Ref. Vol3 p336
*/


#define INCL_BASE   /* This is needed to pull in the stuff from os2.h */
#define INCL_AVIO   /* THis is needed to pull in even more!           */
#define INCL_WININPUT
#include <os2.h>            /* This pulls in lots of stuff */

/* Includes */
 
#include <ctype.h>          /* Character types */
#include <stdio.h>          /* Standard i/o */
#include <io.h>             /* File io function declarations */
#include <process.h>            /* Process-control function decl's */
#include <string.h>         /* String manipulation declarations */
#include <stdlib.h>         /* Standard library declarations */
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>           /* Time functions */
#include <direct.h>         /* Directory function declarations */
#include <signal.h>
#include "ckcker.h"         /* Kermit definitions */
#include "ckofns.h"
#include "ckcdeb.h"         /* Typedefs, debug formats, etc */



/*
 Variables available to outside world:
 
   dftty  -- Pointer to default tty name string, like "/dev/tty".
   dfloc  -- 0 if dftty is console, 1 if external line.
   dfprty -- Default parity
   dfflow -- Default flow control
   ckxech -- Flag for who echoes console typein:
     1 - The program (system echo is turned off)
     0 - The system (or front end, or terminal).
   functions that want to do their own echoing should check this flag
   before doing so.
 
 Functions for assigned communication line (either external or console tty):
 
   sysinit()               -- System dependent program initialization
   syscleanup()            -- System dependent program shutdown
   ttopen(ttname,local,mdmtyp) -- Open the named tty for exclusive access.
   ttclos()                -- Close & reset the tty, releasing any access lock.
   ttpkt(speed,flow,parity)-- Put the tty in packet mode
                or in DIALING or CONNECT modem control state.
   ttvt(speed,flow)        -- Put the tty in virtual terminal mode.
   ttinl(dest,max,timo)    -- Timed read line from the tty.
   ttinc(timo)             -- Timed read character from tty.
   ttchk()                 -- See how many characters in tty input buffer.
   ttxin(n,buf)            -- Read n characters from tty (untimed).
   ttol(string,length)     -- Write a string to the tty.
   ttoc(c)                 -- Write a character to the tty.
   ttflui()                -- Flush tty input buffer.
   ttspeed()               -- Speed of tty line.
   
Functions for console terminal:
 
   conraw()  -- Set console into Raw mode
   concooked() -- Set console into Cooked mode
   conoc(c)  -- Unbuffered output, one character to console.
   conol(s)  -- Unbuffered output, null-terminated string to the console.
   conola(s) -- Unbuffered output, array of strings to the console.
   conxo(n,s) -- Unbuffered output, n characters to the console.
   conchk()  -- Check if characters available at console (bsd 4.2).
        Check if escape char (^\) typed at console (System III/V).
   coninc(timo)  -- Timed get a character from the console.
 Following routines are dummies:
   congm()   -- Get console terminal mode.
   concb()   -- Put console into single char mode with no echo.
   conres()  -- Restore console to congm mode.
   conint()  -- Enable console terminal interrupts.
   connoi()  -- No console interrupts.
 
Time functions
 
   sleep(t)  -- Like UNIX sleep
   msleep(m) -- Millisecond sleep
   ztime(&s) -- Return pointer to date/time string
   rtimer() --  Reset timer
   gtimer()  -- Get elapsed time since last call to rtimer()
*/


/* Defines */

#define DEVNAMLEN   14

/* Declarations */
 
/* dftty is the device name of the default device for file transfer */
/* dfloc is 0 if dftty is the user's console terminal, 1 if an external line */
 
extern int parity;
CHAR dopar();

    char *dftty = "COM1";
    int dfloc = 1;
 
    int far dfprty = 0;     /* Default parity (0 = none) */
    int far ttprty = 0;     /* Parity in use. */
    int far ttmdm = 0;      /* Modem in use. */
    int far dfflow = 1;     /* Xon/Xoff flow control */
    int far odsr=1;         /* Flow control with DSR (output) ON default */
    int far idsr=1;         /* Flow control with DSR (input)  ON default */
    int far octs=1;         /* Flow control with CTS (output) ON default */

    int backgrd = 0;            /* Assume in foreground (no '&' ) */
 
int ckxech = 1; /* 0 if system normally echoes console characters, else 1 */
 
/* Declarations of variables global within this module */
 
static struct rdchbuf_rec {     /* Buffer for serial characters */
    unsigned char buffer[256];
    USHORT length, index;
} rdchbuf;

static long tcount;         /* Elapsed time counter */
 
static HFILE ttyfd = -1;        /* TTY file descriptor */

DCBINFO ttydcb;

static char xtnded = 0;         /* Pending extended keystroke? */
static char xtndedcode;         /* Pending extended keystroke. */


/*  S Y S I N I T  --  System-dependent program initialization.  */
 
sysinit() {
    conraw();                     /* Keyboard to raw mode */
    PM_init("C-Kermit for OS/2"); /* Initialise the Presentation */
                                  /* Manager thread */
    return(0);
}


/*  S Y S C L E A N U P  --  System-dependent program cleanup.  */
 
syscleanup() {
    concooked();    /* Keyboard to cooked mode */

    /* Kermit has been causing "System internal errors at 00A8:56E2" which */
    /* crash OS/2 and are extremely annoying.  These happen mid program    */
    /* sometimes, especially doing a directory but mainly on program       */
    /* shutdown.  So shutdown the PM thread specifically.                  */
    killpm();

    return(0);
}


/*  T T O P E N  --  Open a tty for exclusive access.  */
 
/*  Returns 0 on success, -1 on failure.  */
/*
  If called with lcl < 0, sets value of lcl as follows:
  0: the terminal named by ttname is the job's controlling terminal.
  1: the terminal named by ttname is not the job's controlling terminal.
  But watch out: if a line is already open, or if requested line can't
  be opened, then lcl remains (and is returned as) -1.
*/
ttopen(ttname,lcl,modem) char *ttname; int *lcl, modem; {
 
    char *x; extern char* ttyname();
    char cname[DEVNAMLEN+4];
    USHORT action;
    BYTE dummy;
 
    if (ttyfd != -1) return(0);     /* If already open, ignore this call */
    if (*lcl == 0) return(-1);      /* Won't open in local mode */
    
    ttmdm = modem;          /* Make this available to other fns */
    if (DosOpen(ttname,&ttyfd,&action,0L,0,1,
            OPEN_ACCESS_READWRITE | 
            OPEN_SHARE_DENYREADWRITE | 
            OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_WRITE_THROUGH,
            0L)) 
        {
        ttyfd = -1;
        return(-1);
        }
 
    rdchbuf.length = rdchbuf.index = 0;
    debug(F111,"ttopen ok",ttname,*lcl);

/* Caller wants us to figure out if line is controlling tty */
    if (*lcl == -1) {
    *lcl = 1;           /* Can never transfer with console */
    }
 
    /* Read DCB */
    if (DosDevIOCtl(&ttydcb,NULL,ASYNC_GETDCBINFO,1,ttyfd)) {
        ttyfd = -1;         /* Not a serial port */
        return(-1);
    }

    ttydcb.fbTimeout &= 0xF9;
    ttydcb.fbTimeout |= 0x04;   /* Read "some" data from line mode */

//    /* Read "some" data from line mode */
//    ttydcb.fbTimeout |= MODE_READ_TIMEOUT;   
    
//    ttydcb.fbTimeout ^= MODE_NO_WRITE_TIMEOUT;  /* Disable infinite writes */
//    ttydcb.fbTimeout |= MODE_NO_WRITE_TIMEOUT;  /* write waits             */

//    ttydcb.usWriteTimeout = 999;             /* 10 seconds */

    if(DosDevIOCtl(NULL,&ttydcb,ASYNC_SETDCBINFO,1,ttyfd)!=0)
        {
        printf("ttopen - Failed to set line parameters\n");
        return(-1);
        }
    ttprty = dfprty;        /* Make parity the default parity */
    if (ttsettings(ttprty,0)) return(-1);
    return(ttflui());
}

/*  T T S E T T I N G S  --  Set the device driver parity and stop bits */

ttsettings(par,stop) int par, stop; {
    char data[4];


    /* Get line */
    if (DosDevIOCtl(data,NULL,ASYNC_GETLINECTRL,1,ttyfd)) return(-1);
    switch (par) {
        case 'e':
        data[0] = 7;    /* Data bits */
        data[1] = 2;
        break;
    case 'o':
        data[0] = 7;    /* Data bits */
        data[1] = 1;
        break;
    case 'm':
        data[0] = 7;    /* Data bits */
        data[1] = 3;
        break;
    case 's':
        data[0] = 7;    /* Data bits */
        data[1] = 4;
        break;
    default :
        data[0] = 8;    /* Data bits */
        data[1] = 0;    /* No parity */
    }
    switch (stop) {
        case 2:
            data[2] = 2;    /* Two stop bits */
            break;
        case 1:
            data[2] = 0;    /* One stop bit */
            break;
        default: ;      /* No change */
    }
    /* Set line */
    if (DosDevIOCtl(NULL,data,ASYNC_SETLINECTRL,1,ttyfd)) return(-1);
    return(0);
}

/*  T T I S C O M  --  Is the given handle an open COM port? */

ttiscom(HFILE f) {
    ttclos();
    ttyfd = f;
    /* Read DCB */
    if (DosDevIOCtl(&ttydcb,NULL,ASYNC_GETDCBINFO,1,ttyfd)) {
        ttyfd = -1;         /* Not a serial port */
        return( 0 /* bad */ );
    }
    return( 1 /* good */ );
}

/*  T T C L O S  --  Close the TTY.  */
 
ttclos() {
    if (ttyfd == -1) return(0);     /* Wasn't open. */
    DosClose(ttyfd);
    ttyfd = -1;
    return(0);
}

/*  T T S P E E D  --  return speed of tt line, or of default line */

ttspeed() {
    char df;
    int sp=-1;
    
    df = (ttyfd == -1);
    if (df) if (ttopen(dftty,&sp,0)) return(-1);
    if (DosDevIOCtl(&sp,NULL,ASYNC_GETBAUDRATE,1,ttyfd)) sp=-1;
    return(sp);
}

int ttgspd()
    {
    return(ttspeed());
    }
 
/*  T T H A N G -- Hangup phone line */
 
tthang() {
    char parms[2];
    USHORT data;
    
    if (ttyfd == -1) return(0);     /* Not open, so must be hung up */
    parms[0] = 0x00;
    parms[1] = 0xFE;            /* Drop DTR */
    return(DosDevIOCtl(&data,parms,ASYNC_SETMODEMCTRL,1,ttyfd));
}
 
 
/*  T T R E S  --  Restore terminal to "normal" mode.  */
 
ttres() {               /* Restore the tty to normal. */
    if (ttyfd == -1) return(-1);    /* Not open */
    return(0);
}

#define DIALING 4       /* for ttpkt parameter */
#define CONNECT 5

/*  T T P K T  --  Condition the communication line for packets. */
/*      or for modem dialing */
 
/*  If called with speed > -1, also set the speed.  */
/*  Returns 0 on success, -1 on failure.  */
 
ttpkt(speed,flow,parity) int speed, flow, parity; {
    int s;
    USHORT x;
 
    if (ttyfd < 0) return(-1);      /* Not open. */

    if (speed > -1) {
    if ((x = ttsspd(speed))<0) return(-1);      /* Check the speed */
    /* Set the speed */
    if (DosDevIOCtl(NULL,&x,ASYNC_SETBAUDRATE,1,ttyfd)) return(-1);
    }

    ttprty = parity;
    if (ttsettings(ttprty,0)) return(-1);
    debug(F101,"ttpkt setting ttprty","",ttprty);

    if (flow == DIALING) {      /* Ignore DCD */
    debug(F100,"ttpkt DIALING","",0);
    /* Driver default is to do so */
    } else
    if (flow == CONNECT) {      /* Pay attention to DCD */
    debug(F100,"ttpkt CONNECT","",0);
    /* We're warned against doing so, for some reason, so no action */
    } else
    return(os2setflow(flow));
}


/*  T T V T -- Condition communication line for use as virtual terminal  */
 
ttvt(speed,flow) int speed, flow; {
    USHORT x;

    if (ttyfd < 0) return(-1);      /* Not open. */

    if (speed > -1) {
    if ((x = ttsspd(speed))<0) return(-1);      /* Check the speed */
    /* Set the speed */
    if (DosDevIOCtl(NULL,&x,ASYNC_SETBAUDRATE,1,ttyfd)) return(-1);
    }
    ttprty = parity;
    if (ttsettings(ttprty,0)) return(-1);

    return(os2setflow(flow));
}


/*  O S 2 S E T F L O W -- set flow state of tty */

/*  If flow = 0 turn receive flow XON/XOFF control off, otherwise turn it on.
    If successful, return 0, otherwise return -1.  */

os2setflow(flow) 
    int flow; 
    {
    extern HFILE ttyfd;
    extern DCBINFO ttydcb;
    extern int odsr;    /* Hardware handshaking controls */
    extern int idsr;
    extern int octs;

    /* Get the current settings */
    if (DosDevIOCtl(&ttydcb,NULL,ASYNC_GETDCBINFO,1,ttyfd)) 
        return(-1);
    
    ttydcb.fbFlowReplace &= 0xFD;           /* Set RX flow off */
    if (flow != 0)                          
        ttydcb.fbFlowReplace |= 0x02;       /* Set RX flow on */
        
    /* Set the DSR/CSR handshaking. Set the bits first */
    ttydcb.fbCtlHndShake |= 
        (MODE_DSR_HANDSHAKE | MODE_DSR_SENSITIVITY | MODE_CTS_HANDSHAKE);

    /* now clear them if necessary */
    ttydcb.fbCtlHndShake ^= ( (odsr!=0) ? 0 : MODE_DSR_HANDSHAKE);
    ttydcb.fbCtlHndShake ^= ( (idsr!=0) ? 0 : MODE_DSR_SENSITIVITY);
    ttydcb.fbCtlHndShake ^= ( (octs!=0) ? 0 : MODE_CTS_HANDSHAKE);

    /* Set DCB */
    if (DosDevIOCtl(NULL,&ttydcb,ASYNC_SETDCBINFO,1,ttyfd)) return(-1);

    return(0);
}


/*  T T S S P D  --  Return the speed if OK, otherwise -1 */
 
ttsspd(speed) int speed; {
    int s, spdok;
 
    if (speed < 0) return(-1);
    spdok = 1;          /* Assume arg ok */
    switch (speed) {
        case 110:
        case 150:
        case 300:
        case 600:
        case 1200:
        case 2400:
        case 4800:
        case 9600:
        case 19200: break;
        default:
            spdok = 0;
        fprintf(stderr,"Unsupported line speed - %d\n",speed);
        fprintf(stderr,"Current speed not changed\n");
        break;
    }
    if (spdok) return(speed); else return(-1);
 }


/*  T T F L U I  --  Flush tty input buffer */
 
ttflui() {
    char parm=0;
    long int data;
    int i;

    rdchbuf.index = rdchbuf.length = 0;     /* Flush internal buffer */
    DosDevIOCtl(&data,&parm,0x0001,11,ttyfd);   /* Flush driver buffer */
    return(0);
}


/*  T T C H K  --  Tell how many characters are waiting in tty input buffer  */
 
ttchk() {
    USHORT data[2];
    if(DosDevIOCtl(data,NULL,ASYNC_GETINQUECOUNT,1,ttyfd)) return(0);
    else return((rdchbuf.length-rdchbuf.index)+data[0]);
}
 
 
/*  T T X I N  --  Get n characters from tty input buffer  */
 
/*  Returns number of characters actually gotten, or -1 on failure  */
 
/*  Intended for use only when it is known that n characters are actually */
/*  available in the input buffer.  */
 
ttxin(n,buf) int n; char *buf; {
    int i, j;
    
    if (ttyfd < 0) return(-1);      /* Not open. */
    i = 0;
    while (i < n) {
        if ((j = ttinc(0)) < 0) break;
        buf[i++] = j;
    }
    return(i);
}

/*  T T O L  --  Similar to "ttinl", but for writing.  */
 
ttol(s,n) int n; char *s; {
    USHORT i;
    CHAR pkt[1100];         /* So big that we needn't check */
    
    if (ttyfd < 0) return(-1);      /* Not open. */

    if (!parity) 
        {
        if(DosWrite(ttyfd,s,n,&i)) 
            return(-1);
        else 
            return(i);
        } 
    else 
        {
        for (i=0;i<n;i++) 
            pkt[i]=dopar(s[i]);
        if (DosWrite(ttyfd,pkt,n,&i)) 
            return(-1);
        else 
            return(i);
        }
}
 
 
/*  T T O C  --  Output a character to the communication line  */
 
ttoc(c) char c; {
    USHORT i;
    CHAR ch = dopar(c);
    
    if (ttyfd < 0) return(-1);      /* Not open. */
    if(DosWrite(ttyfd,&ch,1,&i)) 
        {
        bleep();
        return(-1);
        }
    else 
        return(i);
}


/*  T T O C I  --  Output a character to the communication line immediately */
 
ttoci(c) char c; {
    USHORT i;
    CHAR ch = dopar(c);
    
    if (ttyfd < 0) return(-1);      /* Not open. */
    if(DosDevIOCtl(NULL,&ch,ASYNC_TRANSMITIMM,1,ttyfd)) return(-1);
    else return(1);
}


/*  T T I N L  --  Read a record (up to break character) from comm line.  */
/*
  If no break character encountered within "max", return "max" characters,
  with disposition of any remaining characters undefined.  Otherwise, return
  the characters that were read, including the break character, in "dest" and
  the number of characters read as the value of the function, or 0 upon end of
  file, or -1 if an error occurred.  Times out & returns error if not completed
  within "timo" seconds.
*/
ttinl(dest,max,timo,eol) int max,timo; CHAR *dest, eol; {
    int x = 0, c, i, m;
 
    if (ttyfd < 0) return(-1);      /* Not open. */
    *dest = '\0';           /* Clear destination buffer */
    i = 0;              /* Next char to process */
    while (1) {
    if ((c = ttinc(timo)) == -1) {
        x = -1;
        break;
    }
        dest[i] = c;            /* Got one. */
    if (dest[i] == eol) {
        dest[++i] = '\0';
        return(i);
    }
    if (i++ > max) {
        debug(F101,"ttinl buffer overflow","",i);
//        debug(F101,"ttinl buffer overflow","",j);
        x = -1;
        break;
    }
    }
    debug(F100,"ttinl timout","",0);    /* Get here on timeout. */
    debug(F111," with",dest,i);
    return(x);              /* and return error code. */
}


/*  T T I N C --  Read a character from the communication line  */

/* Note that the parameter is the time to wait in *centiseconds*, not secs. */
/* The time should be in secs for consitency with the other modules in      */
/* kermit.  To retain the option of using times of less than 1s a negative  */ 
/* parameter is interpreted as meaning multiples of 0.01s                  */
ttinc(timo) int timo; {
    int m, i;
    char ch = 0;
 
    m = (ttprty) ? 0177 : 0377;     /* Parity stripping mask. */
    if (ttyfd < 0) return(-1);      /* Not open. */
    if (timo == 0) 
        {                               /* Untimed. */
        ttydcb.usReadTimeout = 9;       /* Test every  0.1s per call */
        if (DosDevIOCtl(NULL,&ttydcb,ASYNC_SETDCBINFO,1,ttyfd)) return(-1);
        do i = rdch(); while (i < 0);   /* Wait for a character. */
        return(i & m);
        } 
    else if(timo<0)
        {
        timo= (timo*(-1))-1;            /* Convert time from cseconds */
        }                               /* OS/2 units */
    else 
        {
        timo = (timo*100)-1;        /* Convert timo from seconds */
        }                           /* to OS/2 units */

    if (timo != ttydcb.usReadTimeout+1) 
        { /* Set timeout value */
        ttydcb.usReadTimeout = timo;
        if (DosDevIOCtl(NULL,&ttydcb,ASYNC_SETDCBINFO,1,ttyfd)) return(-1);
        }

    i = rdch();
    if (i < 0) 
        return(-1);
    else 
        return(i & m);
    }

/*  RDCH -- Read characters from the serial port, maintaining an internal
            buffer of characters for the sake of efficiency. */
static rdch() {

    if (rdchbuf.index == rdchbuf.length) {
        rdchbuf.index = 0;
        if (DosRead(ttyfd,rdchbuf.buffer,
                sizeof(rdchbuf.buffer),&rdchbuf.length)) {
            rdchbuf.length = 0;
                return(-1);
            }
        }
    return( (rdchbuf.index < rdchbuf.length) ? rdchbuf.buffer[rdchbuf.index++]
                         : -1 );
}    


/*  T T S N D B  --  Send a BREAK signal  */
 
ttsndb() {
    USHORT i;
    
    DosDevIOCtl(&i,NULL,ASYNC_SETBREAKON,1,ttyfd);  /* Break on */
    DosSleep(1000L);                    /* ZZZzzz */
    DosDevIOCtl(&i,NULL,ASYNC_SETBREAKOFF,1,ttyfd); /* Break off */
}

/*  S L E E P  --  Emulate the Unix sleep function  */

sleep(t) unsigned t; {
    ULONG lm = t*1000L;

    DosSleep(lm);
}


/*  M S L E E P  --  Millisecond version of sleep().  */
 
/* Intended only for small intervals.  For big ones, just use sleep(). */
 
msleep(m) int m; {
    ULONG lm = m;
    
    DosSleep(lm);
}


/*  R T I M E R --  Reset elapsed time counter  */
 
rtimer() {
    tcount = time( (long *) 0 );
}
 
 
/*  G T I M E R --  Get current value of elapsed time counter in seconds  */
 
gtimer() {
    int x;
    x = (int) (time( (long *) 0 ) - tcount);
    rtimer();
    return( (x < 0) ? 0 : x );
}
 
 
/*  Z T I M E  --  Return date/time string  */
 
ztime(s) char **s; {
    long clock_storage;
 
    clock_storage = time( (long *) 0 );
    *s = ctime( &clock_storage );
}

/*  C O N O C  --  Output a character to the console terminal  */
 
conoc(c) char c; {
//    write(1,&c,1);
    AVIOwrttyc(&c,1);
}
 

/*  C O N X O  --  Write x characters to the console terminal  */
 
conxo(x,s) char *s; int x; {
//    write(1,s,x);
      AVIOwrttyc(s,x);
}
 

/*  C O N O L  --  Write a NULL termintated line to the console terminal  */
 
conol(s) char *s; {
    int len;
    len = strlen(s);
//    write(1,s,len);
    AVIOwrttyc(s,len);
}
 

/*  C O N O L A  --  Write an array of lines to the console terminal */
 
conola(s) char *s[]; {
    int i;
    for (i=0 ; *s[i] ; i++) conol(s[i]);
}
 

/*  C O N O L L  --  Output a string followed by CRLF  */
 
conoll(s) char *s; {
    conol(s);
//    write(1,"\r\n",2);
    conol("\r\n");    
}


/*  C O N C H K  --  Return how many characters available at console  */
 
conchk() {
    extern next_put_pos;
    extern next_get_pos;
    int waiting;

    DosEnterCritSec();
    if(next_put_pos< next_get_pos)
        {
        waiting = 128-next_get_pos+next_put_pos;
        }
    else
        {
        waiting = next_put_pos - next_get_pos;
        }
    
    DosExitCritSec();
    return(waiting);

}


/*  C O N I N C  --  Get a character from the console  */
 
coninc(timo) int timo; {
    /* returns -1 if no character received within timo secs */
    int c;

    if(timo>0)
        {
        c = buff_tgetch((LONG)(timo*1000));   /* Return after waiting */
        if(c==-1)                             /* timo seconds */
            return(c);
        }
    else
        c = buff_getch();              /* Indefinite wait */


    if((c&0xFF)==0)
        {
        c = c>>8;                      /* Command line needs backspace to be */
        if(c==VK_BACKSPACE)            /* to be 8, VMS needs backspace to be */
            c=8;                       /* 127, Unix needs to have both!      */
        }
    else
        c &= 0xFF;

    return(c&0xFF);
}


conraw() {
    KBDINFO k;
    
    k.cb = 10;
    k.fsMask = 0x06;    /* Set raw mode */
    k.chTurnAround = 0;
    k.fsInterim = 0;
    k.fsState = 0;
    return(0);  /* Need to find out how this is done */
//    return(KbdSetStatus(&k,0));
}

concooked() {
    KBDINFO k;
    
    k.cb = 10;
    k.fsMask = 0x09;    /* Set cooked mode */
    k.chTurnAround = 0;
    k.fsInterim = 0;
    k.fsState = 0;
    return(0);      /* Need to find out if this is possible under PM */
//    return(KbdSetStatus(&k,0));
}

/*  C O N G M  -- Get console terminal mode. */

congm() {}


/*  C O N C B  -- Put console into single char mode with no echo. */

concb(esc) char esc; {}


/*  C O N R E S -- Restore console to congm mode. */

conres() {}


/*  C O N I N T -- Enable console terminal interrupts. */

conint(f) int (*f)(); {}


/*  C O N N O I -- No console interrupts. */

connoi() {}

/*****************************************************************************/

/*
  The following code tries to implement the Unix 'alarm' function under OS/2.
*/

#include <signal.h>
#include <malloc.h>

#define THRDSTKSIZ  2048
static  char *alarmstack;
static  ULONG alarmtime;
static  ULONG tsem = 0, hold = 0;
static  int pid;
static  int initialised = 0;

VOID FAR alarmthread(void) 
    {
    while(1) 
        {
        DosSemSetWait(&hold,-1L);   /* Wait until we're told to go */
        if ( DosSemSetWait(&tsem,alarmtime*1000L) == ERROR_SEM_TIMEOUT ) 
            {
//            raise(SIGUSR1);
//            if(DosFlagProcess(pid,1,0,0))
              if(DosFlagProcess(pid,FLGP_PID,PFLG_A,0))
                printf("Failed to raise timeout signal!\n");
            }
        }
    }

alarminit() {
    USHORT threadid;
    pid = getpid();
    if ( (alarmstack=malloc(THRDSTKSIZ)) == NULL) {
        printf("Not enough space for alarm thread stack\n");
        doexit(BAD_EXIT);
    }

    if((threadid=_beginthread(alarmthread,alarmstack,THRDSTKSIZ,NULL))==-1)
        {
        printf("Can't create alarm thread\n");
        doexit(BAD_EXIT);
        }
    while (!hold) DosSleep(300L);   /* Wait for thread to start */
    initialised = 1;
}

/*  A L A R M  --  Issue a signal after a specified time */

/* Call with t=0 to cancel an alarm.  t>0 is the time in seconds after which
   a signal will be generated.  The signal will be a User Flag A signal, which
   thread 1 must acknowledge with a call to alarmack().  The time t can be
   up to 64k seconds.
   */
void alarm(t) unsigned t; {
    if (!initialised) alarminit();
    alarmtime = t;          /* In seconds */
    DosSemClear(&tsem);     /* stopping an existing alarm */
    if (t>0) DosSemClear(&hold);    /* starting a new one */
}

/*  A L A R M A C K  --  Acknowledge alarm call received */

alarmack() {
    signal(SIGUSR1,SIG_ACK);
}

zstime(char * f, struct zattr *yy)
    {
    return(0);
    }
    
int os2_set_hand(y)
    int y;
    {     
    extern int com_odsr;
    extern int com_idsr;
    extern int com_octs;
    
    switch(y)
        {
        case ODSR:
            return( seton(&odsr) );
        case IDSR:
            return( seton(&idsr) );
        case OCTS:
            return( seton(&octs) );
        default:
            return(-2);
        }
    }
    
int ttsetspeed(int speed)
    {
    extern HFILE ttyfd;
    USHORT x;

    /* Check it's a valid speed */
    if ((x = ttsspd(speed))<0) 
        return(-1);

    /* Set it */
    if (DosDevIOCtl(NULL,&x,ASYNC_SETBAUDRATE,1,ttyfd)) 
        return(-1);

    return(0);
    }

int ttnewport(s)
char * s;       /* The name of the new port to open */
    {
    extern char ttname[];
    extern char * dftty;        /* Default line name */
    extern int dfloc;           /* ? */
    extern int mdmtyp;          /* Modem type */
    extern int tvtflg;          /* ? */
    extern int success;    
    extern int local;
    extern int network;
    extern int speed;
    extern int parity;
    extern int stop;
    extern int flow;

    int x;
    int y;

    x = strcmp(s,dftty) ? -1 : dfloc; /* Maybe let ttopen figure it out */
    y = mdmtyp;

    ttclos();               /* close old line, if any was open */
    tvtflg = 0;
    if (ttopen(s,&x,y) < 0 ) 
        {                   /* Can we open the new line? */
        local = dfloc;      /* Go back to normal */
        network = 0;
        return(success = 0);/* and return failure */
        }

    if (x > -1) 
        local = x;          /* Opened ok, set local/remote. */

    network = (y < 0);          /* Remember connection type. */
    strcpy(ttname,s);           /* Copy name into real place. */

    /* Set the parity */
    ttsettings(parity,0);
    
    /* Set the speed */
    ttsetspeed(speed);

    /* Set the flow*/
    os2setflow(flow);

    debug(F111,"set line ",ttname,local);
    return(success = 1);
    }
