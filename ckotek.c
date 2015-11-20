/******************************************************************************
File name:  ckogra.c    Rev: 01  Date: 08-Dec-89 Programmer: C.P.Armstrong

File title: Contains the Tektronix emulation code

Contents:   

Modification History:
    01  08-Dec-89   C.P.Armstrong   created

******************************************************************************/
#define INCL_DOS
#define INCL_WIN
#define INCL_GPIPRIMITIVES
#define INCL_AVIO
#include <OS2.h>
#include <string.h>
#include <stdlib.h>

#include "ckcasc.h"
#include "ckcker.h"
#include "ckopm.h"
#include "ckotek.h"
#include "ckokey.h"

#define POST_DELAY 100L     /* Duration of pause after failure to post a */
                            /* graphics command */

void vt100(unsigned char);
void ipadl25(void);


int far Term_mode=VT100;    /* Default mode is VT100 */
int far Alpha_mode=1;       /* Write at g cursor or t cursor position */
char far process_mode=0;    /* Are we in the midst of processing ESC or */
                            /* vector or nothing */
char far last_process=0;    /* Escape commands don't exit FS or GS mode */
char far first_vec=0;
int far indx=0;             /* Index into character processing buffer */
int far tekg_x=0;           /* Current tek coords of current graphics */
int far tekg_y=0;           /* position */
int far teka_x=0;
int far teka_y=0;           /* Current tek alphanumeric coords */
        
int far dump_plot=1;        /* Dump file of any plotting for printing */
int far dump_format=NODMP;  /* Default dump file format is HPGL */

SHORT   poly_plot_count;    /* Poly line plotting variables */
PPOINTL poly_plot_array;
char draw_mode;

char far gin_terminator=CR; /* Character send at end of GIN address */
char far mouse_but1='C';
char far mouse_but2=' ';
char far mouse_but3='E';


char process_buf[PROCBUFSIZE];   /* buffer for string being processed */

/******************************************************************************
Function:       Tek_scrinit()

Description:    Initialises the system for doing Tektronix emulation.
                Does nothing for OS/2 PM

Syntax:         int Tek_scrinit(page)
                    int page;  Clear screen if 1, don't if 0

Returns:        0 if can't do Tektronix
                1 if init was sucessful

Mods:           08-Dec-89 C.P.Armstrong created
                30-Jan-90 C.P.Armstrong Updates VT100 line 25

******************************************************************************/
int Tek_scrinit(page)
    int page;
    {
    extern int dump_plot;
    extern int tekg_x,tekg_y;
    extern int teka_x,teka_y;
    extern char *usertext;

    /* Under OS/2 a graphics card is virtually mandatory - and the PM window */
    /* is already alive.  We could hide the VT100 screen though.             */

    /* Open up the plot dump file - if required */
    if(dump_plot)
        gfile_open();


    if(page)
        Tek_page();

    return(1);
    }
    
/******************************************************************************
Function:       Tek_finish()

Description:    Finishes Tek emulation mode. Closes the dump file if necessary.
                And any other Tek exiting things that are required.

Syntax:         void Tek_finish()

Returns:        nothing

Mods:           02-Mar-90 C.P.Armstrong created

******************************************************************************/
void Tek_finish()
    {
    extern int Term_mode;
    extern int dump_plot;
    
    Term_mode = VT100;
    
    if(dump_plot)
        gfile_close();

    }

/******************************************************************************
Function:       Tek_process()

Description:    Processes characters sent from rdserwrtscr().

Syntax:         void Tek_process(c)
                    unsigned c;  The character to be processed

Returns:        nothing

Mods:           08-Dec-89 C.P.Armstrong created

******************************************************************************/
void Tek_process(c)
    unsigned c;  /* The character to be processed */
    {
    extern char first_vec;
    extern char process_mode;
    extern char last_process;
    extern int indx;
    extern HWND cdecl hwndGFrame;
    
    int rejected=0;

    do
        {
        switch(c)           /* Look for special characters first */
            {
            case (VK_SHPRINTSCRN<<8):  /* Shift printscreen button */
                gfile_dump();
                return;
            case SYN:       /* Delay characters */
            case NUL:
                return;
            case RS:
                Tek_changemode(INCREMODE);
                return;
            case FS:
                indx=0;
                first_vec=0;
                Tek_changemode(PVECTMODE);
                return;
            case GS:
                first_vec=1;
                indx=0;
                Tek_changemode(VECTMODE);
                return;
            case US:
                Tek_changemode(ALPHAGRA);
                indx=0;
                return;
            case CR:
                if(process_mode==GINMODE)
                    Tek_ginend();

                if(process_mode!=ALPHANUM)
                    {
                    Tek_changemode(ALPHANUM);
                    indx=0;
                    return;
                    }
                 
                 break;
            case ESC:
                Tek_changemode(ESCMODE);
                indx=0;
                return;
            default:
                break;
            }

        switch(process_mode)                /* It's not a special character */
            {
            case VECTMODE:
            case PVECTMODE:
                if(c & 0x60)  /* Vector mode is exited by sending a */
                    rejected=Tek_gsprocess((char)c);/* non-Tek coord character*/
                else                                /* or if the process      */
                    rejected=1;                     /* buffer is full.        */

                if(rejected)               /* Enter default mode and reprocess*/
                    {                      /* character.                      */
                    indx=0;
                    Tek_changemode(ALPHANUM); 
                    }
                break;
            case ESCMODE:
                rejected = Tek_escprocess((char) c);
                break;
            case BYPASS:
            case GINMODE:       /* Input is ignored if in GIN mode */
                break;
            case INCREMODE:
                rejected = Tek_incmode((char)c);
                break;
            case ALPHANUM:
            case ALPHAGRA:
            default:
                Tek_write((char)c);
                rejected = 0;   /* If all else fails print the character */
            }       /* End of process_mode switch */
        }while(rejected==1);    /* try reprocessing the character */
    return;
    }


/******************************************************************************
Function:       Tek_gsprocess()

Description:    Receives and decodes tektronix plot codes.
                Also handles the dot drawing mode.

Syntax:         int Tek_gsprocess(c)
                    char c;  A component of a Tek plot code

Returns:        0 if no error, 1 if invalid character

Mods:           08-Dec-89 C.P.Armstrong created
                11-Feb-90 C.P.Armstrong FS mode handling added

******************************************************************************/

int Tek_gsprocess(c)
    char c;  /* A component of a Tek plot code */
    {
    extern char process_buf[];
    extern char first_vec;
    extern int indx;  /* Index into the process buf */

    int x,y;            /* New x and y coords to go to */

    /* Ignore spurious CRs and LFs */
    if((c==CR) || (c==LF))
        return(0);
    else if(indx<PROCBUFSIZE)
        process_buf[indx]=c;
    else
        return(1);

    /* Was it a LO_X */
    if((process_buf[indx] & LO_Y_EX)!=LO_X)
        {
        indx++;
        return(0);
        }
    Tek_vectdecode(&x,&y);

    if(process_mode == PVECTMODE)
        {
        Tek_gplot(x,y);
        }
    else
        {
        if(first_vec)        /* First vector after GS is address to move to */
            {
            Tek_gmove(x,y);
            first_vec=0;
            }
        else
            {
            if(draw_mode!=1) /* Turn poly plotting on */
                Tek_poly(1);
            Tek_gdraw(x,y);
            }
        }        
    indx=0;
    return(0);
    }

/******************************************************************************
Function:       Tek_vectdecode()

Description:    Converts the tek 4/5 characters into X and Y coordinates.

Syntax:         void Tek_vectdecode(x,y)
                    int * x; Returned x coord
                    int * y; Returned y coord

Returns:        nothing

Mods:           11-Dec-89 C.P.Armstrong

******************************************************************************/
void Tek_vectdecode(x,y)
    int * x; /* Returned x coord */
    int * y; /* Returned y coord */
    {
    extern char process_buf[];
    static char hi_x=0;     /* These are statics as less than 4 bytes can be */
    static char hi_y=0;     /* send.  Under these cricumstances the previous */
    static char lo_x=0;     /* values for the unsent bytes must be used. I   */
    static char lo_y=0;     /* think!                                        */
    static char lsbxy=0;

    int had_y=0;
    int had_lx=0;
    int had_lsbxy=0;
    int flag=0;
    int hi_res_mode=0;
    int pbi=0;
    
    /* Abbreviated address decoding regime ;
        Only HI Y changes       Hi Y and Lo X sent
        Only LO Y changes       Lo Y and Lo X sent
        Only Hi X changes       Lo Y Hi X and Lo X sent
        Only Lo X changes       Lo X sent

    There is no way of knowing whether the address mode is 1024 or 4096.  This
    is a pity as the decoding methods are different.  One requires expanding
    by 32, the other by 128.
    */

    pbi=0;

    do                                              /* Whoever dreamed this */
        {                                           /* up should be shot    */
        flag = process_buf[pbi] & LO_Y_EX;
        if(flag==HI_X_Y)
            {
            if(had_y)
                hi_x=process_buf[pbi];
            else
                {
                hi_y=process_buf[pbi];
                had_y=1;
                }
             }
        else if(flag==LO_Y_EX)
            {
            if((process_buf[pbi+1] & LO_Y_EX)==LO_Y_EX)
                {
                lsbxy=process_buf[pbi];
                lo_y=process_buf[pbi+1];
                had_lsbxy=1;
                pbi++;
                }
             else                       
                {                       
                lo_y=process_buf[pbi];
                }
             had_y=1;
             }
        else if(flag==LO_X)
            {
            lo_x=process_buf[pbi];
            had_lx=1;
            }
        pbi++;
    }while((!had_lx) && pbi<PROCBUFSIZE);

    /* Convert the separate bytes to x,y coords */
    *x = (hi_x-32)*32 + (lo_x-64);
    *y = (hi_y-32)*32 + (lo_y-96);

    /* The hires mode bytes sent for 4096 by 4096 mode effectively ignored */
    /* as they cause X and Y to multiplied by 4.  Since the max values used*/
    /* by the emulator are 1024 X and Y would only have to be divided by   */
    /* 4, thus losing the LSBXY bits.  However the code below could be used*/
    /* to put all values into 4014 space, should 4096 by 4096 ever be      */
    /* resolvable.                                                         */
//    if(!had_lsbxy)      /* lsbxy is always received in 4014 mode.  So if it */
//        lsbxy=0;        /* wasn't received make sure that it is zero.       */
//    *x = (*x << 2) + (lsbxy&3);
//    *y = (*y << 2) + ((lsbxy&12)>>2);

    return;
    }

/******************************************************************************
Function:       Tek_gmove()

Description:    Does a Tek graphics move

Syntax:         void Tek_gmove(x,y)
                    int x,y;  Coords to move to

Returns:        nothing

Mods:           11-Dec-89 C.P.Armstrong created

******************************************************************************/

void Tek_gmove(x,y)
    int x,y;  /* Coords to move to */
    {
    extern int dump_plot;
    extern int tekg_x,tekg_y;
    extern HWND cdecl hwndGFrame;
    while(!WinPostMsg(hwndGFrame,WM_USER,MPFROM2SHORT(x,y),MPFROMCHAR('m')))
        {
        DosSleep(POST_DELAY);
        }
    tekg_x = x;
    tekg_y = y;
    
    if(dump_plot);
        gfile_move(x,y);
    }
    
/******************************************************************************
Function:       Tek_gdraw()

Description:    Draws a line from the current graphics position to the
                point x,y.

Syntax:         void Tek_gdraw(x,y)
                    int x,y;  Coord to draw line to

Returns:        nothing

Mods:           11-Dec-89 C.P.Armstrong created

******************************************************************************/
void Tek_gdraw(x,y)
    int x,y;  /* Coord to draw line to */
    {
    extern int dump_plot;
    extern int tekg_x,tekg_y;
    extern HWND cdecl hwndGFrame;
    extern char draw_mode;
    extern SHORT poly_plot_count;
    extern PPOINTL poly_plot_array;  /* Pointer to array of POINTLs */

    if(draw_mode==0)
        {
        while(!WinPostMsg(hwndGFrame,WM_USER,MPFROM2SHORT(x,y),
          MPFROMCHAR('d')))
            {
            DosSleep(POST_DELAY);
            }
        }

    else
        {
        poly_plot_array = (PPOINTL) realloc(poly_plot_array,
            sizeof(POINTL)*(poly_plot_count+1));
        poly_plot_array[poly_plot_count].x=x;
        poly_plot_array[poly_plot_count].y=y;
        poly_plot_count++;
        }
        
    tekg_x = x;
    tekg_y = y;
    if(dump_plot)
        gfile_draw(x,y);
    }

/******************************************************************************
Function:       Tek_gplot()

Description:    Plot a point at position point x,y.

Syntax:         void Tek_gplot(x,y)
                    int x,y;  Coord to draw line to

Returns:        nothing

Mods:           11-Dec-89 C.P.Armstrong created

******************************************************************************/
void Tek_gplot(x,y)
    int x,y;  /* Coord to plot at */
    {
    extern int dump_plot;
    extern int tekg_x,tekg_y;
    extern HWND cdecl hwndGFrame;
    while(!WinPostMsg(hwndGFrame,WM_USER,MPFROM2SHORT(x,y),MPFROMCHAR('d')))
        {
        DosSleep(POST_DELAY);
        }
    tekg_x = x;
    tekg_y = y;
    if(dump_plot)
        gfile_plot(x,y);
    }



/******************************************************************************
Function:       Tek_page()

Description:    Clears the Tektronix emulation screen - the Gpi window - and
                homes the graphics cursor.

Syntax:         void Tek_page()

Returns:        nothing

Mods:           12-Dec-89 C.P.Armstrong created

******************************************************************************/
void Tek_page()
    {
    extern int dump_plot;
    extern int tekg_x,tekg_y;
    extern HWND cdecl hwndGFrame;
    while(!WinPostMsg(hwndGFrame,WM_USER,0,MPFROMCHAR('c')))
        {
        DosSleep(POST_DELAY);
        }

    Tek_gmove(0,0);
    tekg_x = 0;
    tekg_y = 0;

    if(dump_plot)
        {
        gfile_rewind();
        gfile_move(0,0);
        }

    }

/******************************************************************************
Function:       Tek_escprocess()

Description:    Processes escape strings

Syntax:         int Tek_escprocess(c)
                    char c;

Returns:        0 if no error, 1 if invalid character

Mods:           11-Dec-89 C.P.Armstrong dummy created
                01-Feb-90 C.P.Armstrong GIN processing added
                29-Apr-90 C.P.Armstrong Tek mode exit via esc sequence updates
                                        status line.
******************************************************************************/
int Tek_escprocess(c)
    char c; 
    {
    extern char process_buf[];
    extern char process_mode;
    extern char last_process;
    extern int indx;  /* Index into the process buf */
    static char esctype;

    switch(c)
        {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case '`':
            Tek_setlinetype(c);
            break;

        case 'E':
            Tek_status();
            break;
        case 'Z':
            Tek_termid();
            break;
        case CR:
        case LF:
            return(0);
        case FF:
            Tek_page();
            break;
        case SUB:                   /* Control-Z */
            Tek_ginini();           /* Start gin mode */
            return(0);              /* Do not resotor previous mode */
        case '[':
            esctype=SQBRA;
            return(0);              /* Command incomplete - get the rest */
        case '"':
            esctype=QUOTE;
            return(0);              /* command incomplete - get the rest */
        default:
            if(indx<PROCBUFSIZE)
                {
                process_buf[indx]=c;
                indx++;
                }
            else    /* If buffer is full then quit escaping */
                break;

            switch(esctype)
                {
                case SQBRA:
                    if(c>='a' && c<='z')  /* ESC [ commands end in a letter */
                        {
                        process_buf[min(indx,PROCBUFSIZE)]='\0';
                        if(strcmp(process_buf,"?38l")==0)
                            {
                            Tek_finish();
                            ipadl25();  /* Reset the status lines */
                            esctype=0;
                            }
                        }       /* End of test for end of command received */
                    else        /* Not got end of command so get the rest */
                        return(0);
                    break;
                case QUOTE:
                    if(c=='g')
                        {
                        if(process_buf[indx-2]=='4' || 
                            process_buf[indx-2]=='5')
                            Tek_ginini();
                        }
                    else
                        return(0);  /* Not got end of command so get the rest */
                    break;
                default:
                    break;
                }           /* End of multiple char interpretation */
            break;
            }               /* End of main esc process switch */

    /* Only completed ESC commands get here - so terminate escape processing */
    /* and restore the previous mode.                                        */
    Tek_endesc();
    return(0);
    }
    
/******************************************************************************
Function:       Tek_write()

Description:    Prints a character in Tek alphanumeric mode and advances the
                alpha cursor to the next position.

Syntax:         void Tek_write(c)
                    char c;  The character to display

Returns:        nothing

Mods:           11-Dec-89 C.P.Armstrong dummy created

******************************************************************************/
void Tek_write(c)
    char c;  /* The character to display */
    {
    extern HWND cdecl hwndGFrame;
    extern int tekg_x,tekg_y;
    extern char process_mode;
    MPARAM mp1;
    char cell[2];
    cell[1]=0;
    cell[0]=c;

    switch(process_mode)
        {
        case ALPHAGRA:
            mp1 = MPFROMP(cell);
            while(!WinPostMsg(hwndGFrame,WM_USER,mp1,MPFROMCHAR('s')))
                {
                DosSleep(POST_DELAY);
                }
            
            if(dump_plot)
                gfile_print(cell);

            tekg_x+=MAXCHARWIDTH;  /* This depends on the character size in use */
            Tek_gmove(tekg_x,tekg_y);
            break;
        case ALPHANUM:
            /* For time being send it to Vio screen using vt100 write mode */
            vt100(c);
            break;
        default:
            break;
        }
    return;
    }
    
/******************************************************************************
Function:       Tek_endesc()

Description:    Terminates ESC sequence processing and returns to the previous
                processing mode.

Syntax:         void Tek_endesc()

Returns:        nothing

Mods:           11-Feb-90 C.P.Armstrong created

******************************************************************************/
void Tek_endesc()
    {
    extern char process_mode;
    extern char last_process;
    extern int indx;

    Tek_changemode(last_process);
    indx=0;
    return;
    }


/******************************************************************************
Function:       Tek_ginini()

Description:    Puts the Tektronix emulator into GIN mode.  This causes a
                cross hair cursor to be displayed on the graphics screen.
                When the user presses a button or presses a key, the coords
                of the mouse are read and sent to the host.  The coords, if
                echoed by the host, or not printed on screen.  The reading and
                encoding of the coords are dealt with by the PM thread.  This
                inserts the values into the keyboard buffer where it is read
                and sent by the vt100read routine.

Syntax:         void Tek_ginini()

Returns:        nothing

Mods:           01-Feb-90 C.P.Armstrong created

******************************************************************************/
void Tek_ginini()
    {
    extern char process_mode;
    extern int indx;
    extern HWND cdecl hwndGFrame;
    
    if(process_mode == GINMODE)
        return;
    else
        Tek_changemode(GINMODE);
        
    indx=0;
    
    /* Tell the PM thread to put to enter GIN mode */
    SetGinMode(hwndGFrame,1);
    
    return;
    }

/******************************************************************************
Function:       Tek_ginend()

Description:    Puts the Tektronix emulator back into alphanumeric mode,
                removes the cross hair cursor and enables echoing of characters
                if necessary.

Syntax:         void Tek_ginend()

Returns:        nothing

Mods:           01-Feb-90 C.P.Armstrong created

******************************************************************************/
void Tek_ginend()
    {
    extern char process_mode;
    extern int indx;
    extern HWND cdecl hwndGFrame;

    if(process_mode != GINMODE)
        return;

    indx=0;
    
    /* Tell the PM thread to enter GIN mode */
    SetGinMode(hwndGFrame,0);
    
    return;
    }

/******************************************************************************
Function:       Tek_ginencode()

Description:    Encodes a graphics position into Tektronix coords for
                sending as a GIN response.
                                                                
                N.B. The buffer to receive the encoded coords must be at least
                7 characters long, 4 for the coords, 1 for the char and 1 for
                the GIN terminator and 1 for the NUL terminator.

Syntax:         Tek_ginencode(x,y,c,gin_term,buf)
                    int x,y;    Coords to encode
                    char c,gin_term ;    Character to be sent, GIN terminator
                    char * buf; Buffer to receive encoded coords

Returns:        nothing

Mods:           01-Feb-90 C.P.Armstrong created

******************************************************************************/
void Tek_ginencode(x,y,c,gin_term,buf)
    int x,y;    /* Coords to encode */
    char c,gin_term ;    /* Character to be sent */
    char * buf; /* Buffer to receive encoded coords */
    {
    buf[0] = c;
    buf[1] = (char) ( (x / 32) + 32 );             /* Hi-X */
    buf[2] = (char) ( (x % 32) + 32 );             /* Lo-X */
    buf[3] = (char) ( (y / 32) + 32 );             /* Hi-Y */
    buf[4] = (char) ( (y % 32) + 32 );             /* Lo-Y */
    buf[5] = gin_term;
    buf[6]=0;
    return;
    }
    
/******************************************************************************
Function:       Tek_setlinetype()

Description:    Sets the line type used when drawing lines.
                The Tek line style characters are;
                    a   fine dots
                    b   short dashes
                    c   dash dot
                    d   long dash dot
                    e   dash dot dot
                    `   solid line
                By a stroke of luck these coincide almost exactly with the 
                Gpi line styles.

Syntax:         void Tek_setlinetype(c)
                    char c;     Tek line type character

Returns:        nothing

Mods:           11-Feb-90 C.P.Armstrong created

******************************************************************************/
void Tek_setlinetype(c)
    char c;     /* Tek line type character */
    {
    extern HWND cdecl hwndGFrame;
    LONG gpi_linestyle;
    switch(c)
        {
        case 'a' :  gpi_linestyle=LINETYPE_DOT;break;
        case 'b' :  gpi_linestyle=LINETYPE_SHORTDASH;break;
        case 'c' :  gpi_linestyle=LINETYPE_DASHDOT;break;
        case 'd' :  gpi_linestyle=LINETYPE_LONGDASH;break;
        case 'e' :  gpi_linestyle=LINETYPE_DASHDOUBLEDOT;break;
        case '`' :  gpi_linestyle=LINETYPE_SOLID;break;
        default  :  gpi_linestyle=LINETYPE_SOLID;break;
        }

    while(!WinPostMsg(hwndGFrame,WM_USER,MPFROMLONG(gpi_linestyle),
      MPFROMCHAR('l')))
        {
        DosSleep(POST_DELAY);
        }
    }

/******************************************************************************
Function:       Tek_incmode()

Description:    Processes the Tek increment mode characters.

Syntax:         int Tek_incmode(c)
                    char c;  Tek increment chartacters

Returns:        0 if a valid character, 1 otherwise

Mods:           11-Feb-90 C.P.Armstrong created

******************************************************************************/
int Tek_incmode(c)
    char c;  /* Tek increment chartacters */
    {
    extern int dump_plot;
    extern int tekg_x,tekg_y;
    extern HWND cdecl hwndGFrame;

    static char pen=' ';
    
    switch(c)
        {
        case 'P':           /* The "Pen" commands */
        case ' ':  pen=c;
            return(0);
        case 'A': tekg_x++;         break;
        case 'E': tekg_x++;tekg_y++;break;
        case 'D': tekg_y++;         break;
        case 'F': tekg_x--;tekg_y++;break;
        case 'B': tekg_x--;         break;
        case 'J': tekg_x--;tekg_y--;break;
        case 'H': tekg_y--;         break;
        case 'I': tekg_x++;tekg_y--;break;
        default:
            return(1);
        }    
    if(pen==' ')
        {
        while(!WinPostMsg(hwndGFrame,WM_USER,MPFROM2SHORT(tekg_x,tekg_y),
          MPFROMCHAR('m')))
            {
            DosSleep(POST_DELAY);
            }
        if(dump_plot)
            gfile_move(tekg_x,tekg_y);
        }
    else
        {
        while(!WinPostMsg(hwndGFrame,WM_USER,MPFROM2SHORT(tekg_x,tekg_y),
          MPFROMCHAR('p')))
            {
            DosSleep(POST_DELAY);
            }
        if(dump_plot)
            gfile_plot(tekg_x,tekg_y);
        }

    return(0);
    }

/******************************************************************************
Function:       Tek_status()

Description:    Not implimented yet

Syntax:         void Tek_status()

Returns:        

Mods:           

******************************************************************************/
void Tek_status()
    {
    return;
    }

/******************************************************************************
Function:       Tek_termid()

Description:    Not implimented yet

Syntax:         void Tek_termid()

Returns:        nothing

Mods:           

******************************************************************************/
void Tek_termid()
    {
    return;
    }
    
/******************************************************************************
Function:       Tek_poly()

Description:    Handles the starting and finishing of polyline drawing mode.
                In theory every Tek draw command can be a polyline.  Of course
                most of them are simple single line draws.

Syntax:         void Tek_poly(mode)
                    int mode;  0 for end, 1 for start polyline drawing

Returns:        nothing

Mods:           14-Jan-90 C.P.Armstrong created
                13-Feb-90 C.P.Armstrong Modified for Kermit
******************************************************************************/
void Tek_poly(mode)
    int mode;  /* 0 for end, 1 for start polyline drawing */
    {
    extern PPOINTL poly_plot_array;  /* Pointer to array of POINTLs */
    extern SHORT   poly_plot_count;  /* no. of POINTLs in array     */
    extern char    draw_mode;        /* are we doing a poly line    */
    extern HWND cdecl hwndGFrame;
    
    if(mode==1 && draw_mode==0)
        {
        poly_plot_count=0;
        draw_mode=1;
        poly_plot_array=(PPOINTL) malloc(0);  /* Set up a block */
        }
     else if(mode==0 && draw_mode==1)
        {
        draw_mode=0;            /* Don't bother doing a poly line for 1 point */
        if(poly_plot_count==1)
            {
            Tek_gdraw((int)poly_plot_array[0].x,(int)poly_plot_array[0].y);
            free(poly_plot_array);
            }
        else            
            {
            while(!WinPostMsg(hwndGFrame,WM_USER,
              MPFROMP(poly_plot_array),
              MPFROM2SHORT((SHORT)'y',poly_plot_count)))
                {
                DosSleep(POST_DELAY);
                }
            }
        }
     
     return;
     }


/******************************************************************************
Function:       Tek_changemode()

Description:    Changes the processing mode to the mode supplied.  This function
                should be used as it checks to see if the current mode is the
                Vector mode.  If it is then the polyline plotting buffer is
                closed and sent to the PM for processing.

Syntax:         void Tek_changemode(new_mode)
                    int new_mode;  New mode to select

Returns:        nothing

Mods:           13-Feb-90 C.P.Armstrong created

******************************************************************************/

void Tek_changemode(new_mode)
    char new_mode;  /* New mode to select */
    {
    extern char process_mode;
    extern char last_process;
    extern char draw_mode;
    
    if((process_mode==VECTMODE) && (draw_mode==1))
        Tek_poly(0);
        
    last_process = process_mode;
    process_mode=new_mode;
    }
    
    
