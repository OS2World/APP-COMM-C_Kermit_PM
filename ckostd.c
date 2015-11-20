/******************************************************************************
File name:  ckostd.c    Rev: 01  Date: 31-Jan-90 Programmer: C.P.Armstrong

File title: Replaces some of the standard C stdio commands so that output
            can be directed into an AVIO PM windows.

Contents:   

Modification History:
    01  31-Jan-90   C.P.Armstrong   created

******************************************************************************/
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSSESMGR
#define INCL_WIN
#define INCL_AVIO
#define INCL_VIO
#include <os2.h>     
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <process.h>    /* Process-control function declarations */
#include <malloc.h>
#include <io.h>
#include "ckcker.h"
#include "ckofns.h" 
#include "ckopm.h"

/* These are also defined in CKOCON.C (instead of .h for some reason! */
#define SENDCHAR    sendchar
#define UPWARD      6
#define DOWNWARD    7
typedef int     bool;
typedef unsigned char screenmap[4000];
typedef struct ascreen_rec {    /* Structure for saving screen info */
    unsigned char   ox;
    unsigned char   oy;
    unsigned char   att;
    screenmap       scrncpy;
}               ascreen;


void do_aviotopm(struct avio_cellstr *);
void newses(char*,char*,char*,int*);


/* putchar is used alot in kermit.  Of course it fails under the PM.  So here */
/* is a PM version.  Note that it will do a CRLF for an LF.                   */
/* NB.  For MSC 5.1 putchar is a macro and should be undefined.               */
/*      With MSC 6.0 there is a problem somewhere which causes the linker to  */
/*      give a multiply defined symbol error for putchar() (not for printf()  */
/*      or puts() though!                                                     */

int putchar(int c)
    {
    struct avio_cellstr ac;
    
    ac.fun = PRINTF;
    ac.string = (PCH) &c;
    ac.len    = 1;
    do_aviotopm(&ac); 
    return(1);
    }

/* puts() is also used alot - and it fails...... */
int puts(s)
    const char far * s;
    {           
    struct avio_cellstr ac;
    ac.fun = PRINTF;
    ac.string = (char far *) s;
    ac.len = strlen(s);
    do_aviotopm(&ac); 
    return(1);
    }


/* Replaces write(stdout...) function calls */
int write_buf(char * s, int x)
    {
    struct avio_cellstr ac;
    ac.fun = PRINTF;
    ac.string = s;
     ac.len = x;
    do_aviotopm(&ac); 
    return(x);
    }

/******************************************************************************
Function:       pm_system()

Description:    Spawns a new OS/2 command line interpreter, passing a command
                line if required.  The output from the command interpreter is
                displayed on an AVIO screen.

Syntax:         int do_os2_cmd(command)
                    char * command;

Returns:        The number of characters entered into the buffer

Mods:           12-Sep-89 C.P.Armstrong created
                13-Sep-89 C.P.Armstrong modified to ensure that all the handles
                                        opened are closed prior to exit from the
                                        function.  Surprisingly this
                                        has to include the duplicate handles.
                05-Dec-89 C.P.Armstrong A Modified do_os2_cmd()
******************************************************************************/
int system(command)
    const char * command;
    {
    HFILE ipiper;          /* Pipe handles to receive from DOS */
    HFILE ipipew;
    HFILE opiper;          /* Pipe handles to send to DOS */
    HFILE opipew;

    int hdoscapt;
    USHORT cpid,tpid;

    HFILE ostdout;  /* Keep record of original stdin/out */
    HFILE ostdin;
    HFILE ostderr;

    RESULTCODES resc,temp;
    USHORT end;

    ULONG  ramsem=0;                /* Semaphore handle */
    ULONG  wramsem=0;
    USHORT wend,wlen;
    USHORT errcode,werrcode;
    char * buffer;    
    int  length=80;
    char far * pipeinput = buffer;  /* offset into buffer at which to */
                                    /* place pipeinput */      
    int polen=80;
    char far * pipeout;
    char c;

    if(stricmp("CMD",command)==0)
        {
        newses("cmd.exe","OS/2 Command Line Interpreter","",& (int) errcode);
        return(0);
        }
    
    /* create the receive and tranmit pipes. Use default pipe size */
    if( ((errcode = DosMakePipe(&ipiper, &ipipew, 0))!=0) ||
        (DosMakePipe(&opiper, &opipew, 0)!=0) )  
        {
        printf("Can't redirect DOS output - Pipe error!");
        close(ipiper);       /* Close these just in case one was opened */
        close(ipipew);
        close(opiper);
        close(opipew);
        return(0);
        }
     
    /* Copy the standard I/O handles - from now on there are 7 files open! */
    ostdout=(HFILE) dup(fileno(stdout)); 
    ostderr=(HFILE) dup(fileno(stderr));
    ostdin = (HFILE) dup(fileno(stdin));

    /* Set the standard I/O handles to be the pipe handles */
    dup2((int) ipipew, fileno(stdout));
    dup2((int) ipipew, fileno(stderr));
    dup2((int) opiper, fileno(stdin));

    /* Spawn the command using something we can test for the end of. */
    /* The standard output and standard error devices should be set  */
    /* to the pipe write handle. */

    cpid = spawnlp(P_NOWAIT, "cmd.exe", "cmd", "/c", command,NULL);

    /* Reset the parents standard out */
    dup2((int) ostdout,fileno(stdout));
    dup2((int) ostderr,fileno(stderr)); 
    dup2((int) ostdin, fileno(stdin));

    close(ostdout);        /* The duplicate standard I/O handles */
    close(ostderr);        /* are finished with now, so remove them */
    close(ostdin);
    if(cpid <= 0)
        {
        printf("Error starting CMD\n");
        close(ipiper);
        close(ipipew);
        close(opiper);
        close(opipew);
        return(0);
        }
   
    if((buffer = (char far *)malloc(length))==NULL)
        {
        close(ipiper);
        close(ipipew);
        close(opiper);
        close(opipew);
        return(0);
        }
 
    if((pipeout = (char far *)malloc(polen))==NULL)
        {
        free(buffer);
        close(ipiper);
        close(ipipew);
        close(opiper);
        close(opipew);
        return(0);
        }


    pipeinput = buffer;
    wlen=0;
    pipeout[0]='\0';

    /* Read data in the pipe until the task finishes */
    do
        {
        DosSemSet(&ramsem);                 /* Set the semaphore */

        end=0;
        /* Read string from pipe asynchronously */

        if(DosReadAsync(ipiper,             /* Handle of file */
                     &ramsem,               /* Clear semaphore when finished */
                     &errcode,              /* Return error code */
                     pipeinput,             /* buffer */
                     length-1, &end)!=0)    /* length and # bytes read */
            {
            printf("Problem reading DOS output");
            break;
            }
                     
        /* Now wait for either the end of the process or the semaphore to */
        /* be cleared */
        tpid=cpid+1;  /* Dummy value! */
        while((cpid!=tpid)&& (DosSemWait(&ramsem,50L)==ERROR_SEM_TIMEOUT))
            {
            /* See if process has finished */
            DosCwait(DCWA_PROCESSTREE, DCWW_NOWAIT, &temp, &tpid, cpid);
            if( (c=(char)buff_tgetch(50L))>0)
                {
                if((c==13) || (wlen>(polen-2)))
                    {
                    pipeout[wlen++]=10;
                    pipeout[wlen]=0;
                    DosSemSet(&wramsem);
                    if( DosWriteAsync(opipew,&wramsem,&werrcode,pipeout,wlen,&wend)!=0)
                        printf("\nProblem writing DOS input\n");
                    wlen=0;
                    }
                else if(c==8)
                    {
                    if(wlen>0)
                        {
                        wlen--;
                        printf("\b \b");
                        }
                    }
                else
                    {
                    printf("%c",c);
                    pipeout[wlen]=c;
                    wlen++;
                    }   /* End of char handling ifs */
                }   /* End of getch if */
            }  /* End write while */

        pipeinput[end] = '\0';              /* Terminate the string */

        /* PM multi-thread compatible */
        AVIOwrttyc(pipeinput,end);
        }while(cpid!=tpid);
                                                                      
    if(cpid!=tpid)                          /* Make sure process is dead */
         {
         printf("Killing command process\n");
         DosKillProcess(DKP_PROCESSTREE, cpid);
         }
         
    close(ipiper);       /* Make sure everything is closed otherwise we run */
    close(ipipew);       /* out of file handles pretty damn quick */
    close(opiper);
    close(opipew);
    free(pipeout);
    free(buffer);

    return(pipeinput-buffer);   /* Return with the # of bytes read */
    }

/* There are too many printf's to go replacing them all with something     */
/* compatible with the AVIO or PM.  So the only thing to do is to re-write */
/* printf to work in the AVIO or PM. The only disadvantage to this is that */
/* I have no idea how to calculate the length of the buffer required to put*/
/* the formatted string in.  Needless to say I'm not going to actually     */
/* rewrite printf, vsprintf will do nicely!! The formatted string can then */
/* be passed to something that can print to the AVIO screen.  Unfortunately*/
/* only the VioWrtTTY function is really compatible with printf but it     */
/* writes characters and some default attribute.  The other functions wrap */
/* at the end of a line but do not scroll at the end of the screen         */
/* another Microsoft fuck-up or do they blame IBM.                         */

int far printf(format,...)
    const char* format;
    {
    struct avio_cellstr ac;
    va_list varg_ptr;        /* Pointer to the variable argument list */
    char buffer[512];        /* Buffer used to store the formatted string*/
    int len;

    va_start(varg_ptr,format);   /* Get start of optional argument list*/

//    if((buffer = (char far *) malloc(sizeof(char)*512))==NULL)
//        return(0);                       /* Max 512 char output*/

    vsprintf(buffer,format,varg_ptr);    /* Put formatted string into buffer */

    len = strlen(buffer);
    
    ac.fun = PRINTF;
    ac.string = (PCH) buffer;
    ac.len    = len;
    do_aviotopm(&ac); 
//    free(buffer);
    return(len);
    }
    
#include <signal.h>

void (far cdecl * far cdecl os_signalA( hand ) ) ()
    void (far cdecl *hand)();
    {
    PFNSIGHANDLER oldsig;
    USHORT oldaction;
    USHORT error;

    
    error = DosSetSigHandler((PFNSIGHANDLER)hand,&oldsig,&oldaction,SIGA_ACCEPT,
                SIG_PFLG_A);
    
    if(error)
        {
        printf("Error setting alarm signal handler\n");
        return(SIG_ERR);
        }
   return((void (far *)())oldsig);
   }
   
void perror(sc)
    const char far * sc;
    {
    char far * s;
    s = (char *) sc;
    printf("%s",_strerror(s));
    }
    
/******************************************************************************
Function:       newses()

Description:    Creates a new OS/2 session.  The parameters for the session
                should be modified before calling this routine.  The default
                paramters create an indepentant PM windowed session with a
                screen size for use with the EDC scan programs.

Syntax:         void newses(program, title, input, error)
                    char * program; Program file name (-path)
                    char * title;   String to be used as session title
                    char * input;   input parmas for the program
                    int  * error;   Variable for return error

Returns:        Nothing
                Error = 0 for successful creation
                error = 1 if unsuccessful

Mods:           14-Nov-89 C.P.Armstrong created

******************************************************************************/

void newses(program, title, input, error)
    char * program; /* Program file name (-path) */
    char * title;   /* String to be used as session title */
    char * input;   /* input parmas for the program */
    int  * error;   /* Variable for return error */
    {
    USHORT sesid;
    USHORT pid;
    STARTDATA sd;

    sd.Length  =      50;
    sd.Related =     TRUE;  /* Child session */
    sd.FgBg =        FALSE; /* In the foreground */
    sd.TraceOpt =    0;
    sd.TermQ =       0;
    sd.Environment = NULL;
    sd.InheritOpt =  1;
    sd.SessionType = 2;     /* Do it in a window */
    sd.IconFile =    "";
    sd.PgmHandle =   0L;
    sd.PgmControl =  2;     /* Window is maximized */
    sd.InitXPos =    0;
    sd.InitYPos =    0;
    sd.InitXSize =   0;
    sd.InitYSize =   0;


    sd.PgmTitle=title;
    sd.PgmName=program;
    sd.PgmInputs=input;
    
    if(DosStartSession(&sd, &sesid, &pid)!=0)
        *error=1;
    else
        *error=0;
        
    return;
    }
 
                                    
