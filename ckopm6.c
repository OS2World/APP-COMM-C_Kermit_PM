/******************************************************************************
File name:  ckopm6.c    Rev: 01  Date: 27-May-90 Programmer: C.P.Armstrong

File title: More kermit PM interface.  Mainly printing and metafile stuff.

Contents:   

Modification History:
    01  27-May-90   C.P.Armstrong   created

******************************************************************************/
#define INCL_GPI    
#define INCL_WIN
#define INCL_AVIO
#define INCL_PIC            /* For the PicPrint function */
#define INCL_DEV
#include <OS2.h>
#include <stdio.h>
#include <string.h>
#include "ckopm.h"
#include "ckorc.h"

#define DEFAULT_META "kerdef.met"

/******************************************************************************
Function:       do_print()

Description:    Prints out the graphics display to the default printer. 
                This is done by creating a metafile and using the OS/2 1.2
                function PicPrint to print the metafile.

                Uses a default file name of DEFAULT_META.  This file is
                deleted before calling the metafile name.

Syntax:         int do_print(HAB hab,HWND hwnd)      
                hab  - threads anchor block
                hwnd - display window handle

Returns:        0 if successful

Mods:           27-May-90 C.P.Armstrong created

******************************************************************************/
int do_print(HAB hab,HWND hwnd)
    {
    unlink(DEFAULT_META);

    if(MakeMetaFile(DEFAULT_META,hab,hwnd)!=0)
        {
        dbprintf("do_print - MakeMetaFile failed\n");
        return(-1);
        }

    /* Try using the new 1.2 PicPrint */
    if(!PicPrint(hab,DEFAULT_META,PIP_MF,NULL))
        {
        dbprintf("FALSE return from PicPrint\n");
        return(-1);
        }
    else
        dbprintf("PicPrint returned TRUE\n");

    return(0);
    }

/******************************************************************************
Function:       MakeMetaFile()

Description:    Writes out the current graphics window display to a metafile.

Syntax:         int MakeMetaFile(char * metname,HAB hab,HWND hwnd)
                    metname - name for the metafile, must not exist already.
                    hwnd - Handle of the window containing the graphics
                    hab  - threads anchor block

Returns:        0 if successful

Mods:           27-May-90 C.P.Armstrong created

******************************************************************************/
int MakeMetaFile(char * metname,HAB hab,HWND hwnd)
    {
    extern struct plot_command pcroot;
  
    struct plot_command * pcn;

    SHORT len;
    DEVOPENSTRUC dop;
    HDC hdcm;
    LONG lrc;
    HPS hpsm;
    HMF hm;
    SIZEL szl;
    SWP swp;

/* The MS.Prog.Ref says only the necessary params must be passed and to pass */
/* 4 for OD_METAFILE.  Initializing only the first 4 causes DevOpenDC to mpf.*/
/* Passing 2 for 4 initialized params alos causes DevOpenDC to mpf.  So far  */
/* it seems that initializing all params and sending 2 as the number required*/
/* works - OS/2 1.2 "November" with no CSDs.                                 */

    /* Open the device context */
    dop.pszLogAddress = NULL;
    dop.pszDriverName = "DISPLAY";
    dop.pdriv = NULL;
    dop.pszDataType = NULL;
    dop.pszComment = NULL;
    dop.pszQueueProcName=NULL;
    dop.pszQueueProcParams=NULL;
    dop.pszSpoolerParams=NULL;
    dop.pszNetworkParams=NULL;


    dbprintf("MakeMetaFile - about to open DC\n");

    if( (hdcm = DevOpenDC(hab, (LONG) OD_METAFILE, "*",
            2L, (PDEVOPENDATA) &dop, NULL)) == DEV_ERROR)
        {
        dbprintf("MakeMetaFile - Failed to open metafile\n");
        return(-1);
        }

    /* Lets associate */
    szl.cx = 0L;
    szl.cy = 0L;
    
    if( (hpsm = GpiCreatePS(hab,hdcm,&szl, 
            PU_PELS | GPIA_ASSOC)) == GPI_ERROR)
        {
        dbprintf("MakeMetaFile - Failed to create metafile PS\n");
        return(-1);
        }
            
    /* Determine page size */
    if(!DevQueryCaps(hdcm,CAPS_WIDTH,2L, (PLONG) &szl))
        {
        dbprintf("MakeMetaFile - Failed to determine page size\n");
        return(-1);
        }
    else
        dbprintf("MakeMetaFile - page size is %ld,%ld\n",szl.cx,szl.cy);
    
    /* Convert to PS units */
    if( GpiConvert(hpsm, CVTC_DEVICE,CVTC_WORLD,1L,(PPOINTL) &szl)!=GPI_OK)
        {
        dbprintf("MakeMetaFile - Failed to convert units\n");
        return(-1);
        }
    else
        dbprintf("MakeMetaFile - converted page size %ld,%ld\n",szl.cx,szl.cy);

    swp.cx = (SHORT) szl.cx;
    swp.cy = (SHORT) szl.cy;

    /* Select a vector font */    
    if( SelectFont(hpsm, 4L,"Helv",1,0,0)!=GPI_OK)
        dbprintf("MakeMetaFile - Error selecting Helv vector font\n");
    else    
        {                                   /* Size the font */
        SetCharBox(hpsm, 
          MAXCHARWIDTH * swp.cx/MAXXRES, 
          MAXCHARHEIGHT * swp.cy/MAXYRES);
        }

    /* Plot loop */
    pcn = &pcroot;
    do
        {
        pc_interp(*pcn,&swp,0,hpsm);
        pcn = pcn->next;
        }
    while(pcn != NULL);
    
    /* Delete PS, close device etc. */
    GpiAssociate(hpsm,NULL);
    hm = DevCloseDC(hdcm);

    if(GpiSaveMetaFile(hm,metname)!=GPI_OK)
        {
        dbprintf("MakeMetaFile - Problem saving the meta file\n");
        }
    GpiDestroyPS(hpsm);

    return(0);
    }
    
/******************************************************************************
Function:       do_meta()

Description:    Saves the current graphics display to a metafile.  The filename
                is chosen by means of the fileopen dialog.
                
                If the a file with the chosen name exists it is deleted before
                the metafile is written.

Syntax:         int do_meta(hab,hwnd)
                    HAB hab;    Threads anchor block
                    HWND hwnd;  Graphics display handle

Returns:        0 if successful

Mods:           27-May-90 C.P.Armstrong created

******************************************************************************/
int do_meta(hab,hwnd)
    HAB hab;
    HWND hwnd;
    {
    struct dlgopn dlo;
    char metname[81];

    dlo.title = "Metafile Name";
    dlo.name = metname;
    strcpy(metname,"*.MET");

    /* Call the filename find dialog */
    WinDlgBox(HWND_DESKTOP,hwnd,FileOpnDlgProc,(HMODULE)0,IDD_LG2,&dlo);
    if(metname[0]=='\0')
       {
       return(0);
       }

    unlink(metname);
    if(MakeMetaFile(metname,hab,hwnd)!=0)
        {
        dbprintf("do_meta - MakeMetaFile failed\n");
        return(-1);
        }
    
    return(0);
    }
    

// The code below can be used to print out using OS/2 1.1, which does not have
// the PicPrint routine.  It was tested using an HP Laserjet II and seemd to
// work okay except for the vector graphics which would cause the program to
// protection fault when the Gpi function to set the font was called.
//    extern struct plot_command pcroot;
//    char defprn[40];
//    char details[256];
//
//    char * driver;
//    char * devnme;    
//    char * logport;
//    struct plot_command * pcn;
//
//    SHORT len;
//    DEVOPENSTRUC dop;
//    HDC hdcp;
//    LONG lrc;
//    HPS hpsp;
//    SIZEL szl;
//    SWP swp;
//    USHORT jobid;
//    LONG jlen;
//    
//    PDRIVDATA pd;

//    /* Get the default printer name */
//    len = WinQueryProfileString(hab,
//            "PM_SPOOLER",
//            "PRINTER",
//            "",
//          defprn,
//            40);
            
//    defprn[len-2]=0;  /* Null teriminate and remove final ";" */
    
                      /* This is the default printer name "PRINTER1" */

//    len = WinQueryProfileString(hab,
//            "PM_SPOOLER_PRINTER",
//            defprn,
//            "",
//            details,
//            256);

//    /* Format of details is ;
//        LPT1;LASERJET;LPT1Q;netinfo;
//       We want the driver and the logical port */
//
//    if( (driver = strchr(details,';'))==NULL)
//        {
//        dbprintf("do_print - Bad details format\n\n");
//        return(-1);
//        }
//
//    driver++;
//    if( (logport = strchr(driver,';'))==NULL)
//        {
//        dbprintf("do_print - Bad driver format\n\n");
//        return(-1);
//        }
//
//    *logport=0;     /* Null terminate driver(s) */
//    logport++;
//    
//    if( strchr(logport,';')==NULL)      /* Strcspn does not give an error */
//        {
//        dbprintf("do_print - Bad port format\n\n");
//        return(-1);                     /* if the character is not found */
//        }
//    else
//        logport[strcspn(logport,";")]=0;
//
//    if( strchr(driver,',')!=NULL)
//        driver[strcspn(driver,",")]=0;
//        
//
//    /* With the Laserjet driver the "details" look like
//        LPT1;LASERJET.Laserjet II;LPT1Q;;
//       The driver is LASERJET.DRV.  The driver needs to know the model, but it
//       doesn't say where this info should be put.  By trial and error I've 
//       found that putting it in a DRIVDATA struct gets printout on my laserjet
//       II.
//    */
//    if( strchr(driver,'.')!=NULL)       /* Get rid of "." if it's there */
//        {
//        devnme = &driver[1+strcspn(driver,".")];
//        /* Nul terminate the driver name */
//        driver[strcspn(driver,".")]=0;
//        }
//    else
//        {
//        devnme=NULL;
//        }
//
//    /* Get driver details */
//    len = DevPostDeviceModes(hab,(LONG) NULL,driver,devnme,
//      logport,DPDM_QUERYJOBPROP);
//    pd = malloc(len*sizeof(LONG));
//    DevPostDeviceModes(hab,pd,driver,devnme,
//      logport,DPDM_QUERYJOBPROP);
//
//
//    dbprintf("Printer device is %s\nPrinter driver is %s\nLogical port is %s\n",
//      devnme,driver,logport);
//    
//    /* Open the device context */
//    dop.pszLogAddress = logport;
//    dop.pszDriverName = driver;
//    dop.pdriv = pd;
//    dop.pszDataType = "PM_Q_STD";
//
// 
//
//    if( (hdcp = DevOpenDC(hab, OD_QUEUED, "*",
//            4L, (PDEVOPENDATA) &dop, NULL)) == DEV_ERROR)
//        { 
//        pm_err("Failed to open printer device");
//        dbprintf("do_print - Failed to open device\n\n");
//        return(-1);
//        }
//
//
//
//    /* Tell it we're starting a job */
//    if( (lrc = DevEscape(hdcp,DEVESC_STARTDOC,
//               (LONG)   strlen("Kermit graph"),
//                        "Kermit graph",
//               (PLONG) NULL, (PBYTE) NULL))!=DEV_OK)
//        {
//        dbprintf("do_print - job start failed\n\n");
//        return(-1);
//        }
//
//    /* Lets associate */
//    szl.cx = 0L;
//    szl.cy = 0L;
//    
//    if( (hpsp = GpiCreatePS(hab,hdcp,&szl, 
//            PU_PELS | GPIF_DEFAULT | GPIT_NORMAL | GPIA_ASSOC)) == GPI_ERROR)
//        {
//        dbprintf("do_print - Failed to create PS\n\n");
//        return(-1);
//        }
//            
//    /* Determine page size */
//    if(!DevQueryCaps(hdcp,CAPS_WIDTH,2L, (PLONG) &szl))
//        {
//        dbprintf("do_print - Failed to determine page size\n\n");
//        return(-1);
//        }
//    else
//        dbprintf("do_print - page size is %ld,%ld\n",szl.cx,szl.cy);
//    
//    /* Convert to PS units */
//    if( GpiConvert(hpsp, CVTC_DEVICE,CVTC_WORLD,1L,(PPOINTL) &szl)!=GPI_OK)
//        {
//        dbprintf("do_print - Failed to convert units\n\n");
//        return(-1);
//        }
//    else
//        dbprintf("do_print - converted page size %ld,%ld\n",szl.cx,szl.cy);
//
//    
//    swp.cx = (SHORT) szl.cx;
//    swp.cy = (SHORT) szl.cy;
//    
////    dbprintf("do_print - Trying to select the Helv vector font\n");
//
//    /* Select a vector font */    
////    if( SelectFont(hpsp, 10L,"Helv",1,0,0)!=GPI_OK)
////        dbprintf("********** do_print - Error selecting Helv vector font\n");
////    else
////        {
////        dbprintf("do_print - Font selected\n");
////        SetCharBox(hpsp,                    /* Size the font */
////          MAXCHARWIDTH * swp.cx/MAXXRES, 
////          MAXCHARHEIGHT * swp.cy/MAXYRES);
////        dbprintf("do_print - Char box set\n");
////        }
//
//    /* Plot loop */
//    pcn = &pcroot;
//    do
//        {
//        pc_interp(*pcn,&swp,0,hpsp);
//        pcn = pcn->next;
//        }
//    while(pcn != NULL);
//
//    dbprintf("do_print - Now let's try to print it\n");
//
//    jlen = sizeof(USHORT);
//    /* Tell queue we've finished document */
//    if( (lrc = DevEscape(hdcp,DEVESC_ENDDOC,
//                         0L, (PBYTE) NULL,
//                 (PLONG) &jlen, (PBYTE) &jobid))!=DEV_OK)
//        {
//        dbprintf("do_print - Failed end print job\n");
//        }
//
//    /* Delete PS, close device etc. */
//    GpiAssociate(hpsp,NULL);
//    DevCloseDC(hdcp);
//    GpiDestroyPS(hpsp);
//
//    if(pd!=NULL)
//        free(pd);
//
//    dbprintf("do_print - Normal exit\n\n");
//    return(0);
