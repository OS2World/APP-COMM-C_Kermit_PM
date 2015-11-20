/*  C K U C M D . H  --  Header file for Unix cmd package  */
 
/*
 Author: Frank da Cruz (fdc@columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 First released January 1985.
 Copyright (C) 1985, 1990, Trustees of Columbia University in the City of New 
 York.  Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained. 
*/
 
/* Special getchars... */
 
#ifdef vax11c
#define getchar()   vms_getchar()
#endif
 
#ifdef aegis
#undef getchar
#define getchar()   coninc(0)
#endif
 
#ifdef AMIGA
#undef getchar
#define getchar() coninc(0)
#endif

/* Sizes of things */
 
#define HLPLW   78			/* Width of ?-help line */
#define HLPCW   19			/* Width of ?-help column */
#define HLPBL  100			/* Help string buffer length */
#define ATMBL  256			/* Command atom buffer length*/
#define CMDBL 1024			/* Command buffer length */
 
/* Special characters */
 
#define RDIS 0022			/* Redisplay   (^R) */
#define LDEL 0025			/* Delete line (^U) */
#define WDEL 0027			/* Delete word (^W) */
 
/* Keyword table flags */
 
#define CM_INV 1			/* Invisible keyword */
 
/* Token flags */

#define CMT_COM 0			/* Comment (; or #) */
#define CMT_SHE 1			/* Shell escape (!) */
#define CMT_LBL 2			/* Label (:) */
#define CMT_FIL 3			/* Indirect filespec (@) */

/* Keyword Table Template */
 
struct keytab {				/* Keyword table */
    char *kwd;				/* Pointer to keyword string */
    int val;				/* Associated value */
    int flgs;				/* Flags (as defined above) */
};

/* Macro support */

struct mtab {				/* Like keyword table */
    char *kwd;				/* But with pointers for vals */
    char *val;				/* instead of ints. */
    int flgs;
};

/* Maximum number of macro definitions allowed */
#define MAC_MAX 256
