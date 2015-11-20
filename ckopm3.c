/******************************************************************************
File name:  ckopm3.c    Rev: 01  Date: 22-Mar-90 Programmer: C.P.Armstrong

File title: Yet more functions which can't be fitted into ckopm1 or ckopm2
            because of the idiot memory restrictions present in V5.1 of
            MS C.  These problems primarily seem to be associated with the
            enormous OS/2 include files!
            
            This file contains mostly menu and dialog functions.

Contents:   

Modification History:
    01  22-Mar-90   C.P.Armstrong   created

******************************************************************************/
#define INCL_WIN
#define INCL_GPI 
#define INCL_DOS
#define INCL_AVIO
#include <OS2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ckorc.h"
#include "ckcker.h"   /* For VT100 and TEKTRONIX */
#include "ckotek.h"   /* For HPGL,TEK,WPG */
#include "ckopm.h"

MRESULT EXPENTRY ComsDlgProc(HWND,USHORT,MPARAM,MPARAM);
MRESULT EXPENTRY TermDlgProc(HWND,USHORT,MPARAM,MPARAM);
MRESULT EXPENTRY MiscDlgProc(HWND,USHORT,MPARAM,MPARAM);
int ComsDlgInit(HWND);
int ComsDlgExit(HWND);
int ComsDlgCmnd(HWND,int);
int TermDlgInit(HWND);
int TermDlgExit(HWND);
int TermDlgCmnd(HWND,int);
int MiscDlgInit(HWND);
int MiscDlgExit(HWND);
int MiscDlgCmnd(HWND,int);


int seslogdlg(HWND);
int tralogdlg(HWND);
int deblogdlg(HWND);
int pktlogdlg(HWND);

void cdecl pm_err(char *);          /* declared in ckopm.h.  Redeclared here */
void cdecl pm_msg(char*,char*);     /* to avoid having to include all the    */
                                    /* OS2.H stuff required for ckopm.h in an*/
                                    /* attempt to beat the MSC5.1 out of heap*/
                                    /* messages which occur for tiny files.  */
void buff_insert(int);

/******************************************************************************
Function:       menu_routine()

Description:    Called when a bottom level menu item is selected

Syntax:         int menu_routine(hwndmenu,hwnd,msg,mp1,mp2)
                    HWND hwndmenu;  Handle of the menu
                    HWND hwnd;      Handle of the calling window
                    HVPS hvps;      Handl to Vio presentation space
                    USHORT msg;     Parameters passed to the calling window
                    MPARAM mp1;
                    MPARAM mp2 ;

Returns:        0 at present

Mods:           22-Mar-90 C.P.Armstrong created
                01-Apr-90 C.P.Armstrong Edit menu support added. Params now
                                        include an HVPS - which can be null.

******************************************************************************/
int menu_routine(HAB hab,HWND hwndmenu, HWND hwnd, HVPS hvps, 
                 USHORT msg,MPARAM mp1,MPARAM mp2)
    {
    int log_mode;

    switch(SHORT1FROMMP(mp1))
        {
        case IDM_TERM:
            WinDlgBox(HWND_DESKTOP,hwnd,TermDlgProc,(HMODULE)0,IDD_TERM,NULL);
            break;
        case IDM_COMMS:
            WinDlgBox(HWND_DESKTOP,hwnd,ComsDlgProc,(HMODULE)0,IDD_COMMS,NULL);
            return(0);
        case IDM_NETWS:
            pm_msg("Coming soon....","Not implimented yet");
            break;
        case IDM_MISC:
            WinDlgBox(HWND_DESKTOP,hwnd,MiscDlgProc,(HMODULE)0,IDD_MISC,NULL);
            return(0);
        case IDM_SESSLOG:
            seslogdlg(hwnd);
            break;
        case IDM_TRNLOG:
            tralogdlg(hwnd);
            return(0);
        case IDM_DEBUG:
            deblogdlg(hwnd);
            return(0);
        case IDM_PAKT:
            pktlogdlg(hwnd);
            return(0);
        case IDM_REMDIR:
            pm_msg("Coming soon....","Do a remote dir");
            break;
        case IDM_REMCD:
            pm_msg("Coming soon....","Do a remote cd");
            break;
        case IDM_SEND:
            pm_msg("Coming soon....","Send file(s)");
            break;
        case IDM_GET:
            pm_msg("Coming soon....","Get file(s)");
            break;
        case IDM_COPY:
            do_copy(1,hvps,hab);
            break;
        case IDM_APP:
            do_copy(2,hvps,hab);
            break;
        case IDM_COPYPASTE:
            do_copy(1,hvps,hab);
        case IDM_PASTE:
            do_paste(hab);
            break;
        case IDM_PRINTG:
            do_print(hab,hwnd);
            break;
        case IDM_METAG:
            do_meta(hab,hwnd);
            break;
        default:
            pm_err("Bad menu message");
            break;
        }
    return(0);
    }
    
int init_menu(MPARAM mp1,HWND hwndMenu,HWND hwnd, HAB hab)
   {
   extern int marking;
   USHORT us;
   
   switch (SHORT1FROMMP (mp1))
        {
        case IDM_EDIT:
             WinSendMsg (hwndMenu, MM_SETITEMATTR,
                         MPFROM2SHORT (IDM_COPY, TRUE),
                         MPFROM2SHORT (MIA_DISABLED,
                              (marking==2) ? 0 : MIA_DISABLED)) ;

             WinSendMsg (hwndMenu, MM_SETITEMATTR,
                         MPFROM2SHORT (IDM_APP, TRUE),
                         MPFROM2SHORT (MIA_DISABLED,
                              (marking==2) ? 0 : MIA_DISABLED)) ;

             WinSendMsg (hwndMenu, MM_SETITEMATTR,
                         MPFROM2SHORT (IDM_COPYPASTE, TRUE),
                         MPFROM2SHORT (MIA_DISABLED,
                              (marking==2) ? 0 : MIA_DISABLED)) ;
             WinSendMsg (hwndMenu, MM_SETITEMATTR,
                         MPFROM2SHORT (IDM_PASTE, TRUE),
                         MPFROM2SHORT (MIA_DISABLED,
                  WinQueryClipbrdFmtInfo (hab, CF_TEXT, &us)
                                 ? 0 : MIA_DISABLED)) ;
             WinSendMsg (hwndMenu, MM_SETITEMATTR,
                         MPFROM2SHORT (IDM_VIEWCLIP, TRUE),
                         MPFROM2SHORT (MIA_DISABLED,MIA_DISABLED)) ;

             return 0 ;
       default:
           return(1);
   
       }

   return(1);
   }

/******************************************************************************
Function:       TermDlgProc()

Description:    Handles the communications parameters dialog.

Syntax:         MRESULT EXPENTRY ComsDlgProc(hwnd,msg,mp1,mp2)
                    HWND hwnd       Standard details passed to a 
                    USHORT msg      client procedure
                    MPARAM mp1
                    MPARAM mp2

Returns:        0 if message handled or result of WinDefDlgProc

Mods:           22-Mar-90 C.P.Armstrong created

******************************************************************************/
MRESULT EXPENTRY TermDlgProc(HWND hwnd, USHORT msg, MPARAM mp1,MPARAM mp2)
    {
    switch(msg)
        {
        case WM_INITDLG:
            TermDlgInit(hwnd);
            return(0);

        case WM_COMMAND:
            switch(COMMANDMSG(&msg)->cmd)
                {
                case DID_EXIT:
                    TermDlgExit(hwnd);
                case DID_CANCEL:
                    WinDismissDlg(hwnd,TRUE);
                    return(0);
                default:
                    break;
                }
            break;
        case WM_CONTROL:
            if(TermDlgCmnd(hwnd,SHORT1FROMMP(mp1))==0)
                  return(0);
            break;
            
        default:
            break;
        }
    return(WinDefDlgProc(hwnd,msg,mp1,mp2));
    }

int TermDlgExit(HWND hwnd)
    {
    extern void ipadl();
    extern int Term_mode;   /* VT100 or TEKTRONIX */
    extern int dump_format; /* Tek dump file,  HPGL, TEK, WPG */
    extern int cmask;       /* 7bit = 0177, 8bit = 0377 */
    extern int conected;
    extern ULONG threadsem; /* Set if in conect mode */

    int index;
    int newdf;

    index = (int)WinSendDlgItemMsg(hwnd,DID_HPGLDMP,
        BM_QUERYCHECKINDEX,NULL,NULL);
    switch(index)
        {
        case 1:     /* HPGL dump */
            newdf = HPGL;
            break;
        case 2:     /* No plot */
            newdf = NODMP;
            break;
        default:
            newdf = NODMP;
            break;
        }

    if( (Term_mode==TEKTRONIX) && (newdf != dump_format) )
        {
        /* Close the current one */
        gfile_close();
        /* set new format and open the dump file */
        dump_format = newdf;
        gfile_open();
        }
    else
        dump_format = newdf;



    index = (int)WinSendDlgItemMsg(hwnd,DID_VT100,BM_QUERYCHECKINDEX,NULL,NULL);
    switch(index)
        {
        case 1:                 /* VT100 selected */
            /* Close previous mode if necessary */
            if(Term_mode == TEKTRONIX)
                {
                Tek_finish();
                }

            Term_mode=VT100;
            break;
        case 2:                 /* Tektronix selected */
            /* Close previous mode if necessary */
            if(Term_mode == TEKTRONIX)
                break;
            
            Term_mode = TEKTRONIX;
            Tek_scrinit(1);
            break;
        default:
            break;
        }       

    index = (int)WinSendDlgItemMsg(hwnd,DID_TERM7BIT,
                    BM_QUERYCHECKINDEX,NULL,NULL);
    switch(index)
        {
        case 1:     /* 7bit */
            cmask = 0177;
            break;
        case 2:     /* 8bit */
            cmask = 0377;
            break;
        }
    
    /* Set the status line..... if in connect mode */
    if(conected==1)
        {               /* A kludge to get the conect() thread to update the */
        buff_insert(0); /* status line. Cant do from PM thread because of    */
        }               /* sem deadlock problems. scankey() intercepts a code*/
                        /* of 0 and does an ipadl25()                        */

    return(0);
    }

int TermDlgInit(HWND hwnd)
    {
    extern int Term_mode;   /* VT100 or TEKTRONIX */
    extern int dump_format; /* Tek dump file,  HPGL, TEK, WPG */
    extern int cmask;       /* 7bit = 0177, 8bit = 0377 */


    /* Check the current terminal emulation mode */
    switch(Term_mode)
        {
        case VT100:
            WinSendDlgItemMsg(hwnd,DID_VT100,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case TEKTRONIX:
            WinSendDlgItemMsg(hwnd,DID_TEK4014,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        default:
            break;
        }

    /* Check the current byte size parameter */
    switch(cmask)
        {
        case 0177:
            WinSendDlgItemMsg(hwnd,DID_TERM7BIT,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case 0377:
            WinSendDlgItemMsg(hwnd,DID_TERM8BIT,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        default:
            break;
        }
        
    /* Check the current Tektronix dump mode */
    switch(dump_format)
        {
        case NODMP:
            WinSendDlgItemMsg(hwnd,DID_NODMP,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case HPGL:
            WinSendDlgItemMsg(hwnd,DID_HPGLDMP,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case TEK:
            WinSendDlgItemMsg(hwnd,DID_TEKDMP,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case WPG:
            WinSendDlgItemMsg(hwnd,DID_WPGDMP,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        default:
            break;
        }

    return(0);
    }                                                       
    
int TermDlgCmnd(HWND hwnd,int cmnd)
    {
    return(-1);
    }



/******************************************************************************
Function:       ComsDlgProc()

Description:    Handles the communications parameters dialog.

Syntax:         MRESULT EXPENTRY ComsDlgProc(hwnd,msg,mp1,mp2)
                    HWND hwnd       Standard details passed to a 
                    USHORT msg      client procedure
                    MPARAM mp1
                    MPARAM mp2

Returns:        0 if message handled or result of WinDefDlgProc

Mods:           22-Mar-90 C.P.Armstrong created

******************************************************************************/
MRESULT EXPENTRY ComsDlgProc(HWND hwnd, USHORT msg, MPARAM mp1,MPARAM mp2)
    {
    switch(msg)
        {
        case WM_INITDLG:
            ComsDlgInit(hwnd);
            return(0);

        case WM_COMMAND:
            switch(COMMANDMSG(&msg)->cmd)
                {
                case DID_EXIT:                /* Must come directly before    */
                    if(ComsDlgExit(hwnd)!=0)  /* DID_CANCEL as the DID_CANCEL */
                        break;                /* is used to dismiss the dialog*/
                case DID_CANCEL:
                    WinDismissDlg(hwnd,TRUE);
                    return(0);
                default:
                    break;
                }
            break;
        case WM_CONTROL:
            if(ComsDlgCmnd(hwnd,SHORT1FROMMP(mp1))==0)
                  return(0);
            break;
            
        default:
            break;
        }
    return(WinDefDlgProc(hwnd,msg,mp1,mp2));
    }
    
/******************************************************************************
Function:       ComsDlgExit()

Description:    Sets new values for comunications parameters according to
                the entries made in the dialog.
                Note that this routine actually opens the new port, having
                closed any previously opened ports.

Syntax:         int ComsDlgExit(HWND hwnd)

Returns:        0 if port open successful
               -1 if unsuccessful

Mods:           26-Mar-90 C.P.Armstrong created

******************************************************************************/
int ComsDlgExit(HWND hwnd)
    {
    extern int ttnewport(char*);
    extern char ttname[];
    extern int speed;
    extern int odsr;
    extern int idsr;
    extern int octs;
    extern int flow;    /* Hardware handshaking Xon/Xoff=1, none=0 */
    extern char turnch; /* Handshake character */
    extern int parity;  /* Note that 8th-bit prefixing depends on this too */
    extern int conected;

    int temp;
    int old_speed,old_octs,old_odsr,old_idsr,old_flow,old_parity;
    char new_port[10];
    
    /* Make copy of the old values so we can reset them if an error occurs */
    old_speed=speed;
    old_octs=octs;
    old_odsr=odsr;
    old_idsr=idsr;
    old_flow=flow;
    old_parity=parity;


    /* Find which coms port is checked */
    temp = (int)WinSendDlgItemMsg(hwnd,DID_1COM,BM_QUERYCHECKINDEX,NULL,NULL);
    switch(temp)
        {
        case 1:
            strcpy(new_port,"COM1");
            break;
        case 2:
            strcpy(new_port,"COM2");
            break;
        case 3:
            strcpy(new_port,"COM3");
            break;
        default:
            strcpy(new_port,"ERR");
            break;
        }
        
    /* Get the speed */
    WinQueryDlgItemShort(hwnd,DID_BAUD,(PSHORT)&speed,FALSE);
    
    /* The parity */
    temp = (int) WinSendDlgItemMsg(hwnd,DID_EVEN,BM_QUERYCHECKINDEX,NULL,NULL);
    switch(temp)  /* Even,odd,mark,space,none */
        {
        case 1:
            parity='e';
            break;
        case 2:
            parity='o';
            break;
        case 3:
            parity='m';
            break;
        case 4:
            parity='s';
            break;
        case 5:
            parity=0;
            break;
        default:
            parity=0;
            break;
        }


    /* Do the hardware handshaking */
    temp = (int)WinSendDlgItemMsg(hwnd,DID_XHND,BM_QUERYCHECKINDEX,NULL,NULL);
    switch(temp)  /* hard,xon/xoff */
        {
        case 1:
            flow=0;
            break;
        case 2:
            flow=1;
            break;
        default:
            flow=1;
            break;
        }
    
    /* Do the hardware flow */
    odsr = (int)WinSendDlgItemMsg(hwnd,DID_ODSR,BM_QUERYCHECK,NULL,NULL);
    idsr = (int)WinSendDlgItemMsg(hwnd,DID_IDSR,BM_QUERYCHECK,NULL,NULL);
    octs = (int)WinSendDlgItemMsg(hwnd,DID_OCTS,BM_QUERYCHECK,NULL,NULL);
    

    /* Now try to open the port with the new settings */
    if( ttnewport(new_port)!=1)   /* Should check this doesn't happen */
        {                         /* during a transfer!               */
        pm_err("Failed to open port");
        /* Reset the parameters to the original values */
        speed  =    old_speed ;
        octs   =    old_octs  ;
        odsr   =    old_odsr  ;
        idsr   =    old_idsr  ;
        flow   =    old_flow  ;
        parity =    old_parity;
        return(-1);
        }

    /* Set the status line..... if in connect mode */
    if(conected==1)
        {               /* A kludge to get the conect() thread to update the */
        buff_insert(0); /* status line. Cant do from PM thread because of    */
        }               /* sem deadlock problems. scankey() intercepts a code*/
 
    return(0);
    }

int ComsDlgInit(HWND hwnd)
    {
    extern char ttname[];
    extern int speed;
    extern int odsr;
    extern int idsr;
    extern int octs;
    extern int flow;    /* Hardware handshaking Xon/Xoff=1, none=0 */
    extern char turnch; /* Handshake character */
    extern int parity;  /* Note that 8th-bit prefixing depends on this too */
    
    char * caps_port;

    /* Set the com ttname - capitalise first */
    caps_port = strupr(ttname);
    if(strcmp(caps_port,"COM1")==0)
        WinSendDlgItemMsg(hwnd,DID_1COM,BM_SETCHECK,MPFROMSHORT(1),0L);
    else if(strcmp(caps_port,"COM2")==0)
        WinSendDlgItemMsg(hwnd,DID_2COM,BM_SETCHECK,MPFROMSHORT(1),0L);
    else if(strcmp(caps_port,"COM3")==0)
        WinSendDlgItemMsg(hwnd,DID_3COM,BM_SETCHECK,MPFROMSHORT(1),0L);

    /* Set the speed text */
    WinSetDlgItemShort(hwnd,DID_BAUD,speed,FALSE);

    /* Set the parity */
    switch(parity)
        {
        case 'e':
            WinSendDlgItemMsg(hwnd,DID_EVEN,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case 'o':
            WinSendDlgItemMsg(hwnd,DID_ODD,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case 'm':
            WinSendDlgItemMsg(hwnd,DID_MARK,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case 's':
            WinSendDlgItemMsg(hwnd,DID_SPACE,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        case 0:
            WinSendDlgItemMsg(hwnd,DID_NONE,BM_SETCHECK,MPFROMSHORT(1),0L);
            break;
        default:
            break;
        }

    /* The data bits */
    
    /* The stop bits */

    /* Hardware handshaking */
    if( flow >0 )
        WinSendDlgItemMsg(hwnd,DID_XHND,BM_SETCHECK,MPFROMSHORT(1),0L);
    else
        WinSendDlgItemMsg(hwnd,DID_HRDHND,BM_SETCHECK,MPFROMSHORT(1),0L);

    /* Hardware flow control */
    WinSendDlgItemMsg(hwnd,DID_ODSR,BM_SETCHECK,
            MPFROMSHORT( ((odsr>0 )?1:0) ),0L);
    WinSendDlgItemMsg(hwnd,DID_IDSR,BM_SETCHECK,
            MPFROMSHORT( ((idsr>0 )?1:0) ),0L);
    WinSendDlgItemMsg(hwnd,DID_OCTS,BM_SETCHECK,
            MPFROMSHORT( ((octs>0 )?1:0) ),0L);

    return(0);
    }                                                       
    
int ComsDlgCmnd(HWND hwnd,int cmnd)
    {
    char buf[10];

    switch(cmnd)
        {
        case DID_IDSR:
        case DID_OCTS:
        case DID_ODSR:
            if(WinSendDlgItemMsg(hwnd,cmnd,BM_QUERYCHECK,NULL,NULL)==0)
                WinSendDlgItemMsg(hwnd,cmnd,BM_SETCHECK,MPFROMSHORT(1),NULL);
            else
                WinSendDlgItemMsg(hwnd,cmnd,BM_SETCHECK,NULL,NULL);
            return(0);
        default:     
            break;
        }
    return(-1);
    }


/******************************************************************************
Function:       MiscDlgProc()

Description:    Handles the miscellaneous parameters dialog.

Syntax:         MRESULT EXPENTRY MiscDlgProc(hwnd,msg,mp1,mp2)
                    HWND hwnd       Standard details passed to a 
                    USHORT msg      client procedure
                    MPARAM mp1
                    MPARAM mp2

Returns:        0 if message handled or result of WinDefDlgProc

Mods:           25-Apr-90 C.P.Armstrong created

******************************************************************************/
MRESULT EXPENTRY MiscDlgProc(HWND hwnd, USHORT msg, MPARAM mp1,MPARAM mp2)
    {
    switch(msg)
        {
        case WM_INITDLG:
            MiscDlgInit(hwnd);
            return(0);

        case WM_COMMAND:
            switch(COMMANDMSG(&msg)->cmd)
                {
                case DID_EXIT:
                    MiscDlgExit(hwnd);
                case DID_CANCEL:
                    WinDismissDlg(hwnd,TRUE);
                    return(0);
                default:
                    break;
                }
            break;
        case WM_CONTROL:
            if(MiscDlgCmnd(hwnd,SHORT1FROMMP(mp1))==0)
                  return(0);
            break;
            
        default:
            break;
        }
    return(WinDefDlgProc(hwnd,msg,mp1,mp2));
    }

int MiscDlgExit(HWND hwnd)
    {
    extern int paste_eol_char;

    WinQueryDlgItemShort(hwnd, DID_PEOL,(PSHORT)&paste_eol_char,FALSE);

    return(0);
    }

int MiscDlgInit(HWND hwnd)
    {
    extern int paste_eol_char;

    /* Set the paste end of line character */
    WinSetDlgItemShort(hwnd, DID_PEOL, (SHORT) paste_eol_char,FALSE);

    return(0);
    }                                                       
    
int MiscDlgCmnd(HWND hwnd,int cmnd)
    {
    return(-1);
    }
    
    
