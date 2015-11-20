/******************************************************************************
Header name:  ckotek.h    Rev: 01  Date: 02-Mar-90 Programmer: C.P.Armstrong

Header title:   Definitions used for Tektronix emulation

Description:    

Modification History:
    01  02-Mar-90   C.P.Armstrong   created

******************************************************************************/
#define HI_X_Y   0x20
#define LO_X     0x40
#define LO_Y_EX  0x60

/* Input processing modes */
#define ALPHANUM    0       /* Lines of text */
#define ESCMODE     1
#define VECTMODE    2       /* Line draw mode */
#define ALPHAGRA    3       /* Text at graphics position */
#define GINMODE     4
#define PVECTMODE   5       /* Point plot mode */
#define INCREMODE   6       /* Incremental point plotting mode */
#define BYPASS      7

/* ESC command types */
#define SQBRA  1
#define QUOTE  2

/* Nominal maximum plot resolution */       /* These are also in ckopm.h */
#define MAXXRES             1024
#define MAXYRES              780
#define MAXCHARHEIGHT         15
#define MAXCHARWIDTH          14

/* Buffer sizes */
#define PROCBUFSIZE 10

/* Plot dump file formats */
#define NODMP   0       /* No dump file */
#define HPGL    1       /* HPGL format dump file */
#define WPG     2       /* WordPerfect format dump file */
#define TEK     3       /* Tektronix format dump file */

/* Plot dump output commands */
#define HPGL_COM "print hpgl.plt"



int  Tek_scrinit(int);     /* Initialises the system for Tektronix input */
void Tek_finish(void);
void Tek_process(unsigned); /* Tektronix input processing */
int  Tek_gsprocess(char);   /* Processes the GS and FS commands */
int  Tek_escprocess(char);  /* Processes TEK escape commands */
void Tek_vectdecode(int*,int*);  /* Converts TEK position string into coords */
void Tek_gmove(int,int);         /* Does a move in TEK coords */
void Tek_gdraw(int,int);         /* Does a draw in TEK coords */
void Tek_gplot(int,int);         /* Plots a point in TEK coords */
void Tek_write(char);            /* Puts a string on the TEK display */
void Tek_page(void);             /* Clears the TEK display */
void Tek_ginend(void);           /* Ends GIN input */
void Tek_ginini(void);           /* Starts GIN input */
void Tek_endesc(void);           /* Exits escape processing mode */
void Tek_ginencode(int,int,char,char,char*); /* TEK coords to TEK string */
void Tek_setlinetype(char);      /* Sets the line style */
int  Tek_incmode(char);          /* Processes TEK incremental mode */
void Tek_status(void);           /* Not implimented */
void Tek_termid(void);           /* Not implimented */
void Tek_poly(int);              /* Controls use of polyline buffer */
void Tek_changemode(char);       /* Switches processing modes */

int   gfile_dump(void);
void  gfile_move(int, int);
void  gfile_draw(int,int);
void  gfile_plot(int,int);
void  gfile_rewind(void);
void  gfile_mover(int,int);
void  gfile_drawr(int,int);
void  gfile_plotr(int,int);
void  gfile_print(char *);
int   gfile_open(void);
void  gfile_close(void);
void  gfile_init(void);
int   fileopen(char *);
