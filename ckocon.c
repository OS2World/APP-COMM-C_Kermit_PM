 char           *connv = "OS/2 connect command, 26-Jan-90";
/* C K O C O N  --  Kermit connect command for OS/2 systems */
/*
 * Author: Chris Adie (C.Adie@uk.ac.edinburgh) Copyright (C) 1988 Edinburgh
 * University Computing Service Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it is
 * not sold for profit, provided this copyright notice is retained. 
 *
 *
 * Incorporates a VT102 emulator, together with its screen access routines.  If
 * the code looks a bit funny sometimes, its because it was machine
 * translated to 'C'. 
 */

/*
26-Jan-90 C.P.Armstrong puts() added for PM
*/

/*
 *
 * =============================#includes===================================== 
 */

#include <ctype.h>      /* Character types */
#include <io.h>         /* File io function declarations */
#include <process.h>    /* Process-control function declarations */
#include <stdlib.h>     /* Standard library declarations */
#include <sys\types.h>
#include <sys\stat.h>
#include <direct.h>     /* Directory function declarations */
#include <stdio.h>
#include <string.h>
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#define INCL_WININPUT
#define INCL_VIO
#include <os2.h>     
//#include "ckofns.h"  /* No room for this!! */
//#include "ckopm.h"
#include "ckcasc.h"
#include "ckcker.h"     /* Kermit definitions */
#include "ckokey.h"
#include "ckofns.h"
/*
 *
 * =============================#defines====================================== 
 */

/* in ckodrv.c   */ 
 void print_screen(char far *);
/* in ckopm2.c */
void AVIOReadCellStr();
void AVIOWrtCellStr();
void AVIOScrollRt();
void AVIOScrollLf();
void AVIOWrtNCell();
void AVIOWrtCharStrAtt();
void AVIOScrollUp();
void AVIOSetCurPos();
void AVIOGetCurType();
void AVIOSetCurType();
void AVIOGetCurPos();
int PM_getViotitle();
void PM_setViotitle();
/* in ckopm1.c */
int buff_getch();

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#define SENDSTR2    sendstr
#define SENDCHAR    sendchar
#define UPWARD      6
#define DOWNWARD    7
#define LBUFSIZE    144 /* No of lines in extended buffer */
#define DEFTABS     \
"00000000T0000000T0000000T0000000T0000000T0000000T0000000T0000000T0000000T0000000";

/*
 *
 * =============================typedefs====================================== 
 */

typedef char    fulstring[256];
typedef int     bool;
typedef unsigned char screenmap[4000];
typedef struct ascreen_rec {    /* Structure for saving screen info */
    unsigned char   ox;
    unsigned char   oy;
    unsigned char   att;
    screenmap       scrncpy;
}               ascreen;

/*
 *
 * =============================externals===================================== 
 */

extern int      local, speed, escape, duplex, parity, flow, seslog, cmask;
extern char     ttname[], sesfil[];
extern CHAR     dopar();
extern bool     Term_mode;
/*
 *
 * =============================static=variables============================== 
 */

int  far conected=0;
long twochartimes;
char termessage[80];
static FILE    *lst;
static bool     lstclosed = TRUE;
static char     coloroftext = 7, colorofback = 0;
int colorofunderline = 4;
ascreen  vt100screen, commandscreen;
static enum {
    mono,
    colour
}               adapter;
unsigned char   attribute, savedattribute, line25attribute;
unsigned char defaultattribute=0x07;
static struct {
    unsigned        reversed:1;
    unsigned        blinking:1;
    unsigned        underlined:1;
    unsigned        bold:1;
    unsigned        invisible:1;
}               attrib, savedattrib;
static struct paging_record {
    unsigned char   numlines;   /* no. of lines in extended display buffer */
    unsigned char   topline;
    unsigned char   botline;
    char           *buffer;
};

static struct paging_record far paginginfo =
    {0,0,0,NULL};

unsigned char wherex;
unsigned char wherey;
unsigned char margintop = 1;
unsigned char marginbot = 24;
int      active;

long int far threadsem=0;   /* Semaphore to show thread is running */
ULONG far cr_recd_sem=0;    /* Semaphore to indicate an eol char received */
int far paste_eol_char=10;  /* The end of line char */
static char     usertext[(80) + 1];
static char     exittext[(14) + 1];
static char     helptext[(14) + 1];
static char     filetext[(14) + 1];
static char     hostname[(20) + 1];

static unsigned char far graphicset[32] = {
                 0x20, 0x04, 0xB0, 0x1A, 0x17, 0x1B, 0x19, 0xF8,
                 0xF1, 0x15, 0x12, 0xD9, 0xBF, 0xDA, 0xC0, 0xC5,
                 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC3, 0xB4, 0xC1,
                 0xC2, 0xB3, 0xF3, 0xF2, 0xE3, 0x9D, 0x9C, 0xFA
};
static char     htab[81] = DEFTABS  /* Default tab settings */
static char answerback[81] = "OS/2 Kermit\n";
static int      row;
#ifdef US_CHAR
static unsigned char g0 = 'B';      /* Character 35 is number sign */
static unsigned char g1 = 'B';
#else
static unsigned char g0 = 'A';      /* Char 35 is pound (sterling) sign */
static unsigned char g1 = 'A';
#endif /* US_CHAR */
static unsigned char *g0g1 = &g0;
static bool     literal = FALSE;
static bool     wrapit;
static bool     printon = FALSE;
static bool     screenon = TRUE;
bool     cursoron = TRUE;/* For speed, turn off when busy */
bool     relcursor = FALSE;
static bool     keypadnum = FALSE;
static bool     autowrap = FALSE;
bool     ansi = TRUE;
static bool     keylock = FALSE;
static bool     vt52graphics = FALSE;
unsigned char achar;
static int      column;
static unsigned char saveg0, saveg1, *saveg0g1;
static bool     saverelcursor, saved=FALSE;
bool     dwl[24] = {
       FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
       FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
               FALSE, FALSE, FALSE, FALSE};
bool     dwls = FALSE;   /* For optimisation */
static bool     deccolm = FALSE;
static bool     decscnm = FALSE;
static bool     linemode = FALSE;
static bool     insertmode = FALSE;
static bool     cursorkey = TRUE;
/* escape sequence processing buffer */
bool     escaping = FALSE;
int      escnext = 1;
int      esclast = 0;
unsigned char escbuffer[129];
static unsigned char sgrcols[8] = {0, 4, 2, 6, 1, 5, 3, 7};

/*---------------------------------------------------------------------------*/
/* toplinetocyclicbuffer                                                   */
/*---------------------------------------------------------------------------*/
void toplinetocyclicbuffer()
{
    USHORT          n = 160;
    if (paginginfo.numlines == LBUFSIZE) {
    if (++paginginfo.topline == LBUFSIZE)
        paginginfo.topline = 0;
    } else
    paginginfo.numlines++;
    if (++paginginfo.botline == LBUFSIZE)
    paginginfo.botline = 0;
    AVIOReadCellStr((paginginfo.buffer + 160 * paginginfo.botline), &n, 0, 0);
}

void sendcrlf()
    {
    extern int pasting;
    extern ULONG cr_recd_sem;


    sendcharduplex(CR);
    if (linemode)    /* N.B. the enter key comes as a VK */
        {
        sendcharduplex(LF);
        }

    return;
    }

/*
 * Send a character to the serial line in immediate mode, checking to avoid
 * overwriting a character already waiting to be sent.
 */
void sendchar(unsigned char c) 
    {
    extern int pasting;
    extern ULONG cr_recd_sem;
    int i=0;
    static int retry=5;

    DosSemSet(&cr_recd_sem);

    while (ttoc(c)<0 && i++<retry)
        DosSleep(twochartimes);

    if (i>=retry) 
        {
        active = FALSE;
        strcpy(termessage,"Cannot transmit to serial port!\n");
        }
    else if(pasting>0 && c==CR)
        {
        DosSemWait(&cr_recd_sem, 10000L);
        }
    }

/* ------------------------------------------------------------------ */
/* ipadl25 -                                                          */
/* ------------------------------------------------------------------ */
void ipadl25()
{   
    switch(Term_mode)
        {
        case TEKTRONIX:   strcpy(usertext,"C-Kermit TEK4014"); 
            PM_setViotitle("Tektronix 4010/4014 Emulation");
            break;
        case VT100:       strcpy(usertext,"C-Kermit VT102"); 
            PM_setViotitle("DEC VT102 Emulation");
            break;
        default:          strcpy(usertext,"Unknown mode"); break;
        }

    
    sprintf(filetext, "%1d baud", speed);
    strcpy(helptext, "Help: ^ ?");
    helptext[7] = (char) ctl(escape);
    strcpy(exittext, "Exit: ^ c");
    exittext[7] = (char) ctl(escape);
    strcpy(hostname, ttname);
    line25();
}
/* ------------------------------------------------------------------ */
static void
doprinton(bool on)
{
    if (on) {
    if (lstclosed) {
        lst = fopen("prn", "w");
        lstclosed = FALSE;
    }
    } else {
    if (!(lstclosed)) {
        fclose(lst);
        lstclosed = TRUE;
    }
    }
    printon = on;
}
/* ----------------------------------------------------------------- */
/* ClrScreen -                                                       */
/* ----------------------------------------------------------------- */
static void clrscreen()
{
    int             i, j;
    int             nlines;
    USHORT          n;
    char            cells[80][2];

    /* copy lines on screen to extended display buffer */
    n = sizeof(cells);
    for (i = 23; i >= 0; i--) {
    AVIOReadCellStr((char *) cells, &n, i, 0);
    for (j = 0; j < 80; j++) {
        if (cells[j][0] != 32)
        break;
    }
    if (j < 80)
        break;
    }
    nlines = i;         /* no. of nonblank lines-1 */
    for (i = 0; i <= nlines; ++i) {
    paginginfo.botline = (paginginfo.botline + 1) % LBUFSIZE;
    if (paginginfo.numlines < LBUFSIZE)
        paginginfo.numlines = paginginfo.numlines + 1;
    else
        paginginfo.topline = (paginginfo.topline + 1) % LBUFSIZE;
    AVIOReadCellStr((paginginfo.buffer + 160 * paginginfo.botline), &n, i, 0);
    }
    for (i = 0; i < 24; i++)
    dwl[i] = FALSE;
    dwls = FALSE;
    clearscreen();
}
static void
readmchar_escape()
{
    /* Stores character in achar directly */
    if (escnext <= esclast) {
    achar = escbuffer[escnext];
    escnext = escnext + 1;
    } else
    achar = 0;
}
static int pnumber(achar)
    unsigned char  *achar;
{
    int             num = 0;

    while (isdigit(*achar)) {   /* get number */
    num = (num * 10) + (*achar) - 48;
    readmchar_escape();
    }
    return (num);
}

void decdwl_escape(bool dwlflag)
{
    unsigned char   linenumber;
    unsigned char   newx;
    char            cells[80][2];
    int             i;
    USHORT          n;
    /* Decdwl */
    linenumber = wherey - 1;
    if (dwlflag != dwl[linenumber]) {
    /* change size */
    n = sizeof(cells);
    AVIOReadCellStr((char *) cells, &n, linenumber, 0);
    if (dwlflag) {      /* make this line double size */
        for (i = 39; i >= 0; --i) { /* expand */
        cells[2 * i][0] = cells[i][0];
        cells[2 * i + 1][0] = ' ';
        }
        newx = (wherex - 1) * 2 + 1;
        dwls = TRUE;
    } else {        /* make this line single size */
        for (i = 0; i <= 39; ++i)
        cells[i][0] = cells[2 * i][0];
        for (i = 40; i <= 79; ++i)
        cells[i][0] = ' ';
        newx = (wherex - 1) / 2 + 1;
        dwls = FALSE;
        for (i = 0; i < 24; i++)
        if (dwl[i]) {
            dwls = TRUE;
            break;
        }
    }
    AVIOWrtCellStr((char *) cells, n, linenumber, 0);
    dwl[linenumber] = dwlflag;
    if (newx >= 80)
        newx = 79;
    lgotoxy(newx, wherey);
    }
}    

/* Modified C.P.A. to trap switch to Tektronix mode commands */

void vtescape()
{
    unsigned char   j;
    unsigned char   k;
    unsigned char   l;
    unsigned char   blankcell[2];
    int             i;
    int             pn[11];
    bool            private;
    char            tempstr[20];
    int             fore, back;
    escaping = FALSE;
    escnext = 1;

    readmchar_escape();
    
    /* Trap the enter Tektronix mode commands - and set tektronix mode */
    if(achar==12)  /* ESC Ctrl-L Clear Tek screen - use this to enter */
        {          /* Tek mode as well as the other commands */
        if(Tek_scrinit(1)==1)   /* The other Tek entry command is */
            {                   /* ESC [ ? 3 8 h                  */
            Term_mode=TEKTRONIX;
            ipadl25();          /* Update the Vio screen mode line */
            return;
            }
        }

    if (screenon || (achar == '[')) {
    /* screen escape sequences  */
    switch (achar) {
        /* First Level */
    case '[':
        {
        /* Left square bracket */
        readmchar_escape();
        switch (achar) {/* Second level */
        case 'A':
            cursorup();
            wrapit = FALSE;
            break;
        case 'B':
            cursordown();
            wrapit = FALSE;
            break;
        case 'C':
            cursorright();
            if (dwl[wherey - 1])
            cursorright();
            break;
        case 'D':
            cursorleft();
            if (dwl[wherey - 1])
            cursorleft();
            break;
        case 'J':   /* Erase End of Display */
            clreoscr_escape();
            break;
        case 'K':
            clrtoeol();
            break;
        case '?':
            private = TRUE;
            readmchar_escape();
            goto LB2001;
        case 'f':
        case 'H':   /* Cursor Home */
            lgotoxy(1, relcursor ? margintop : 1);
            break;
        case 'g':
            htab[wherex] = '0';
            break;
        case '}':
        case 'm':   /* Normal Video - Exit all attribute modes */
            attribute = defaultattribute;
            attrib.blinking = FALSE;
            attrib.bold = FALSE;
            attrib.invisible = FALSE;
            attrib.underlined = FALSE;
            attrib.reversed = FALSE;
            break;
        case 'r':   /* Reset Margin */
            setmargins(1, 24);
            lgotoxy(1, 1);
            break;
        case 'c':
        case 'h':
        case 'l':
        case 'n':
        case 'x':
            pn[1] = 0;
            private = FALSE;
            k = 1;
            goto LB2003;
        case ';':
            pn[1] = 0;
            private = FALSE;
            k = 1;
            goto LB2002;
        case 'L':
        case 'M':
        case '@':
        case 'P':
            pn[1] = 1;
            private = FALSE;
            k = 1;
            goto LB2002;
        default:    /* Pn - got a number */
            private = FALSE;
        LB2001:
            {       /* Esc [ Pn...Pn x   functions */
            pn[1] = pnumber(&achar);
            
            /* So far the characters received should be ESC [ ? number */
            /* If number is 38 then it might be an enter Tektronix mode*/
            /* command.  pnumber leaves achar pointing to the character*/
            /* after the buffer.  If it's an h or l then we've got our */
            /* Tektronix command */
            if((pn[1] == 38) && ((achar=='h') || (achar=='l')))
                {
                if(achar=='h')
                    {
                    if(Tek_scrinit(0)==1)
                        {
                        Term_mode=TEKTRONIX;
                        ipadl25();
                        }
                    }
                return;     /* Either way return */
                }

            k = 1;
        LB2002:
            while (achar == ';') {  /* get Pn[k] */
                readmchar_escape();
                k++;
                if (achar == '?') {
                readmchar_escape();
                }
                pn[k] = pnumber(&achar);
            }
            pn[k + 1] = 1;
        LB2003:
            switch (achar) {    /* third level */
            case 'A':
                do {
                cursorup();
                wrapit = FALSE;
                pn[1] = pn[1] - 1;
                }
                while (!(pn[1] <= 0));
                break;
            case 'B':
                do {
                cursordown();
                wrapit = FALSE;
                pn[1] = pn[1] - 1;
                }
                while (!(pn[1] <= 0));
                break;
            case 'C':
                do {
                cursorright();
                if (dwl[wherey - 1])
                    cursorright();
                pn[1] = pn[1] - 1;
                }
                while (pn[1] > 0);
                break;
            case 'D':
                do {
                cursorleft();
                if (dwl[wherey - 1])
                    cursorleft();
                pn[1] = pn[1] - 1;
                } while (pn[1] > 0);
                break;
            case 'f':
            case 'H':
                /* Direct cursor address */
                if (pn[1] == 0)
                pn[1] = 1;
                if (relcursor)
                pn[1] += margintop - 1;
                if (pn[2] == 0)
                pn[2] = 1;
                if (dwl[pn[1] - 1]) {
                pn[2] = 2 * pn[2] - 1;
                if (pn[2] > 80)
                    pn[2] = 79;
                } else if (pn[2] > 80)
                pn[2] = 80;
                wrapit = FALSE;
                lgotoxy(pn[2], pn[1]);
                break;
            case 'c':   /* Device Attributes */
                if (pn[1] == 0)
                sendstr("[?6;2c");
                break;
            case 'g':
                if (pn[1] == 3) {
                /* clear all tabs */
                for (j = 1; j <= 80; ++j)
                    htab[j] = '0';
                } else if (pn[1] == 0)
                /* clear tab at current position */
                htab[wherex] = '0';
                break;
            case 'h':   /* Set Mode */
                for (j = 1; j <= k; ++j)
                if (private)
                    switch (pn[j]) {    /* Field specs */
                    case 1: /* DECCKM  */
                    cursorkey = TRUE;
                    break;
                    case 2: /* DECANM : ANSI/VT52 */
                    ansi = TRUE;
                    vt52graphics = FALSE;
                    break;
                    case 3: /* DECCOLM : Col = 132 */
                    deccolm = TRUE;
                    clrscreen();
                    break;
                    case 4: /* DECSCLM */
                    break;
                    case 5: /* DECSCNM */
                    if (decscnm)
                        break;  /* Already set */
                    decscnm = TRUE;;
                    reversescreen();
                    defaultattribute = (coloroftext << 4) | colorofback;
                    attribute = defaultattribute;
                    if (attrib.reversed) {
                        back = attribute & 0x07;
                        fore = (attribute & 0x70) >> 4;
                        if (adapter == colour && attrib.underlined) {
                        fore = colorofunderline;
                        }
                        attribute = (back << 4) | fore;
                    }
                    if (attrib.underlined) {
                        if (adapter == mono && !attrib.reversed)
                        attribute = (attribute & 0xF8) | colorofunderline;
                    }
                    if (attrib.blinking)
                        attribute |= 0x80;
                    if (attrib.bold)
                        attribute |= 8;
                    if (attrib.invisible) {
                        i = (attribute & 0xF8);
                        attribute = i | ((i >> 4) & 7);
                    }
                    break;
                    case 6: /* DECOM : Relative origin */
                    relcursor = TRUE;
                    lgotoxy(1, margintop);
                    break;
                    case 7: /* DECAWM */
                    autowrap = TRUE;
                    break;
                    case 8: /* DECARM */
                    break;
                    case 9: /* DECINLM */
                    break;
                    default:
                    break;
                } else
                    switch (pn[j]) {
                    case 2: /* Keyboard locked */
                    keylock = TRUE;
                    break;
                    case 4: /* Ansi insert mode */
                    insertmode = TRUE;
                    break;
                    case 20:    /* Ansi linefeed mode */
                    linemode = TRUE;
                    break;
                    default:
                    break;
                    }
                break;
            case 'l':
                /* Reset Mode */
                for (j = 1; j <= k; ++j)
                if (private)
                    switch ((pn[j])) {  /* Field specs */
                    case 1: /* DECCKM  */
                    cursorkey = FALSE;
                    break;
                    case 2: /* DECANM : ANSI/VT52 */
                    ansi = FALSE;
                    vt52graphics = FALSE;
                    break;
                    case 3: /* DECCOLM : 80 col */
                    deccolm = FALSE;
                    clrscreen();
                    break;
                    case 4: /* DECSCLM */
                    break;
                    case 5: /* DECSCNM */
                    if (!decscnm)
                        break;
                    decscnm = !decscnm;
                    reversescreen();
                    defaultattribute = (colorofback << 4) | coloroftext;
                    attribute = defaultattribute;
                    if (attrib.reversed) {
                        back = attribute & 0x07;
                        fore = (attribute & 0x70) >> 4;
                        if (adapter == colour && attrib.underlined) {
                        fore = colorofunderline;
                        }
                        attribute = (back << 4) | fore;
                    }
                    if (attrib.underlined) {
                        if (adapter == mono && !attrib.reversed)
                        attribute = (attribute & 0xF8) | colorofunderline;
                    }
                    if (attrib.blinking)
                        attribute |= 0x80;
                    if (attrib.bold)
                        attribute |= 8;
                    if (attrib.invisible) {
                        i = (attribute & 0xF8);
                        attribute = i | ((i >> 4) & 7);
                    }
                    break;
                    case 6: /* DECOM : Relative origin */
                    relcursor = FALSE;
                    lgotoxy(1, 1);
                    break;
                    case 7: /* DECAWM */
                    autowrap = FALSE;
                    break;
                    case 8: /* DECARM */
                    break;
                    case 9: /* DECINLM */
                    break;
                    default:
                    break;
                } else
                    switch (pn[j]) {
                    case 2: /* Keyboard unlocked */
                    keylock = FALSE;
                    break;
                    case 4: /* Ansi insert mode */
                    insertmode = FALSE;
                    break;
                    case 20:    /* Ansi linefeed mode */
                    linemode = FALSE;
                    break;
                    default:
                    break;
                    }
                break;
            case 'i':   /* Printer Screen  on / off */
                for (j = 1; j <= k; ++j)
                switch ((pn[j])) {  /* Field specs */
                case 4:
                    doprinton(FALSE);
                    break;
                case 5:
                    doprinton(TRUE);
                    break;
                case 6:
                    screenon = FALSE;
                    break;
                case 7:
                    screenon = TRUE;
                    break;
                default:
                    break;
                }
                break;
            case 'q':
                break;
            case 'n':
                if (pn[1] == 5) {   /* Device Status Report */
                sendstr("[0n");
                } else if (pn[1] == 6) {    /* Cursor Position
                             * Report */
                sendstr("[");
                sprintf(tempstr, "%1d", (int) wherey);  /* row */
                SENDCHAR(tempstr[0]);
                if (tempstr[1])
                    SENDCHAR(tempstr[1]);
                SENDCHAR(';');
                sprintf(tempstr, "%1d", (int) wherex);  /* col */
                SENDCHAR(tempstr[0]);
                if (tempstr[1])
                    SENDCHAR(tempstr[1]);
                SENDCHAR('R');
                }
                break;
            case 'x':   /* Request terminal Parameters */
                if (pn[1] > 1)
                break;
                tempstr[0] = '[';
                tempstr[1] = (pn[1] == 0) ? '2' : '3';
                tempstr[2] = ';';
                switch (parity) {
                case 'e':
                tempstr[3] = '5';
                tempstr[5] = '2';
                break;
                case 'o':
                tempstr[3] = '4';
                tempstr[5] = '2';
                break;
                case 0:
                tempstr[3] = '1';
                tempstr[5] = '1';
                break;
                default:
                tempstr[3] = '1';
                tempstr[5] = '2';
                break;
                }
                tempstr[4] = ';';
                switch (speed) {
                case 50:
                i = 0;
                break;
                case 75:
                i = 8;
                break;
                case 110:
                i = 16;
                break;
                case 133:
                i = 14;
                break;
                case 150:
                i = 32;
                break;
                case 200:
                i = 40;
                break;
                case 300:
                i = 48;
                break;
                case 600:
                i = 56;
                break;
                case 1200:
                i = 64;
                break;
                case 1800:
                i = 72;
                break;
                case 2000:
                i = 80;
                break;
                case 2400:
                i = 88;
                break;
                case 3600:
                i = 96;
                break;
                case 4800:
                i = 104;
                break;
                case 9600:
                i = 112;
                break;
                case 19200:
                i = 120;
                break;
                default:
                i = 120;
                break;
                }
                sprintf(&tempstr[6], ";%d;%d;1;0x", i, i);
                sendstr(tempstr);
                break;
            case 'm':
            case '}':
                for (j = 1; j <= k; ++j)
                switch ((pn[j])) {  /* Field specs */
                case 0: /* normal */
                    attribute = defaultattribute;
                    attrib.blinking = FALSE;
                    attrib.bold = FALSE;
                    attrib.invisible = FALSE;
                    attrib.underlined = FALSE;
                    attrib.reversed = FALSE;
                    break;
                case 1: /* bold */
                    attrib.bold = TRUE;
                    attribute |= 8;
                    break;
                case 4: /* underline */
                    if (attrib.underlined)
                    break;
                    attrib.underlined = TRUE;
                    if (adapter == mono && attrib.reversed)
                    break;
                    attribute = (attribute & 0xF8) | colorofunderline;
                    break;
                case 5: /* blink */
                    attrib.blinking = TRUE;
                    attribute |= 0x80;
                    break;
                case 7: /* reverse video */
                    if (attrib.reversed)
                    break;
                    attrib.reversed = TRUE;
                    fore = defaultattribute >> 4;
                    back = defaultattribute & 0x07;
                    if (adapter == colour && attrib.underlined) {
                    fore = colorofunderline;
                    }
                    if (attrib.invisible)
                    fore = back;
                    attribute = (attribute & 0x88) | back << 4 | fore;
                    break;
                case 8: /* invisible */
                    if (attrib.invisible)
                    break;
                    attrib.invisible = TRUE;
                    i = (attribute & 0xF8);
                    attribute = i | ((i >> 4) & 7);
                    break;
                case 30:
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                case 36:
                case 37:
                    /* select foreground */
                    i = (attribute & 248);
                    attribute = (i | sgrcols[pn[j] - 30]);
                    break;
                case 40:
                case 41:
                case 42:
                case 43:
                case 44:
                case 45:
                case 46:
                case 47:
                    /* select back ground */
                    i = (attribute & 143);
                    l = sgrcols[pn[j] - 40];
                    attribute = (i | ((l << 4)));
                    break;
                default:
                    break;
                }
                break;
            case 'r':   /* set margin */
                if ((k < 2) || (pn[2] == 0))
                pn[2] = 24;
                if (pn[1] == 0)
                pn[1] = 1;
                if ((pn[1] > 0) && (pn[1] < pn[2]) && (pn[2] < 25)) {
                setmargins(pn[1], pn[2]);
                lgotoxy(1, relcursor ? margintop : 1);
                }
                break;
            case 'J':
                switch ((pn[1])) {
                case 0: /* clear to end of screen */
                clreoscr_escape();
                break;
                case 1: /* clear to beginning */
                clrboscr_escape();
                break;
                case 2: /* clear all of screen */
                clrscreen();
                break;
                default:
                break;
                }
                break;
            case 'K':
                switch ((pn[1])) {
                case 0: /* clear to end of line */
                clrtoeol();
                break;
                case 1: /* clear to beginning */
                clrbol_escape();
                break;
                case 2: /* clear line */
                clrline_escape();
                break;
                default:
                break;
                }
                break;
            case 'L':   /* Insert lines */
                for (i = 1; i <= pn[1]; ++i)
                scroll(DOWNWARD, wherey - 1, marginbot - 1);
                break;
            case 'M':   /* Delete lines */
                for (i = 1; i <= pn[1]; ++i)
                scroll(UPWARD, wherey - 1, marginbot - 1);
                break;
            case '@':   /* Insert characters */
                blankcell[0] = ' ';
                blankcell[1] = defaultattribute;
                pn[1] *= dwl[wherey - 1] ? 2 : 1;
                if (pn[1] > 81 - wherex)
                pn[1] = 81 - wherex;
                AVIOScrollRt(wherey - 1, wherex - 1, wherey - 1, 79, 
                    pn[1], blankcell);
                break;
            case 'P':   /* DeleteChar */
                blankcell[0] = ' ';
                blankcell[1] = defaultattribute;
                pn[1] *= dwl[wherey - 1] ? 2 : 1;
                if (pn[1] > 81 - wherex)
                pn[1] = 81 - wherex;
                AVIOScrollLf(wherey - 1, wherex - 1, wherey - 1, 79, 
                    pn[1], blankcell);
                break;
            default:
                break;
            }
            }
            break;
        }
        }           /* Left square bracket */
        break;
    case '7':       /* Save cursor position */
        saved = TRUE;
        row = wherey;
        column = wherex;
        savedattribute = attribute;
        savedattrib = attrib;
        saverelcursor = relcursor;
        saveg0 = g0;
        saveg1 = g1;
        saveg0g1 = g0g1;
        break;
    case '8':       /* Restore Cursor Position */
        if (!saved) {   /* Home cursor */
        lgotoxy(1, relcursor ? margintop : 1);
            break;
        }
        saved = FALSE;
        lgotoxy(column, row);
        attribute = savedattribute;
        attrib = savedattrib;
        relcursor = saverelcursor;
        g0 = saveg0;
        g1 = saveg1;
        g0g1 = saveg0g1;
        break;
    case 'A':
        if (!ansi)      /* VT52 control */
        cursorup();
        break;
    case 'B':
        if (!(ansi))
        /* VT52 control */
        cursordown();
        break;
    case 'C':
        if (!(ansi))
        /* VT52 control */
        cursorright();
        break;
    case 'D':
        if (!(ansi))
        /* VT52 control */
        cursorleft();
        else {
        /* Index */
        if ((wherey >= marginbot))
            scroll(UPWARD, margintop - 1, marginbot - 1);
        else
            cursordown();
        }
        break;
    case 'E':       /* Next Line */
        wrtch(13);
        wrtch(10);
        break;
    case 'F':
        if (!ansi)
        vt52graphics = TRUE;
        break;
    case 'G':
        if (!ansi)
        vt52graphics = FALSE;
        break;
    case 'H':
        if (ansi) {
        /* Set Tab Stop */
        htab[wherex] = 'T';
        }
        /* Set Tab Stop */
        else
        lgotoxy(1, 1);
        /* VT52 control */
        break;
    case 'I':
        if (!(ansi)) {
        /* VT52 control */
        if ((margintop < wherey))
            cursorup();
        else
            scroll(DOWNWARD, margintop - 1, marginbot - 1);
        }
        break;
    case 'J':
        if (!(ansi))
        /* VT52 control */
        clreoscr_escape();
        break;
    case 'K':
        if (!(ansi))
        /* VT52 control */
        clrtoeol();
        break;
    case 'M':
        /* Reverse Index */
        if (margintop >= wherey)
        scroll(DOWNWARD, margintop - 1, marginbot - 1);
        else
        cursorup();
        break;
    case 'Y':
        if (!(ansi)) {  /* VT52 control */
        /* direct cursor address */
        readmchar_escape();
        row = achar - 31;
        readmchar_escape();
        column = achar - 31;
        lgotoxy(column, row);
        }
        /* direct cursor address */
        break;
    case 'Z':
        if (ansi) {
        /* Device Attributes */
        /* Send  Esc[?6;2c */
        sendstr("[?6;2c");
        }
        /* Device Attributes */
        else
        /* VT52 control */
        SENDSTR2("/Z");
        break;
    case 'c':
        /* Reset */
        defaultattribute = coloroftext + (colorofback << 4);
        attribute = defaultattribute;
        attrib.blinking = FALSE;
        attrib.bold = FALSE;
        attrib.invisible = FALSE;
        attrib.underlined = FALSE;
        attrib.reversed = FALSE;
        g0 = g1 = 'A';
        g0g1 = &g0;
        printon = FALSE;
        screenon = TRUE;
        vt52graphics = FALSE;
        saved = FALSE;
        linemode = FALSE;
        insertmode = FALSE;
        cursorkey = TRUE;
        keypadnum = FALSE;
        autowrap = FALSE;
        ansi = TRUE;
        keylock = FALSE;
        deccolm = decscnm = FALSE;
        for (i = 0; i < 24; i++)
        dwl[i] = FALSE;
        dwls = FALSE;
        for (i = 1; i < 80; i++)
        htab[i] = (i % 8) == 0 ? 'T' : '0';
        relcursor = FALSE;
        setmargins(1, 24);
        clrscreen();
        break;
    case '#':
        /* Esc # sequence */
        readmchar_escape();
        switch (achar) {
        case '3':
        decdwl_escape(TRUE);
        break;
        case '4':
        decdwl_escape(TRUE);
        break;
        case '5':
        decdwl_escape(FALSE);
        break;
        case '6':
        decdwl_escape(TRUE);
        break;
        case '8':
        {
            char            cell[2];
            cell[0] = 'E';
            cell[1] = 7;
            /* Self Test */
            AVIOWrtNCell(cell, 1920, 0, 0);
            setmargins(1, 24);
            lgotoxy(1, 1);
        }
        break;
        /* Self Test */
        default:
        break;
        }
        break;
        /* Esc # sequence */
    case '=':
        keypadnum = FALSE;
        break;
    case '>':
        keypadnum = TRUE;
        break;
    case '<':
        /* VT52 control */
        ansi = TRUE;
        break;
    case '(':
        readmchar_escape();
        g0 = achar;
        break;
    case ')':
        readmchar_escape();
        g1 = achar;
        break;
    default:
        if (achar == 12) {
        lgotoxy(1, 1);
        clrscreen();
        }
        break;
    }
    /* First Level Case  */
    }
    /* screen escape sequences  */
    if (printon) {
    fprintf(lst, "%c", 27);
    if (esclast > 0) {
        /* print esc sequence */
        for (i = 1; i <= esclast; ++i)
        fprintf(lst, "%c", escbuffer[i]);
    }
    }
}
/******************************************************************************
Function:       vt100read()

Description:    This is the function that reads commands from the keyboard,
                interpretes them and then sends them out over the communication
                line.
                
                Unfortunately the PM does not supply all the information
                required to use the extended keys properly.  It does not seem
                to be possible to distinguish between the keypad and the grey
                cursor keys with NumLock off.  Ctrl-, Shift- and Alt- function
                keys do nor generate codes at all!.
                
                The 'DOS' like OS/2 kermit uses the ALT-number keys - how? They
                do not appear here.......

Syntax:         vt100read(a,b)
                    unsigned char a,b; a = ASCII code, b = SCAN code

Returns:        nothing

Mods:           xx-xxx-xx C.J.Adie(?) created
                06-Dec-89 C.P.Armstrong modified for PM. The scan codes returned
                                        by PM do not necessarily agree with the
                                        'DOS' ones.  The extended keys are 
                                        turned
                                        into "virtual" keys so the
                                        case statement has been converted to
                                        use these.  Eventually the virtual keys
                                        should be remapable.  In addition the
                                        screen rollback will be done by
                                        something other than the grey cursor
                                        keys which I want to be able to use
                                        with the remote computer.

******************************************************************************/
void vt100read(a, b) unsigned char a, b;
{
    extern   int    pasting;
    unsigned char   achar;
    unsigned char   bchar;
    int             i;
    int             il;
    char            str[3];
    int             prt;
    int             st;
    unsigned char   nolblines;
    unsigned char   linesleft;
    unsigned char   nextline;
    unsigned char   nlines;

    /* In the PM thread the virtual keys are processed before the regular
    character keys.  This means that shift-backtab generates a unique code as
    do the main keyboard Enter and keypad enter keys. For the sake of
    consistency the virtual spacekey is returned as a character, as are the
    control characters. */

    achar = a;
    bchar = b;
LB1:
//    if (achar == 8) 
//        {
//        /* backspace is mapped to del */
//        achar = 127;
//        bchar = 0;
//        }

    if (achar != 0) 
        {
        if(achar!=CR)
            sendcharduplex(achar);
        else
            sendcrlf();
        } 
    else 
        {
        switch ((int) (bchar)) 
            {
            case VK_UP:
                /* up */
                if (ansi) {
                if (cursorkey)
                    sendstrduplex("OA");
                else
                    sendstrduplex("[A");
                } else
                sendstrduplex("A");
                break;
            case VK_LEFT:
                /* left */
                if (ansi) {
                if (cursorkey)
                    sendstrduplex("OD");
                else
                    sendstrduplex("[D");
                } else
                sendstrduplex("D");
                break;
            case VK_RIGHT:
                /* right */
                if (ansi) {
                if (cursorkey)
                    sendstrduplex("OC");
                else
                    sendstrduplex("[C");
                } else
                sendstrduplex("C");
                break;
            case VK_DOWN:
                /* down */
                if (ansi) {
                if (cursorkey)
                    sendstrduplex("OB");
                else
                    sendstrduplex("[B");
                } else
                sendstrduplex("B");
                break;
            case VK_BACKTAB:        /* Backtab */
                /* backspace */
                sendcharduplex(8);
                break;
            case VK_BACKSPACE:
            case VK_DELETE:
                /* delete */
                sendcharduplex(127);
                break;
            case VK_F2:
            case VK_NUMLOCK:
                /* PF1 */
                if (ansi)
                    sendstrduplex("OP");
                else
                    sendstrduplex("P");
                break;
            case KPSLASH:  /* Should be keypad / */
                /* PF2 */
                if (ansi)
                sendstrduplex("OQ");
                else
                sendstrduplex("Q");
                break;
            case KPSTAR:  /* Should be  keypad * */
                /* PF3 */
                if (ansi)
                sendstrduplex("OR");
                else
                sendstrduplex("R");
                break;
            case KPMINUS:
                /* PF4 */
                if (ansi)
                sendstrduplex("OS");
                else
                sendstrduplex("S");
                break;
            case KP7:
                vt_kp_send('w'); break;
            case KP8:
                vt_kp_send('x'); break;
            case KP9: 
                vt_kp_send('y'); break;
            case KP4:
                vt_kp_send('t'); break;
            case KP5:
                vt_kp_send('u'); break;
            case KP6:
                vt_kp_send('v'); break;
            case KP1:
                vt_kp_send('q'); break;
            case KP2:
                vt_kp_send('r'); break;
            case KP3:
                vt_kp_send('s'); break;
            case KP0:
                vt_kp_send('p'); break;
            case KPPLUS:                   /* numeric - */
                vt_kp_send('m'); break;
            case VK_F3:                    /* numeric , no equivalent for PC */
                vt_kp_send('l'); break;
            case KPDOT:                    /* numeric . */
                vt_kp_send('n'); break;
            case VK_ENTER:                 /* numeric enter */
                vt_kp_send('M'); break;
            case VK_HOME:        /* Home */
                {
                strcpy(usertext, " Scroll lock on");
                strcpy(exittext, "Exit: Home");
                *helptext = '\0';
                line25();
                do
                    scankey(&(achar), &(bchar));
                while (!(bchar == VK_HOME));
                ipadl25();
                }
                break;
                
    /* These keys should not be used for the scrollback as they are useful */
    /* editing keys in the terminal emulator.                              */
            case VK_PAGEUP:        /* Not in paging mode so beep */
                bleep();
                break;
            case VK_PAGEDOWN:      /* Enter extended display mode */
                if (paginginfo.numlines == 0)
                    bleep();
                else 
                    {
                    screen_rollback(&achar,&bchar);
                    }

                if (((achar != 0) || (bchar != VK_PAGEDOWN)))
                    goto LB1;
                /* to process key */
                break;
            default:
                break;
            }
    /* of case bchar */
        }
}


/* ================================================================== */
/* */
/* VT100 emulate a dec vt100 terminal writing a character            */
/* */
/* ================================================================== */
void vt100(vtch)
    unsigned char   vtch;
{
    int             i, j;
    char           *s, str[2];
    /* ----------------------------------------------------------------- */
    /* ------------------------------------------------------------------ */
    /* vt100 */
    if (screenon) {
    if (vtch < 32) {    /* Control Character */
        /* Dump the line buffer first */
        chars_out();

        achar = vtch;   /* Let the rest of this module see the value */
        switch (achar) {
        case LF:
                if (linemode) wherex=1;
            wrtch(achar);
            break;
        case CR:
            wrtch(achar);
            break;
        /* ESC */
        case 27:
            vtescape();
            break;
        case 14:        /* SO */
            g0g1 = &g1;
            break;
        /* SI */
        case 15:
            g0g1 = &g0;
            break;
        /* BS */
        case 8:
            wrtch(achar);
            break;
        case 12:        /* FF */
        case 11:        /* VT */
            /* take as lf */
            achar = 10;
            wrtch(10);
            break;
        case 7:     /* BEL */
            bleep();
            break;
        case 5:     /* ENQ */
            s = answerback;
            while (*s)
                SENDCHAR(*s++);
            break;
        case 9:     /* tab character */
        j = dwl[wherey - 1] ? 2 : 1;
        i = wherex;
        if (j == 2 && htab[(i - 1) / j + 1] == 'T') {
            i++;
            cursorright();
        }
        if (i < 80)
            do {
            i++;
            cursorright();
            } while ((htab[(i - 1) / j + 1] != 'T') && (i < 81 - j));
        break;
        default:        /* ignore it */
        break;
        }
        /* end of Control Character */
    } else {
        if (vtch != DEL) {  /* Normal char */
        if (ansi) {
            if (*g0g1 == 'A') { /* UK ascii set */
            if (vtch == 35)
                vtch = 156;
            } else if ((*g0g1 == '0') && (95 <= vtch) && (vtch <= 126)) {
            literal = TRUE;
            vtch = graphicset[vtch - 95];
            }
        } else {
            if (vt52graphics && (95 <= vtch) && (vtch <= 126)) {
            literal = TRUE;
            vtch = graphicset[vtch - 95];
            }
        }                            

        if (wherex != (dwl[wherey - 1] ? 79 : 80)) {
            chars_in(vtch);          /* Most chars end up here - */
//          wrtch(vtch);             /* so save them here too    */
           if (dwl[wherey - 1])
//            wrtch(' ');
            chars_in(' ');
            wrapit = FALSE;
            } 
        else 
            {
            chars_out();   /* Dump the buffer */
            if (wrapit) 
                {   /* Next line  */
                if (marginbot <= wherey) 
                    {  /* Scroll up */
                    scroll(UPWARD, margintop - 1, marginbot - 1);
                    lgotoxy(1, wherey);
                    } 
                else
                    lgotoxy(1, wherey + 1);

                wrtch(vtch);   /* save the character here */

                if (dwl[wherey - 1])  /* add a space */
                    wrtch(' ');

                wrapit = FALSE; 
                } 
            else 
                {                    /* put char on col 80 */
                i = dwl[wherey - 1] ? 2 : 1;
                str[0] = vtch;
                str[1] = ' ';
                AVIOWrtCharStrAtt(&vtch, i, wherey - 1, 80 - i, &attribute);
                literal = FALSE;
                if ((autowrap && !deccolm))
                    wrapit = TRUE;
                }
        }
        }           /* Normal char */
    }
    }
    if (printon && (vtch != 27))
    fprintf(lst, "%c", vtch);
}

/* ------------------------------------------------------------------ */
/* Vt100read -                                                        */
/* ------------------------------------------------------------------ */
/* save current status of screen */
static void savescreen(scrn) ascreen *scrn;
{
    USHORT          n = sizeof(screenmap);
    scrn->ox = wherex;
    scrn->oy = wherey;
    scrn->att = attribute;
    AVIOReadCellStr((char *) (scrn->scrncpy), &n, 0, 0);
}                                                         

/* restore state of screen */
void restorescreen(scrn) ascreen *scrn;
{
    movetoscreen(scrn->scrncpy, 1, 1, 4000);
    attribute = scrn->att;
    wherey = scrn->oy;
    wherex = scrn->ox;
    lgotoxy(wherex, wherey);
}

void sendcharduplex(unsigned char c)
{
    SENDCHAR(c);
    if (duplex) {
    cwrite(c);
    if (seslog)
        if (zchout(ZSFILE, c) < 0)
        seslog = 0;
    }
}
static void
sendstrduplex(unsigned char *s)
{
    SENDCHAR(27);
    if (duplex) {
    cwrite(27);
    if (seslog)
        if (zchout(ZSFILE, 27) < 0)
        seslog = 0;
    }
    while (*s) {
    SENDCHAR(*s);
    if (duplex) {
        cwrite(*s);
        if (seslog)
        if (zchout(ZSFILE, *s) < 0)
            seslog = 0;
    }
    s++;
    }
}

/* CHECKSCREENMODE  --  Make sure we are in a 25 x 80 mode */
void checkscreenmode()
{
//    VIOMODEINFO     m;
//   VIOCONFIGINFO   cfg;
    char            cell[2];
//    int             i;
//
//    m.cb = 12;
//    if (AVIOGetMode(&m))
//    return;
//    cfg.cb = 10;
//    if (AVIOGetConfig(0, &cfg))
//    return;
//    switch (cfg.adapter) {
//    case 0:         /* Monochrome */
//    m.fbType = 0;
//    m.color = 0;
//    m.hres = 720;
//    m.vres = 350;
//    adapter = mono;
//    colorofunderline = 1;
//    break;
//    case 1:         /* CGA */
//    m.fbType = 1;
//    m.color = 4;
//    m.hres = 640;
//    m.vres = 200;
//    adapter = colour;
//    break;
//    default:            /* Assume EGA/VGA */
//    m.fbType = 1;
//    m.color = 4;
//    m.hres = 640;
//    m.vres = 350;
//    adapter = colour;
//    }
//    if (m.row == 25 && m.col == 80)
//    return;         /* We're happy with this */
//    m.row = 25;
//    m.col = 80;
//    if (i = AVIOSetMode(&m)) {
//    printf("Error %d returned by AVIOSetMode\n", i);
//    return;
//    }
    cell[0] = ' ';
    cell[1] = 0x07;
    AVIOScrollUp(0, 0, -1, -1, -1, cell); /* Clear the screen */
    AVIOSetCurPos(0, 0);  /* Home the cursor */
    DosBeep(1000,20);
}


   
void doesc(c) char
    c;
{
    CHAR            d;
    char            temp[50];

    while (1) {
    if (c == escape) {  /* Send escape character */
        d = dopar(c);
        SENDCHAR(d);
        return;
    } else
     /* Or else look it up below. */ if (isupper(c))
        c = tolower(c);

    switch (c) {

    case 'c':       /* Close connection */
    case '\03':
        active = 0;
        return;

    case 'b':       /* Send a BREAK signal */
    case '\02':
        ttsndb();
        return;

    case 'h':       /* Hangup */
    case '\010':
        tthang();
        active = 0;
        return;

    case '0':       /* Send a null */
        c = '\0';
        d = dopar(c);
        SENDCHAR(d);
        return;

    case SP:        /* Space, ignore */
        return;

    default:        /* Other */
        conoc(BEL);
        return;     /* Invalid esc arg, beep */
    }
    }
}

/* DOESC  --  Process an escape character argument  */
static int      helpcol, helprow;
static int      helpwidth;

void helpstart(int w, int h)
{
    unsigned char   cell[2];

    cell[1] = attribute | 0x08;
    helpwidth = w;
    helpcol = (80 - w) / 2;
    helprow = (24 - h) / 2;
    cell[0] = 201;      /* Top left corner */
    AVIOWrtNCell(cell, 1, helprow, helpcol);
    cell[0] = 205;      /* Horizontal */
    AVIOWrtNCell(cell, helpwidth, helprow, helpcol + 1);
    cell[0] = 187;      /* Top right corner */
    AVIOWrtNCell(cell, 1, helprow, helpcol + helpwidth + 1);
}

void helpline(s)
    char           *s;
{
    unsigned char   cell[2];
    int             i;

    i = strlen(s);
    helprow++;
    cell[1] = attribute | 0x08;
    cell[0] = 186;      /* Vertical */
    AVIOWrtNCell(cell, 1, helprow, helpcol);
    AVIOWrtCharStrAtt(s, i, helprow, helpcol + 1, &cell[1]);
    cell[0] = ' ';
    AVIOWrtNCell(cell, helpwidth - i, helprow, helpcol + 1 + i);
    cell[0] = 186;      /* Vertical */
    AVIOWrtNCell(cell, 1, helprow, helpcol + helpwidth + 1);
}

void helpend()
{
    unsigned char   cell[2];

    helprow++;
    cell[1] = attribute | 0x08;
    cell[0] = 200;      /* Bottom left corner */
    AVIOWrtNCell(cell, 1, helprow, helpcol);
    cell[0] = 205;      /* Horizontal */
    AVIOWrtNCell(cell, helpwidth, helprow, helpcol + 1);
    cell[0] = 188;      /* Bottom right corner */
    AVIOWrtNCell(cell, 1, helprow, helpcol + helpwidth + 1);
}


/*---------------------------------------------------------------------------*/
/* scankey                                                                 */
/*---------------------------------------------------------------------------*/
void scankey(ach, bch) unsigned char *
                    ach, *bch;
{
    extern char far * ulVideoBuffer;  /* The screen address from ckopm1.c */
    extern int Term_mode;
    /*
     * Wait for a keypress, return the ASCII code in *ach and the scan in
     * *bch 
     */
    int k;
    do
        {
        k = buff_getch(); /* Waits for a key to be entered into the PM buffer */
        *ach = (unsigned char) (k&0xFF);
        *bch = (unsigned char) (k>>8);
        if(*bch==VK_PRINTSCRN)
            {
            print_screen(ulVideoBuffer);
            }
        else if(*bch==VK_SHPRINTSCRN)
            {
            switch(Term_mode)
                {
                case TEKTRONIX:
                    DosEnterCritSec();
                    Tek_process(k&0xFF00);
                    DosExitCritSec();
                    break;
                default:
                    break;
                }
            *bch=VK_PRINTSCRN;  /* Force getting another character */
            }
        /* If user changes the screen emu mode during a conect   */
        /* the PM thread can't call ipadl25 to update the status */
        /* line. Semaphores are used to control the processing   */
        /* of screen writing messages to the PM thread. Generally*/
        /* the poster must wait for the PM thread to clear a     */
        /* semaphore before deleting the string buffer. This     */
        /* causes deadlock when the poster is actually the PM    */
        /* thread.  The only convenient way is to get the conect */
        /* thread to do it.  This way ensures that the change is */
        /* obvious immediatly.                                   */
        else if((*bch==0) && (*ach==0))
            {
            ipadl25();
            *bch=VK_PRINTSCRN;
            }
        }while(*bch==VK_PRINTSCRN);
                                    
    if (*ach == 0xE0)       /* Extended scan code */
    if (*bch >= 0x47 && *bch <= 0x53)
        *ach = 0;
}
/*---------------------------------------------------------------------------*/
/* scrninit                                                                */
/*---------------------------------------------------------------------------*/
static int      scrninitialised = 0;
void scrninit()
{
    SEL             selector;

    if (!scrninitialised) {
    scrninitialised = 1;
    defaultattribute = (colorofback << 4) + coloroftext;
    attribute = defaultattribute;
    line25attribute = (coloroftext << 4) + colorofback;
    /* Initialise paging info */
    if (DosAllocSeg(LBUFSIZE * 160, &selector, 0)) {
        printf("\nDosAllocSeg fails in scrninit\n");
        exit(1);
    }
    paginginfo.buffer = MAKEP(selector, 0);
    paginginfo.numlines = 0;
    paginginfo.topline = 0;
    paginginfo.botline = LBUFSIZE - 1;
    clearscreen();
    savescreen(&vt100screen);
    }
}
/*---------------------------------------------------------------------------*/
/* bleep                                                                   */
/*---------------------------------------------------------------------------*/
void bleep()
{
    DosBeep(440, 200);
}
/*---------------------------------------------------------------------------*/
/* wrtch                                                                   */
/*---------------------------------------------------------------------------*/
void wrtch(unsigned char ch)
{
    unsigned char   cell[2];
    if (ch >= ' ' || literal) { /* Normal character */
    if (ansi && insertmode) {
        cell[0] = ch;
        cell[1] = attribute;
        AVIOScrollRt(wherey - 1, wherex - 1, wherey - 1, 79, 1, cell);
    } else
        AVIOWrtCharStrAtt(&ch, 1, wherey - 1, wherex - 1, &attribute);
    literal = FALSE;
    if (++wherex > 80) {
        wherex = 1;
        wrtch(LF);
    }
    } else {            /* Control character */
    switch (ch) {
    case LF:
        if (wherey == marginbot) {
        if (margintop == 1)
            toplinetocyclicbuffer();
        scroll(UPWARD, margintop - 1, marginbot - 1);
        } else {
        wherey++;
        if (wherey == 25)
            wherey--;
        }
        break;
    case CR:
        wherex = 1;
        break;
    case BS:
        if (wherex > 1)
        wherex--;
        break;
    case 12:
        if (wherex < 80)
        wherex++;
        break;
    case BEL:
        DosBeep(400, 350);
        break;
    default:{       /* Ignore */
        }
    }
    }
    if (cursoron)
    AVIOSetCurPos(wherey - 1, wherex - 1);
}
/*---------------------------------------------------------------------------*/
/* killcursor                                                              */
/*---------------------------------------------------------------------------*/
VIOCURSORINFO crsr_info;
void killcursor()
{
    VIOCURSORINFO   nocursor;

    if (!cursoron)
    return;
    AVIOGetCurType(&crsr_info);   /* Store current cursor type */
    nocursor = crsr_info;   /* MS C allows this */
    nocursor.attr = -1;
    AVIOSetCurType(&nocursor);/* Hide cursor */
    return;
}
/*---------------------------------------------------------------------------*/
/* line25                                                                  */
/*---------------------------------------------------------------------------*/
void line25()
{
    char            s[80];
    int             i;
    char            attr;

    for (i = 0; i < 80; i++)
    s[i] = ' ';
    strinsert(&s[00], usertext);
    strinsert(&s[36], exittext);
    strinsert(&s[20], helptext);
    strinsert(&s[65], filetext);
    strinsert(&s[50], hostname);
    attr = line25attribute;
    AVIOWrtCharStrAtt(s, 80, 24, 0, &attr);
}

void clreoscr_escape()
{       
    char            cell[2];
    int             i;

    if (wherex == 1 && wherey == 1) {
    clrscreen();
    return;
    }
    cell[0] = ' ';
    cell[1] = defaultattribute;
    i = 1920 - (((wherey - 1) * 80) + (wherex - 1));
    AVIOWrtNCell(cell, i, wherey - 1, wherex - 1);
    for (i = wherey - 1; i < 24; i++)
    dwl[i] = FALSE;
    dwls = FALSE;
    for (i = 0; i < 24; i++)
    if (dwl[i]) {
        dwls = TRUE;
        break;
    }
}

void clrboscr_escape()
{   
    char            cell[2];
    int             i;

    cell[0] = ' ';
    cell[1] = defaultattribute;
    i = ((wherey - 1) * 80) + wherex;
    AVIOWrtNCell(cell, i, 0, 0);
    for (i = 0; i < wherey; i++)
    dwl[i] = FALSE;
    dwls = FALSE;
    for (i = 0; i < 24; i++)
    if (dwl[i]) {
        dwls = TRUE;
        break;
    }
}

void clrbol_escape()
{       
    char            cell[2];

    cell[0] = ' ';
    cell[1] = defaultattribute;
    AVIOWrtNCell(cell, wherex, wherey - 1, 0);
}

void clrline_escape()
{
    char            cell[2];

    cell[0] = ' ';
    cell[1] = defaultattribute;
    AVIOWrtNCell(cell, 80, wherey - 1, 0);
}


/* ------------------------------------------------------------------ */
/* CursorUp -                                                         */
/* ------------------------------------------------------------------ */
static void cursorup()
{
    if ((relcursor ? margintop : 1) != wherey)
    lgotoxy(wherex, wherey - 1);
}
/* ------------------------------------------------------------------ */
/* CursorDown -                                                       */
/* ------------------------------------------------------------------ */
static void
cursordown()
{
    if ((relcursor ? marginbot : 24) != wherey)
    lgotoxy(wherex, wherey + 1);
}
/* ------------------------------------------------------------------ */
/* CursorRight -                                                      */
/* ------------------------------------------------------------------ */
static void
cursorright()
{
    if (wherex < (dwl[wherey - 1] ? 79 : 80))
    lgotoxy(wherex + 1, wherey);
}
/* ------------------------------------------------------------------ */
/* CursorLeft -                                                       */
/* ------------------------------------------------------------------ */
static void
cursorleft()
{
    if (wherex > 1)
    lgotoxy(wherex - 1, wherey);
}
/* ------------------------------------------------------------------ */
/* ReverseScreen                              */
/* ------------------------------------------------------------------ */
static void reversescreen()
{
    unsigned char   back;
    unsigned char   fore;
    int             i, r, c;
    USHORT          n;
    unsigned char   cell[160];

    n = sizeof(cell);
    for (r = 0; r < 24; r++) {  /* flip row */
    AVIOReadCellStr(cell, &n, r, 0);
    for (c = 1; c < n; c += 2) {    /* do each cell in row */
        back = (cell[c] & 0x70) >> 4;
        fore = (cell[c] & 0x07);
        if (fore == (char) colorofunderline)
        cell[c] ^= 0x70;
        else
        cell[c] = (cell[c] & 0x88) | fore << 4 | back;
    }
    AVIOWrtCellStr(cell, n, r, 0);
    }
}




/*---------------------------------------------------------------------------*/
/* clearscreen                                                             */
/*---------------------------------------------------------------------------*/

void clearscreen()
{
    char            cell[2];
    cell[0] = ' ';
    cell[1] = defaultattribute;
    AVIOWrtNCell(cell, 1920, 0, 0);
    lgotoxy(1, 1);
}
/*---------------------------------------------------------------------------*/
/* lgotoxy                                                                 */
/*---------------------------------------------------------------------------*/
void lgotoxy(x, y) int
    x, y;
{
    wherex = x;
    wherey = y;
    if (cursoron)
    AVIOSetCurPos(wherey - 1, wherex - 1);
}
/*---------------------------------------------------------------------------*/
/* scroll                                                                  */
/*---------------------------------------------------------------------------*/
void scroll(updown, top, bottom) 
    int       updown;
    unsigned char   top, bottom;
{    
    char            blankcell[2];
    int             i;

    blankcell[0] = ' ';
    blankcell[1] = defaultattribute;
    switch (updown) {
    case UPWARD:
    AVIOScrollUp(top, 0, bottom, 79, 1, blankcell);
    if (dwls) {
        for (i = top; i < bottom; i++)
        dwl[i] = dwl[i + 1];
        dwl[bottom] = FALSE;
    }
    break;
    case DOWNWARD:
    AVIOScrollDn(top, 0, bottom, 79, 1, blankcell);
    if (dwls) {
        for (i = bottom; i > top; i--)
        dwl[i] = dwl[i - 1];
        dwl[top] = FALSE;
    }
    break;
    default: /* ignore */ ;
    }
    if (dwls) {
    dwls = FALSE;
    for (i = 0; i < 24; i++)
        if (dwl[i]) {
        dwls = TRUE;
        break;
        }
    }
}


/*---------------------------------------------------------------------------*/
/* movetoscreen                                                              */
/*---------------------------------------------------------------------------*/
void movetoscreen(source, x, y, len) 
    char * source, x, y;
    int             len;
{
    AVIOWrtCellStr(source, len, y - 1, x - 1);
}

/*---------------------------------------------------------------------------*/
/* cleartoeol                                                              */
/*---------------------------------------------------------------------------*/
void clrtoeol()
{
    char            cell[2];

    cell[0] = ' ';
    cell[1] = defaultattribute;
    AVIOWrtNCell(cell, 81 - wherex, wherey - 1, wherex - 1);
}
/*---------------------------------------------------------------------------*/
/* setmargins                                                              */
/*---------------------------------------------------------------------------*/
void setmargins(top, bot) char
    top, bot;
{
    margintop = top;
    marginbot = bot;
}


/* ------------------------------------------------------------------ */
/* cwrite                                                             */
/* ------------------------------------------------------------------ */
void cwrite(ch)
    unsigned char   ch;
{
    extern int Term_mode;

    /* If in tektronix mode don't use this function to process the input */
    if(Term_mode == TEKTRONIX)
        {
        Tek_process(ch);
        return;
        }

    {
    /* check and process escape sequence */
    /* escape */
    if (ch == 27) 
        {                   
        chars_out();        /* This prevents vt100() ever seeing ESC so we */
        escaping = TRUE;    /* need to dump the line buffer here */
        esclast = 0;
        } 
    else 
        {
        if (!(escaping)) 
            {
            /* can send it to vt100 to be processed */
            vt100(ch);
            } 
        else 
            {
            /* in the middle of an escape sequence */
            if (((ch == 24) || (ch == 26)))
                /* cancelled */
                escaping = FALSE;
            else 
                {
                if (ch < 32) 
                    {
                    if (ch == 8) 
                        {  /* Backspace */
                        if (esclast >= 1)
                            esclast--;
                        }
                    else if(ch==FF)
                        {
                        /* add to buffer */
                        esclast = esclast + 1;
                        escbuffer[esclast] = ch;
                        /* The only ESC containing FF is the Tek page */
                        vtescape();
                        }
                    } 
                else {
            /* add to control string */
            if (esclast < 128) {
                /* add to buffer */
                esclast = esclast + 1;
                escbuffer[esclast] = ch;
            }        

            /* now check to see if sequence complete */
            {
            /* Check for Tektronix screen clear command */

                if (ansi) {
                if (escbuffer[1] != '[') {
                    char            c = escbuffer[1];
                    if ((c != '#' && c != '(' && c != ')' && c != 'O'
                     && c != '?') || esclast >= 2) {
                    if ((escbuffer[1] != 'Y'))
                        vtescape();
                    else {
                        if ((esclast == 3))
                        vtescape();
                    }
                    }
                } else {
                    /* check for terminating character */
                    if ((((64 <= ch) && (ch <= 126)) && (esclast > 1)))
                    vtescape();
                }
                } else {
                /* vt52 mode */
                if ((escbuffer[1] != 'Y'))
                    vtescape();
                else {
                    if (esclast == 3)
                    vtescape();
                }
                }
            }
            }
        }
        }
    }
    }
}

void strinsert(d, s) char *
                    d, *s;
{
    while (*s)
    *d++ = *s++;
}

void sendstr(s) char *
                    s;
{
    SENDCHAR(27);
    while (*s)
    SENDCHAR(*s++);
}

/*
 * RDSERWRTSCR  --  Read the comms line and write to the screen. This
 * function is executed by a separate thread. 
 */
VOID FAR rdserwrtscr(VOID)
    {
    extern ULONG cr_recd_sem;
    extern int paste_eol_char;
    int             i;
    int buf[90];
    char countin;
    char countout;


    DosSemClear(&threadsem);    /* Let him know we've started */
//    while (active) 
//        {
//        countin=0;
//        while((buf[countin]=ttinc(50))>=0 && countin<90)
//            countin++;
//
//        countout=0;
//        while(countout<=countin)
//            {
//            cwrite(buf[countout] & cmask);
//            countout++;
//            }
//
//        if(seslog)
//            {
//            countout=0;
//            while(countout<=countin && seslog!=0)
//                {
//                if(zchout(ZSFILE,buf[countout] & cmask)<0)
//                    seslog=0;
//                countout++;
//                }
//            }
//
    while (active) 
        {            
        if ((i = ttinc(-25)) >= 0) 
            {
            if (cursoron)
                {
                if (ttchk() > 40) 
                    {
                    killcursor();
                    cursoron = FALSE;
                    }
                 }

            if( (i & cmask)==paste_eol_char) /* tell the write thread we've */
                DosSemClear(&cr_recd_sem);   /* read an EOL.                */

            cwrite(i & cmask);

            if (seslog) 
                {
                if (zchout(ZSFILE, i & cmask) < 0)
                    seslog = 0;
                }
            }
        else if (!cursoron)
             newcursor();
        }
    DosEnterCritSec();      /* Stop thread 1 discarding our stack before
                 * we've gone */
    DosSemClear(&threadsem);    /* Tell him we're going to die */
    _endthread();               /* Use the C MTRTL function to end */
}


/* CONECT  --  Perform terminal connection  */
conect()
{
extern int conected;
#define THRDSTKSIZ  3072        /* ex 2048 */
    char         *  stack;  /* Pointer to stack for second thread */
    USHORT          len, x, y;
    TID             threadid;
    int             c;      /* c is a character, but must be signed
                               integer to pass thru -1, which is the
                               modem disconnection signal, and is
                               different from the character 0377 */
    char            errmsg[50], *erp, ac, bc, ss[80];

    char * titlebuf;

    if (speed < 0) {
    printf("Sorry, you must set speed first.\n");
    return (-2);
    }
    twochartimes = 22000L / speed;
    if ((escape < 0) || (escape > 0177)) {
    printf("Your escape character is not ASCII - %d\n", escape);
    return (-2);
    }
    if (ttopen(ttname, &local, 0) < 0) {
    erp = errmsg;
    sprintf(erp, "Sorry, can't open %s", ttname);
    perror(errmsg);
    return (-2);
    }
    /* Condition console terminal and communication line */

    if (ttvt(speed, flow) < 0) {
    printf("Sorry, Can't condition communication line\n");
    return (-2);
    }
    checkscreenmode();
    AVIOGetCurPos(&y, &x);
    wherex = x + 1;
    wherey = y + 1;
    savescreen(&commandscreen);
    scrninit();
    restorescreen(&vt100screen);

    /* Save the current Vio title bar */
    titlebuf = (char *) malloc( PM_getViotitle(NULL) );
    PM_getViotitle(titlebuf);



    /* Create a thread to read the comms line and write to the screen */
    DosSemSet(&threadsem);  /* Thread 2 will clear this when it starts */
    active = 1;             /* So thread 2 doesn't end at once */
    termessage[0] = 0;

    if( (stack=malloc(THRDSTKSIZ)) == NULL)
        {
        printf("Can't do connect - no memory!");
        return(-2);
        }

    if( (threadid=_beginthread(rdserwrtscr,stack,THRDSTKSIZ,NULL)) == -1)
        {
        printf("Sorry, can't create thread\n");
        return (-2);
        }
    DosSemWait(&threadsem, -1L);/* Wait for thread to start */
    DosSemSet(&threadsem);  /* Thread 2 will clear this on termination */

    ipadl25();
    conected=1;
    while (active) 
        {        /* Read the keyboard and write to comms line */
        scankey(&ac, &bc);

        c = ac & cmask;     /* Get character from keyboard */
        if ((c & 0177) == escape) 
            { /* Look for escape char - does not use rtl */
            c = coninc(0) & 0177;   /* Got esc, get its arg */
            if (c == '?')
                c = helpconnect();
            doesc(c);       /* And process it */
            } 
        else 
            {        /* Ordinary character */
            if (!keylock)
                vt100read(ac, bc);
            }

    }               /* while (active) */


    DosSemWait(&threadsem, -1L);/* Wait for other thread to terminate */

    conected=0;
    savescreen(&vt100screen);
    restorescreen(&commandscreen);
    
    /* restore the Vio title bar */
    PM_setViotitle(titlebuf);
    free(titlebuf);
    free(stack);

    if (termessage[0]!='\0') printf(termessage);
}

/* HELPCONNECT  --  Give help message for connect.  */
helpconnect()
{
    int             c;
    unsigned char   ac, bc;
    char            cell[2];
    ascreen         tempscreen;
    static char    *hlpmsg[10] = {
                  "",
                  " Type :",
                  "",
                  "    C    to close the connection",
                  "    0    (zero) to send a null",
                  "    B    to send a BREAK",
                  "    H    to hangup and close connection",
                  "   ^]    to send the escape character",
                  "  space  to cancel",
                  ""};
    *(hlpmsg[7] + 4) = ctl(escape);
    savescreen(&tempscreen);
    killcursor();
    helpstart(41, 10);
    for (c = 0; c < 10; c++)
    helpline(hlpmsg[c]);
    helpend();
    cell[0] = ' ';
    cell[1] = line25attribute;
    AVIOWrtNCell(cell, 80, 24, 0);    /* Erase line 25 */
    scankey(&ac, &bc);
    c = ac & cmask;     /* Get character from keyboard */
    newcursor();
    restorescreen(&tempscreen);
    return ((int) c);
}


/*---------------------------------------------------------------------------*/
/* newcursor                                                               */
/*---------------------------------------------------------------------------*/
void newcursor()
{
    AVIOSetCurType(&crsr_info);
    AVIOSetCurPos(wherey - 1, wherex - 1);
    cursoron = TRUE;
}

/* CHSTR  --  Make a printable string out of a character  */

char 
* chstr(c) int
    c;
{
    static char     s[8];
    char           *cp = s;

    if (c < SP) {
    sprintf(cp, "CTRL-%c", ctl(c));
    } else
    sprintf(cp, "'%c'\n", c);
    cp = s;
    return (cp);
}

void vt_kp_send(c)
    char c;
    {
    extern ansi;

    char str[3];
    
    if (ansi)
        strcpy(str, "O ");
    else
        strcpy(str, "? ");
    str[1] = c;
    sendstrduplex(str);
    }
 
 
void screen_rollback(pachar,pbchar)
    unsigned char *  pachar;
    unsigned char *  pbchar;
    {
    int             i;
    int             il;
    char            str[3];
    int             prt;
    int             st;
    unsigned char   nolblines;
    unsigned char   linesleft;
    unsigned char   nextline;
    unsigned char   nlines;
    ascreen     *   savedscreen;/* screen save info */

    if((savedscreen = (ascreen *) malloc(sizeof(ascreen)))==NULL)
        {
        bleep();
        return;
        }

    savescreen(savedscreen);
    killcursor();
    linesleft = paginginfo.numlines;
    nextline = paginginfo.botline;
    do 
        {
        if (*pbchar == VK_PAGEDOWN) 
            {
            if (linesleft == 0)
                bleep();
            else 
                {
                /* scroll up a page */
                nlines = linesleft;
                if (nlines > 24)
                    nlines = 24;
                for (il = 1; il <= nlines; ++il) 
                    {
                    scroll(DOWNWARD, 0, 23);
                    movetoscreen(paginginfo.buffer + 160 * nextline, 1, 1, 160);
                    if (nextline == 0)
                        nextline = LBUFSIZE - 1;
                    else
                        nextline = nextline - 1;
                    }
                il--;
                linesleft = linesleft - nlines;
                } 
            }
        else if (*pbchar == VK_PAGEUP) 
            {
            nlines = 24;
            do 
                {
                nextline = nextline + 1;
                if (nextline >= LBUFSIZE)
                    nextline = 0;
                linesleft = linesleft + 1;
                /* lines of ext display above top of the screen */
                nolblines = paginginfo.numlines - linesleft;
                /* no. of ext disp buffer lines on screen */
                scroll(UPWARD, 0, 23);
                if (nolblines >= 24) 
                    {
                    /* move from buffer */
                    i = nextline;
                    i = (i + 24);
                    if (i >= LBUFSIZE)
                        i = i - LBUFSIZE;
                    movetoscreen(paginginfo.buffer + 160 * i, 1, 24, 160);
                    } 
                else 
                    {
                    /* move from the screen copy */
                    movetoscreen(&(savedscreen->scrncpy[(23 - nolblines) * 160]), 
                        1, 24, 160);
                    }
                nlines = nlines - 1;
                }while (!(((nlines == 0) || 
                 (linesleft == paginginfo.numlines))));
            } 
        else
            linesleft = paginginfo.numlines;

        if (linesleft != paginginfo.numlines) 
            {
            strcpy(helptext, "PgUp : up");
            strcpy(exittext, "PgDn : down");
            *usertext = '\0';
            *filetext = '\0';
            *hostname = '\0';
            line25();
            scankey(pachar, pbchar);
            }
        }while (linesleft != paginginfo.numlines);

    restorescreen(savedscreen);
    free(savedscreen);

    newcursor();
    ipadl25();
    return;
    }
    
    
//char linebuf[81];
char * plinebuf;
int next=0;
unsigned char wcx,wcy;
ULONG lbuf_sem=0;

void chars_in(c)
    unsigned char c;
    {
    extern char * plinebuf;
    extern unsigned char wherex,wherey;
    extern int next;
    extern unsigned char wcx,wcy;
    extern ULONG lbuf_sem;
    
    /* 20-Mar-90 Semaphore with chars_out() to prevent chars_in zeroing */
    /*           or providing a new plinebuf while chars_out is in the  */
    /*           process of sending the old plinebuf to be printed.     */
    /*           I think this was causing the peculiar effects when     */
    /*           many bels were received in quick succession.           */

    if(ttchk()==0)
        {
        if(next>0)
            chars_out();
        wrtch(c);
        }
    else
        {
        DosSemRequest(&lbuf_sem,SEM_INDEFINITE_WAIT);
        if(next>79)
            {
            plinebuf[80]=0;
            DosSemClear(&lbuf_sem);
            chars_out();
            DosSemRequest(&lbuf_sem,SEM_INDEFINITE_WAIT);
            }

        if(next==0)     /* Starting a new fill so save position of */
            {           /* start of string.                        */
            if((plinebuf = (char *)malloc(81))==NULL)
                {           /* Couldn't allocate memory so write character */
                wrtch(c);   /* directly.                                   */
                DosSemClear(&lbuf_sem);
                return;
                }
            wcx = wherex-1;
            wcy = wherey-1;
            }

        plinebuf[next]=c;
        next++;

        if(++wherex>80)  /* Need to update wherex, as wrtch would have done */
            {
            DosSemClear(&lbuf_sem);
            chars_out();
            wherex = 1;
            wrtch(LF);
            }
        else
            DosSemClear(&lbuf_sem);

        }    
    return;
    }
    
void chars_out()
    {
    extern char * plinebuf;
    extern unsigned char wherex,wherey;
    extern int next;
    extern unsigned char wcx,wcy;
    extern unsigned char attribute;
    extern ULONG lbuf_sem;

    DosSemRequest(&lbuf_sem,SEM_INDEFINITE_WAIT);
    if((next>0) && (plinebuf!=NULL))
        {
        AVIOdWrtCharStrAtt(plinebuf,next,wcy,wcx,&attribute);
        next=0;
        }
    DosSemClear(&lbuf_sem);
    
    return;
    }

/* This function works in "kernel" mode but does not seem to work in PM    */
/* mode, unfortunately.  Using the Kbd function fail also.  This means     */
/* that NumLock cannot be used to produce an emulation of the VT100 "Gold" */
/* key.  At least I tried!!                                                */
/* As it stands this function manages to toggle the NumLock light once,    */
/* from then on no keyboard input can be entered in the PM!  Regardless of */
/* the program!!.                                                          */
int numlock_toggle()
    {
    static HFILE hkbd=0;
    SHIFTSTATE sstate;
    USHORT action,err;
    BYTE imode,omode;

//    if(hkbd == 0)
//        {
//        if((err=DosOpen("KBD$",&hkbd,&action,NULL,0,FILE_OPEN,
//                OPEN_ACCESS_READWRITE | 
//                OPEN_SHARE_DENYNONE | 
//                OPEN_FLAGS_FAIL_ON_ERROR,
//                NULL))!=0)
//            {
//            dbprintf("Failed to open keyboard - error = %d\n",err);
//            }
//        }
//
//    if((err=DosDevIOCtl(&imode,0L,KBD_GETINPUTMODE,IOCTL_KEYBOARD,hkbd))!=0)
//        {
//        dbprintf("Failed to get KBD_INPUTMODE\n");
//        }
//    else
//        dbprintf("input mode is %X\n",(unsigned int)imode);
//
//    omode = imode | SHIFT_REPORT_MODE;
//
//    if((err=DosDevIOCtl(0L,&omode,KBD_SETINPUTMODE,IOCTL_KEYBOARD,hkbd))!=0)
//        {
//        dbprintf("Failed to set KBD_INPUTMODE\n");
//        }
//    else
//        dbprintf("input mode set to %X\n",(unsigned int)omode);
//
//    if((err=DosDevIOCtl((PBYTE)&sstate, 0L,KBD_GETSHIFTSTATE,IOCTL_KEYBOARD,hkbd))
//            !=0)
//        {
//        dbprintf("Failed to get shift state - error = %d\n",err);
//        }
//    
//    sstate.fsState ^= NUMLOCK_ON;
//    
//    if((err=DosDevIOCtl(0L,(PBYTE)&sstate,KBD_SETSHIFTSTATE,IOCTL_KEYBOARD,hkbd))
//            !=0)
//        {
//        dbprintf("Failed to set shift state - error = %d\n",err);
//        }                                                  
//
//    if((err=DosDevIOCtl(0L,&imode,KBD_SETINPUTMODE,IOCTL_KEYBOARD,hkbd))!=0)
//        {
//        dbprintf("Failed to reset KBD_INPUTMODE\n");
//        }
//    else
//        dbprintf("input mode reset to %X\n",(unsigned int)imode);
//
////    DosClose(hkbd);
    return(0);
    }
    
