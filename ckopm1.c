/******************************************************************************
File name:  pminfn.c     Rev: 01  Date: 24-Nov-89 Programmer: C.P.Armstrong

File title: Provides an interface to the Presentation Manager for programs
            which were originally designed as MS-DOS command line like
            applications.  This file contains code which initializes and
            starts a thread which processes Presentation Manager message queue
            events.  An AVIO presentation space is created for the programs I/O.
            This has the advantage of being fixed pitch and directly addressable
            by the program.  Unfortunately it would appear that a number of the
            Vio functions don't work with PM presentation spaces when used
            outside of the main PM thread.  This means that all screen I/O
            must be performed directly.

Contents:   

Modification History:
    01  24-Nov-89 C.P.Armstrong created
        27-Nov-89 C.P.Armstrong A VioWrtTTY look-alike which is compatible with
                                multi-threaded AVIO PM programs.  Frills such as
                                cursor hiding, colour matching etc.
        05-Feb-90 C.P.Armstrong PM thread termination does not shut down whole 
                                process.
******************************************************************************/
#define INCL_DOS
#define INCL_WIN
#define INCL_GPI    
#define INCL_VIO
#define INCL_AVIO
#include <OS2.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include "ckopm.h"
#include "ckorc.h"     /* Resource defines */

#define VIOTITLETEXT  "C-Kermit Command Mode"

#define WINDPROCSTACKSIZE   9000  /* Last value was 10000 */
#define YES                    1
#define NO                     0
#define DEFAULTCOLOUR       0x1E
#define MAXPC               1024
#define CURBLINK               1

/* Window handles, flags, semaphores etc.. */
HAB     hab;
HWND    hwndTFrame;        /* Frame handle of the main window */
HWND    hwndGFrame;        /* Frame handle of the child window */
HWND    hwndText;          /* Handle to the Vio text window */
HWND    hwndGraph;         /* Handle to the Gpi graph window */
HDC     hdc;
HPS     gpi_hps;           /* Handle to the Gpi window PS */
ULONG far wind_sem=0;        /* Set when video buffer is allocated */
ULONG far vio_sem=0;         /* Used by the Vio routines when they are done */
ULONG far kbd_sem=0;         /* Cleared when a character is put */
                             /* into the buffer */
ULONG far paint_sem=0;          /* Set when repaint desired */
ULONG far stop_painting_sem=0;  /* Set when about to clear */
ULONG far pm_die_sem=0;         /* Set by main thread, cleared by PM thread */
                                /* before dying */


/* Screen variables */
char far * dispbuf="C.P.A. 1989";/* Pointer to string to be output to screen */
char far * ulVideoBuffer;    /* Pointer to the video buffer */
USHORT  usVideoLength;       /* Length of the video buffer */
HVPS    hvps;                /* Handle to video presenation space */
                             /* used by VioShowBuf */
int far scrwidth=80;         /* Screen size in characters - used by vWrtchar()*/
int far scrheight=25;
int     Vwinheight;          /* Size of Vio window in pels (including the     */
int     Vwinwidth;           /* border etc..                                  */
extern char defaultattribute;    /* Colour attribute used for the Vio screen */
COLOR   pmbgcol;                 /* PM colours - defaultattribute is */
                                 /* converted into */
COLOR   pmfgcol;                 /* these for PM colouring.                  */
char    cursor_displayed=0;      /* Record of whether cursor should be on    */
USHORT  curblink;                /* id of cursor timer */


/* Misc definitions */
char debugmsg=0;
char Vio_visible=1;             /* Is Vio screen visible */  
SWP pc_swp;                     /* Data used by the repaint thread */
char minied;                    /* Indicates window is minimized */

MPARAM dum1;
MPARAM dum2;

/* Function definitions */
MRESULT EXPENTRY GpiWndProc(HWND,USHORT,MPARAM,MPARAM);
MRESULT EXPENTRY VioWndProc(HWND,USHORT,MPARAM,MPARAM);
void dbprintf(const char *,...);
int  DecodePMChar(MPARAM,MPARAM);
void Put_cursor_onscreen(HWND,HVPS);
void TitleText(MPARAM,MPARAM);
int  DoPMTekGin(HWND,USHORT*);
int  numlock_toggle();
int  numlock_status();
void avio_dispatch(struct avio_cellstr *, HVPS);

int do_mark(HWND,MPARAM,MPARAM,HVPS);
int do_copy(int,HVPS,HAB);
int end_mark(HWND,MPARAM,MPARAM,HVPS);
int start_mark(HWND,MPARAM,MPARAM,HVPS);
int quit_mark(HWND,MPARAM,MPARAM,HVPS,HAB);

// void far pc_paint_thread();
/******************************************************************************
Function:       PM_init()

Description:    Initializes the PM message queue thread, client window routine
                etc..

Syntax:         int PM_init()

Returns:        0 if successful
                -1 if the thread create fails

Mods:           24-Nov-89 C.P.Armstrong created

******************************************************************************/
int PM_init(title)
    char far * title;
    {
    extern HVPS hvps;   
    extern USHORT usVideoLength;
    extern char far * ulVideoBuffer;
    extern ULONG wind_sem;
    int* wind_proc_stack;
    TID   wind_proc_id;

    int c;
    char buf[90];

    /* allocate stack for window thread */
    if((wind_proc_stack = (int*) malloc(WINDPROCSTACKSIZE))==NULL)
        {
        dbprintf("Can't allocate window thread stack\n");
        DosExit(EXIT_PROCESS,1);
        }

    /* Set the semaphore so it can be cleared when video buffer is alloc'd */
    if(DosSemSet(&wind_sem)!=0)
        {
        dbprintf("Can't set window semaphore\n");
        DosExit(EXIT_PROCESS,1);
        }

    _beginthread(window_thread,wind_proc_stack,WINDPROCSTACKSIZE,title);

    /* Wait for semaphore to be set indicating that the window has been */
    /* created. If an error has occurred then the program will be ended */
    /* where the error occurs so an indefinite wait should be okay.     */
    DosSemWait(&wind_sem,SEM_INDEFINITE_WAIT);

    return(0);
    }

/******************************************************************************
Function:       window_thread()

Description:    The presentation manager message queue thread.

Syntax:         void window_thread(title)
                    char far * title;  Pointer to main window title

Returns:        nothing

Mods:           07-Dec-89 C.P.Armstrong
                05-Feb-90 C.P.Armstrong  Termination of this thread does not 
                                         cause termination of the whole process.
                25-Feb-90 C.P.Armstrong  Function clears a semaphore just prior
                                         to dying.

******************************************************************************/
void far window_thread(title)
    char far * title;
    {
    extern HWND hwndTFrame,hwndGFrame,hwndText,hwndGraph;
    extern HDC hdc;
    extern Vwinheight,Vwinwidth;
    extern ULONG wind_sem;
    extern ULONG pm_die_sem;
    extern USHORT  curblink; 
    static CHAR szClientClass[]="Vio window";
    static CHAR szGraphClass[]="Graph window";
    static ULONG flTFrameFlags = FCF_TITLEBAR
                                | FCF_SIZEBORDER
                                | FCF_MINMAX;
    static ULONG flGFrameFlags = FCF_TITLEBAR | FCF_SIZEBORDER 
                                | FCF_SHELLPOSITION 
                                | FCF_ICON
                                | FCF_SYSMENU
                                | FCF_MENU
                                | FCF_MINMAX ;
    HMQ hmq;
    QMSG qmsg;
    SIZEL sizl;
    RECTL rcl;
    SWP swp;
    SWCNTRL swctl;
    HSWITCH hswitch;
    PID pid;
    LONG bord_title;


    hab = WinInitialize(0);
    hmq=WinCreateMsgQueue(hab,0);
    
    /* Register the Vio window */
    WinRegisterClass(hab, szClientClass, VioWndProc,CS_SIZEREDRAW,0);
    
    /* Register the graphics window */
    WinRegisterClass(hab, szGraphClass, GpiWndProc,CS_SIZEREDRAW,0);


    /* Create the main (Gpi) window */
    hwndGFrame = WinCreateStdWindow(HWND_DESKTOP,WS_VISIBLE,
                      &flGFrameFlags,
                      szGraphClass, 
                      title, WS_CLIPCHILDREN, (HMODULE)NULL, 
                      ID_KERMIT, &hwndGraph);

    /* Create the child (Vio) window */
    hwndTFrame = WinCreateStdWindow(hwndGraph,WS_VISIBLE,&flTFrameFlags,
                          szClientClass, VIOTITLETEXT, 0L, (HMODULE)NULL, 
                          0, &hwndText);

    /* Determine size of Vio window */
    DevQueryCaps(hdc, CAPS_CHAR_WIDTH, 2L, (PLONG) &sizl);
    sizl.cx *= (long) scrwidth;
    sizl.cx += (LONG)(2*WinQuerySysValue(HWND_DESKTOP,SV_CXSIZEBORDER));
    Vwinwidth = (int) sizl.cx;

    sizl.cy *= (long) scrheight;
    bord_title = (LONG) (WinQuerySysValue(HWND_DESKTOP,SV_CYTITLEBAR)+
                    (2*WinQuerySysValue(HWND_DESKTOP,SV_CYSIZEBORDER)));
    sizl.cy += bord_title;
    Vwinheight = (int) sizl.cy;

    /* Make initial com line window just fit the GPI window */
    WinQueryWindowRect(hwndGFrame,&rcl);     /* Get GPI frame size */
    WinCalcFrameRect(hwndGFrame,&rcl,TRUE);  /* Convert to client window size */

    /* Use Gpi client size as Vio Frame size */
    WinSetWindowPos(hwndTFrame,HWND_TOP,(SHORT)0,(SHORT)0,
            (SHORT) (rcl.xRight-rcl.xLeft), (SHORT)(rcl.yTop-rcl.yBottom) ,
            SWP_SIZE|SWP_ZORDER|SWP_SHOW|SWP_MOVE|SWP_ACTIVATE);

    WinSendMsg(hwndTFrame,WM_SETICON,
              WinQuerySysPointer(HWND_DESKTOP,SPTR_APPICON,FALSE),
              NULL);

    /* Add program title to the task switch list */
    if((hswitch = AddToSwitch(hwndGFrame,NULL,title))==NULL )
        {
        pm_err("Failed to add program to switch list");
        }
              
    /* The window should be up and running now so tell the main thread */
    DosSemClear(&wind_sem);

    /* Start the timer for the blinking cursor - using the system */
    /* value for the rate */
    WinStartTimer(hab,hwndText,CURBLINK,(USHORT)WinQuerySysValue(HWND_DESKTOP,
                    SV_CURSORRATE)/2 );


    while(WinGetMsg(hab,&qmsg,NULL,0,0))
        WinDispatchMsg(hab,&qmsg);
        
    WinStopTimer(hab,hwndText,CURBLINK);
    WinDestroyWindow(hwndTFrame);
    WinDestroyWindow(hwndGFrame);
    WinDestroyMsgQueue(hmq);
    WinRemoveSwitchEntry(hswitch);
    WinTerminate(hab);
    if(DosSemWait(&pm_die_sem,SEM_IMMEDIATE_RETURN)!=0)
        {
        DosEnterCritSec();
        DosSemClear(&pm_die_sem);
        dbprintf("Exiting window thread\n");
        DosExit(EXIT_THREAD,0);
        }
    dbprintf("Window thread killing whole process\n");
    DosExit(EXIT_PROCESS,0);
    }

/******************************************************************************
Function:       VioWndProc()

Description:    Handles the Vio window text input and output

Syntax:         

Returns:        

Mods:           07-Dec-89 C.P.Armstrong

******************************************************************************/
MRESULT EXPENTRY VioWndProc(HWND hwnd,USHORT msg,MPARAM mp1,MPARAM mp2)
    {
    extern USHORT  curblink;                /* id of cursor timer */
    extern HVPS hvps;
    extern USHORT usVideoLength;
    extern char far * ulVideoBuffer;
    extern COLOR pmbgcol,pmfgcol;
    extern char  defaultattribute;
    extern int scrheight, scrwidth;
    extern int Vwinheight,Vwinwidth;
    extern ULONG wind_sem;
    extern ULONG vio_sem;
    extern char Vio_visible;
    extern int marking;

    static HPS hps;  /* PS handle used by Gpi calls */
    extern HDC hdc;
    RECTL rcl;
    SIZEL sizl;
    long rgbcol;
    PSWP pswp;
    QMSG qmsg;
    
    int i,j,t,x,y;
    struct avio_cellstr * pac;
    
    switch(msg)
        {
        case WM_CREATE:
            hdc = WinOpenWindowDC(hwnd);
            sizl.cx = sizl.cy = 0;
            hps = GpiCreatePS(hab,hdc,&sizl, PU_PELS | GPIF_DEFAULT | 
                                             GPIT_MICRO | GPIA_ASSOC);
            VioCreatePS(&hvps,scrheight,scrwidth,0,1,(HVPS)NULL);
            VioAssociate(hdc,hvps);
            
            VioGetBuf((PULONG)&ulVideoBuffer,&usVideoLength,hvps);
            VioWrtNAttr(&defaultattribute, usVideoLength,0,0,hvps);
            
            return(0L);
            
       case WM_TIMER:
            if(SHORT1FROMMP(mp1)==CURBLINK)
                {
                flash_cursor(hvps);
                return(0L);
                }
             break;

       case WM_ADJUSTWINDOWPOS:   // about to size - remove cursor
            show_cursor(NO,hvps);
            return(0L);

       case WM_SIZE:
            quit_mark(hwnd,mp1,mp2,hvps,hab);            
            /* Prevent the size from being made bigger than the video buffer */
            /* First calculate the extra dimensions of the frame window */

            /* Calculate Frame window size - */
            /* this allows for menus, scroll bars etc. */
            rcl.xRight  = SHORT1FROMMP(mp2);
            rcl.xLeft   = 0;
            rcl.yTop    = SHORT2FROMMP(mp2);
            rcl.yBottom = 0;
            WinCalcFrameRect(hwndTFrame,&rcl,FALSE);  /* Get frame size */
            i =  (int) (rcl.xRight-rcl.xLeft);
            j = (int) (rcl.yTop-rcl.yBottom);

            t=0;
            
            /* Do the check */
            if(i>(Vwinwidth))
                {
                i = Vwinwidth;
                t = 1;
                }
            else if(j>(Vwinheight))
                {   
                j = Vwinheight;
                t = 1;
                }

            if(t==1)
                {
                WinSetWindowPos(hwndTFrame,0,0,0,i,j,SWP_SIZE);
                MPFROM2SHORT(i,j);
                return(FALSE);
                }
                
            WinDefAVioWindowProc(hwnd,msg,mp1,mp2);
            show_cursor(YES,hvps);
            Put_cursor_onscreen(hwnd,hvps);
            return(0L);
            
       case WM_SETSELECTION:
           if(SHORT1FROMMP(mp1))
               {                
               /* We are being selected */
               show_cursor(YES,hvps);
               }
            else
               {
               /* We are being deselected */
               show_cursor(NO,hvps);
               }
            
        case WM_PAINT:
            WinBeginPaint(hwnd,hps,NULL);
            WinQueryWindowRect(hwnd,&rcl);
            pmbgcol = VioToRgbColor((BYTE)(defaultattribute>>((char)4)));
            pmfgcol = VioToRgbColor(defaultattribute);

            WinFillRect(hps,&rcl,pmbgcol);
            
            VioShowPS(scrheight,scrwidth,0,hvps);
            WinEndPaint(hps);
            return(0L);
            
        case WM_HELP:
            pm_msg("Help", "Try typing ? at the command line prompt.\n");
            return(0L);

        case WM_CHAR:
            /* There seems to be no other way of getting keyboard input to */
            /* the parent window.                                          */
            quit_mark(hwnd,mp1,mp2,hvps,hab);            

            if(TestPMGinMode())
                {
                WinSendMsg(hwndGraph,WM_CHAR,mp1,mp2);
                return((MRESULT)1);
                }


            if(CHARMSG(&msg)->fs & KC_KEYUP)            /* Ignore key ups */
                {
                return(0L);
                }

            DecodePMChar(mp1,mp2);
            
//          break;
            return(0L);

        case WM_BUTTON1DOWN:
            start_mark(hwnd,mp1,mp2,hvps);
            break;

            
        case WM_BUTTON1UP:
            end_mark(hwnd,mp1,mp2,hvps);
            break;

        case WM_MOUSEMOVE:
            do_mark(hwnd,mp1,mp2,hvps);
            break;



        case WM_HIDE:
            if(mp1==(MPARAM)1)
                {
                WinShowWindow(hwndTFrame,FALSE);
                Vio_visible=0;
                }
            else
                {
                WinShowWindow(hwndTFrame,TRUE);
                Vio_visible=1;
                }

            return(0L);

        case WM_KERAVIO:
            quit_mark(hwnd,mp1,mp2,hvps,hab);            

            pac = (struct avio_cellstr *) PVOIDFROMMP(mp1);

            /* Do the avio command */
            process_avio(pac,hvps);
              
            DosSemClear(&vio_sem);
            return(0L);

        case WM_TITLETEXT:
            TitleText(mp1,mp2);
            return(0L);

        case WM_CURCHECK:
            quit_mark(hwnd,mp1,mp2,hvps,hab);            
            Put_cursor_onscreen(hwnd,hvps);
            return(0L);

        case WM_DESTROY:
            VioAssociate(NULL,hvps);
            VioDestroyPS(hvps);
            GpiDestroyPS(hps);
            return(0L);
        }
    
    return(WinDefWindowProc(hwnd,msg,mp1,mp2));
    }

   
   
