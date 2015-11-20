/*
 File CKUXLA.H
 Language and Character Set Support for UNIX and VAX/VMS.

 This file should be used as a template for the language support files
 for other C-Kermit implementations -- Macintosh, OS/2, Amiga, etc.

 Author: F. da Cruz, Columbia University, 1990.
*/

/* Codes for local file character sets */

#define FC_USASCII 0   /* North American English */
#define FC_UKASCII 1   /* United Kingdom English */
#define FC_DUASCII 2   /* Dutch ISO 646 NRC */
#define FC_FIASCII 3   /* Finnish ISO 646 NRC */
#define FC_FRASCII 4   /* French ISO 646 NRC */
#define FC_FCASCII 5   /* French Canadian ISO 646 NRC */
#define FC_GEASCII 6   /* German ISO 646 NRC */
#define FC_HUASCII 7   /* Hungarian ISO 646 NRC */
#define FC_ITASCII 8   /* Italian *ISO 646 NRC */
#define FC_NOASCII 9   /* Norwegian and Danish ISO 646 NRC */
#define FC_POASCII 10  /* Portuguese ISO 646 NRC */
#define FC_SPASCII 11  /* Spanish ISO 646 NRC */
#define FC_SWASCII 12  /* Swedish ISO 646 NRC */
#define FC_CHASCII 13  /* Swiss ISO 646 NRC */

/* 8-bit character sets for Unix (ha!) */
#define FC_1LATIN  14  /* ISO 8859-1 Latin Alphabet 1 */
#define FC_DECMCS  15  /* DEC Multinational Character Set */

#define MAXFCSETS 15   /* Highest file character-set number */

