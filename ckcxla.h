/* File ckcxla.h -- Character-set-related definitions, system independent */

/* Codes for Kermit Transfer Syntax Level */

#define TS_L0 0		 /* Level 0 (normal ASCII) */
#define TS_L1 1		 /* Level 1 (one character set other than ASCII) */
#define TS_L2 2		 /* Level 2 (multiple character sets in same file) */

#define UNK 63		 /* Symbol to use for unknown character (63 = ?) */

/* Codes for languages */

#define L_ASCII       0  /* ASCII, American English */
#define L_USASCII     0  /* ASCII, American English */
#define L_BRITISH     1  /* United Kingdom English */
#define L_DUTCH       2  /* Dutch */
#define L_FINNISH     3  /* Finnish */
#define L_FRENCH      4  /* French */
#define L_FR_CANADIAN 5  /* French Canadian */
#define L_GERMAN      6  /* German */
#define L_HUNGARIAN   7  /* Hungarian */
#define L_ITALIAN     8  /* Italian */
#define L_NORWEGIAN   9  /* Norwegian */
#define L_PORTUGUESE 10  /* Portuguese */
#define L_SPANISH    11  /* Spanish */
#define L_SWEDISH    12  /* Swedish */
#define L_SWISS      13  /* Swiss */
#define L_DANISH     14  /* Danish */
#define MAXLANG      15  /* Number of languages */

/* Ones below are not used yet */
#define L_RUSSIAN    15
#define L_JAPANESE   16
#define L_CHINESE    17
#define L_KOREAN     18
#define L_ARABIC     19
#define L_HEBREW     20
#define L_GREEK      21
#define L_TURKISH    22
/* Obviously this list needs to be expanded and organized */

/* Designators for 8-bit single-byte ISO and other standard character sets */
/* to be used in Kermit's transfer syntax.  Note that symbols must be unique */
/* in the first 8 characters, because some C preprocessors have this limit. */

#define TC_NORMAL  0  /* Normal traditional ordinary Kermit transfer syntax */
#define TC_USASCII 0  /* (for convenience) */
#define USASCII    0  /* (for convenience) */
#define TC_1LATIN  1  /* ISO 8859-1, Latin-1 */
#define MAXTCSETS  1  /* Highest transfer character-set number */

/* The ones below are not used yet... */
#define TC_2LATIN  2  /* ISO 8859-2, Latin-2 */
#define TC_3LATIN  3  /* ISO 8859-3, Latin-3 */
#define TC_4LATIN  4  /* ISO 8859-4, Latin-4 */
#define TC_5LATIN  5  /* ISO 8859-9, Latin-5 */
#define TC_CYRILL  6  /* ISO-8859-5, Latin/Cyrillic */
#define TC_ARABIC  7  /* ISO-8859-6, Latin/Arabic */
#define TC_GREEK   8  /* ISO-8859-7, Latin/Greek */
#define TC_HEBREW  9  /* ISO-8859-8, Latin/Hebrew */
#define TC_CZECH  10  /* Czech Standard */
#define TC_JIS208 11  /* Japanese JIS X 0208 multibyte set */
#define TC_CHINES 12  /* Chinese Standard GB 2312-80 */
#define TC_KOREAN 13  /* Korean KS C 5601-1987 */
#define TC_I10646 14  /* ISO DP 10646 (not defined yet!) */
#define TC_UNICOD 15  /* Unicode (not defined yet!) */
/* and possibly many others... */

/* Structure for character-set information */

struct csinfo {
    char *name;				/* Name of character set */
    int size;				/* Size (128 or 256)     */
    int code;				/* Like TC_1LATIN, etc.  */
    char *designator;			/* Designator, like I2/100 = Latin-1 */
};

/* Structure for language information */

struct langinfo {
    int id;				/* Language ID code (L_whatever) */
    int fc;				/* File character set to use */
    int tc;				/* Transfer character set to use */
    char *description;			/* Description of language */
};

/* Now take in the system-specific definitions */

#ifdef BSD4				/* BSD Unix */
#include "ckuxla.h"
#endif

#ifdef BSD29				/* BSD Unix */
#include "ckuxla.h"
#endif

#ifdef UXIII				/* AT&T UNIX */
#include "ckuxla.h"
#endif

#ifdef V7				/* Bell V7 UNIX */
#include "ckuxla.h"
#endif

#ifdef vms				/* VAX/VMS */
#include "ckuxla.h"
#endif

#ifdef MAC				/* Macintosh */
#include "ckmxla.h"
#endif

#ifdef OS2				/* IBM OS/2 */
#include "ckoxla.h"
#endif

#ifdef AMIGA				/* Commodore Amiga */
#include "ckixla.h"
#endif

#ifdef datageneral			/* Data General MV AOS/VS */
#include "ckdxla.h"
#endif

/* end of ckcxla.c */
