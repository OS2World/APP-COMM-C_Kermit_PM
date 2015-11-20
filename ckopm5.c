/******************************************************************************
File name:  ckopm5.c    Rev: 01  Date: 29-Apr-90 Programmer: C.P.Armstrong

File title: Yet more PM interdace!

Contents:   File open dialog stuff

Modification History:
    01  29-Apr-90   C.P.Armstrong   created

******************************************************************************/
#define INCL_AVIO
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ckorc.h"
#include "ckopm.h"



SHORT ParseFileName (CHAR *pcOut, CHAR *pcIn)
     {
          /*----------------------------------------------------------------
             Input:    pcOut -- Pointer to parsed file specification.
                       pcIn  -- Pointer to raw file specification.
                       
             Returns:  0 -- pcIn had invalid drive or directory.
                       1 -- pcIn was empty or had no filename.
                       2 -- pcOut points to drive, full dir, and file name.

             Changes current drive and directory per pcIn string.
            ----------------------------------------------------------------*/

     CHAR   *pcLastSlash, *pcFileOnly ;
     ULONG  ulDriveMap ;
     USHORT usDriveNum, usDirLen = 64 ;

     strupr (pcIn) ;

               // If input string is empty, return 1

     if (pcIn [0] == '\0')
          return 1 ;

               // Get drive from input string or current drive

     if (pcIn [1] == ':')
          {
          if (DosSelectDisk (pcIn [0] - '@'))
               return 0 ;

          pcIn += 2 ;
          }
     DosQCurDisk (&usDriveNum, &ulDriveMap) ;

     *pcOut++ = (CHAR) usDriveNum + '@' ;
     *pcOut++ = ':' ;
     *pcOut++ = '\\' ;

               // If rest of string is empty, return 1

     if (pcIn [0] == '\0')
          return 1 ;

               // Search for last backslash.  If none, could be directory.

     if (NULL == (pcLastSlash = strrchr (pcIn, '\\')))
          {
      if (!DosChDir (pcIn, 0L))
               return 1 ;

                    // Otherwise, get current dir & attach input filename

          DosQCurDir (0, pcOut, &usDirLen) ;

          if (strlen (pcIn) > 12)
               return 0 ;

          if (*(pcOut + strlen (pcOut) - 1) != '\\')
               strcat (pcOut++, "\\") ;

          strcat (pcOut, pcIn) ;
          return 2 ;
          }
               // If the only backslash is at beginning, change to root

     if (pcIn == pcLastSlash)
          {
      DosChDir ("\\", 0L) ;

          if (pcIn [1] == '\0')
               return 1 ;

          strcpy (pcOut, pcIn + 1) ;
          return 2 ;
          }
               // Attempt to change directory -- Get current dir if OK

     *pcLastSlash = '\0' ;

     if (DosChDir (pcIn, 0L))
          return 0 ;

     DosQCurDir (0, pcOut, &usDirLen) ;

               // Append input filename, if any

     pcFileOnly = pcLastSlash + 1 ;

     if (*pcFileOnly == '\0')
          return 1 ;

     if (strlen (pcFileOnly) > 12)
          return 0 ;

     if (*(pcOut + strlen (pcOut) - 1) != '\\')
          strcat (pcOut++, "\\") ;

     strcat (pcOut, pcFileOnly) ;
     return 2 ;
     }

/******************************************************************************
Function:       FillDirListBox()

Description:    Fills a PM list box with available drives and directories

Syntax:         VOID FillDirListBox (HWND hwnd, CHAR *pcCurrentPath)

Returns:        nothing

Mods:           ??-???-?? C.Petzold created

******************************************************************************/
VOID FillDirListBox (HWND hwnd, CHAR *pcCurrentPath)
     {
     static CHAR szDrive [] = "  :" ;
     FILEFINDBUF findbuf ;
     HDIR        hDir = 1 ;
     SHORT       sDrive ;
     USHORT      usDriveNum, usCurPathLen, usSearchCount = 1 ;
     ULONG       ulDriveMap ;

     DosQCurDisk (&usDriveNum, &ulDriveMap) ;
     pcCurrentPath [0] = (CHAR) usDriveNum + '@' ;
     pcCurrentPath [1] = ':' ;
     pcCurrentPath [2] = '\\' ;
     usCurPathLen = 64 ;
     DosQCurDir (0, pcCurrentPath + 3, &usCurPathLen) ;

     WinSetDlgItemText (hwnd, DID_LG_CD, pcCurrentPath) ;
     WinSendDlgItemMsg (hwnd, DID_DIRLIST, LM_DELETEALL, NULL, NULL) ;

     for (sDrive = 0 ; sDrive < 26 ; sDrive++)
          if (ulDriveMap & 1L << sDrive)
               {
               szDrive [1] = (CHAR) sDrive + 'A' ;

               WinSendDlgItemMsg (hwnd, DID_DIRLIST, LM_INSERTITEM,
                                  MPFROM2SHORT (LIT_END, 0),
                                  MPFROMP (szDrive)) ;
               }

     DosFindFirst ("*.*", &hDir, 0x0017, &findbuf, sizeof findbuf,
                              &usSearchCount, 0L) ;
     while (usSearchCount)
          {
          if (findbuf.attrFile & 0x0010 &&
                    (findbuf.achName [0] != '.' || findbuf.achName [1]))
               
               WinSendDlgItemMsg (hwnd, DID_DIRLIST, LM_INSERTITEM,
                                  MPFROM2SHORT (LIT_SORTASCENDING, 0),
                                  MPFROMP (findbuf.achName)) ;

          DosFindNext (hDir, &findbuf, sizeof findbuf, &usSearchCount) ;
          }
     }

/******************************************************************************
Function:       FillFileListBox()

Description:    Gets fills a PM list box with file names in current
                directory satisfying the selection pattern supplied

Syntax:         VOID FillFileListBox (HWND hwnd,char * fspec)

Returns:        nothing

Mods:           ??-???-?? C.Petzold created
                29-Apr-90 C.P.Armstrong filespec added

******************************************************************************/
VOID FillFileListBox (HWND hwnd,char * fspec)
     {
     FILEFINDBUF findbuf ;
     HDIR        hDir = 1 ;
     USHORT      usSearchCount = 1 ;
     static char fs[80];

     if(fs[0]==0)
        strcpy(fs,"*.*");

     if(fspec!=NULL)
        strcpy(fs,fspec);


     WinSendDlgItemMsg (hwnd, DID_FILELIST, LM_DELETEALL, NULL, NULL) ;

     DosFindFirst (fs, &hDir, 0x0007, &findbuf, sizeof findbuf,
                              &usSearchCount, 0L) ;
     while (usSearchCount)
          {
          WinSendDlgItemMsg (hwnd, DID_FILELIST, LM_INSERTITEM,
                             MPFROM2SHORT (LIT_SORTASCENDING, 0),
                             MPFROMP (findbuf.achName)) ;

          DosFindNext (hDir, &findbuf, sizeof findbuf, &usSearchCount) ;
          }
     }


/******************************************************************************
Function:       FileOpnDlgProc()

Description:    Handles the "Logging..." dialog.
                On receiving the WM_INITDLG message mp2 should point to
                an int value describing the logging mode desired.  This is
                used to set up various features of the dialog, like file name
                extension, dialog title and whether to close or open the
                file.
                Presently;
                    mode = 0  is session logging
                    mode = 1  is transaction logging
                    mode = 2  is debug logging
                    mode = 3  is packet logging

Syntax:         MRESULT EXPENTRY FileOpnDlgProc(hwnd,msg,mp1,mp2)
                    HWND hwnd       Standard details passed to a 
                    USHORT msg      client procedure
                    MPARAM mp1
                    MPARAM mp2

Returns:        0

Mods:           24-Mar-90 C.P.Armstrong created

******************************************************************************/
MRESULT EXPENTRY FileOpnDlgProc(HWND hwnd, USHORT msg, MPARAM mp1,MPARAM mp2)
    {
    static CHAR szBuffer [80] ;
    static CHAR szCurrentPath [80];
    SHORT       sSelect ;

    static char * deffs=NULL;

    static struct dlgopn * pdo;

    switch(msg)
        {
        case WM_INITDLG:
            pdo = PVOIDFROMMP(mp2);
            FileOpnDlgInit(hwnd,mp2,szCurrentPath);
            pdo->name[0]='\0';
            return(0);

        case WM_COMMAND:
            switch(COMMANDMSG(&msg)->cmd)
                {
                case DID_LG_OPCL:      /* Selected the open/close button */
                    if(FileOpnDlgExit(hwnd,szCurrentPath)!=0)
                        return(0);
                    else
                        /* Copy filename to the supplied buffer */
                        strcpy(pdo->name,szCurrentPath);    
                case DID_CANCEL:
                    WinDismissDlg(hwnd,TRUE);
                    pdo=NULL;
                    return(0);
                default:
                    break;
                }
            break;
        case WM_CONTROL:
               if (SHORT1FROMMP (mp1) == DID_DIRLIST ||
                   SHORT1FROMMP (mp1) == DID_FILELIST)
                    {
                    sSelect = (USHORT) WinSendDlgItemMsg (hwnd,
                                                  SHORT1FROMMP (mp1),
                                                  LM_QUERYSELECTION, 0L, 0L) ;

                    WinSendDlgItemMsg (hwnd, SHORT1FROMMP (mp1),
                                       LM_QUERYITEMTEXT,
                                       MPFROM2SHORT (sSelect, sizeof szBuffer),
                                       MPFROMP (szBuffer)) ;
                    }

               switch (SHORT1FROMMP (mp1))             // Control ID
                    {
                    case DID_DIRLIST:
                         switch (SHORT2FROMMP (mp1))   // notification code
                              {
                              case LN_ENTER:
                                   if (szBuffer [0] == ' ')
                                        DosSelectDisk (szBuffer [1] - '@') ;
                                   else
                                        DosChDir (szBuffer, 0L) ;

                                   FillDirListBox (hwnd, szCurrentPath) ;
                                   FillFileListBox (hwnd,NULL) ;

                                   return 0 ;
                              }
                         break ;

                    case DID_FILELIST:
                         switch (SHORT2FROMMP (mp1))   // notification code
                              {
                              case LN_SELECT:
                                   WinSetDlgItemText (hwnd, DID_LG_LOGNME,
                                                      szBuffer) ;
                                   return 0 ;

                              case LN_ENTER:
                                   if(FileOpnDlgExit (hwnd,szCurrentPath) ==0)
                                    {
                                    strcpy(pdo->name,szCurrentPath);
                                    WinDismissDlg (hwnd, TRUE) ;
                                    pdo=NULL;
                                    return 0 ;
                                    }
                              }     /* End of LN_ switch */
                         break ;
                    }   /* End of DID_ switch */
               break ;

            
        default:
            break;
        }
    return(WinDefDlgProc(hwnd,msg,mp1,mp2));
    }
 
/******************************************************************************
Function:       FileOpnDlgInit()

Description:    Initialises the "Logging..." dialog according to the mode it
                has been invoked under, either "session", "transaction",
                "debugging" or "packet"

Syntax:         int FileOpnDlgInit(HWND hwnd,MPARAM mp1,char * path)

Returns:        0

Mods:           25-Mar-90 C.P.Armstrong created

******************************************************************************/
int FileOpnDlgInit(HWND hwnd,MPARAM mp2, char * path)
    {
    char drv[_MAX_DRIVE+_MAX_DIR]; /* Buffers required to break log file name */
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];        /* into it's components.                   */
//    char defext[_MAX_EXT];

    char button[7];
    char * title;
    char * defext;

    struct dlgopn * pdo;
    
    pdo = PVOIDFROMMP(mp2);

    defext = pdo->name;
    title  = pdo->title;

    
    FillDirListBox (hwnd, path) ;       /* Fill the directory box */
    FillFileListBox (hwnd,defext) ;     /* Fill the file box */

    WinSendDlgItemMsg (hwnd, DID_LG_LOGNME, EM_SETTEXTLIMIT,
                             MPFROM2SHORT (80, 0), NULL) ;


    WinSetDlgItemText(hwnd,DID_LG_LOGNME,defext);
    WinSetDlgItemText(hwnd,DID_LG_TITLE,title);
    WinSetDlgItemText(hwnd,DID_LG_OPCL,"Open");
    return(0);
    }    


/******************************************************************************
Function:       FileOpnDlgExit()

Description:    Takes the values entered into the Logging dialog procedure
                and performs the required operation.

Syntax:         int FileOpnDlgExit(HWND hwnd,char * path)

Returns:        0

Mods:           25-Mar-90 C.P.Armstrong created

******************************************************************************/
int FileOpnDlgExit(HWND hwnd,char * path)
    {
    char buf[81];

    WinQueryDlgItemText(hwnd,DID_LG_OPCL,80,buf);
    WinQueryDlgItemText (hwnd, DID_LG_LOGNME,
                      80, buf) ;

    if(strchr(buf,'*')!=NULL)
       {
       FillDirListBox (hwnd, path) ;
       FillFileListBox (hwnd,buf) ;
       return(-1);
       }


    switch (ParseFileName (path, buf))
         {
         case 0:
              WinAlarm (HWND_DESKTOP, WA_ERROR) ;
              FillDirListBox (hwnd, path) ;
              FillFileListBox (hwnd,NULL) ;
              return -1 ;

         case 1:
              FillDirListBox (hwnd, path) ;
              FillFileListBox (hwnd,NULL) ;
              WinSetDlgItemText (hwnd, DID_LG_LOGNME, "") ;
              return -1 ;

         case 2:
             /* Filename is in path.. */
             break;
         }
        
    return(0);
    }


/******************************************************************************
Function:       FileOpnDlgCmnd()

Description:    This routine could be used to check the existence of files and
                directories and provide the user with messages to the effect
                that they do/do not already exist.
                
                Of course the "logging..." dialog could also include a list
                of files in the named directory, and this routine would be
                responsible for updating that list when the current name is
                changed.

Syntax:         int FileOpnDlgCmnd(HWND hwnd,int cmnd)

Returns:        -1 if command not processed.
                0  if command processed.

Mods:           25-Mar-90 C.P.Armstrong created

******************************************************************************/
int FileOpnDlgCmnd(HWND hwnd,int cmnd)
    {
    return(-1);
    }
    
/******************************************************************************
Function:       FileClsDlgProc()

Description:    Simply displays the name and nature of the open file
                and gives the user the option to close the file.  

                On initialisation mp2 should point to a dlgopn structure. 
                On entering .name should have the name of the file to close
                and .title the title of the dialog.

                On exit the first byte of name is set to a non-zero value.  
                0 indicates the user canceled the close request.    

Syntax:         MRESULT EXPENTRY FileClsDlgProc(hwnd,msg,mp1,mp2)
                    mp2 should be a "struct dlgopn *"

Returns:        dlgopn.name[0] == 0  User canceled
                dlgopn.name[0] != 0  User wants to close

Mods:           30-Apr-90 C.P.Armstrong created

******************************************************************************/
MRESULT EXPENTRY FileClsDlgProc(HWND hwnd, USHORT msg, MPARAM mp1,MPARAM mp2)
    {
    static struct dlgopn * pdo;

    switch(msg)
        {
        case WM_INITDLG:
            pdo = PVOIDFROMMP(mp2);
            WinSetDlgItemText(hwnd,DID_LG_LOGNME,pdo->name);
            WinSetDlgItemText(hwnd,DID_LG_TITLE,pdo->title);
            pdo->name[0]='\0';
            return(0);

        case WM_COMMAND:
            switch(COMMANDMSG(&msg)->cmd)
                {
                case DID_LG_OPCL:      /* Selected the open/close button */
                    pdo->name[0]='\1';
                case DID_CANCEL:
                    WinDismissDlg(hwnd,TRUE);
                    pdo=NULL;
                    return(0);
                default:
                    break;
                }
            break;
        }
    return(WinDefDlgProc(hwnd,msg,mp1,mp2));
    }
 

int seslogdlg(HWND hwnd)
    {
    extern int seslog;
    extern char sesfil[];
    extern int sesopn(char*);

    struct dlgopn dlo;

    char fname[81];

    dlo.title = "Session Log";
    dlo.name = fname;

    if(seslog)                 /* Already open - close it  */
       {
       strcpy(fname,sesfil);
       WinDlgBox(HWND_DESKTOP,hwnd,FileClsDlgProc,(HMODULE)0,IDD_LG,&dlo);
       if(fname[0]!='\0')
         seslog=sesopn("");
       }
    else
       {
       strcpy(fname,"*.SES");
       
       /* Call the filename find dialog */
       WinDlgBox(HWND_DESKTOP,hwnd,FileOpnDlgProc,(HMODULE)0,IDD_LG2,&dlo);
       
       if(fname[0]!='\0')
          seslog=sesopn(fname);         
       }
    return(0);
    }


int tralogdlg(HWND hwnd)
    {
    extern int tralog;
    extern char trafil[];
    extern int traopn(char*);

    struct dlgopn dlo;

    char fname[81];

    dlo.title = "Transaction Log";
    dlo.name = fname;

    if(tralog)                 /* Already open - close it  */
       {
       strcpy(fname,trafil);
       WinDlgBox(HWND_DESKTOP,hwnd,FileClsDlgProc,(HMODULE)0,IDD_LG,&dlo);
       if(fname[0]!='\0')
         tralog=traopn("");
       }
    else
       {
       strcpy(fname,"*.TRN");
       
       /* Call the filename find dialog */
       WinDlgBox(HWND_DESKTOP,hwnd,FileOpnDlgProc,(HMODULE)0,IDD_LG2,&dlo);
       
       if(fname[0]!='\0')
          tralog=traopn(fname);         
       }
    return(0);
    }

int deblogdlg(HWND hwnd)
    {
    extern int deblog;
    extern char debfil[];
    extern int debopn(char*);

    struct dlgopn dlo;

    char fname[81];

    dlo.title = "Debugging Log";
    dlo.name = fname;

    if(deblog)                 /* Already open - close it  */
       {
       strcpy(fname,debfil);
       WinDlgBox(HWND_DESKTOP,hwnd,FileClsDlgProc,(HMODULE)0,IDD_LG,&dlo);
       if(fname[0]!='\0')
         deblog=debopn("");
       }
    else
       {
       strcpy(fname,"*.DBG");
       
       /* Call the filename find dialog */
       WinDlgBox(HWND_DESKTOP,hwnd,FileOpnDlgProc,(HMODULE)0,IDD_LG2,&dlo);
       
       if(fname[0]!='\0')
            deblog=debopn(fname);         
       }
    return(0);
    }

int pktlogdlg(HWND hwnd)
    {
    extern int pktlog;
    extern char pktfil[];
    extern int pktopn(char*);

    struct dlgopn dlo;

    char fname[81];
 
    dlo.title = "Packet Log";
    dlo.name = fname;

    if(pktlog)                 /* Already open - close it  */
       {
       strcpy(fname,pktfil);
       WinDlgBox(HWND_DESKTOP,hwnd,FileClsDlgProc,(HMODULE)0,IDD_LG,&dlo);
       if(fname[0]!='\0')
         pktlog=pktopn("");
       }
    else
       {
       strcpy(fname,"*.PKT");
       
       /* Call the filename find dialog */
       WinDlgBox(HWND_DESKTOP,hwnd,FileOpnDlgProc,(HMODULE)0,IDD_LG2,&dlo);
       
       if(fname[0]!='\0')
            pktlog=pktopn(fname);         
       }
    return(0);
    }
