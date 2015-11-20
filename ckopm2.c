/******************************************************************************
File name:  ckopm2.c     Rev: 01  Date: 07-Dec-89 Programmer: C.P.Armstrong

File title: Functions called from the PM thread

Contents:   

Modification History:
    01  07-Dec-89   C.P.Armstrong   created
    02  29-Jan-90   C.P.Armstrong   Various mods to the pc functions. 
                                    Inter-thread AVIO functions moved to this 
                                    file to ease ckopm1 compiling.
    03  11-Feb-90   C.P.Armstrong   More functionality added to the pc_ 
                                    functions.
    04  12-Feb-90   C.P.Armstrong   killpm() changed
******************************************************************************/
#define INCL_DOS
#define INCL_WIN
#define INCL_GPI    
#define INCL_AVIO
#define INCL_VIO
#include <OS2.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include "ckopm.h"
#include "ckotek.h"
#include "ckokey.h"

#define LBSZ 81
#define KEYBUFFERSIZE        128


//unsigned char linebuf[LBSZ];


/* Keyboard buffering variables */
int keybuffer[KEYBUFFERSIZE];  /* The buffer for the key presses */
int keybuffer_size=KEYBUFFERSIZE;   /* The buffer size */
int far next_put_pos=0;        /* Offset of next character into buffer */
int far next_get_pos=0;        /* Offset of next character from buffer */
ULONG far kbdovr_sem=0;       /* Keyboard buffer is in overflow condition */


void Tek_ginencode();
int get_paste_char(void);

/******************************************************************************
Function:       clrgwind()

Description:    Clears the graphics window

Syntax:         void clrgwind()

Returns:        nothing

Mods:           30-Nov-89 C.P.Armstrong created

******************************************************************************/
void clrgwind()
    {
    extern HWND hwndGraph;
    
    WinPostMsg(hwndGraph,WM_USER,MPFROM2SHORT(0,0),MPFROMCHAR('c'));
    }


///******************************************************************************
//Function:       vWndSize()
//
//Description:    Provides access to the Vio screen size params for functions
//                which can't get at them directly (i.e. compiled with FORTRAN
//                conventions etc.).  The size is in number of Vio characters.
//
//Syntax:         vWndSize(width,height)
//                    int * width;  # of columns
//                    int * height; # of rows.
//
//Returns:        nothing
//
//Mods:           28-Nov-89 C.P.Armstrong created
//
//******************************************************************************/
//
//void vWndSize(width,height)
//    int * width;  /* # of columns */
//    int * height; /* # of rows. */
//    {
//    extern int scrwidth;
//    extern int scrheight;
//    
//    *width = scrwidth;
//    *height = scrheight;
//    
//    return;
//    }
//
/******************************************************************************
Function:       killpm()

Description:    Shuts down the PM message queue thread.  If this thread does
                not shut down the whole program after 5s then killpm() does
                it.  

Syntax:         void killpm()

Returns:        nothing

Mods:           28-Nov-89 C.P.Armstrong created
                12-Feb-90 C.P.Armstrong modified for use with kermit.  Does not
                                        wait after sending the kill message.
                                        The PM thread is modified to kill only
                                        it's thread.
                25-Feb-90 C.P.Armstrong Function waits for the PM thread to die
                                        before returning.

******************************************************************************/
void killpm()
    {
    extern HWND cdecl hwndTFrame;
    extern ULONG pm_die_sem;

    if(DosSemSet(&pm_die_sem)==0) /* This fails the PM thread has  */
        {                         /* already started to exit       */
        WinPostMsg(hwndTFrame,WM_DESTROY,MPFROM2SHORT(0,0),MPFROM2SHORT(0,0));
        WinPostMsg(hwndTFrame,WM_QUIT   ,MPFROM2SHORT(0,0),MPFROM2SHORT(0,0));
        DosSemWait(&pm_die_sem,5000L);
        }
    }

/******************************************************************************
Function:       pltnow()

Description:    Changes the flag used to cause immediate update of the screen
                on sending a point to be plotted.
                Set value to 1 for immediate updating, 0 if the update can
                wait until a whole load of points have been sent.
                
                Manet as a convenient interface for FORTRAN programs. 

Syntax:         void fortran pltnow( value );
                    int * value;  To update or not

Returns:        nothing

Mods:           30-Nov-89 C.P.Armstrong

******************************************************************************/

void fortran pltnow( value )
    int * value;  /* To update or not */
    {
    extern char cdecl immediate_update;

    immediate_update= (char) *value;
    }

/******************************************************************************
Function:       pm_err()

Description:    Displays a message box containing a string indicating the
                nature of the error.

Syntax:         void pm_msg(title,message)
                    char * message;  Text to display in box;

Returns:        nothing

Mods:           05-Dec-89 C.P.Armstrong created

******************************************************************************/

void pm_msg(title,message)
    char * title;
    char * message;  /* Text to display in box; */
    {
    USHORT idWin;
    USHORT flst;
    HWND cur_wind;

    flst = MB_OK|MB_ICONEXCLAMATION;
    cur_wind = WinQueryActiveWindow(HWND_DESKTOP, FALSE);
    WinMessageBox(HWND_DESKTOP, cur_wind, message, title,0,flst);
    
    return;
    }

void pm_err(message)
    char * message;
    {
    pm_msg("Error!!",message);
    }

/******************************************************************************
Function:       dbprintf()

Description:    Prints a window message box containing debugging information.
                This must only be called from the PM thread.

Syntax:         void dbprintf(format,...)
                    const char *;  Format string

Returns:        nothing

Mods:           06-Dec-89 C.P.Armstrong created

******************************************************************************/
void dbprintf(format,...)
    const char * format;  /* Format string */
    {
    extern char debugmsg;
    va_list varg_ptr;
    char * bufp;  /* Buffer for the formatted string */
    static FILE * dbh;

//    return;

    va_start(varg_ptr,format);   /* Get start of optional argument list*/    
                       

    if((bufp = (char *) malloc(512))==NULL) 
        {
        pm_err("I think you are running out of memory");
        return;
        }
        
    vsprintf(bufp, format, varg_ptr);
    if(debugmsg<1)
        {
        if(dbh==NULL)
            dbh = fopen("debug","w");
        fprintf(dbh,"%s",bufp);
        fflush(dbh);
        }
    else
        pm_msg("Debug Info",bufp);
    
    free(bufp);
    return;
    }
 


/******************************************************************************
Function:       pc_interp()

Description:    Interprets the plot commands sent from the main thread to the
                PM thread.

Syntax:         void pc_interp(pc,swp,mode,hps)
                    struct plot_command pc;  The plot command to interpret
                    PSWP swp;                Dimension and coords of window to 
                                             plot in
                    char mode;               0 text, 1 no text
                    HPS hps;                 Handle to the PS.

Returns:        nothing

Mods:           13-Dec-89 C.P.Armstrong created
                12-Jan-90 C.P.Armstrong Function must now be passed a pointer
                                        to an SWP structure.  For some reason 
                                        passing structures does not work in with
                                        IDA - it works with Kermit. Perhaps a
                                        problem with mixed languages.
                14-Jan-90 C.P.Armstrong Poly line support added.  Uses ptl.x
                                        to store the pointer to an array of
                                        POINTLs.
                11-Feb-90 C.P.Armstrong Point plot mode and set line type added.
******************************************************************************/
void pc_interp(pc,swp,mode,hps)
    struct plot_command pc;  /* The plot command to interpret */
    PSWP swp;                /* Dimension and coords of window to plot in */
    char mode;  
    HPS hps;                 /* Handle to the PS. */
    {
    POINTL ptl;
    PPOINTL ptlc;
    PPOINTL ptlo;
    char far * pso;
    int i;

    switch(pc.c)
        {
        case 'm':       /* Means do a move */
            ptl.x = pc.ptl.x * swp->cx / MAXXRES;
            ptl.y = pc.ptl.y * swp->cy / MAXYRES;
            GpiMove(hps, &ptl);
            break;
        case 'd':
            ptl.x = pc.ptl.x * swp->cx / MAXXRES;
            ptl.y = pc.ptl.y * swp->cy / MAXYRES;
            GpiLine(hps,&ptl);
            break;
        case 'p':
            ptl.x = pc.ptl.x * swp->cx / MAXXRES;
            ptl.y = pc.ptl.y * swp->cy / MAXYRES;
            GpiMarker(hps,&ptl);
            break;
        case 'l':
            GpiSetLineType(hps,ptl.x);
            break;
        case 's':
            if(mode<1)
                {
                pso = (char far *) pc.ptl.x;
                GpiCharString(hps, (long) strlen(pso),pso);
                }
            break;

        case 'y':  /* Polyline drawing mode */
            /* Allocate memory for scaled copy of array */
            ptlc = (PPOINTL) malloc(sizeof(POINTL)*(int)pc.ptl.y);
            ptlo = (PPOINTL) pc.ptl.x;
            
            /* fill copy with scaled values */
            for(i=0;i< (int)pc.ptl.y;i++)
                {
                ptlc[i].x = ptlo[i].x * swp->cx / MAXXRES;
                ptlc[i].y = ptlo[i].y * swp->cy / MAXYRES;
                }

            GpiPolyLine(hps,pc.ptl.y,ptlc);
            free(ptlc);
            break;
        default:
            break;
        }
    return;
    }


/******************************************************************************
Function:       pc_delete()

Description:    Deletes the plot command linked lists starting from the 
                root command - freeing any reserved string space on the way.

Syntax:         void pc_delete(pctop)
                    struct plot_command * pctop; First item in linked list

Returns:        nothing

Mods:           13-Dec-89 C.P.Armstrong created
                14-Jan-90 C.P.Armstrong Polyline support added

******************************************************************************/

void pc_delete(pctop)
    struct plot_command * pctop; /* First item in linked list */
    {
    struct plot_command * pcc;
    struct plot_command * pclast;

    pcc = pctop;
    do
        {
        /* Free any string space first */
        if(pcc->c=='s' || pcc->c=='y')
            free( (void *) (pcc->ptl.x) );
        
        pclast=pcc;              /* Record current address */
        pcc = pcc->next;         /* Point to the next command */
        
        if(pclast!=pctop)        /* Delete previous command if not the root */
            free( (void far *) pclast);
        }
    while(pcc!=NULL);
    
    /* Reset pctop values to start values */
    pctop->c='\0';
    pctop->next=NULL;

    return;
    }


/******************************************************************************
Function:       pc_save()

Description:    Adds a new plot command to the linked list of plot commands

Syntax:         struct plot_command * pc_save( pc,mp1,mp2 )
                    struct plot_command pc;  current last plot command in list
                    MPARAM mp1;     Parameter 1 as passed to window routine
                    MPARAM mp2;     Parameter 2 as passed to window routine

Returns:        pointer to the new plot command.

Mods:           14-Jan-90 C.P.Armstrong created
                11-Feb-90 C.P.Armstrong Handling for plotting and setting line
                                        types

******************************************************************************/

struct plot_command * pc_save( pcurrent,mp1,mp2 )
    struct plot_command * pcurrent;  /* current last plot command in list */
    MPARAM mp1;     /* Parameter 1 as passed to window routine */
    MPARAM mp2;     /* Parameter 2 as passed to window routine */
    {
    struct plot_command * pcold;
    char far * psi;                 /* pointer to a string from mp */
    char new_command=0;
    char c;
    
    c = CHAR1FROMMP(mp2);

    /* Allocate space for a new plot command */
    pcold = pcurrent;
    if((pcurrent->next==NULL) && (c!='\0'))  /* Check it's not the first in */ 
        {                                    /* list */
        pcurrent->next = 
            (struct plot_command *) malloc(sizeof(struct plot_command));
        pcurrent=pcurrent->next;
        pcurrent->next=NULL;
        new_command=1;
        }

    /* pcurrent is new command unless it's the first entry in the list */
    /* which does not need to be allocated as it is always present.    */
    pcurrent->c = c;

    switch(c)
        {
        case 's':
            /* Make up the pointer to the string */
            psi = (char far *) PVOIDFROMMP(mp1);     

            /* Store string */
            pcurrent->ptl.x = (LONG) malloc(strlen(psi)+1);  
            strcpy((char*)pcurrent->ptl.x, psi);
            break;
        case 'y':
            /* make up the pointer to the pointl structure */
            /* This assumes that the function building the POINTL array */
            /* will not destroy it.  Only pc_delete should free the     */
            /* memory allocated for the array.                          */
            pcurrent->ptl.x = (ULONG) PVOIDFROMMP(mp1);
            /* Save the number of POINTLs */
            pcurrent->ptl.y = (ULONG) SHORT2FROMMP(mp2);
            break;

        case 'm':
        case 'd':
        case 'p':
            pcurrent->ptl.x = (LONG) SHORT1FROMMP(mp1);
            pcurrent->ptl.y = (LONG) SHORT2FROMMP(mp1);
            break;
        case 'l':
            pcurrent->ptl.x = LONGFROMMP(mp1);
            break;
        default:
            /* If it's not one of those we'd better get rid of it! */
            /* but only if it's a new command */
            if(new_command)
                {
                free(pcurrent);
                pcurrent=pcold;
                }
            break;
        }
     
    return(pcurrent);
    }

/* This GIN mode handling routines come next */

ULONG gin_wait_sem=0;
ULONG gin_received_sem=0;
CHAR  gin_mode=0;

LONG mouse_adj_xpos;
LONG mouse_adj_ypos;
CHAR gin_c;
/******************************************************************************
Function:       SetPMGinMode()

Description:    Puts the GPI window into graphic inupt mode a la Tektronix

Syntax:         void SetPMGinMode(hwnd,mode)
                    HWND hwnd;    Handle to window
                    MPARAM mode;  Open or close GIN mode

Returns:        nothing

Mods:           26-Jan-90 C.P.Armstrong created

******************************************************************************/
void SetPMGinMode(hwnd,mode,hptr)
    HWND hwnd;    /* Handle to window */
    MPARAM mode;  /* Open or close GIN mode */
    HPOINTER hptr;
    {
    extern CHAR gin_mode;
    
    if(mode==(MPARAM)1)     /* Enter GIN mode */
        {
        /* Change pointer to a cross */
        WinSetPointer(HWND_DESKTOP, hptr);
        gin_mode=1;
        }
    else
        {
        /* Change pointer back to normal */
        WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP,SPTR_ARROW,
                FALSE));
        gin_mode=0;
        }
    }
                    
/******************************************************************************
Function:       DoPMTekGin()

Description:    Sends the coordinates of the mouse when a button was pressed
                to the waiting thread - if GIN mode has been selected.
                The coords returned are scaled up to those used by
                the plot routines in the thread.
                
                The mouse coords are scaled and then converted to Tektronix
                values. A string consisting of the coords, the character pressed
                and the GIN terminator are then inserted into the inter thread 
                keyboard buffer.  

Syntax:         int DoPMTekGin(hwnd, pmsg)
                    HWND hwnd;   Window handle
                    USHORT * pmsg;   Message sent to window (contains mouse coords).

Returns:        1 if we were in GIN mode
                0 if not in GIN mode.

Mods:           01-Feb-90 C.P.Armstrong Based on DoPMGin, no semaphores, string
                                        inserted into keyboard buffer etc.
******************************************************************************/
int DoPMTekGin(hwnd, pmsg)
    HWND hwnd;      /* Window handle */
    USHORT* pmsg;   /* Message sent to window (contains mouse coords). */
    {
    extern CHAR  gin_mode;
    extern LONG mouse_adj_xpos;
    extern LONG mouse_adj_ypos; 
    extern CHAR gin_c;
    extern char gin_terminator;
    extern char mouse_but1;
    extern char mouse_but2;
    extern char mouse_but3;

    
    SWP swp;
    POINTL pointerpos;
    char ginbuf[7];

    if(gin_mode!=1)
        return(0);
    
    if(*pmsg==WM_CHAR)
        {
        WinQueryPointerPos(HWND_DESKTOP,&pointerpos);
        WinMapWindowPoints(HWND_DESKTOP,hwnd,&pointerpos,1);
        mouse_adj_xpos = pointerpos.x;
        mouse_adj_ypos = pointerpos.y;
        gin_c = (CHAR) CHARMSG(pmsg)->chr;
        }
     else
        {
        /* Get the window position of the mouse */
        mouse_adj_xpos = MOUSEMSG(pmsg)->x;
        mouse_adj_ypos = MOUSEMSG(pmsg)->y;
        /* On our Vax sending a non character, VK_BUTTON1, screws things up */
        switch(*pmsg)
            {
            case WM_BUTTON1DOWN: gin_c = mouse_but1; break;
            case WM_BUTTON2DOWN: gin_c = mouse_but2; break;
            case WM_BUTTON3DOWN: gin_c = mouse_but3; break;
            default:        gin_c = ' ';
            }
        }

    /* Get the window size */
    WinQueryWindowPos(hwnd,&swp);    

    /* Scale up the mouse position */
    mouse_adj_xpos = mouse_adj_xpos * MAXXRES / swp.cx ;
    mouse_adj_ypos = mouse_adj_ypos * MAXYRES / swp.cy ;

    /* Build the Tek cursor address string */
    Tek_ginencode((int)mouse_adj_xpos,(int)mouse_adj_ypos,gin_c,
        gin_terminator,ginbuf);
    
    /* Insert it into the buffer */
    buff_strins(ginbuf);

    return(1);
    }
    
    
/******************************************************************************
Function:       TestPMGinMode()

Description:    Test to see if GINmode is selected or not.

Syntax:         BOOL TestPMGinMode()

Returns:        1 if in GIN mode
                0 if in "normal" mode

Mods:           27-Jan-90 C.P.Armstrong created

******************************************************************************/
BOOL TestPMGinMode()
    {
    extern CHAR  gin_mode;
    return( gin_mode );
    }
    
/******************************************************************************
Function:       SetGinMode()

Description:    This is used by the none PM thread to enable/disable the 
                graphic input mode.

Syntax:         void SetGinMode(hwnd,mode)
                    HWND hwnd;  WIndow handling the GIN
                    int  mode;  Enable=1/disable=0 GIN mode

Returns:        nothing

Mods:           29-Jan-90 C.P.Armstrong created

******************************************************************************/
void SetGinMode(hwnd,mode)
    HWND hwnd;  /* Window handling GIN mode */
    int  mode;
    {
    WinPostMsg(hwnd,WM_GIN,(MPARAM)mode,0);
    return;
    }

/******************************************************************************
Function:       GetGinCoords()

Description:    Gets GIN information for the non PM thread.  GIN mode input
                is terminated if the user presses a mouse button or the 
                keyboard.

Syntax:         void GetGinCoords(xpos,ypos,ch)
                    int* xpos;      Adjusted X pos of cursor
                    int* ypos;      Adjusted Y position
                    char* ch;       Character pressed to terminate input

Returns:        nothing

Mods:           29-Jan-90 C.P.Armstrong created

******************************************************************************/
void cdecl GetGinCoords(xpos,ypos,ch)
    int* xpos;
    int* ypos;
    char* ch;
    {
    extern ULONG gin_received_sem;
    extern ULONG gin_wait_sem;
    extern LONG  mouse_adj_xpos;
    extern LONG  mouse_adj_ypos;
    extern CHAR  gin_c;

    /* Clear the received flag - this tells the PM thread we are ready for */
    /* another value.                                                      */
    DosSemClear(&gin_received_sem);
    
    /* Set the waiting flag and wait forever for it to be cleared */
    DosSemSetWait(&gin_wait_sem,SEM_INDEFINITE_WAIT);
    
    /* Copy the values */
    *xpos = (int) mouse_adj_xpos;
    *ypos = (int) mouse_adj_ypos;
    *ch   = gin_c;

    return;
    }

/******************************************************************************
Function:       AddToSwitch()

Description:    Adds the current process to the switch list with the supplied
                title.

Syntax:         AddToSwitch(hwndFrame, hwndIcon, title)
                    HWND hwndFrame;  Windows frame handle
                    HWND hwndIcon;   Windows icon handle
                    char * title;    Title to go in switch list

Returns:        The switch handle or NULL if an error occurs

Mods:           19-Jan-90 C.P.Armstrong created

******************************************************************************/
HSWITCH AddToSwitch(hwndFrame, hwndIcon, title)
    HWND hwndFrame;  /* Windows frame handle */
    HWND hwndIcon;   /* Windows icon handle */
    CHAR * title;    /* Title to go in switch list */
    {
    PID pid;
    SWCNTRL swctl;
    HSWITCH hswitch;

    if(!WinQueryWindowProcess(hwndFrame,&pid,NULL))
        return(NULL);
    else
        {
        swctl.hwnd=hwndFrame;
        swctl.hwndIcon = hwndIcon;
        swctl.hprog = NULL;
        swctl.idProcess = (USHORT) pid;
        swctl.idSession = (USHORT) 0;
        swctl.uchVisibility = SWL_VISIBLE;
        swctl.fbJump = SWL_JUMPABLE;
        strcpy(swctl.szSwtitle,title);
        swctl.fReserved = 0;             
        }
 
    return(WinAddSwitchEntry(&swctl));
    }




/******************************************************************************
Function:       do_aviotopm()

Description:    Posts an AVIO message to the PM queue thread for processing.
                It seems that AVIO functions can not be used by non message
                threads.

Syntax:         void do_aviotopm(struct avio_cellstr *ac)

Returns:        nothing

Mods:           00-Dec-89 C.P.Armstrong created

******************************************************************************/
void do_aviotopm(struct avio_cellstr *ac)
    {
    extern HWND hwndTFrame;
    extern ULONG vio_sem;

    /* Kermit gets a single character and then sends it to the PM, not */
    /* very efficient. It uses WRCCHSTAT, one char at a time. This     */
    /* attempts to trap those single writes and store them until a line*/
    /* is available.                                                   */
    if(ac->fun == WRCCHSTATD)
        WinPostMsg(hwndTFrame, WM_KERAVIO, MPFROMP(ac),0);
    else if(DosSemRequest(&vio_sem,1000L)==0)
        {
        WinPostMsg(hwndTFrame, WM_KERAVIO, MPFROMP(ac),0);
        /* Don't wait for the special WrtCharStrAtt */
            DosSemWait(&vio_sem,5000L);
        }
    return;
    }

/******************************************************************************
Function:       process_avio()

Description:    Performs the VIO action requested by the non PM thread.

Syntax:         void process_avio(pac)
                    struct avio_cellstr * pac;
                    HVPS hvps;

Returns:        nothing

Mods:           31-Jan-90 C.P.Armstrong created

******************************************************************************/
void process_avio(pac,hvps)
    struct avio_cellstr * pac;
    HVPS hvps;
    {
    switch(pac->fun)
        {
        case RDCELLSTR:
            VioReadCellStr(pac->string, pac->plen, pac->row, pac->col, hvps);
            break;
        case WRCELLSTR:
            VioWrtCellStr(pac->string, pac->len, pac->row, pac->col, hvps);
            break;
        case WRNCELL:
            VioWrtNCell(pac->pcell, pac->len, pac->row, pac->col, hvps);
            break;
        case SCROLLRT:
            VioScrollRt(pac->row, pac->col, pac->hite, pac->wid, 
                pac->len, pac->pcell, hvps);
            break;
        case SCROLLLF:
            VioScrollLf(pac->row, pac->col, pac->hite, pac->wid, 
                pac->len, pac->pcell, hvps);
            break;       
        case SCROLLUP:
            VioScrollUp(pac->row, pac->col, pac->hite, pac->wid, 
                pac->len, pac->pcell, hvps);
            break; 
        case SCROLLDN:
            VioScrollDn(pac->row, pac->col, pac->hite, pac->wid, 
                pac->len, pac->pcell, hvps);
            break;     
        case WRCCHSTATD:
            if((pac!=NULL) && (pac->string!=NULL))
                {
                VioWrtCharStrAtt(pac->string,pac->len,pac->row,pac->col,
                    pac->pcell,hvps);
                free(pac->string);
                free(pac);
                }
            break;
        case WRCCHSTAT:
            VioWrtCharStrAtt(pac->string,pac->len,pac->row,pac->col,
                pac->pcell,hvps);
            break;
        case SETCURPOS:
            VioSetCurPos(pac->row,pac->col,hvps);
            break;
        case GETCURPOS:
            VioGetCurPos(&(pac->row), &(pac->col),hvps);
            break;
        case GETCURTYP:
            VioGetCurType( (PVIOCURSORINFO) pac->string,hvps);
            break;
        case SETCURTYP:
            VioSetCurType( (PVIOCURSORINFO) pac->string,hvps);
            break;
        case PRINTF:
            vWrtchar(pac->string,pac->len,hvps);
            break;
        }
    return;
    }

void AVIOReadCellStr(PCH string,PUSHORT len, USHORT row, USHORT col)
    {
    struct avio_cellstr ac;
    
    ac.fun = RDCELLSTR;
    ac.string = string;
    ac.plen    = len;
    ac.row    = row;
    ac.col    = col;
    do_aviotopm(&ac);
    return;

    }
    
void AVIOWrtCellStr(PCH string, USHORT len, USHORT row, USHORT col)
    {
    struct avio_cellstr ac;
    
    ac.fun = WRCELLSTR;
    ac.string = string;
    ac.len    = len;
    ac.row    = row;
    ac.col    = col;

    do_aviotopm(&ac);
    return;
    }

void AVIOWrtNCell(PBYTE cell,  USHORT len, USHORT row, USHORT col)
    {
    struct avio_cellstr ac;
    
    ac.fun    = WRNCELL;
    ac.pcell   = cell;
    ac.len    = len;
    ac.row    = row;
    ac.col    = col;
    do_aviotopm(&ac);
    return;
    }

void AVIOScrollRt(USHORT trow, USHORT lcol, USHORT brow, USHORT rcol, 
                  USHORT len,  PBYTE cell)
    {
    struct avio_cellstr ac;
    
    ac.fun    = SCROLLRT;
    ac.pcell   =  cell;
    ac.len    = len;
    ac.row    = trow;
    ac.col    = lcol;
    ac.hite   = brow;
    ac.wid    = rcol;

    do_aviotopm(&ac);
    return;
    }

void AVIOScrollLf(USHORT trow, USHORT lcol, USHORT brow, USHORT rcol, 
                 USHORT len,   PBYTE cell)
    {
    struct avio_cellstr ac;
    
    ac.fun = SCROLLLF;
    ac.pcell   =  cell;
    ac.len    = len;
    ac.row    = trow;
    ac.col    = lcol;
    ac.hite   = brow;
    ac.wid    = rcol;

    do_aviotopm(&ac);
    return;
    }


void AVIOScrollUp(USHORT trow, USHORT lcol, USHORT brow, USHORT rcol, 
                    USHORT len, PBYTE cell)
    {
    struct avio_cellstr ac;
    
    ac.fun = SCROLLUP;
    ac.pcell   =  cell;
    ac.len    = len;
    ac.row    = trow;
    ac.col    = lcol;
    ac.hite   = brow;
    ac.wid    = rcol;

    do_aviotopm(&ac);
    return;
    }

void AVIOScrollDn(USHORT trow, USHORT lcol, USHORT brow, USHORT rcol, 
                    USHORT len, PBYTE cell)
    {
    struct avio_cellstr ac;
    
    ac.fun = SCROLLDN;
    ac.pcell   =  cell;
    ac.len    = len;
    ac.row    = trow;
    ac.col    = lcol;
    ac.hite   = brow;
    ac.wid    = rcol;

    do_aviotopm(&ac);
    return;
    }

void AVIOdWrtCharStrAtt(string, len, row, col, pbAttr)
                       PCH string;      /* pointer to string to write */
                       USHORT len;    /* length of string           */
                       USHORT row;       /* starting position (row)    */
                       USHORT col;    /* starting position (column) */
                       PBYTE pbAttr;       /* pointer to attribute       */
    {
    struct avio_cellstr * pac;

    if( (pac = (struct avio_cellstr *)malloc(sizeof(struct avio_cellstr)))
            ==NULL)
        return;

    pac->fun = WRCCHSTATD;
    pac->string = string;
    pac->len    = len;
    pac->row    = row;
    pac->col    = col;
    pac->pcell   = pbAttr;

    do_aviotopm(pac);
    return;
    }



void AVIOWrtCharStrAtt(string, len, row, col, pbAttr)
                       PCH string;      /* pointer to string to write */
                       USHORT len;    /* length of string           */
                       USHORT row;       /* starting position (row)    */
                       USHORT col;    /* starting position (column) */
                       PBYTE pbAttr;       /* pointer to attribute       */
    {
    struct avio_cellstr ac;
    
    ac.fun = WRCCHSTAT;
    ac.string = string;
    ac.len    = len;
    ac.row    = row;
    ac.col    = col;
    ac.pcell   = pbAttr;

    do_aviotopm(&ac);
    return;
    }


void AVIOSetCurPos(usRow, usColumn)
    USHORT usRow;       /* row position    */
    USHORT usColumn;    /* column position */
    {
    struct avio_cellstr ac;
    
    ac.fun = SETCURPOS;
    ac.row    = usRow;
    ac.col    = usColumn;
    do_aviotopm(&ac);
    return;
    }

void AVIOGetCurPos(pusRow, pusColumn)
    PUSHORT pusRow;       /* row position    */
    PUSHORT pusColumn;    /* column position */
    {
    struct avio_cellstr ac;
    
    ac.fun = GETCURPOS;
    do_aviotopm(&ac);
    *pusRow = ac.row;
    *pusColumn = ac.col;
    return;
    }

void AVIOGetCurType(PVIOCURSORINFO string)
    {
    struct avio_cellstr ac;
    
    ac.fun = GETCURTYP;
    ac.string = (PCH) string;
    do_aviotopm(&ac); 
    return;
    }

void AVIOSetCurType(PVIOCURSORINFO string)
    {
    struct avio_cellstr ac;
    
    ac.fun = SETCURTYP;
    ac.string = (PCH) string;
    do_aviotopm(&ac); 
    return;
    }

void AVIOwrttyc(s,x)
    char* s;
    int  x;
    {
    struct avio_cellstr ac;
    
    ac.fun = PRINTF;
    ac.string = (PCH) s;
    ac.len    = x;
    do_aviotopm(&ac); 
    return;
    }
    

/******************************************************************************
Function:       PM_setViotitle()

Description:    Changes the title text used by the Vio screen.
                Unfortunately this is not possible from a non PM message queue
                containing thread.  Thus a message has to be posted to the 
                message queue thread which is then processed by a separate
                function.

Syntax:         void PM_setViotitle(title)
                    char * title; The string to use - it sould be constant

Returns:        nothing

Mods:           03-Feb-90 C.P.Armstrong created
                25-Mar-90 C.P.Armstrong Need to use this from within the PM
                                        thread (when user changes emu mode 
                                        during a conect).  Waiting on a sem is
                                        no good because the PM thread is the
                                        one doing the waiting.In the absence of
                                        a better way the buffer must be a 
                                        constant, as must TitleText structure.

******************************************************************************/
void PM_setViotitle(title)
    char * title; /* The string to use - it sould be constant */
    {
    extern HWND hwndTFrame;
    ULONG sm;
    static struct TitleText_command ttc;
                                
    ttc.action = SET;
    ttc.hwnd   = hwndTFrame;
    ttc.buffer = title;
    ttc.sem    = &sm;
//    DosSemSet(&sm);
    WinPostMsg(hwndTFrame,WM_TITLETEXT, MPFROMP(&ttc),0);
    
//    DosSemWait(&sm,SEM_INDEFINITE_WAIT);
    
    return;
    }
    
/******************************************************************************
Function:       PM_getViotitle()

Description:    Returns the current Vio title text.  If the function is
                called with a NULL buffer for the text then only the length
                of the title text is returned. It is assumed by the routine that
                the buffer is long enough to receive the full title text, 
                including the NUL terminator.
                The function is designed to be called once with a NUL buffer, 
                reserve the memory, then call again with the full length buffer.

Syntax:         int PM_getViotitle(title)
                    char * title;  Buffer to receive the title

Returns:        The length of the title

Mods:           03-Feb-90 C.P.Armstrong created

******************************************************************************/
int PM_getViotitle(title)
    char * title;  /* Buffer to receive the title */
    {
    extern HWND hwndTFrame;
    ULONG sm;
    struct TitleText_command ttc;
                                
    ttc.action = LENGTH;
    ttc.hwnd   = hwndTFrame;
    ttc.buffer = NULL;
    ttc.len    = 0;
    ttc.sem    = &sm;
    DosSemSet(&sm);
    WinPostMsg(hwndTFrame,WM_TITLETEXT, MPFROMP(&ttc),0);
    DosSemWait(&sm,SEM_INDEFINITE_WAIT);

    if(title != NULL)
        {
        ttc.action = GET;
        ttc.hwnd   = hwndTFrame;
        ttc.buffer = title;
        ttc.sem    = &sm;
        DosSemSet(&sm);
        WinPostMsg(hwndTFrame,WM_TITLETEXT, MPFROMP(&ttc),0);
        DosSemWait(&sm,SEM_INDEFINITE_WAIT);
        }

    return(ttc.len);
    }

/******************************************************************************
Function:       TitleText()

Description:    Processes a title text command.  This function should only be
                called from a thread containing a PM message queue.  Use
                PM_getViotitle and PM_setViotitle to send commands to the
                PM message queue.

Syntax:         void TitleText(mp1,mp2)
                    MPARAM mp1,mp2;  Parameters passed to the client window
                                     procudure.

Returns:        nothing

Mods:           03-Feb-90 C.P.Armstrong created

******************************************************************************/
void TitleText(mp1,mp2)
    MPARAM mp1,mp2;
    {
    struct TitleText_command * pttc;
    
    pttc = (struct TitleText_command *) PVOIDFROMMP(mp1);
    
    switch(pttc->action)
        {
        case SET:
            WinSetWindowText(pttc->hwnd,pttc->buffer);
            break;
        case GET:
            pttc->len = WinQueryWindowText(pttc->hwnd,pttc->len,pttc->buffer);
            break;
        case LENGTH:
            pttc->len = WinQueryWindowTextLength(pttc->hwnd)+1;
            break;
        default:
            break;
        }

    /* clear the semaphore so the waiting thread knows we've finished */
    DosSemClear(pttc->sem);
    
    return;
    }
    
    
    
/******************************************************************************
Function:       DecodePMChar()

Description:    Decodes a PM WM_CHAR message into something more usable.  
                Distinguishes between Num Keypad presses with NumLock on and
                off and returns a pseudo scan code value defined in ckopm.h.
                The decoded key press is inserted into the inter thread buffer
                with buff_insert().

Syntax:         int DecodePMChar(mp1,mp2)
                    MPARAM mp1,mp2

Returns:        0

Mods:           29-Jan-90 C.P.Armstrong created

******************************************************************************/
int DecodePMChar(mp1,mp2)
//    PUSHORT pmsg;
    MPARAM mp1,mp2;
    {
    extern char debugmsg;
    SHORT vkey,chr,fs,dum;
    unsigned char scancode; 


    fs  = SHORT1FROMMP(mp1);
    vkey=SHORT2FROMMP(mp2);
    chr =SHORT1FROMMP(mp2);
    scancode = CHAR4FROMMP(mp1);

    /* Weed out the keypad keys with NumLock off */
    if( (scancode >= KP7 && scancode<=KPDOT)
         || (scancode==KPSLASH || scancode==KPSTAR))
         {
         /* Most annoyingly the grey keypad keys are tagged as being CHAR keys*/
         /* regardless off whether the NumLock key is on or off.  Fing typical*/

         if((fs & KC_CHAR) && (numlock_status()&1)) /* Keypad keys are tagged */
                                                    /* as vkeys even */
            buff_insert(chr);           /* with the NumLock on.  So force them*/
         else                           /* to be numeric here. */
            buff_insert(scancode<<8);
         return(0);
         }

    if(fs&KC_VIRTUALKEY)
        {
        /* Seems to be no other way to do it!! */
        switch(vkey)
            {
            // THE shift keys are put in here
            case VK_SHIFT:
            case VK_CTRL:
            case VK_ALT:
            case VK_CAPSLOCK:
                numlock_toggle();
            case VK_SCRLLOCK:
            case VK_ALTGRAF:
                return(0);
            case VK_ESC:       buff_insert(27); return(0);
            case VK_SPACE:     buff_insert(32); return(0);
            case VK_TAB:       buff_insert(9);  return(0);
            case VK_NEWLINE:   buff_insert(13); return(0);
            case VK_PRINTSCRN:
                /* Check if Shift-PrintScreen */
                vkey = WinGetKeyState(HWND_DESKTOP,VK_SHIFT);
                if(vkey & 0x8000)
                    vkey = (VK_PRINTSCRN | 0x20);
                else
                    vkey = VK_PRINTSCRN;

                buff_insert(vkey<<8);
                break;
            case VK_NUMLOCK:
                if(fs & KC_CTRL)
                    {
                    dbprintf("Got ctrl-NumLock");
                    return(0);
                    }
                else                 /* This switches the NumLock state so it */
                    numlock_toggle();/* stays the same. Well it will when I   */
                return(0);           /* find out how to get around the PM!    */
            default: 
                buff_insert(vkey<<8);
            }
        }
    else if(fs & KC_CHAR)        /* Normal key */
        {
        buff_insert((unsigned int) chr);
        }
    else if((fs&KC_CTRL) && (chr>0))
        {
        buff_insert((unsigned int) (chr & 0x1F));
        }
    else if((fs&KC_ALT) && (chr>0))
        {
        dbprintf("We got an Alt key\nInserting %x",((scancode|0x80)<<8));
        buff_insert((unsigned int) ((scancode|0x80)<<8));
        }
    else
        {
        dbprintf("Unrecognized character");
        }

    return(0);
    }
    
 
/******************************************************************************
Function:       RgbToVioColor()

Description:    Converts a Presentation Manager colour code into a Vio
                colour code.

Syntax:         BYTE RgbToVioColor(clrRgb)
                    COLOR clrRgb;       RGB colour code to be converted

Returns:        The Vio colour code in the low nibble.

Mods:           06-Oct-89 C.P.Armstrong Copied from p328 of Prog OS/2 P.M.

******************************************************************************/
BYTE RgbToVioColor(COLOR clrRgb)
    {
    BYTE bIrgb;
    RGB  rgb;
    
    bIrgb = 0;
//    rgb = MAKETYPE(clrRgb,RGB);       /* This causes segment violations */
//    rgb = *((RGB*) &clrRgb)  ;        /* for some reason.               */
    rgb.bBlue  = (BYTE) (clrRgb & 0xFFl);
    rgb.bGreen = (BYTE) ((clrRgb & 0xFF00l) / 0x100);
    rgb.bRed   = (BYTE) ((clrRgb & 0xFF0000l) / 0x10000);


    if(rgb.bBlue  >= 0x80) bIrgb |= '\x01';
    if(rgb.bGreen >= 0x80) bIrgb |= '\x02';
    if(rgb.bRed   >= 0x80) bIrgb |= '\x04';
    
    if( rgb.bBlue  >= 0XC0 ||
        rgb.bGreen>= 0XC0 ||
        rgb.bRed  >= 0XC0)
        bIrgb |=8;
        
    if( bIrgb==0 && rgb.bBlue  >= 0X40 &&
        rgb.bGreen>= 0X40 &&
        rgb.bRed  >= 0X40)
        bIrgb = 8;
        
    return(bIrgb);
    }
 
/******************************************************************************
Function:       VioToRgbColor()

Description:    Converts a vio screen attribute to the PM rgb equivalent,
                or thereabouts.  Note that only the lo nibble is used,
                background colours must be moved into the lo nibble before
                conversion.

Syntax:         LONG VioToRgbColor(attr)
                    BYTE attr;  The lo nible of a Vio attribute

Returns:        The colour index value as used by WinFillRect.  These might
                not give the correct colours if the palette has been messed
                around with in which case the Gpi nearest colour function
                will probably need to be used.  The RGB value is not returned
                since it is next to useless as far as I can make out.

Mods:           27-Nov-89 C.P.Armstrong created

******************************************************************************/
LONG rgbs[16]=
    {
    CLR_BLACK             ,
    CLR_DARKBLUE          ,
    CLR_DARKGREEN         ,
    CLR_DARKCYAN          ,
    CLR_DARKRED           ,
    CLR_DARKPINK          ,
    CLR_BROWN             ,
    CLR_PALEGRAY          ,
    CLR_DARKGRAY          ,
    CLR_BLUE              ,
    CLR_GREEN             ,
    CLR_CYAN              ,
    CLR_RED               ,
    CLR_PINK              ,
    CLR_YELLOW            ,
    CLR_WHITE
    };

LONG VioToRgbColor(attr)
    BYTE attr;  /* The lo nible of a Vio attribute */
    {
    extern LONG rgbs[];

    return(rgbs[attr&0xF]);
    }

/******************************************************************************
Function:       buff_insert()

Description:    Inserts keystrokes from the keyboard into a
                circular buffer.  When the buffer is filled input is
                "wrapped" back to the begining.  
                This function is meant to be used in conjunction with
                buff_empty()..
                The offset of the position in the buffer for the next
                character is maintained by this function.

Syntax:         void buff_insert(c)
                    unsigned int c;

Returns:        nothing

Mods:           24-Nov-89 C.P.Armstrong created

******************************************************************************/
void buff_insert(int c)
    {
    extern ULONG kbd_sem;
    extern int keybuffer[];     /* The buffer for the key presses */
    extern int keybuffer_size;  /* The buffer size */
    extern int next_put_pos;        /* Offset of next character */
    extern int next_get_pos;
    extern ULONG kbdovr_sem;


    if(WinMsgSemWait(&kbdovr_sem,SEM_IMMEDIATE_RETURN)!=0)
        {
        DosSemClear(&kbd_sem);  /* Make sure this is clear */
        if(WinMsgSemWait(&kbdovr_sem,10000L)!=0)
            {
            pm_err("Keyboard buffer not being emptied!");
            return;
            }
        }


    /* Stop other threads from taking things out of the buffer */
    DosEnterCritSec();
    keybuffer[next_put_pos] = c;
    next_put_pos++;

    /* See if we need to wrap */
    if(next_put_pos>=keybuffer_size)
        next_put_pos=0;     
        
    /* If in and out indices are now equal then we are about to overflow */
    if(next_put_pos==next_get_pos)   /* This has to be cleared by buff_empty */
        DosSemSet(&kbdovr_sem);      /* before anything can be inserted */

    DosExitCritSec();

    if(DosSemClear(&kbd_sem)!=0)
        pm_err("buff_insert - Failed to clear kbd_sem");

    return;
    }

/******************************************************************************
Function:       buff_empty()

Description:    Takes key presses from the buffer filled by buff_fill.
                The ASCII code is returned.  If a special key is pressed the
                lo byte of the return value is zero and the high byte is the
                scan code.

Syntax:         int buff_empty()

Returns:        The character if one is present.
                -1 if no character in buffer

Mods:           22-Nov-89 C.P.Armstrong
                24-Apr-90 C.P.Armstrong Removes chars from paste buffer in
                                        preference to the keyboard buffer.

******************************************************************************/
int buff_empty()
    {
    extern int keybuffer[];     /* The buffer for the key presses */
    extern int keybuffer_size;  /* The buffer size */
    extern int next_put_pos;    /* Offset of next character to be inserted  */
    extern int next_get_pos;    /* Offset of next character to be extracted */
    extern int buff_overflow;
    extern ULONG kbdovr_sem;
    extern int pasting;
    int c;

    if(pasting>0 && ((c=get_paste_char())>0))
        return(c);

    /* Freeze the other threads and check to see if the buffer is empty */
    DosEnterCritSec();
    if( (next_get_pos == next_put_pos) 
       && (DosSemWait(&kbdovr_sem,SEM_IMMEDIATE_RETURN)==0) )
        {
        DosSemClear(&kbdovr_sem);  /* We set it - so clear it! */
        DosExitCritSec();
        return(-1);
        }



    c = keybuffer[next_get_pos];
    DosExitCritSec();
    next_get_pos++;                       

    /* Wrap buffer pointer if necessary */
    if(next_get_pos>= keybuffer_size)
        next_get_pos=0;

    /* Clear the overflow flag */
    DosSemClear(&kbdovr_sem);

    if(c==0 && pasting>0)
        return(get_paste_char());
    else
        return(c);
    }
 

/******************************************************************************
Function:       buff_tgetch()

Description:    Returns a character from the PM keyboard buffer.  If no key
                is present on entereing then the function waits for one for up
                to the specified time.  Note that the character is returned in 
                the lo byte and the scan code in the high byte of the word.
                
                The routine uses a semaphore to determine the wait time.  So
                supplying a time of SEM_INDEFINITEWAIT or SEM_IMMEDIATE_RETURN
                will have the same effect as when these values are used with
                semaphores.  The values are defined in OS.H

Syntax:         int buff_tgetch(time)
                    LONG time;  Time to wait for a character in milliseconds

Returns:        The character

Mods:           04-Dec-89 C.P.Armstrong created
                11-Mar-90 C.P.Armstrong Cursor visible check request added
                24-Apr-90 C.P.Armstrong A -1 from buff_empty() only causes a 
                                        return if time is not 
                                        SEM_INDEFINITE_WAIT.
******************************************************************************/
int buff_tgetch(time)
    LONG time;
    {
    extern ULONG kbd_sem;
    int c;
    
    do
        {
        if(buff_test() == 0)
            {
            /* Tell PM to reposition cursor */
            CurScrChk();
            /* Max 10s wait to set the semaphore */   
            DosSemRequest(&kbd_sem,100L); 
            /* Now wait for buff_insert() to clear it */
            DosSemWait(&kbd_sem,time);   
            }   /* buff_empty() returns -1 when paste buffer empties */
        }while( ((c=buff_empty())==-1) && (time==SEM_INDEFINITE_WAIT));

    return(c);
    }
    

int buff_getch()
    {
    return(buff_tgetch(SEM_INDEFINITE_WAIT));
    }



/******************************************************************************
Function:       buff_strins()

Description:    Inserts a string of characters into the keyboard buffer

Syntax:         void buff_strins(zstring)
                    char * zstring;  the Nul terminated string

Returns:        nothing

Mods:           28-Nov-89 C.P.Armstrong created

******************************************************************************/
void buff_strins(zstring)
    char * zstring;  /* the Nul terminated string */
    {
    int i,l;
    l=strlen(zstring);
    
    for(i=0;i<l;i++)
        {
        buff_insert((int)zstring[i]);
        DosSleep(200L);
        }
    
    return;
    }


/******************************************************************************
Function:       buff_test()

Description:    Tests for the presence of a character without removing any
                characters if present.

Syntax:         int buff_test()

Returns:        0 if no chars
                >0 if chars present

Mods:           27-Nov-89 C.P.Armstrong created

******************************************************************************/
int buff_test()
    {
    extern int pasting;
    extern int next_put_pos;    /* Offset of next character to be inserted  */
    extern int next_get_pos;    /* Offset of next character to be extracted */
    extern ULONG kbdovr_sem;
    int c;
    
    if(pasting>0)
        return(1);


    /* Freeze the other threads and check to see if the buffer is empty */
    DosEnterCritSec();
    if((next_get_pos == next_put_pos) && 
      (DosSemWait(&kbdovr_sem,SEM_IMMEDIATE_RETURN)==0))
        c=0;
    else
        c=1;
        
    DosExitCritSec();
    return(c);
    }


/******************************************************************************
Function:       vWrtchar()

Description:    Writes a single character out to a Vio screen and advances the
                cursor to the next position.  The cursor is wrapped at the
                end of the line.  If the end of the screen is reached then
                the whole screen is scrolled by 1 line.  This replaces the
                VioWrtTTY function and is compatible with multi-threaded
                PM programs.  VioWrTTY fails in multi-threaded programs when
                used outside the main window thread.  Note that the colour
                is not written, only the character.
                
                The address of the video buffer must be declared externally.
                Also the screen size must be declared externally.
                
                Note this behaviour of this function wrt LF may seem strange.
                A CRLF sequence is produced when the function receives an LF.
                This is in keeping with the behaviour of printf().  A CR
                produces just a CR.  The ENTER key usually produces a CR so
                any key interpretation function should convert this to LF.
                
                Function will move backwards one position on receipt of a
                '\b'. To erase a character a "\b \b" sequence must be sent.
                
Syntax:         void vWrtchar(c,l,h)
                    char * c;  Pointer to the string
                    int    l;  length of the string
                    HVPS   h;  handle to the presentation space (PS)

Returns:        nothing

Mods:           27-Nov-89 C.P.Armstrong created

******************************************************************************/
void vWrtchar(c,l,h)
    char * c;  /* Pointer to the string */
    int    l;  /* length of the string */
    HVPS   h;  /* handle to the presentation space (PS) */
    {
    extern char far * ulVideoBuffer;
    extern char defaultattribute;
    extern ULONG vio_sem;
    extern HVPS hvps;
    extern int scrwidth;
    extern int scrheight;
    extern char defaultattribute;

    USHORT row,col;     /* Present cursor position */
    USHORT orow;
    int i;              /* Character to print */
    int off;
    BYTE cell[2];

    VioGetCurPos(&row,&col,hvps);
    orow=row;

    for(i=0;i<l;i++)
        {
        switch(c[i])
            {
            case '\a':
                DosBeep(3000,100);
                return;
            case '\t':
                do
                    {
                    col++;
                    if(col>=scrwidth)  /* Row scrolling is dealt with below */
                        {
                        col=0;
                        row++;
                        }
                    }
                while(col%5);
                col--;  /* Reset to correct position below */
                break;
            case '\b':
                col-=2;
                if(col<0)         /* Did we delete past bol */
                    {
                    row--;
                    col=scrwidth-2;
                    }
                if(row<0)        /* Are we trying to delete past top of */
                    {            /* window?  If so stomp on it.         */    
                    row=0;
                    col=-1;
                    }
                break;
            case '\n':
                row++;
            case '\r':
                col=-1; /* It'll get set to 0 at the end */
                break;
            
            default:                
                off = ((row*scrwidth)+col)*2;
                ulVideoBuffer[ off ] = c[i];
                ulVideoBuffer[off+1] = defaultattribute;
            }
        
        col++;
        if(col>=scrwidth)
            {
            col=0;
            row++;
            }
        
        if(row>=scrheight)
            {
            row=scrheight-1;
            cell[0]=' ';
            cell[1]=defaultattribute;
            VioScrollUp(0,0,scrheight,scrwidth,1,cell,hvps);
            }
            
        }        

        /* Move the cursor first - this stops the new chars flashing */
        VioSetCurPos(row,col,hvps);
        
        /* Unfortunately the PM freezes if deselection or resizing occurs */
        /* during output.  This does not seem to happen if now output is  */
        /* going on so I assume it must be something to do with this.     */
        /* I've had a lot of problems with the buffer showing.            */
        /* VioShowBuf ususally freezes completely. Maybe VioShowPS can't  */
        /* handle working in the background. Either that or there needs to*/
        /* be some way of figuring out when the move is completly done so */
        /* things can carry on.                                           */

        VioShowPS(row-orow+1,scrwidth,(orow*scrwidth),hvps);

        DosSemClear(&vio_sem);
        return;
        }

/******************************************************************************
Function:       flash_cursor()

Description:    Toggles the cursor on and off - if cursor display is enabled
                by show_cursor().

Syntax:         void flash_cursor(hvps)
                    HVPS hvps;  Vio PS handle

Returns:        

Mods:           

******************************************************************************/
void flash_cursor(hvps)
    HVPS hvps;  /* Vio PS handle */
    {
    extern char cursor_displayed;
    VIOCURSORINFO vc;
    
    if(cursor_displayed==1)
        {
        if(VioGetCurType(&vc,hvps)==0)
            {
            vc.attr = (vc.attr==0) ? -1 : 0;
            VioSetCurType(&vc,hvps);
            }
        }
    return;
    }




/******************************************************************************
Function:       show_cursor()

Description:    Shows or hides the Vio cursor

Syntax:         BOOL show_cursor(yn)
                    BOOL yn;    1 = Show the cursor, 0 = hide it
                    HVPS hvps;  Cursor belongs to this presentation space

Returns:        0 if cursor hidden
                1 if cursor displayed

Mods:           06-Oct-89 C.P.Armstrong created
                07-Dec-89 C.P.Armstrong Modified for use with software blink. 
                                        (Not implimented).

******************************************************************************/
BOOL show_cursor(yn,hvps)
    BOOL yn;    /* Show the cursor? */
    HVPS hvps;  /* Cursor belongs to this presentation space */

    {
    extern char cursor_displayed;
    VIOCURSORINFO vc;
    BOOL oa;            // Old state
    static char state=1;
    
    if(VioGetCurType(&vc,hvps)==0)
        {
        oa = (vc.attr==0) ? 1 : 0; 
        vc.attr = yn ? 0 : -1;

        VioSetCurType(&vc,hvps);
        if(yn)
          cursor_displayed=1;
        else
            cursor_displayed=0;
        }
    else
        {    // In case of error assume the cursor isn't there
        oa=0;
        cursor_displayed=0;
        }
    
    return(oa);
    }
  
/******************************************************************************
Function:       Put_cursor_onscreen()

Description:    Adjusts the Vio origin so that the Vio cursor position lies
                within the current window.

Syntax:         void Put_cursor_onscreen(hwndClient, hvps)
                    HWND hwndClient; Handle of the window (not the Frame window)
                    HVPS hvps      ; Handle to the associated vio pres. space.

Returns:        nothing

Mods:           11-Mar-90 C.P.Armstrong created

******************************************************************************/
void Put_cursor_onscreen(HWND hwndClient, HVPS hvps)
    {
    SHORT crow,ccol;
    SHORT orow,ocol;
    RECTL clrctl;
    SIZEL sizl;
    SWP swp;
    HDC hdc;
    VIOMODEINFO vioinf;
    int repos=0;

    /* Get Vio cursor position */
    VioGetCurPos(&crow,&ccol,hvps);
    
    /* Get Vio window size and offset within the frame window */
    WinQueryWindowPos(hwndClient,&swp);
    
    /* Get the Vio screen size */
    if((repos =VioGetMode(&vioinf,hvps))!=0)
        {
        /* This does not work.  The return code is 494 which doesn't help    */
        /* much.  As usual things don't work like the manual says they       */
        /* should which squashes any hope of making this a general function. */
        /* I'll leave the code in on the off chance that MS pull their       */
        /* fingers out and one day make things work like they should.        */
        /* INVALID_HANDLE        436 */
        /* INVALID_LENGTH        438 */
        /* MODE                  355 */
        /* INVALID_PARMS         421 */
        vioinf.row=25;
        vioinf.col=80;
        }

    /* Get the size in pixels of a character */
    hdc = WinQueryWindowDC(hwndClient);
    DevQueryCaps(hdc, CAPS_CHAR_WIDTH, 2L, (PLONG) &sizl);


    /* Figure out window size in rows and cols */
    swp.cy = swp.cy/ (SHORT)sizl.cy;
    swp.cx = swp.cx/ (SHORT)sizl.cx;
    
    /* Get the current origin for the Vio screen (top left row/col) */
    VioGetOrg(&orow,&ocol,hvps);

    /* Is cursor visible then */
    /* Is cursor below bottom edge?  or are there too few lines displayed */
    if((crow>=(swp.cy+orow)) || ((crow-orow)<swp.cy) )   
        {
        orow = crow-swp.cy+1;
        if(orow<0)
            orow = 0;
        if(orow>vioinf.row-2)
            orow = vioinf.row-2;
        repos = 1;
        }
     else if(crow < orow)      /* Is cursor above top edge */
        {
        orow = crow;
        repos = 1;
        }

    /* Is cursor beyond right edge, or are too few cols displayed? */
    if((ccol>=(swp.cx+ocol)) || ((ccol-ocol)<swp.cx) )   
        {
        ocol = ccol-swp.cx+1;
        if(ocol<0)
            ocol = 0;
        if(ocol > vioinf.col-2)
            ocol = vioinf.col-2;
        repos = 1;
        }
     else if(ccol < ocol)      /* Is cursor beyond left edge */
        {
        ocol = ccol;
        repos = 1;
        }

    /* If so reposition the client window in the frame window */
    if(repos!=0)
        {
        VioSetOrg(orow,ocol,hvps);
        }

    return;
    }
    
/******************************************************************************
Function:       CurScrChk()

Description:    Tells the PM thread to put the cursor in the current Vio window.

Syntax:         void CurScrChk()

Returns:        nothing

Mods:           16-Mar-90 C.P.Armstrong

******************************************************************************/
void CurScrChk()
    {
    extern HWND hwndTFrame;

    WinPostMsg(hwndTFrame,WM_CURCHECK,(MPARAM) NULL,(MPARAM) NULL);
    }
 
 
int numlock_status()
    {
    BYTE skt[256];
    
    WinSetKeyboardStateTable(HWND_DESKTOP,skt,FALSE);

    return(skt[VK_NUMLOCK]);

    }
