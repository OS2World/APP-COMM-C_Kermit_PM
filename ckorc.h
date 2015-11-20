/******************************************************************************
Header name:  ckorc.h     Rev: 01  Date: 22-Mar-90 Programmer: C.P.Armstrong

Header title: C-Kermit resource header

Description:  Contains the definitions used to reference the resources used in
              C-Kermit for the OS/2 Presentation Manager.

Modification History:
    22-Mar-90 C.P.Armstrong created
    04-May-90 C.P.Armstrong ID_KERMIT changed to 1 so the icon appears in the
                            prog.list under OS/2 1.2 wit RC 1.2

******************************************************************************/

#define     ID_KERMIT       1   /* Main ID for menu,minimized icon and      */
                                /* accel table MUST be 1 for icon to appear */
                                /* on the start progs list!                 */
#define     ID_CROSSPTR     2   /* Cross pointer for Tek4014 gin mode       */

#define     IDM_SET         10  /* Set menu item */
#define     IDM_TERM        20
#define     IDM_COMMS       30
#define     IDM_NETWS       40
#define     IDM_MISC        42

#define     IDM_LOG         50  /* Logging menu item */
#define     IDM_SESSLOG     60
#define     IDM_TRNLOG      70
#define     IDM_DEBUG       80
#define     IDM_PAKT        82

#define     IDM_REMOTE      90  /* Remote command menu item */
#define     IDM_REMDIR      100
#define     IDM_REMCD       110
#define     IDM_REMTYPE     112

#define     IDM_TFR         120 /* File transfer menu item */
#define     IDM_SEND        130
#define     IDM_GET         140
#define     IDM_TAKE        150
#define     IDM_TRANSMIT    160

#define     IDM_EDIT        170
#define     IDM_COPY        180
#define     IDM_APP         190
#define     IDM_COPYPASTE   200
#define     IDM_PASTE       210
#define     IDM_VIEWCLIP    220
#define     IDM_PRINTG      230
#define     IDM_METAG       240

/* Don't modify the stuff below directly.  Use the DLGBOX editor.  */
/* Unfortunatly DLGBOX doesn't understand C comments so the stuff below will */
/* need to be extracted first, the comments and the stuff about DID_CANCEL   */
/* removed.DID_CANCEL is defined in PMWIN.H for some reason.                 */

/* This is from COMS.H */
#ifndef DID_CANCEL
#define DID_CANCEL    301
#endif
#define DID_FILELIST    352
#define DID_DIRLIST     351
#define DID_NODMP       330
#define DID_WPGDMP      329
#define DID_TEKDMP      328
#define DID_HPGLDMP     327
#define DID_XFCDE       311
#define DID_XFCDX       310
#define DID_DTABT8      309
#define DID_DTABT7      308
#define DID_DTABT5      307
#define DID_SPACE       306
#define DID_MARK        305
#define DID_EXIT        300
#define DID_XHND        299
#define DID_HRDHND      298
#define DID_STPBT2      297
#define DID_STPBT15     296
#define DID_STPBT1      295
#define DID_NONE        294
#define DID_ODD         293
#define DID_EVEN        292
#define DID_OCTS        291
#define DID_IDSR        290
#define DID_ODSR        289
#define DID_BAUD        288
#define IDD_COMMS       284
#define DID_1COM        312
#define DID_2COM        313
#define DID_3COM        314
#define DID_LG_TITLE    316
#define DID_LG_OPCL     317
#define DID_LG_CD       318
#define DID_LG_LOGNME   319
#define IDD_LG          320
#define DID_LG_DEFEXT   321
#define IDD_TERM        322
#define DID_VT100       323
#define DID_TEK4014     324
#define DID_TERM7BIT    325
#define DID_TERM8BIT    326
#define IDD_MISC        331
#define DID_PEOL        332
#define IDD_LG2         350
/* End of COMS.H */
