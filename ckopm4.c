/******************************************************************************
File name:  ckopm4.c    Rev: 01  Date: 01-Apr-90 Programmer: C.P.Armstrong

File title: The Gpi window procedure and some associated functions.

Contents:   GpiWndProc()

Modification History:
    01  01-Apr-90   C.P.Armstrong   created

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <process.h>
#define INCL_DOS
#define INCL_WIN
#define INCL_GPI    
#define INCL_DOSERRORS      /* For the block marking */
#define INCL_VIO            /* and copying routines  */
#define INCL_AVIO
#include <OS2.h>
#include "ckcker.h"   /* For VT100 and TEKTRONIX */
#include "ckopm.h"
#include "ckorc.h"    /* Resource defines */

/* Whoever thought up the defaults for GpiLoadFonts should be shot.  The */
/* function wont look in the current directory and if an extension is    */
/* supplied it wont look in the "libpath".  If no path or extension is   */
/* supplied it uses an extension of .DLL (all references in the refs.    */
/* say to use .FON for font files).  So there is no choice but to call   */
/* ckofon.dll since I have no idea where users will want to keep the     */
/* font files - presumably in the libpath somewhere.                     */

#define REPAINTSTACKSIZE 8192
#define FONFILE "ckofon"  


/* Function definitions */
MRESULT EXPENTRY GpiWndProc(HWND,USHORT,MPARAM,MPARAM);
MRESULT EXPENTRY VioWndProc(HWND,USHORT,MPARAM,MPARAM);
void dbprintf(const char *,...);
int  DecodePMChar(MPARAM,MPARAM);
void Put_cursor_onscreen(HWND,HVPS);
void TitleText(MPARAM,MPARAM);
int  DoPMTekGin(HWND,USHORT*);
void avio_dispatch(struct avio_cellstr *, HVPS);
int menu_routine(HAB,HWND,HWND,HVPS,USHORT,MPARAM,MPARAM);
int init_menu(MPARAM,HWND,HWND,HAB);

/* Block marking and copying routiens */
int mouse_to_curpos(int,int,int *,int *,HVPS,HWND);
int do_mark(HWND,MPARAM,MPARAM,HVPS);
int end_mark(HWND,MPARAM,MPARAM,HVPS);
int start_mark(HWND,MPARAM,MPARAM,HVPS);
int range_reverse(int,int,int,int,int,HVPS);
int fill_clipboard(char far * ,int,HAB);
char * copy_block(int,int,int,int,HVPS);
int get_viops_size(int*,int*,HVPS);
void buff_strins(char *);
void far pc_paint_thread(struct pc_paint * pc_p);



void cdecl pm_err(char *);          /* declared in ckopm.h.  Redeclared here */
void cdecl pm_msg(char*,char*);     /* to avoid having to include all the    */
                                    /* OS2.H stuff required for ckopm.h in an*/
                                    /* attempt to beat the MSC5.1 out of heap*/
                                    /* messages which occur for tiny files.  */
void buff_insert(int);




/****** Initialised global variables */
char far * copyb=NULL;              /* Paste buffer */
int far pasting=0;
char   immediate_update=1;          /* If 0 do not draw point now      */
struct plot_command far pcroot=     /* First plot command structure of */
    {                               /* linked list */
    '\0',
    0L,
    0L,
    NULL
    };
static struct plot_command * pcurrent= /* Pointer to current plot command */
    &pcroot;                           /* static makes it local to this file */

/**** Uninitialised global variables ****/
int pc_out;                     /* next value to be taken from the buffer    */
int pc_in;                      /* next value to be inserted into the buffer */

/******************************************************************************
Function:       GpiWndProc()

Description:    Main routine which handle the messages sent to the window used
                for Tek graphics

Syntax:         MRESULT EXPENTRY GpiWndProc(HWND hwnd,USHORT msg,MPARAM mp1,MPARAM mp2)

Returns:        Depends on the message

Mods:           01-Apr-90 C.P.Armstrong Well actually I created it ages ago.
                                        Today is the day the "out of near heap"
                                        error finally forced me to reduce the
                                        no. of functions in ckopm1 to 3!!.

******************************************************************************/
MRESULT EXPENTRY GpiWndProc(HWND hwnd,USHORT msg,MPARAM mp1,MPARAM mp2)
    {
    extern HAB hab;
    extern HWND hwndGFrame;
    extern HWND hwndTFrame;
    extern HVPS hvps;
    extern ULONG wind_sem;
    extern COLOR pmbgcol;
    extern COLOR pmfgcol;
    extern char  defaultattribute;
    extern char immediate_update;
    extern char Vio_visible;
    extern struct plot_command pcroot;
    extern struct plot_command * pcurrent;
    extern char minied;              /* Indicates window is minimized */
    extern ULONG paint_sem;          /* Set when repaint desired */
    extern ULONG stop_painting_sem;  /* Set when about to clear */

    extern int Term_mode;

    static HPS gpi_hps;
    static HPOINTER hptr;   /* The cross pointer handle */
    static HWND hwndMenu;
    static struct pc_paint paintdat;
    static struct fontstuff fs;
    static int* repaintstk;
    static TID  repaintthrd;

    HDC gpi_hdc;
    RECTL rcl;
    SIZEL sizl;
    SWP   swp;
    POINTL ptl;             /* Stores the next draw command coords */
    QMSG qmsg;
    static char c;          /* contains the next draw command */
    char far * psi;         /* pointer to a string for graphic input string */
    long rgbcol;
    struct plot_command * pcc;
    
    int i,start,x,y;


    switch(msg)
        {
        case WM_CREATE:
            gpi_hdc = WinOpenWindowDC(hwnd);
            sizl.cx = sizl.cy = 0;
            gpi_hps = GpiCreatePS(hab,gpi_hdc,&sizl, PU_PELS | GPIF_DEFAULT | 
                                             GPIT_MICRO | GPIA_ASSOC);
            
            /* Load the menus */
            hwndMenu = WinWindowFromID(
                            WinQueryWindow(hwnd,QW_PARENT,FALSE),
                            FID_MENU);
            

            if(GpiLoadFonts(hab,FONFILE)!=GPI_OK)
                {
                pm_err("Failed to load TEKTRONIX fonts");
                paintdat.fnt = NULL;
                }
            else 
                {
                fs.name = "Tektronix";
                fs.vect = 0;
                fs.h = 8;
                fs.w = 6;
                /* Let paint thread access the font info */
                paintdat.fnt = &fs;             
                /* Select the Tektronix font */
                SelectFont(gpi_hps,LCID_TEKFONT,fs.name,fs.vect,fs.h,fs.w);
                }

            /* Load the mouse cross pointer  */
            hptr = WinLoadPointer(HWND_DESKTOP,(HMODULE)0,ID_CROSSPTR);

            /* Start the paint thread - the font info is set up above */
            DosSemClear(&(paintdat.StopPaintSem));
            paintdat.root = &pcroot;
            paintdat.hwnd  = hwnd;
            paintdat.fgcol = pmfgcol;
            paintdat.bkcol = pmbgcol;
            
            /* allocate stack for repaint thread */
            if((repaintstk = (int*) malloc(REPAINTSTACKSIZE))==NULL)
                {
                dbprintf("Can't allocate paint_thread stack space\n");
                DosExit(EXIT_PROCESS,1);
                }

            /* pc_paint_thread clears the EndPaintThread when its up and running */
            if(DosSemSet(&(paintdat.EndPaintThread))!=0)
                {
                dbprintf("Can't set EndPaintThread\n");
                DosExit(EXIT_PROCESS,1);
                }

            _beginthread(pc_paint_thread,repaintstk,REPAINTSTACKSIZE,&paintdat);
            DosSemWait(&(paintdat.EndPaintThread),SEM_INDEFINITE_WAIT);

            return(0);
            
        case WM_COMMAND:
            menu_routine(hab,hwndMenu,hwnd,hvps,msg,mp1,mp2);
            break;

        case WM_INITMENU:
            init_menu(mp1,hwndMenu,hwnd,hab);
            break;

        case WM_SIZE:

            /* Adjust VT100 screen to just fit new size if in VT100 mode */
            if(Term_mode==VT100)
                {  
                /* Size the Vio screen */
                WinSetWindowPos(hwndTFrame,HWND_TOP,
                    0,0,
                    SHORT1FROMMP(mp2) , SHORT2FROMMP(mp2) ,
                    SWP_SIZE|SWP_MOVE);
                }
            else
                {
                WinQueryWindowPos(hwndTFrame,&swp);
                swp.cy = SHORT2FROMMP(mp2)-swp.cy;
                /* Set the position of the Vio window in the Gpi */
                WinSetWindowPos(hwndTFrame,HWND_TOP,
                        (SHORT)swp.x,(SHORT)swp.cy,
                        (SHORT)0, (SHORT)0,
                        SWP_MOVE);
                }
                
            /* Reposition the current graphics position */
            if(SHORT1FROMMP(mp1)!=0 && SHORT2FROMMP(mp1)!=0)
                {
                GpiQueryCurrentPosition(gpi_hps, &ptl);
                ptl.x = ptl.x * (LONG)SHORT1FROMMP(mp2) / (LONG) SHORT1FROMMP(mp1);
                ptl.y = ptl.y * (LONG)SHORT2FROMMP(mp2) / (LONG) SHORT2FROMMP(mp1);
                GpiMove(gpi_hps, &ptl);
                }

            break;      /* Now do the default for this window */
        case WM_PAINT:
            /* Stop repaint thread from painting */
            if(DosSemWait(&(paintdat.StartPaintSem),SEM_IMMEDIATE_RETURN)==0)
                {
                DosSemSet(&(paintdat.StopPaintSem));
                DosSemWait(&(paintdat.StopPaintSem),SEM_INDEFINITE_WAIT);
                }
            WinBeginPaint(hwnd,gpi_hps,NULL);
            if(WinQueryWindowULong(WinQueryWindow(hwnd,QW_PARENT,FALSE),
                                    QWL_STYLE) & WS_MINIMIZED)
                {
                minied=1;
                WinSendMsg(hwndTFrame,WM_HIDE,(MPARAM)1L,(MPARAM)0L);
                }
            else 
                {
                /* If we were minimized then show the text window */
                if(minied)
                    {
                    WinSendMsg(hwndTFrame,WM_HIDE,0,0);
                    minied=0;
                    }
                }

            WinQueryWindowRect(hwnd,&rcl);
            pmbgcol = VioToRgbColor((BYTE)(defaultattribute>>4));
            pmfgcol = VioToRgbColor(defaultattribute);

            /* Make sure the paint thread sees the current colours */
            paintdat.fgcol = pmfgcol;
            paintdat.bkcol = pmbgcol;

            /* Make sure this PS knows the current fg colour */
            GpiSetColor(gpi_hps,pmfgcol);

            if(c!='v')
                {
                start = 0;
                pc_out=pc_in;
                WinFillRect(gpi_hps,&rcl,pmbgcol);
                WinQueryWindowPos(hwnd,&(paintdat.pc_swp));
                DosSemClear(&(paintdat.StartPaintSem));
                }
                
            c=0;
            WinEndPaint(gpi_hps);
            return(0);
            
        case WM_USER:
            /* Fill up the plot commands */
            c = CHAR1FROMMP(mp2); 
            if(c=='c')      /* Clear screen - we can deal with this here */
                {
                /* Tell the paint thread to stop */
                DosSemClear(&(paintdat.StopPaintSem));

                /* Free the malloc'd plot command areas */
                pc_delete(&pcroot);
                pcurrent=&pcroot;

                ptl.x=0;
                ptl.y=0;
                /* Home the cursor */
                GpiMove(gpi_hps,&ptl);
                
                /* c=0 causes a screen clear */
                c=0;
                }

            if(c==0 || c=='v')
                {
                WinQueryWindowRect(hwnd,&rcl);
                WinInvalidateRect(hwnd,&rcl,TRUE);
                }
            else
                {
                /* Save the new plot command */
                pcurrent = pc_save(pcurrent,mp1,mp2);
                
                /* Most recent size is stored in the paintdat structure */
                pc_interp(*pcurrent,&(paintdat.pc_swp),0,gpi_hps);
                pc_in++;
                }
            return(0L);

        case WM_GIN:              /* Get coordinate of a mouse press */
            WinPostMsg(hwndTFrame, WM_HIDE,mp1,mp1);
            SetPMGinMode(hwnd,mp1,hptr);   /* mp=1 set GIN mode, 0 clears it */

            return(0L);

        case WM_CHAR:
            if(!(CHARMSG(&msg)->fs & KC_CHAR))
                {
                return(0);
                }
        case WM_BUTTON3DOWN:
        case WM_BUTTON2DOWN:
        case WM_BUTTON1DOWN:
            if(DoPMTekGin(hwnd,&msg))  /* If we are in Gin mode then send */
                return(0L);            /* the waiting routine some coords */
                                       /* else do the WinDefProc thing    */
            break;

        case WM_MOUSEMOVE:
            if(TestPMGinMode())
                {
                WinSetPointer(HWND_DESKTOP, hptr);
                return((MRESULT)1);
                }
            break;

        case WM_CLOSE:            /* User clicked on "Close.." so give prog  */
            GpiDeleteSetId(gpi_hps,LCID_ALL); /* Delete all ids */
            GpiUnloadFonts(hab,FONFILE);      /* unload the kermit fonts */
            break;
        case WM_DESTROY:
            GpiDestroyPS(gpi_hps);
            return(0);

        }
    return(WinDefWindowProc(hwnd,msg,mp1,mp2));
    }


/* Vector font stuff */

/******************************************************************************
Function:       SelectFont()

Description:    Creates and selects the font with the supplied name

Syntax:         LONG SelectFont (hps,lcid ,szFacename, vect,h,w)
                    HPS hps
                    LONG lcid
                    CHAR *szFacename
                    int vect; It's a vector font
                    int h,w;  Height and width of non-vector font
Returns:        

Mods:           19-Jan-90 C.P.Armstrong  Adapted from the PM Toolkit examples
                05-Apr-90 C.P.Armstrong  Selection of a vector font is optional

******************************************************************************/
LONG SelectFont (HPS hps, LONG lcid, CHAR *szFacename,
    int vect,int h,int w)
     {
     FATTRS fat ;

     fat.usRecordLength  = sizeof fat ;
     fat.fsSelection     = 0 ;
     fat.lMatch          = 0 ;
     fat.idRegistry      = 0 ;
     fat.usCodePage      = GpiQueryCp (hps) ;

    if(vect==0)
        {
         fat.lMaxBaselineExt = h ;
         fat.lAveCharWidth   = w ;
         fat.fsType          = 0 ;
         fat.fsFontUse       = 0 ;
         }
    else
        {
        fat.lMaxBaselineExt = 0 ;
        fat.lAveCharWidth   = 0 ;
        fat.fsType          = 0 ;
        fat.fsFontUse       = FATTR_FONTUSE_OUTLINE |
                              FATTR_FONTUSE_TRANSFORMABLE ;
        }



     strcpy (fat.szFacename, szFacename) ;

     if(GpiCreateLogFont (hps, NULL, lcid, &fat)!=2)
        {
        pm_err("Font not selected");
        return(GPI_ERROR);
        }
     return(GpiSetCharSet(hps, lcid));
     }

/******************************************************************************
Function:       SetCharBox()

Description:    Sets the character box size

Syntax:         void setcharbox(hps,width,height)
                    HPS hps;     Presentation space
                    int width;   Width in PELS
                    int height;  Height in PELS

Returns:        nothing

Mods:           19-Jan-90 C.P.Armstrong created

******************************************************************************/

void SetCharBox(hps,width,height)
    HPS hps;
    int width;   /* Width in PELS */
    int height;  /* Height in PELS */
    {                                 
    SIZEF  sizfx ;
                                                   
    sizfx.cx = MAKEFIXED(width,0);
    sizfx.cy = MAKEFIXED(height,0);
    
    GpiSetCharBox(hps,&sizfx);
    
    return;
    }


/******************************************************************************
Function:       do_paste()

Description:    Performs the "paste" operation.  Transfers any text in the
                clipboard to the keyboard input buffer.
                
                Pauses after a CR to let kermit catch up!

Syntax:         int do_paste(HAB hab)

Returns:        0 if successful
                1 if not

Mods:           01-Apr-90 C.P.Armstrong created
                02-Apr-90 C.P.Armstrong Filters out the LFs which cause problems
                        see *           with the asshole VMS editors.
                11-Apr-90 C.P.Armstrong Single LFs replace by CRs.  Paste text 
                        see **          from the OS/2 1.2 system editor causes
                                        errors similar to those caused by LFs.
                                        Function moved to ckopm4 as the addition
                                        of two lines causes MSC5.1 to barf on
                                        this function - even if two massive ones
                                        are removed from ckopm3!!!!!  Gimme
                                        v6.00....... (for ever hopeful)
                20-Apr-90 C.P.Armstrong Removed the wait after CR. Waiting
                                        didn't help as PM couldn't process in-
                                        coming text thus holding up the other
                                        threads.  The wait is now done in the
                                        "conect" thread leaving PM free to
                                        process incoming print messages.
                                *       Turns out LFs are supplied by the vt100
                                        routines when a CR is received and LF
                                        alone causes a backspace with VMS 
                                        editors(!) resulting in long lines...
                                 **     The OS/2 system editor does not insert
                                        CRs or LFs in wrap mode resulting in
                                        very long lines which cause the VMS
                                        editors to choke.
******************************************************************************/
do_paste(HAB hab)
    {
    extern char far * copyb;
    extern int pasting;
    int siz;
    SEL sel;
    char far * clipb;

    if(copyb!=NULL)             /* Copyb is allocated - pasting in progress */
        return(-1);

    if(!WinOpenClipbrd(hab))     /* Open the clipboard */
        {
        pm_err("Failed to open clipboard");
        return(1);
        }


    copyb=NULL;
    if( (sel = (SEL) WinQueryClipbrdData(hab,CF_TEXT)) )
        {
        clipb = MAKEP( sel,0 );   /* Convert selector to pointer to text */
        siz = strlen(clipb)+1;    /* Get length of text already there    */
        if( (copyb=malloc(siz))!=NULL)
            {
            strcpy(copyb,clipb);  /* Copy clipboard text to buffer */          
            }
        }
 
        WinCloseClipbrd(hab);     /* Close the clipboard */

    /* If there was any text, and we copied it okay, copyb is not NULL */
    if(copyb!=NULL)               /* For Kermit simply put it into the */
        {                         /* keyboard buffer.                  */
        pasting=1;                /* Tell buff_empty we are using paste buf */
        buff_insert(0);           /* trigger buff_empty.                    */
        }

    return(0);
    }

/******************************************************************************
Function:       get_paste_char()

Description:    Gets characters out of the paste buffer

Syntax:         int get_paste_char()

Returns:        The character or -1 if the buffer is empty

Mods:           24-Apr-90 C.P.Armstrong created

******************************************************************************/
int get_paste_char()
    {
    extern char far * copyb;
    extern int pasting;
    static int next=0;
    char next_c;
    
    if(copyb==NULL)
        return(-1);

//    if(next>0)              /* Convert lone LFs to CRs */
//        {
//        if((copyb[next]==10) && (copyb[next-1]!=13))
//            copyb[next]=13;
//        }

    if(copyb[next]==10)     /* Skip the LF in CRLFs */
        next++;

    next_c=copyb[next];
    
    if(next_c==0)           /* If we've reached the end clean up */
        {
        next_c=-1;
        next=0;
        free(copyb);
        pasting=0;
        copyb=NULL;
        }
    else
        next++;
        
    return(next_c);
    }




/* Requires INCL_DOSERRORS and INCL_VIO */
int mark_start_row;     /* Start row of Vio marked block */
int mark_start_col;     /* Start col of Vio marked block */
int mark_end_row;     /* Start row of Vio marked block */
int mark_end_col;     /* Start col of Vio marked block */
int marking;            /* 0 if not marking,             */
                        /* 1 if doing mark,              */
                        /* 2 if block marked             */
int direct;             /* direction of marking */
int mcaptive;           /* 1 if mouse is captured */

int start_mark(HWND hwnd,MPARAM mp1,MPARAM mp2,HVPS hvps)
    {
    extern int mark_start_row;  /* Position of the start of the marked block */
    extern int mark_start_col;  
    extern int mark_end_row;
    extern int mark_end_start;
    extern int marking;
    extern int direct;
    extern int mcaptive;

    /* Unhighlight any current marked range */
    direct=1;
    if(marking==2)
        range_reverse(mark_start_row,mark_start_col,mark_end_row,mark_end_col,
            direct,hvps);

    mouse_to_curpos(SHORT1FROMMP(mp1),SHORT2FROMMP(mp1),
        &mark_start_row,&mark_start_col,hvps,hwnd);
    
    mark_end_row = mark_start_row;
    mark_end_col = mark_start_col;   

    direct=0;
    marking = 1;
    
    /* Capture the mouse */
    WinSetCapture(HWND_DESKTOP,hwnd);
    mcaptive=1;
    return(0);
    }
    
int end_mark(HWND hwnd,MPARAM mp1,MPARAM mp2,HVPS hvps)
    {
    extern int mark_end_row;  /* Position of the end of the marked block */
    extern int mark_end_col;  
    extern int marking;
    int temp;

    

    do_mark(hwnd,mp1,mp2,hvps);

    /* Free up the mouse */
    WinSetCapture(HWND_DESKTOP, NULL) ;
    mcaptive=0;

    if(mark_end_row==mark_start_row && mark_end_col==mark_start_col)
        {
        marking=0;
        return(0);
        }


    /* Make sure point one is "higher" than point 2 */
    if( ((mark_end_row==mark_start_row) && (mark_end_col<mark_start_col)) || 
        (mark_end_row<mark_start_row) )
        {
        temp = mark_end_row;
        mark_end_row = mark_start_row;
        mark_start_row = temp;
        
        temp = mark_end_col;
        mark_end_col = mark_start_col;
        mark_start_col = temp;
        }
    marking = 2;                                      

    return(0);
    }
    
int quit_mark(HWND hwnd,MPARAM mp1, MPARAM mp2,HVPS hvps, HAB hab)
    {
    extern int marking;
    if(marking!=0)  
        {
        end_mark(hwnd,mp1,mp2,hvps);
        do_copy(0,hvps,hab);
        }
    return(0);
    }


/******************************************************************************
Function:       do_mark()

Description:    Marks an area of screen according to the mouse position.

Syntax:         int do_mark(HWND hwnd,MPARAM mp1,MPARAM mp2,HVPS hvps)

Returns:        0

Mods:           01-Apr-90 C.P.Armstrong created

******************************************************************************/
int do_mark(HWND hwnd,MPARAM mp1,MPARAM mp2,HVPS hvps)
    {
    extern int mark_start_row;  /* Position of the start of the marked block */
    extern int mark_start_col;  
    extern int mark_end_row;
    extern int mark_end_col;
    extern int marking;
    extern int direct;

    int currow,curcol,temp;     /* Current cursor position */
    int atsrow,atscol,atecol,aterow;
    int newdirect;
    int wasa,nowa;
    int diroff; /* Another fudge factor.  Having marked forwards and are now
                   marking back must not rehighlite current chart until we pass
                   the orig place! */


    if(marking!=1)              /* Not marking a block! */
        return(0);

    mouse_to_curpos(SHORT1FROMMP(mp1),SHORT2FROMMP(mp1),
        &currow,&curcol,hvps,hwnd);

    /* Has cursor moved a character position */
    if( (currow==mark_end_row) && (curcol==mark_end_col) )
        return(0);   

    if( ((currow==mark_end_row) && (curcol<=mark_end_col)) || 
        (currow<mark_end_row) )
        newdirect = -1;
    else
        newdirect = 1;

    /* are we above the start */
    if( (currow==mark_start_row && curcol<mark_start_col) 
        || currow<mark_start_row )
        nowa=1;
    else
        nowa=0;
        
    /* Were we above the start */
    if( (mark_end_col<mark_start_col && mark_end_row==mark_start_row)
        || mark_end_row<mark_start_row)
        wasa=1;
    else
        wasa=0;

    /* Avoid having the start char erased in a move from above to below start */
    /* or a move from below to above */
    /* this is a real kludge.  It double highlights the initial pos, leaving  */
    /* it the same!                                                           */
    if(wasa!=nowa && direct!=0)
        {
        range_reverse(mark_start_row,mark_start_col,mark_start_row,mark_start_col,
            1,hvps);
        }
 

    if( (nowa && (!wasa)  && direct!=0) ||
             ((!nowa) && wasa && direct!=0))
        diroff=0;
    else if(nowa && newdirect==1 && direct!=0) /* above, going forward */
        diroff=-1;
    else if((!nowa) && newdirect==-1 && direct!=0)/* below, going back */
        diroff=1;
    else if(direct!=0)
        {
        mark_end_col+=newdirect;
        diroff=0;
        }
    else
        {
        diroff=0;
        }

    direct=newdirect;

    /* Do mainmark! */
    range_reverse(mark_end_row,mark_end_col,currow,curcol+diroff,direct,hvps);


    /* Update the previous position */
    mark_end_col = curcol;
    mark_end_row = currow;

    /* If back at orig pos, make sure it gets highlit if cursor is moved */
    /* Now needs to be specifically unhighlighted as well */
    if( (mark_start_row==mark_end_row) && (mark_start_col==mark_end_col) )
        {
        range_reverse(mark_start_row,mark_start_col,mark_start_row,mark_start_col,
            1,hvps);
        direct=0;
        }

    return(0);
    }                                 

 

/*
Mode = 0   - Erase marked block - do not copy it
Mode = 1   - If block marked then clear clipboard, copy marked block to it
Mode = 2   - If block marked then append marked block to any text present
*/
do_copy(int mode, HVPS hvps, HAB hab)
    {
    extern int mark_start_row;  /* Position of the start of the marked block */
    extern int mark_start_col;  
    extern int mark_end_row;
    extern int mark_end_col;
    extern int marking;

    char far * block;

    

    if(mode>0)
        {
        if(marking!=2)
            return(0);
        /* Get the marked block */
        if( (block = copy_block(mark_start_row,mark_start_col,
                           mark_end_row,mark_end_col,hvps)) !=NULL)
            {
            fill_clipboard(block,mode,hab);
            }
        }

    /* Un mark the block */
    range_reverse(mark_start_row,mark_start_col,mark_end_row,mark_end_col,
        1,hvps);
    marking=0;

    return(0);
    }


/******************************************************************************
Function:       copy_block()

Description:    Copies the defined Vio block to the clipboard in text format 
                only.  If mode is 0 the previous contents are deleted.  If
                mode is 1 then the copied block is appended to the existing
                text in the clipboard.
                Note the the block coords must have the start character coords
                higher (nearer the top, above, to the left of etc.) the
                end character coords.

                This routine uses a CRLF sequence delimit lines.  The PETZOLD
                clipboard example requires CRLF to produce correct newlines
                so I've chosen to use this as the "standard".... mainly because
                it's what I'm using to test the copy_block function!
                
                A pointer to the string block is now returned.  This is not
                sharable memory.  It should be "free()"d after use as it is
                allocated using malloc().

Syntax:         char * copy_block(strow,stcol,enrow,encol,hvps)
                    int strow,stcol;   First character in block
                    int enrow,encol;   Last character in block
                    HVPS hvps;         AVIO handle to vio screen.

Returns:        NULL if an error occurs
                Pointer to the NUL terminated string block.

Mods:           31-Mar-90 C.P.Armstrong created
                01-Apr-90 C.P.Armstrong Returns pointer to string block.

******************************************************************************/
char * copy_block(strow,stcol,enrow,encol,hvps)
    int strow,stcol;   /* First character in block */
    int enrow,encol;   /* Last character in block */
    HVPS hvps;         /* AVIO handle to vio screen. */
    {
    int sw,sh;          /* Vio screen dims */
    
    char far * cblk;    /* Area for the copied block */
    USHORT off;         /* offset into buffer */
    USHORT get;         /* no. chars to get/got */
    USHORT siz;         /* Size of copied block */
    
    /* First get screen dims */
    if(get_viops_size(&sw, &sh,hvps)!=0)
        return(NULL);
        
    /* Calc size of block in chars with all lines except last terminated by */
    /* a CR and the whole block NUL terminated.                             */
    if(enrow==strow)
        siz = encol-stcol+2;            /* No. chars plus NUL */
    else
        {
        siz = ((enrow-strow-1)*(sw+2)); /* No. full lines and CRLFs */
        siz += (sw-stcol+2);            /* No. chars on first line + CRLF */
        siz += (encol)+2;               /* No. chars on last line plus NUL */
        }

    /* Malloc the block - return NULL if we can't */
    if( (cblk = (char far *) malloc(siz))==NULL)
        return(NULL);


    /* Read the block a line at a time into cblk, inserting CRs at the end of */
    /* all but the last line.                                                 */
    if(enrow==strow)    /* Less than one line is special case */
        {
        get = encol-stcol+1;
        VioReadCharStr(cblk,&get,strow,stcol,hvps);
        cblk[get]='\0';
        }
    else
        {
        /* First line */
        get = sw-stcol;
        VioReadCharStr(cblk,&get,strow,stcol,hvps);
        off=0;
        do
            {
            do                      /* Trim trailing blanks */
                {
                get--;
                }while((cblk[off+get]==' ') && (get>0));
                                    /* off+get now points to last character */                

            cblk[off + (++get)]=13; /* Terminate previous string */
            cblk[off + (++get)]=10;
            off += (get+1);         /* Offset of next string in memory block*/
            strow++;                /* Next row */
            get = (strow==enrow)? encol+1 : sw;
            VioReadCharStr(&cblk[off],&get,strow,0,hvps);
            }while(strow<enrow);
        cblk[off+get]='\0';         /* Nul terminate the string */
        }

    return(cblk);    
    }

/******************************************************************************
Function:       fill_clipboard()

Description:    Transfers a block of marked text to the clipboard.
                In mode 1 the existing clipboard data is erased.
                In mode 2 the new text is added to at the end of any existing
                text in the clipboard.

Syntax:         int fill_clipboard(char far * block,int mode,HAB hab)

Returns:        0 for success
                1 if an error occurs

Mods:           01-Apr-90 C.P.Armstrong created

******************************************************************************/
int fill_clipboard(char far * block,int mode,HAB hab)
    {
    SEL sel;
    char far * clipb;
    char far * cblk;
    int siz;


    if(!WinOpenClipbrd(hab))     /* Open the clipboard */
        {
        pm_err("Failed to open clipboard");
        return(1);
        }
 
    siz=0;
    clipb = NULL;
    if(mode==2)                 /* Mode 2 = append */
        {
        if( (sel = (SEL) WinQueryClipbrdData(hab,CF_TEXT)) )
            {
            clipb = MAKEP( sel,0 );   /* Convert selector to pointer to text */
            siz = strlen(clipb)+1;    /* Get length of text alredy there     */
            }
        }

    /* Add size of new text */
    siz += strlen(block)+1;          /* Note, if appending the memory block  */
                                     /* will be 1 too big - but who cares!   */

    /* Allocate the shared seg to give to clipboard */
    if(DosAllocSeg(siz, &sel, SEG_GIVEABLE)!=0)
        {
        WinCloseClipbrd(hab);       /* Must close the clipboard */
        return(1);
        }
        
    /* Convert shared segment selector to pointer */
    cblk = MAKEP(sel,0); 
    cblk[0]=0;
 
    /* If clipb has been set we should copy its text first */
    if(clipb != NULL)
        strcpy(cblk,clipb);

    /* Now add in the new marked block */
    strcat(cblk,block);

    /* Free the marked block */
    free(block);

    /* Empty the clipboard */ 
    if(!WinEmptyClipbrd(hab))
        pm_err("Failed to empty clipboard");          

    /* Now set it to the new stuff */
    if(!WinSetClipbrdData(hab, (ULONG) sel, CF_TEXT, CFI_SELECTOR))
        pm_err("Failed to place data in clipboard");
    
    if(!WinCloseClipbrd(hab)) /* Close clipboard */
        {
        pm_err(
        "You've got real problems now\nWe failed to close the clipboard");        
        return(1);
        }
    
    return(0);
    }


/******************************************************************************
Function:       mouse_to_curpos()

Description:    Converts a mouse position into a Vio row/col position.  The
                mosue position is relative to the bottom lefthand corner,
                the row/col position is relative to the top left corner of
                the video buffer. i.e. an Vio origin shift is taken into 
                account.
                
                If the position is outside the window the row/col is adjusted
                to be inside the window.
                
                This function can be used with any window coord in pels.

Syntax:         int mouse_to_curpos(mousex,mousey,prow,pcol,hvps,hwnd)
                    int mousex,mousey;  Mouse window x and y pos
                    int * prow, * pcol; Returned vio cursor pos
                    HVPS hvps;          handle to the Vio pres space
                    HWND hwnd;          Handle to the window containing the PS

Returns:        0

Mods:           01-Apr-90 C.P.Armstrong created

******************************************************************************/
int mouse_to_curpos(mousex,mousey,prow,pcol,hvps,hwnd)
    int mousex,mousey;      /* Mouse window x and y pos */
    int * prow, * pcol;     /* Returned vio cursor pos  */
    HVPS hvps;              /* handle to the Vio pres space */
    HWND hwnd;              /* Handle to the window containing the PS */
    {
    SIZEL sizl;
    SWP swp;
    HDC hdc;


    /* Get Vio window size and offset within the frame window */
    WinQueryWindowPos(hwnd,&swp);

    /* Get the size in pixels of a character */
    hdc = WinQueryWindowDC(hwnd);
    DevQueryCaps(hdc, CAPS_CHAR_WIDTH, 2L, (PLONG) &sizl);

    /* Check mouse is in the window */
    if(mousey<0)
        mousey=0;
    else if(mousey>swp.cy)
        mousey=swp.cy;
        
    if(mousex<0)
        mousex=0;
    else if(mousex>swp.cx)
        mousex=swp.cx;

    /* Convert mouse pos to top right origin */
    mousey = swp.cy - mousey;


    /* Calculate mouse pos in row/cols */
    mousex = mousex/ (int)sizl.cx;
    mousey = mousey/ (int)sizl.cy;

    /* Get the current origin for the Vio screen (top left row/col) */
    VioGetOrg((PSHORT)prow,(PSHORT)pcol,hvps);
    
    /* Add offset to mouse pos */
    *prow = *prow + mousey;
    *pcol = *pcol + mousex;

    return(0);
    }
    
int range_reverse(atsrow,atscol,aterow,atecol,direct,hvps)
    int atsrow,atscol;  /* Highest position to start from */
    int aterow,atecol;  /* Postion to go down to          */
    int direct;      /* Forward or reverse marking */
    HVPS hvps;          /* Handle of screen PS */
    {
    extern int scrwidth;  /* This is needed until we find a way of determining it */
    int  atsz;
    unsigned char attbuf[3];

    /* Make sure the last one gets highlit */
    atecol+=direct;
                  
    do
        {
        atsz=2;
        if(VioReadCellStr(attbuf,(PUSHORT)&atsz,atsrow,atscol,hvps)
          ==ERROR_VIO_COL)
            {
            atsrow+=direct;
            atscol=(direct==1) ? 0 : scrwidth-1;
            }
        else
            {
            attbuf[3] = (attbuf[1] << 4) + (attbuf[1] >> 4);
            attbuf[1] = attbuf[3];
            if(VioWrtCellStr(attbuf,2,atsrow,atscol,hvps)!=0)
                return(0);
            atscol+=direct;
            }
        }while( (atscol!=atecol) || (atsrow!=aterow));

    return(0);
    }

/******************************************************************************
Function:       get_viops_size()

Description:    Determines the dimensions of a Vio screen.  VioGetMode is
                supposed to do this but it doesn't work with an AdvancedVIO
                presentatoin space handle.  This function uses the 
                VioReadCharStr() error messages to determine the screen size.

Syntax:         int get_viops_size(width,height,hvps)
                    int* width;
                    int* height;
                    HVPS hvps;

Returns:        1 if an unexpected error occurs
                0 if okay

Mods:           30-Mar-90 C.P.Armstrong created

******************************************************************************/
int get_viops_size(int* width, int* height,HVPS hvps)
    {
    USHORT row,col,sz;
    USHORT err;
    char attr;
        
    row=1;
    col=1;

    /* Do the width first, step 2 at once for speed */
    do
        {
        sz=1;
        }while( (err=VioReadCharStr(&attr,&sz,row,col+=2,hvps))==0);
        
     if(err!=ERROR_VIO_COL)
        return(1);
     
     sz=1;
     col--;
     /* See if we've gone 1 too far */
     if(VioReadCharStr(&attr,&sz,row,col,hvps)!=0)
        col--;
     
    /* Now do the height */
    do
        {
        sz=1;
        }while( (err=VioReadCharStr(&attr,&sz,row+=2,col,hvps))==0);
        
     if(err!=ERROR_VIO_ROW)
        return(1);
     
     sz=1;
     row--;
     
     /* See if we've gone 1 too far */
     if(VioReadCharStr(&attr,&sz,row,col,hvps)!=0)
        row--;
 
    *width=  ++col;     /* We want no. of characters, not max positions */
    *height= ++row;
    return(0);
    }

/******************************************************************************
Function:       pc_paint_thread()

Description:    The thread which does the painting for the Gpi window
                Special situations of which this function must be told;
                    1 About to clear the plot list.
                    2 Want to do a repaint so stop any repaint in progress
                    3 Program ending.

Syntax:         void far pc_paint_thread()

Returns:        nothing

Mods:           13-Feb-90 C.P.Armstrong created

******************************************************************************/
void far pc_paint_thread(struct pc_paint * pc_p)
    {
//    extern ULONG paint_sem;          /* Set when repaint desired */
//    extern ULONG stop_painting_sem;  /* Set when about to clear */
//    extern SWP pc_swp;
//    extern struct plot_command pcroot;
//    extern char minied;
//    extern HPS gpi_hps;
    extern COLOR pmfgcol;
    extern HWND hwndGraph;

    HPS hps;
    HAB painthab;
    int count;

    struct plot_command * pcc;

    painthab=WinInitialize(0);

    DosSemSet(&(pc_p->StartPaintSem)); /* Make sure semaphore is set before */
                                       /* entering the loop */
    /* Clear the EndPaintThread sem so caller knows we are running */
    DosSemClear(&(pc_p->EndPaintThread));
    while(TRUE)
        {
        /* Wait for it to repaint request */
        DosSemWait(&(pc_p->StartPaintSem),SEM_INDEFINITE_WAIT);
        hps = WinGetPS(pc_p->hwnd);

        /* Set parameters for this incarnations of the PS */
        GpiSetColor(hps,pc_p->fgcol);

        if(pc_p->fnt != NULL)
            {
            SelectFont(hps, LCID_TEKFONT, 
              pc_p->fnt->name,pc_p->fnt->vect,pc_p->fnt->h,pc_p->fnt->w);

            /* Size the font */
            SetCharBox(hps, 
              MAXCHARWIDTH* pc_p->pc_swp.cx/MAXXRES, 
              MAXCHARHEIGHT* pc_p->pc_swp.cy/MAXYRES);
            }
 
        pcc=(pc_p->root);                           /* Do the painting */
        count=0;
        do
            {
            pc_interp(*pcc,&(pc_p->pc_swp),0,hps);
            pcc = pcc->next;
            }
        while((pcc != NULL)
         && (DosSemWait(&(pc_p->StopPaintSem),SEM_IMMEDIATE_RETURN)==0));


        WinReleasePS(hps);
        DosSemSet(&(pc_p->StartPaintSem));               /* Reset it */

        /* Set the StopPaint to ack that we've stopped */
        if(DosSemClear(&(pc_p->StopPaintSem))!=0)
            dbprintf("pc_paint_thread - Error clearing StopPaintSem\n");

        dbprintf("Repainting finished\n");
        }

    dbprintf("pc_paint_thread finishing\n");
    WinTerminate(painthab);
    DosEnterCritSec();
    DosSemClear(&(pc_p->EndPaintThread));
    DosExit(EXIT_THREAD,0);        /* This will also do a DosExitCritSec */
    }
