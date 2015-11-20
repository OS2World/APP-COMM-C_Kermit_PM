/******************************************************************************
File name:  ckodrv.c    Rev: 01  Date: 02-Mar-90 Programmer: C.P.Armstrong

File title: Drivers for different types of plot dumping.

Contents:   

Modification History:
    01  02-Mar-90   C.P.Armstrong   created

******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>      /* This for FILE */
#include <fcntl.h>      /* These two     */
#include <io.h>         /* for open()    */
#include <sys\types.h>
#include <sys\stat.h>
#include <process.h>
#include "ckcasc.h"
#include "ckotek.h"


int     hpgl_open( void );
void    hpgl_move(int,int);
void    hpgl_draw(int,int);
void    hpgl_mover(int,int);
void    hpgl_drawr(int,int);
void    hpgl_plot(int,int);
void    hpgl_plotr(int,int);
void    hpgl_close(void);
void    hpgl_init(void);
void    hpgl_print(char *);
void    hpgl_rewind(void);
int     hpgl_dump(void);

extern int dump_plot;
extern int dump_format;

char command[80];

/* Screendump definitions */
#define SCRDMPCOM "detach print screen"
#define SCRDMP    "screen"


/* The HPGL specific globals */
FILE far * hpgl_stream=NULL;                /* Stream handle */
int  far hpgl_pen=4;
char far hpgl_file[]={"HPGL.PLT"};          /* Default output filename */

/******************************************************************************
Function:       print_screen()

Description:    Dumps a character screen out to the printer.

Syntax:         void print_screen(screen_buffer)
                    char far * screen_buffer;  Pointer to the screen buffer

Returns:        nothing

Mods:           04-Mar-90 C.P.Armstrong created

******************************************************************************/
void print_screen(screen_buffer)
    char far * screen_buffer;
    {
    char line_buffer[83];
    int dump_hand;
    int charloop,lineloop;
    int offset;

    line_buffer[80]=CR;
    line_buffer[81]=LF;
    line_buffer[82]=0;
    
    if((dump_hand = fileopen(SCRDMP))==0)
        return;

    for(lineloop=0;lineloop<24;lineloop++)
        {
        offset = lineloop*160;
        for(charloop=0;charloop<80;charloop++)
            {
            line_buffer[charloop] = screen_buffer[offset+(charloop<<1)];
            }
        write(dump_hand,line_buffer,82);
        }
    
    line_buffer[0]=26;
    write(dump_hand,line_buffer,1);
    close(dump_hand);

    /* Spawn an asynchronous system command - don't use the kermit version */
    /* of system().                                                        */
    spawnlp(P_NOWAIT, "cmd.exe", "cmd", "/c", SCRDMPCOM,NULL);
//    system(SCRDMPCOM);
    return;
    }


/******************************************************************************
Function:       gfile_dump()

Description:    Outputs the dump file to the plotter or whatever.

Syntax:         void gfile_dump()

Returns:         The error code from the relevant dump file or -999 if an
                invalid plot mode is selected.

Mods:           15-Jul-89 C.P.Armstrong created
                27-Feb-90 C.P.Armstrong Error codes

******************************************************************************/
int  gfile_dump()
    {
    extern int dump_plot;
    extern int dump_format;
    int error;
    
    if(!dump_plot)      /* Not dumping plot so can't print it out */
        return(0);

    switch( dump_format )
        {
        case HPGL:
            return(hpgl_dump());
        case WPG:
//            return(wpg_dump());
        case TEK:
//            return(tek_dump());
        case NODMP:
        default:
            break;
        }

    return(-999);
    }

/******************************************************************************
Function:       gfile_rewind()

Description:    Moves the file pointer back to the begining and rewrites
                the initialisation header.

Syntax:         void gfile_rewind()

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_rewind()
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_rewind();
            break;
        case WPG:
//            wpg_rewind();
//            break;
        case TEK:
//            tek_rewind();
        case NODMP:
        default:
            break;
        }
    return;
    }

/******************************************************************************
Function:       gfile_print()

Description:    Prints a label at the current pen position.  To make this
                compatible with the screen movement functions it is assumed
                that the character is drawn from the upper left corner.

Syntax:         void gfile_printf(string)
                    char * string;  The nul terminated string to print

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created
******************************************************************************/
void  gfile_print(string)
    char * string;
    
    {            
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_print(string);
            break;
        case WPG:
//            wpg_print(string);
//            break;
        case TEK:
//            tek_print(string);
        case NODMP:
        default:
            break;
        }
    return;
    }

/******************************************************************************
Function:     gfile_plot()

Description:  HPGL plot a point command

Syntax:     void gfile_plot(x,y)
                int x,y;  Point to plot at

Returns:      nothing

Mods:       15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_plot(x,y)
    int x,y;    /* Position to plot at - in cartesian screen units */
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_plot(x,y);
            break;
        case WPG:
//            wpg_plot(x,y);
//            break;
        case TEK:
//            tek_plot(x,y);
        case NODMP:
        default:
            break;
        }
 
    return;
    }
                        

/******************************************************************************
Function:       gfile_plotr()

Description:    HPGL plot a point relative to current position

Syntax:         void gfile_plotr(dx,dy)
                    int dx,dy;  Point relative to current position to plot at

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_plotr(dx,dy)
    int dx,dy;    /* Position to move pen to - in plotter suitable units */
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_plotr(dx,dy);
            break;
        case WPG:
//            wpg_plotr(dx,dy);
//            break;
        case TEK:
//            tek_plotr(dx,dy);
        case NODMP:
        default:
            break;
        }
    return;
    }

/******************************************************************************
Function:       gfile_move()

Description:    Move to position

Syntax:         void gfile_move(x,y)
                    int x,y;  Position to move to

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_move(x,y)
    int x,y;    /* Position to move pen to - in plotter suitable units */
    {
    extern int dump_format;

    switch( dump_format )
        {
        case HPGL:
            hpgl_move(x,y);
            break;
        case WPG:
//            wpg_move(x,y);
//            break;
        case TEK:
//            tek_move(x,y);
        case NODMP:
        default:
            break;
        }

    return; 
    }


/******************************************************************************
Function:       gfile_draw()

Description:    HPGL Draw a line to a point

Syntax:         void gfile_draw(x,y)
                    int x,y;  Point to draw to

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_draw(x,y)
    int x,y;    /* Position to draw to - in plotter suitable units */
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_draw(x,y);
            break;
        case WPG:
//            wpg_draw(x,y);
//            break;
        case TEK:
//            tek_draw(x,y);
        case NODMP:
        default:
            break;
        }
 
    return; 
    }
                
/******************************************************************************
Function:       gfile_mover()

Description:    HPGL Move to a point realtive to the current position

Syntax:         void gfile_mover(dx,dy)
                    int dx,dy;  Relative point to move to

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_mover(dx,dy)
    int dx,dy;    /* Position to move pen to - in plotter suitable units */
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_mover(dx,dy);
            break;
        case WPG:
//            wpg_mover(dx,dy);
//            break;
        case TEK:
//            tek_mover(dx,dy);
        case NODMP:
        default:
            break;
        }
 
    return;
    }


/******************************************************************************
Function:       gfile_drawr()

Description:    HPGL Draw to a point realtive to the current position

Syntax:         void gfile_drawr(dx,dy)
                    int dx,dy;  Relative point to draw to

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_drawr(dx,dy)
    int dx,dy;    /* Position to move pen to - in plotter suitable units */
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_drawr(dx,dy);
            break;
        case WPG:
//            wpg_drawr(dx,dy);
//            break;
        case TEK:
//            tek_drawr(dx,dy);
        case NODMP:
        default:
            break;
        }
 
    return;
    }


/******************************************************************************
Function:       gfile_open()

Description:    Open the dump file and initialise it

Syntax:         int gfile_open()

Returns:        NULL if file open unsuccessful
                1 if open successful

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
int  gfile_open()
    {
    extern int dump_format;
    int yeaorney;
    switch( dump_format )
        {
        case HPGL:
            yeaorney=hpgl_open();
            break;
        case WPG:
//            yeaorney=wpg_open();
//            break;
        case TEK:
//            yeaorney=tek_open();
        case NODMP:
        default:
            break;
        }

    return(yeaorney);
    }


/******************************************************************************
Function:       gfile_init()

Description:    Writes the HPGL initialisation header onto the dump file

Syntax:         void gfile_init()

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_init()
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_init();
            break;
        case WPG:
//            wpg_init();
//            break;
        case TEK:
//            tek_init();
        case NODMP:
        default:
            break;
        }
 
    return;
    }


/******************************************************************************
Function:       gfile_close()

Description:    Closes the HPGL dump file

Syntax:         void gfile_close()

Returns:        nothing

Mods:           15-Jul-89 C.P.Armstrong created

******************************************************************************/
void  gfile_close()
    {
    extern int dump_format;
    switch( dump_format )
        {
        case HPGL:
            hpgl_close();
            break;
        case WPG:
//            wpg_close();
//            break;
        case TEK:
//            tek_close();
        case NODMP:
        default:
            break;
        }
 
    return; 
    }


/****************************************************************************
Function:       fileopen()
 
Description:    Opens a BINARY file regardless of whether it already exists.

Syntax:         int fileopen( fname )
                    char * fnmae;        Name of file to be opened

Returns:        The file handle of the opened file
 
Rev:            7-3-88      CPArmstrong     created
*****************************************************************************/
                                               
int  fileopen(fname)

char* fname;  /* Pointer to the name of the file to be opened */

    {
    int fhout; /* File handle */

    if((fhout = open(fname,O_EXCL|O_CREAT|O_BINARY|O_WRONLY,S_IREAD|S_IWRITE)) 
                == -1)
        {
        if((fhout = open(fname,O_TRUNC|O_BINARY|O_RDWR)) == -1)
            {
            return(-1);
            }
        }
    /* If the output file already exits it is written over */
    
    return(fhout);
    }

/******************************************************************************
Function:       hpgl_dump()

Description:    Dumps the HPGL.PLT file out using the system command specified
                above.

Syntax:         int hpgl_dump()

Returns:        0  if successful
                -1 if file reopen fails.

Mods:           25-Jun-89 C.P.Armstrong created
                27-Feb-90 C.P.Armstrong Returns error codes

******************************************************************************/
int hpgl_dump()
    {
    extern char command[];      /* Declared in gfiledrv.c */
    extern char hpgl_file[];
    extern FILE * hpgl_stream;

    fpos_t filepos;   /* File pointer value before writing */
                      /* This is the way it's done in the example!!! */

    if(hpgl_stream==NULL)
        return(0);
    
    strcpy(command,HPGL_COM);

    fgetpos(hpgl_stream,&filepos);  /* Save file position */

    fprintf(hpgl_stream,"PG;\n");   /* Page eject command */

    hpgl_close();                   /* Close the file */
    
    system(command);                /* Command to print the file */
    
    if((hpgl_stream=fopen(hpgl_file,"r+"))==NULL) /* Modify - file must exist */
        {
        return(-1);
        }
    
    fsetpos(hpgl_stream,&filepos);  /* Reposition file pointer */
    
    return(0);
    }

/******************************************************************************
Function:   hpgl_rewind()

Description:    Moves the file pointer back to the begining and rewrites
                the plotter initialisation.

Syntax:     void hpgl_rewind()

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_rewind()
    {
    extern FILE * hpgl_stream;

    if(hpgl_stream==NULL)
        return;

    /* Go back to begining */
    fseek(hpgl_stream,0L,SEEK_SET);
    
    /* Re init */
    hpgl_init();
    
    return;
    }



/******************************************************************************
Function:   hpgl_print()

Description:    Prints a label at the current pen position.  To make this
                compatible with the screen movement functions it is assumed
                that the character is drawn from the upper left corner.  In
                reality the plotter draws characters from the bottom left.
                This function therefore does a relative move down before
                printing, followed by a relative move up after printing.

Syntax:     void hpgl_printf(string)
                    char * string;  The nul terminated string to print

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created
******************************************************************************/
void  hpgl_print(string)
    char * string;
    
    {            
    extern FILE * hpgl_stream;
    char etx=3;  /* ASCII ETX - label terminator */

    if(hpgl_stream==NULL)
        return;
    /* Move down */
//  hpgl_mover(0,-chrhit());
     
    /* Do label */
    fprintf(hpgl_stream, "LB%s%c",string,etx);
    
    /* Move back up */
//  hpgl_mover(0,chrhit());
    
    return;
    }





/******************************************************************************
Function:     hpgl_plot()

Description:  HPGL plot a point command

Syntax:     void hpgl_plot(x,y)
                int x,y;  Point to plot at

Returns:      nothing

Mods:       25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_plot(x,y)
    int x,y;    /* Position to plot at - in plotter suitable units */
    {
    extern FILE * hpgl_stream;
    if(hpgl_stream!=NULL)
        fprintf(hpgl_stream,"PA;PU%d,%d;PD;PU\n",x,y);
    return;
    }
                        

/******************************************************************************
Function:   hpgl_plotr()

Description:    HPGL plot a point relative to current position

Syntax:     void hpgl_plotr(dx,dy)
                    int dx,dy;  Point relative to current position to plot at

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_plotr(dx,dy)
    int dx,dy;    /* Position to move pen to - in plotter suitable units */
    {
    extern FILE * hpgl_stream;
    if(hpgl_stream!=NULL)
        fprintf(hpgl_stream,"PR;PU%d,%d;PD;PU\n",dx,dy);
    return;
    }


/******************************************************************************
Function:   hpgl_move()

Description:    HPGL move to position

Syntax:     void hpgl_move(x,y)
                    int x,y;  Position to move to

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_move(x,y)
    int x,y;    /* Position to move pen to - in plotter suitable units */
    {
    extern FILE * hpgl_stream;
    if(hpgl_stream!=NULL)
        fprintf(hpgl_stream,"PA;PU%d,%d\n",x,y);
    return;
    }


/******************************************************************************
Function:   hpgl_draw()

Description:    HPGL Draw a line to a point

Syntax:     void hpgl_draw(x,y)
                    int x,y;  Point to draw to

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_draw(x,y)
    int x,y;    /* Position to draw to - in plotter suitable units */
    {
    extern FILE * hpgl_stream;
    if(hpgl_stream!=NULL)
        fprintf(hpgl_stream,"PA;PD%d,%d\n",x,y);
    return;
    }
                
/******************************************************************************
Function:   hpgl_mover()

Description:    HPGL Move to a point realtive to the current position

Syntax:     void hpgl_mover(dx,dy)
                    int dx,dy;  Relative point to move to

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_mover(dx,dy)
    int dx,dy;    /* Position to move pen to - in plotter suitable units */
    {
    extern FILE * hpgl_stream;
    if(hpgl_stream!=NULL)
        fprintf(hpgl_stream,"PR;PU%d,%d\n",dx,dy);
    return;
    }


/******************************************************************************
Function:   hpgl_drawr()

Description:    HPGL Draw to a point realtive to the current position

Syntax:     void hpgl_drawr(dx,dy)
                    int dx,dy;  Relative point to draw to

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_drawr(dx,dy)
    int dx,dy;    /* Position to move pen to - in plotter suitable units */
    {
    extern FILE * hpgl_stream;
    if(hpgl_stream!=NULL)
        fprintf(hpgl_stream,"PR;PD%d,%d\n",dx,dy);
    return;
    }


/******************************************************************************
Function:   hpgl_open()

Description:    Open the HPGL dump file and initialise it

Syntax:     int hpgl_open()

Returns:        NULL if file open unsuccessful
                1 if open successful

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
int  hpgl_open()
    {
    extern FILE * hpgl_stream;
    extern char hpgl_file[];

    int hpgldump_hand;          /* Dump file handle */

    if(hpgl_stream!=NULL)       /* Open already - preserve status */
        return(1);

    /* Open the dump file */
    if( (hpgldump_hand=fileopen(hpgl_file)) == -1)
        {
        return(0);        
        }      /* File open filaure */


    /* Convert handle for use with fprintf etc.. */
    /* Note that the b option results in there being no CRLF combinations in */
    /* the file.  ME deals with this okay but TYPE requires the CR to display*/
    /* properly */

    if( (hpgl_stream=fdopen(hpgldump_hand,"wb+")) == NULL)
        {
        return(0);        
        }           /* Handle conversion error */

    /* Plotter initialise */
    hpgl_init();

    return(1);
    }


/******************************************************************************
Function:   hpgl_init()

Description:    Writes the HPGL initialisation header onto the dump file

Syntax:     void hpgl_init()

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_init()
    {
    extern int hpgl_pen;
    extern FILE * hpgl_stream;

    float chrscalw;   /* Chr width as %age of screen width */
    float chrscalh;   /* Chr height as %age of screen height */
    float t1,t2;      /* temporary values */

    if(hpgl_stream==NULL)
        return;

    fprintf(hpgl_stream,"IN\n");

    /* Set scale factor so full page is sameas full screen */
    fprintf(hpgl_stream,"SC%d,%d,%d,%d\n",
        0,MAXXRES-1,0,MAXYRES-1);

    /* The 0.8 factor is to allow for a space between characters.  The screen */
    /* width includes a space factor normally whereas the HPGL command does */
    /* not allow for any space */
    t1 = (float) MAXCHARWIDTH * 0.8;
    t2 = (float) MAXXRES;
    chrscalw = (t1/t2) *100.0;

    t1 = (float) MAXCHARHEIGHT * 0.8;
    t2 = (float) MAXYRES;
    chrscalh = (t1/t2) *100.0;

    /* Set the character size to correspond to the SCREEN character size */
    fprintf(hpgl_stream,"SR%f,%f\n", chrscalw,chrscalh);

    /* Select a pen size - required for HP Laserjet Plotter Emulation   */
    /* The SPn,w is specific to the Laserjet Emulation cartridge.  This */
    /* set the pen number to the same pen as the width. Valid widths    */
    /* are 1 to 48 and pens 1 to 20.                                    */
    fprintf(hpgl_stream,"SP%d,%d;\n",hpgl_pen,hpgl_pen);

    /* Home the pen */
    hpgl_move(0,0);
    
    return;
    }


/******************************************************************************
Function:   hpgl_close()

Description:    Closes the HPGL dump file

Syntax:     void hpgl_close()

Returns:        nothing

Mods:           25-Jun-89 C.P.Armstrong created

******************************************************************************/
void  hpgl_close()
    {
    extern FILE * hpgl_stream;

    if(hpgl_stream==NULL)
        return;

    /* Write an end-of-file */
    fputc(26,hpgl_stream);

    /* Close the file and reset hpgl_stream to NULL */
    fclose(hpgl_stream);
    hpgl_stream=NULL;
    }
