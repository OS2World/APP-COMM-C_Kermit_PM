/******************************************************************************
Header name:   ckopm.h    Rev: 01  Date: 07-Dec-89 Programmer: C.P.Armstrong

Header title:  Definition of structures and functions used by the PM interface
               outines.

Description:    

Modification History:
    01  07-Dec-89   C.P.Armstrong   created
        31-Jan-90   C.P.Armstrong   Gin and vector stuff added
******************************************************************************/
/* Nominal maximum plot resolution */
#define MAXXRES             1024
#define MAXYRES              780
#define MAXCHARHEIGHT         15
#define MAXCHARWIDTH          14 


/* User defined window messages */
/* WM_USER is used by Gpi */
#define WM_KERAVIO    WM_USER+1
#define WM_HIDE       WM_USER+2
#define WM_GIN        WM_USER+3       /* Used by Gpi */
#define WM_TITLETEXT  WM_USER+4
#define WM_CURCHECK   WM_USER+5


/* avio_command command definitions */
#define RDCELLSTR   0
#define WRCELLSTR   1
#define WRNCELL     2
#define SCROLLRT    3
#define SCROLLLF    4
#define SCROLLUP    5
#define SCROLLDN    6
#define WRCCHSTAT   7
#define SETCURPOS   8
#define GETCURPOS   9
#define GETCURTYP   10
#define SETCURTYP   11
#define PRINTF      12
#define WRCCHSTATD  13

///* Resource file definitions */   Moved to ckorc.h
//#define ID_RESOURCE 1

/* Vector font ID */
#define LCID_VECTFONT 1L
#define LCID_TEKFONT  2L


struct dlgopn
    {
    char * name;   /* Pointer to the buffer for the returned filename */
    char * title;  /* Pointer to the dialog title */
    };


/* Structure definitions */
struct avio_cellstr
    {
    int fun;
    char * string;
    PUSHORT  plen;
    int    len;
    SHORT  row;
    SHORT  col;
    int    hite;
    int    wid;
    char * pcell;
    };
    
struct plot_command
    {
    char   c;
    POINTL ptl;
    struct plot_command * next;
    };
    
struct TitleText_command
    {
    char action;    /* 0 for set, 1 for get, 2 for len */
    HWND hwnd;      /* Window to set text in */
    char * buffer;  /* buffer containing the text or to receive the text */
    SHORT len;
    PULONG sem;     /* Semaphore to clear when command is processed */
    };                                                                
#define SET    0    /* defines for use with TitleText */
#define GET    1
#define LENGTH 2

/* Info requried to select a font with SelectVectFont */
struct fontstuff
    {
    char * name;  /* Name of font */
    int vect;     /* 0 for bitmap, 1 for vector font */
    int h;        /* Height of bitmap font */
    int w;        /* Width of bitmap font */
    };

/* Structure passed to pc_paint_thread.  Must be declared as static by the */
/* thread initiating the paint thread.                                     */
struct pc_paint
    {
    ULONG StartPaintSem;    /* Set when repaint desired */
    ULONG StopPaintSem;     /* Used to stop a reapint in progress */
    ULONG EndPaintThread;   /* Terminate the thread */
    SWP   pc_swp;           /* Dimensions of the repaint window */
    struct plot_command * root; /* First plot_command in linked list */
    HWND hwnd;              /* Handle of window to plot in */
    COLOR fgcol;            /* Default foreground colour */
    COLOR bkcol;            /* Default background colour */
    struct fontstuff * fnt; /* Default font - for use with SelectVectFont */
    };



/* Function definitions */
void far cdecl window_thread();
int  cdecl buff_empty(void);
int  cdecl buff_test(void);
int  cdecl buff_getch(void);
int  cdecl buff_tgetch(long);
void cdecl buff_insert(int);
void cdecl buff_strins(char *);
void cdecl vWrtchar(char*,int,HVPS);
BYTE cdecl RgbToVioColor(COLOR);
LONG cdecl VioToRgbColor(BYTE);
BOOL cdecl show_cursor(BOOL,HVPS);
void flash_cursor(HVPS);
void cdecl pm_err(char *);
void cdecl pc_interp(struct plot_command,PSWP,char,HPS);
void cdecl pc_delete(struct plot_command *);
struct plot_command * cdecl pc_save(struct plot_command *,MPARAM,MPARAM); 
void cdecl pm_err(char *);
void cdecl pm_msg(char*,char*);
void       dbprintf(const char far *,...);
void Put_cursor_onscreen(HWND,HVPS);
void CurScrChk(void);

long cdecl SelectFont(HPS,LONG,CHAR*,int,int,int);
void cdecl SetCharBox(HPS,int,int);
HSWITCH cdecl AddToSwitch(HWND, HWND, CHAR*);
void cdecl SetPMGinMode(HWND,MPARAM,HPOINTER);
int  cdecl DoPMGin(HWND,PUSHORT);
BOOL cdecl TestPMGinMode(void);
void cdecl SetGinMode(HWND,int);
void cdecl GetGinCoords(int*,int*,char*);
void cdecl process_avio(struct avio_cellstr *,HVPS);

int numlock_status(void);
int numlock_toggle(void);

MRESULT EXPENTRY FileOpnDlgProc(HWND,USHORT,MPARAM,MPARAM);
MRESULT EXPENTRY FileClsDlgProc(HWND,USHORT,MPARAM,MPARAM);
int FileOpnDlgInit(HWND,MPARAM,char *);
int FileOpnDlgExit(HWND,char*);
int FileOpnDlgCmnd(HWND,int);

/* Block marking and copying routiens */
int do_copy(int,HVPS,HAB);
int do_paste(HAB);

/* Graphics metafile and printing routines - ckopm6.c */
int do_print(HAB,HWND);
int do_meta(HAB,HWND);
int MakeMetaFile(char * metname,HAB hab,HWND hwnd);

