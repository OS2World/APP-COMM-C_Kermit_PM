/*  C K C X L A  */

/*  C-Kermit tables and functions supporting character set translation.  */
/*
 Author: Frank da Cruz (fdc@columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 Copyright (C) 1989, Trustees of Columbia University in the City of New York.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided
 this copyright notice is retained. 
*/

#ifndef NULL
#define NULL 0
#endif

#include "ckcdeb.h"			/* Includes... */
#include "ckcker.h"
#include "ckucmd.h"
#include "ckcxla.h"

/* Character set translation data and functions */

extern int zincnt;
extern CHAR *zinptr;

int tslevel  = TS_L0;			/* Transfer syntax level (0,1,2) */
int tcharset = TC_USASCII;		/* Transfer syntax character set */
int fcharset = FC_USASCII;		/* Local file character set */
int language = L_USASCII;		/* Language */

/* Nota Bene: I6/100 is the proper designation for Latin-1, but MS-DOS */
/* Kermit 3.00 uses I2/100 (this will be fixed in 3.01).  This table */
/* allows C-Kermit to recognize either one when receiving a file, but */
/* when sending, C-Kermit uses I6/100, which MS-DOS Kermit 3.00 will not */
/* recognize.  Workaround: in MS-DOS Kermit, SET TRANSF CHAR LATIN1, */
/* SET ATTR CHAR OFF. */

struct csinfo tcsinfo[] = {		/* Transfer character-set info */
    "ASCII 7-bit text", 256, TC_NORMAL, "",               /* 0 */        
    "Latin-1 ISO 8859-1 text", 256, TC_1LATIN, "I6/100",  /* 1 */
    "Latin-1 ISO 8859-1 text", 256, TC_1LATIN, "I2/100"   /* 2 */
};
int ntcsets = (sizeof(tcsinfo) / sizeof(struct csinfo));

/* Grrr... Had to back off on moving this to ckuxla.h because that file */
/* is included by more than one module, so link complains about multiple */
/* definitions of _fcsinfo, _fcstab, etc. */

/* File character set information structure, indexed by character set code, */
/* as defined immediately above.  This table must be in order of character */
/* set number! */ 

struct csinfo fcsinfo[] = { /* File character set information... */
  /* Descriptive Name              Size  Designator */
    "US ASCII",                     128, FC_USASCII, NULL,
    "British/UK NRC ISO-646",       128, FC_UKASCII, NULL,
    "Dutch NRC ISO-646",            128, FC_DUASCII, NULL,
    "Finnish NRC ISO-646",          128, FC_FIASCII, NULL,
    "French NRC ISO-646",           128, FC_FRASCII, NULL,
    "French-Canadian NRC ISO-646",  128, FC_FCASCII, NULL,
    "German NRC ISO-646",           128, FC_GEASCII, NULL,
    "Hungarian NRC ISO-646",        128, FC_HUASCII, NULL,
    "Italian NRC ISO-646",          128, FC_ITASCII, NULL,
    "Norwegian/Danish NRC ISO-646", 128, FC_NOASCII, NULL,
    "Portuguese NRC ISO-646",       128, FC_POASCII, NULL,
    "Spanish NRC ISO-646",          128, FC_SPASCII, NULL,
    "Swedish NRC ISO-646",          128, FC_SWASCII, NULL,
    "Swiss NRC ISO-646",            128, FC_CHASCII, NULL,
    "ISO Latin-1",                  256, FC_1LATIN,  NULL,
    "DEC Multinational",            256, FC_DECMCS,  NULL
};    

/* Local file character sets */
/* Includes 7-bit National Replacement Character Sets of ISO 646 */
/* Plus ISO Latin-1 and DEC Multinational Character Set (MCS) */

struct keytab fcstab[] = { /* Keyword table for 'set file character-set' */

/* Keyword             Value       Flags */
    "ascii",            FC_USASCII, 0,	/* ASCII */
    "british",          FC_UKASCII, 0,	/* British NRC */
    "dec-mcs",          FC_DECMCS,  0,  /* DEC multinational character set */
    "dutch",            FC_DUASCII, 0,	/* Dutch NRC */
    "finnish",          FC_FIASCII, 0,	/* Finnish NRC */
    "french",           FC_FRASCII, 0,	/* French NRC */
    "fr-canadian",      FC_FCASCII, 0,	/* French Canadian NRC */
    "german",           FC_GEASCII, 0,	/* German NRC */
    "hungarian",        FC_HUASCII, 0,	/* Hungarian NRC */
    "italian",          FC_ITASCII, 0,	/* Italian NRC */
    "latin-1",          FC_1LATIN,  0,	/* ISO Latin Alphabet 1 */
    "norwegian/danish", FC_NOASCII, 0,	/* Norwegian and Danish NRC */
    "portuguese",       FC_POASCII, 0,	/* Portuguese NRC */
    "spanish",          FC_SPASCII, 0,	/* Spanish NRC */
    "swedish",          FC_SWASCII, 0,	/* Swedish NRC */
    "swiss",            FC_CHASCII, 0	/* Swiss NRC */
};
int nfilc = (sizeof(fcstab) / sizeof(struct keytab)); /* size of this table */

/*
 Languages:

 This table serves two purposes.  First, it allows C-Kermit to have a
 SET LANGUAGE command, which automatically selects the associated file
 character set and transfer character set.  Second, it allows the program
 to apply special language-specific rules when translating from a character
 set that contains national characters into plain ASCII, like German umlaut-a
 becomes ae.
*/

struct langinfo langs[] = {
/*  Language code   File Charset Xfer Charset Name */
    L_USASCII,      FC_USASCII,  TC_USASCII,  "ASCII (American English)",
    L_BRITISH,      FC_UKASCII,  TC_1LATIN,   "British (English)",
    L_DANISH,       FC_NOASCII,  TC_1LATIN,   "Danish",
    L_DUTCH,        FC_DUASCII,  TC_1LATIN,   "Dutch",
    L_FINNISH,      FC_FIASCII,  TC_1LATIN,   "Finnish",
    L_FRENCH,       FC_FRASCII,  TC_1LATIN,   "French",
    L_FR_CANADIAN,  FC_FCASCII,  TC_1LATIN,   "French-Canadian",
    L_GERMAN,       FC_GEASCII,  TC_1LATIN,   "German",
    L_HUNGARIAN,    FC_HUASCII,  TC_1LATIN,   "Hungarian",
    L_ITALIAN,      FC_ITASCII,  TC_1LATIN,   "Italian",
    L_NORWEGIAN,    FC_NOASCII,  TC_1LATIN,   "Norwegian",
    L_PORTUGUESE,   FC_POASCII,  TC_1LATIN,   "Portuguese",
    L_SPANISH,      FC_SPASCII,  TC_1LATIN,   "Spanish",
    L_SWEDISH,      FC_SWASCII,  TC_1LATIN,   "Swedish",
    L_SWISS,        FC_CHASCII,  TC_1LATIN,   "Swiss"
};

/* Translation tables ... */

/*
  Note, many more can and should be added.
  Presently we have only ASCII and Latin-1 as transfer character sets
  and ASCII, Latin-1, DEC-MCS, and many ISO-646 NRCs as file character sets
  For each pair of (file,transfer) character sets, we need two translation
  functions, one for sending, one for receiving.  It is recommended that
  functions and tables for all computers be included in this file, preferably
  without #ifdef's, so that corrections need be made only in one place.
*/

/* Here is the first table, fully annotated... */

CHAR
yl1toas[] = {  /* ISO 8859-1 Latin-1 to ascii */
      /*  Source character    Description               => Translation */
      /*  Dec row/col Set
  0,  /*  000  00/00  C0 NUL  Ctrl-@                    =>  (self)  */
  1,  /*  001  00/01  C0 SOH  Ctrl-A                    =>  (self)  */
  2,  /*  002  00/02  C0 STX  Ctrl-B                    =>  (self)  */
  3,  /*  003  00/03  C0 ETX  Ctrl-C                    =>  (self)  */
  4,  /*  004  00/04  C0 EOT  Ctrl-D                    =>  (self)  */
  5,  /*  005  00/05  C0 ENQ  Ctrl-E                    =>  (self)  */
  6,  /*  006  00/06  C0 ACK  Ctrl-F                    =>  (self)  */
  7,  /*  007  00/07  C0 BEL  Ctrl-G                    =>  (self)  */
  8,  /*  008  00/08  C0 BS   Ctrl-H                    =>  (self)  */
  9,  /*  009  00/09  C0 HT   Ctrl-I                    =>  (self)  */
 10,  /*  010  00/10  C0 LF   Ctrl-J                    =>  (self)  */
 11,  /*  011  00/11  C0 VT   Ctrl-K                    =>  (self)  */
 12,  /*  012  00/12  C0 FF   Ctrl-L                    =>  (self)  */
 13,  /*  013  00/13  C0 CR   Ctrl-M                    =>  (self)  */
 14,  /*  014  00/14  C0 SO   Ctrl-N                    =>  (self)  */
 15,  /*  015  00/15  C0 SI   Ctrl-O                    =>  (self)  */
 16,  /*  016  01/00  C0 DLE  Ctrl-P                    =>  (self)  */
 17,  /*  017  01/01  C0 DC1  Ctrl-Q                    =>  (self)  */
 18,  /*  018  01/02  C0 DC2  Ctrl-R                    =>  (self)  */
 19,  /*  019  01/03  C0 DC3  Ctrl-S                    =>  (self)  */
 20,  /*  020  01/04  C0 DC4  Ctrl-T                    =>  (self)  */
 21,  /*  021  01/05  C0 NAK  Ctrl-U                    =>  (self)  */
 22,  /*  022  01/06  C0 SYN  Ctrl-V                    =>  (self)  */
 23,  /*  023  01/07  C0 ETB  Ctrl-W                    =>  (self)  */
 24,  /*  024  01/08  C0 CAN  Ctrl-X                    =>  (self)  */
 25,  /*  025  01/09  C0 EM   Ctrl-Y                    =>  (self)  */
 26,  /*  026  01/10  C0 SUB  Ctrl-Z                    =>  (self)  */
 27,  /*  027  01/11  C0 ESC  Ctrl-[                    =>  (self)  */
 28,  /*  028  01/12  C0 FS   Ctrl-\                    =>  (self)  */
 29,  /*  029  01/13  C0 GS   Ctrl-]                    =>  (self)  */
 30,  /*  030  01/14  C0 RS   Ctrl-^                    =>  (self)  */
 31,  /*  031  01/15  C0 US   Ctrl-_                    =>  (self)  */
 32,  /*  032  02/00     SP   Space                     =>  (self)  */
 33,  /*  033  02/01  G0 !    Exclamation mark          =>  (self)  */
 34,  /*  034  02/02  G0 "    Doublequote               =>  (self)  */
 35,  /*  035  02/03  G0 #    Number sign               =>  (self)  */
 36,  /*  036  02/04  G0 $    Dollar sign               =>  (self)  */
 37,  /*  037  02/05  G0 %    Percent sign              =>  (self)  */
 38,  /*  038  02/06  G0 &    Ampersand                 =>  (self)  */
 39,  /*  039  02/07  G0 '    Apostrophe                =>  (self)  */
 40,  /*  040  02/08  G0 (    Left parenthesis          =>  (self)  */
 41,  /*  041  02/09  G0 )    Right parenthesis         =>  (self)  */
 42,  /*  042  02/10  G0 *    Asterisk                  =>  (self)  */
 43,  /*  043  02/11  G0 +    Plus sign                 =>  (self)  */
 44,  /*  044  02/12  G0 ,    Comma                     =>  (self)  */
 45,  /*  045  02/13  G0 -    Hyphen, minus sign        =>  (self)  */
 46,  /*  046  02/14  G0 .    Period, full stop         =>  (self)  */
 47,  /*  047  02/15  G0 /    Slash, solidus            =>  (self)  */
 48,  /*  048  03/00  G0 0    Digit 0                   =>  (self)  */
 49,  /*  049  03/01  G0 1    Digit 1                   =>  (self)  */
 50,  /*  050  03/02  G0 2    Digit 2                   =>  (self)  */
 51,  /*  051  03/03  G0 3    Digit 3                   =>  (self)  */
 52,  /*  052  03/04  G0 4    Digit 4                   =>  (self)  */
 53,  /*  053  03/05  G0 5    Digit 5                   =>  (self)  */
 54,  /*  054  03/06  G0 6    Digit 6                   =>  (self)  */
 55,  /*  055  03/07  G0 7    Digit 7                   =>  (self)  */
 56,  /*  056  03/08  G0 8    Digit 8                   =>  (self)  */
 57,  /*  057  03/09  G0 9    Digit 9                   =>  (self)  */
 58,  /*  058  03/10  G0 :    Colon                     =>  (self)  */
 59,  /*  059  03/11  G0 ;    Semicolon                 =>  (self)  */
 60,  /*  060  03/12  G0 <    Less-than sign            =>  (self)  */
 61,  /*  061  03/13  G0 =    Equals sign               =>  (self)  */
 62,  /*  062  03/14  G0 >    Greater-than sign         =>  (self)  */
 63,  /*  063  03/15  G0 ?    Question mark             =>  (self)  */
 64,  /*  064  04/00  G0 @    Commercial at sign        =>  (self)  */
 65,  /*  065  04/01  G0 A    Letter A                  =>  (self)  */
 66,  /*  066  04/02  G0 B    Letter B                  =>  (self)  */
 67,  /*  067  04/03  G0 C    Letter C                  =>  (self)  */
 68,  /*  068  04/04  G0 D    Letter D                  =>  (self)  */
 69,  /*  069  04/05  G0 E    Letter E                  =>  (self)  */
 70,  /*  070  04/06  G0 F    Letter F                  =>  (self)  */
 71,  /*  071  04/07  G0 G    Letter G                  =>  (self)  */
 72,  /*  072  04/08  G0 H    Letter H                  =>  (self)  */
 73,  /*  073  04/09  G0 I    Letter I                  =>  (self)  */
 74,  /*  074  04/10  G0 J    Letter J                  =>  (self)  */
 75,  /*  075  04/11  G0 K    Letter K                  =>  (self)  */
 76,  /*  076  04/12  G0 L    Letter L                  =>  (self)  */
 77,  /*  077  04/13  G0 M    Letter M                  =>  (self)  */
 78,  /*  078  04/14  G0 N    Letter N                  =>  (self)  */
 79,  /*  079  04/15  G0 O    Letter O                  =>  (self)  */
 80,  /*  080  05/00  G0 P    Letter P                  =>  (self)  */
 81,  /*  081  05/01  G0 Q    Letter Q                  =>  (self)  */
 82,  /*  082  05/02  G0 R    Letter R                  =>  (self)  */
 83,  /*  083  05/03  G0 S    Letter S                  =>  (self)  */
 84,  /*  084  05/04  G0 T    Letter T                  =>  (self)  */
 85,  /*  085  05/05  G0 U    Letter U                  =>  (self)  */
 86,  /*  086  05/06  G0 V    Letter V                  =>  (self)  */
 87,  /*  087  05/07  G0 W    Letter W                  =>  (self)  */
 88,  /*  088  05/08  G0 X    Letter X                  =>  (self)  */
 89,  /*  089  05/09  G0 Y    Letter Y                  =>  (self)  */
 90,  /*  090  05/10  G0 Z    Letter Z                  =>  (self)  */
 91,  /*  091  05/11  G0 [    Left square bracket       =>  (self)  */
 92,  /*  092  05/12  G0 \    Reverse slash             =>  (self)  */
 93,  /*  093  05/13  G0 ]    Right square bracket      =>  (self)  */
 94,  /*  094  05/14  G0 ^    Circumflex accent         =>  (self)  */
 95,  /*  095  05/15  G0 _    Underline, low line       =>  (self)  */
 96,  /*  096  06/00  G0 `    Grave accent              =>  (self)  */
 97,  /*  097  06/01  G0 a    Letter a                  =>  (self)  */
 98,  /*  098  06/02  G0 b    Letter b                  =>  (self)  */
 99,  /*  099  06/03  G0 c    Letter c                  =>  (self)  */
100,  /*  100  06/04  G0 d    Letter d                  =>  (self)  */
101,  /*  101  06/05  G0 e    Letter e                  =>  (self)  */
102,  /*  102  06/06  G0 f    Letter f                  =>  (self)  */
103,  /*  103  06/07  G0 g    Letter g                  =>  (self)  */
104,  /*  104  06/08  G0 h    Letter h                  =>  (self)  */
105,  /*  105  06/09  G0 i    Letter i                  =>  (self)  */
106,  /*  106  06/10  G0 j    Letter j                  =>  (self)  */
107,  /*  107  06/11  G0 k    Letter k                  =>  (self)  */
108,  /*  108  06/12  G0 l    Letter l                  =>  (self)  */
109,  /*  109  06/13  G0 m    Letter m                  =>  (self)  */
110,  /*  110  06/14  G0 n    Letter n                  =>  (self)  */
111,  /*  111  06/15  G0 o    Letter o                  =>  (self)  */
112,  /*  112  07/00  G0 p    Letter p                  =>  (self)  */
113,  /*  113  07/01  G0 q    Letter q                  =>  (self)  */
114,  /*  114  07/02  G0 r    Letter r                  =>  (self)  */
115,  /*  115  07/03  G0 s    Letter s                  =>  (self)  */
116,  /*  116  07/04  G0 t    Letter t                  =>  (self)  */
117,  /*  117  07/05  G0 u    Letter u                  =>  (self)  */
118,  /*  118  07/06  G0 v    Letter v                  =>  (self)  */
119,  /*  119  07/07  G0 w    Letter w                  =>  (self)  */
120,  /*  120  07/08  G0 x    Letter x                  =>  (self)  */
121,  /*  121  07/09  G0 y    Letter y                  =>  (self)  */
122,  /*  122  07/10  G0 z    Letter z                  =>  (self)  */
123,  /*  123  07/11  G0 {    Left curly bracket        =>  (self)  */
124,  /*  124  07/12  G0 |    Vertical bar              =>  (self)  */
125,  /*  125  07/13  G0 }    Right curly bracket       =>  (self)  */
126,  /*  126  07/14  G0 ~    Tilde                     =>  (self)  */
127,  /*  127  07/15     DEL  Delete, Rubout            =>  (self)  */
128,  /*  128  08/00  C1      Ctrl-Meta-@               =>  (self)  */
129,  /*  129  08/01  C1      Ctrl-Meta-A               =>  (self)  */
130,  /*  130  08/02  C1      Ctrl-Meta-B               =>  (self)  */
131,  /*  131  08/03  C1      Ctrl-Meta-C               =>  (self)  */
132,  /*  132  08/04  C1 IND  Ctrl-Meta-D               =>  (self)  */
133,  /*  133  08/05  C1 NEL  Ctrl-Meta-E               =>  (self)  */
134,  /*  134  08/06  C1 SSA  Ctrl-Meta-F               =>  (self)  */
135,  /*  135  08/07  C1 ESA  Ctrl-Meta-G               =>  (self)  */
136,  /*  136  08/08  C1 HTS  Ctrl-Meta-H               =>  (self)  */
137,  /*  137  08/09  C1      Ctrl-Meta-I               =>  (self)  */
138,  /*  138  08/10  C1      Ctrl-Meta-J               =>  (self)  */
139,  /*  139  08/11  C1      Ctrl-Meta-K               =>  (self)  */
140,  /*  140  08/12  C1      Ctrl-Meta-L               =>  (self)  */
141,  /*  141  08/13  C1 RI   Ctrl-Meta-M               =>  (self)  */
142,  /*  142  08/14  C1 SS2  Ctrl-Meta-N               =>  (self)  */
143,  /*  143  08/15  C1 SS3  Ctrl-Meta-O               =>  (self)  */
144,  /*  144  09/00  C1 DCS  Ctrl-Meta-P               =>  (self)  */
145,  /*  145  09/01  C1      Ctrl-Meta-Q               =>  (self)  */
146,  /*  146  09/02  C1      Ctrl-Meta-R               =>  (self)  */
147,  /*  147  09/03  C1 STS  Ctrl-Meta-S               =>  (self)  */
148,  /*  148  09/04  C1      Ctrl-Meta-T               =>  (self)  */
149,  /*  149  09/05  C1      Ctrl-Meta-U               =>  (self)  */
150,  /*  150  09/06  C1 SPA  Ctrl-Meta-V               =>  (self)  */
151,  /*  151  09/07  C1 EPA  Ctrl-Meta-W               =>  (self)  */
152,  /*  152  09/08  C1      Ctrl-Meta-X               =>  (self)  */
153,  /*  153  09/09  C1      Ctrl-Meta-Y               =>  (self)  */
154,  /*  154  09/10  C1      Ctrl-Meta-Z               =>  (self)  */
155,  /*  155  09/11  C1 CSI  Ctrl-Meta-[               =>  (self)  */
156,  /*  156  09/12  C1 ST   Ctrl-Meta-\               =>  (self)  */
157,  /*  157  09/13  C1 OSC  Ctrl-Meta-]               =>  (self)  */
158,  /*  158  09/14  C1 PM   Ctrl-Meta-^               =>  (self)  */
159,  /*  159  09/15  C1 APC  Ctrl-Meta-_               =>  (self)  */
 32,  /*  160  10/00  G1      No-break space            =>  SP      */
 33,  /*  161  10/01  G1      Inverted exclamation      =>  !       */
UNK,  /*  162  10/02  G1      Cent sign                 =>  UNK     */
UNK,  /*  163  10/03  G1      Pound sign                =>  UNK     */
UNK,  /*  164  10/04  G1      Currency sign             =>  UNK     */
UNK,  /*  165  10/05  G1      Yen sign                  =>  UNK     */
124,  /*  166  10/06  G1      Broken bar                =>  |       */
UNK,  /*  167  10/07  G1      Paragraph sign            =>  UNK     */
 34,  /*  168  10/08  G1      Diaeresis                 =>  "       */
 67,  /*  169  10/09  G1      Copyright sign            =>  C       */
UNK,  /*  170  10/10  G1      Feminine ordinal          =>  UNK     */
 34,  /*  171  10/11  G1      Left angle quotation      =>  "       */
126,  /*  172  10/12  G1      Not sign                  =>  ~       */
 45,  /*  173  10/13  G1      Soft hyphen               =>  -       */
 82,  /*  174  10/14  G1      Registered trade mark     =>  R       */
UNK,  /*  175  10/15  G1      Macron                    =>  UNK     */
UNK,  /*  176  11/00  G1      Degree sign, ring above   =>  UNK     */
UNK,  /*  177  11/01  G1      Plus-minus sign           =>  UNK     */
UNK,  /*  178  11/02  G1      Superscript two           =>  UNK     */
UNK,  /*  179  11/03  G1      Superscript three         =>  UNK     */
 39,  /*  180  11/04  G1      Acute accent              =>  '       */
117,  /*  181  11/05  G1      Micro sign                =>  u       */
UNK,  /*  182  11/06  G1      Pilcrow sign              =>  UNK     */
UNK,  /*  183  11/07  G1      Middle dot                =>  UNK     */
 44,  /*  184  11/08  G1      Cedilla                   =>  ,       */
UNK,  /*  185  11/09  G1      Superscript one           =>  UNK     */
UNK,  /*  186  11/10  G1      Masculine ordinal         =>  UNK     */
 34,  /*  187  11/11  G1      Right angle quotation     =>  "       */
UNK,  /*  188  11/12  G1      One quarter               =>  UNK     */
UNK,  /*  189  11/13  G1      One half                  =>  UNK     */
UNK,  /*  190  11/14  G1      Three quarters            =>  UNK     */
 63,  /*  191  11/15  G1      Inverted question mark    =>  ?       */
 65,  /*  192  12/00  G1      A grave                   =>  A       */
 65,  /*  193  12/01  G1      A acute                   =>  A       */
 65,  /*  194  12/02  G1      A circumflex              =>  A       */
 65,  /*  195  12/03  G1      A tilde                   =>  A       */
 65,  /*  196  12/04  G1      A diaeresis               =>  A       */
 65,  /*  197  12/05  G1      A ring above              =>  A       */
 65,  /*  198  12/06  G1      A with E                  =>  A       */
 67,  /*  199  12/07  G1      C Cedilla                 =>  C       */
 69,  /*  200  12/08  G1      E grave                   =>  E       */
 69,  /*  201  12/09  G1      E acute                   =>  E       */
 69,  /*  202  12/10  G1      E circumflex              =>  E       */
 69,  /*  203  12/11  G1      E diaeresis               =>  E       */
 73,  /*  204  12/12  G1      I grave                   =>  I       */
 73,  /*  205  12/13  G1      I acute                   =>  I       */
 73,  /*  206  12/14  G1      I circumflex              =>  I       */
 73,  /*  207  12/15  G1      I diaeresis               =>  I       */
UNK,  /*  208  13/00  G1      Icelandic Eth             =>  UNK     */
 78,  /*  209  13/01  G1      N tilde                   =>  N       */
 79,  /*  210  13/02  G1      O grave                   =>  O       */
 79,  /*  211  13/03  G1      O acute                   =>  O       */
 79,  /*  212  13/04  G1      O circumflex              =>  O       */
 79,  /*  213  13/05  G1      O tilde                   =>  O       */
 79,  /*  214  13/06  G1      O diaeresis               =>  O       */
120,  /*  215  13/07  G1      Multiplication sign       =>  x       */
 79,  /*  216  13/08  G1      O oblique stroke          =>  O       */
 85,  /*  217  13/09  G1      U grave                   =>  U       */
 85,  /*  218  13/10  G1      U acute                   =>  U       */
 85,  /*  219  13/11  G1      U circumflex              =>  U       */
 85,  /*  220  13/12  G1      U diaeresis               =>  U       */
 89,  /*  221  13/13  G1      Y acute                   =>  Y       */
UNK,  /*  222  13/14  G1      Icelandic Thorn           =>  UNK     */
115,  /*  223  13/15  G1      German sharp s            =>  s       */
 97,  /*  224  14/00  G1      a grave                   =>  a       */
 97,  /*  225  14/01  G1      a acute                   =>  a       */
 97,  /*  226  14/02  G1      a circumflex              =>  a       */
 97,  /*  227  14/03  G1      a tilde                   =>  a       */
 97,  /*  228  14/04  G1      a diaeresis               =>  a       */
 97,  /*  229  14/05  G1      a ring above              =>  a       */
 97,  /*  230  14/06  G1      a with e                  =>  a       */
 99,  /*  231  14/07  G1      c cedilla                 =>  c       */
101,  /*  232  14/08  G1      e grave                   =>  e       */
101,  /*  233  14/09  G1      e acute                   =>  e       */
101,  /*  234  14/10  G1      e circumflex              =>  e       */
101,  /*  235  14/11  G1      e diaeresis               =>  e       */
105,  /*  236  14/12  G1      i grave                   =>  i       */
105,  /*  237  14/13  G1      i acute                   =>  i       */
105,  /*  238  14/14  G1      i circumflex              =>  i       */
105,  /*  239  14/15  G1      i diaeresis               =>  i       */
UNK,  /*  240  15/00  G1      Icelandic eth             =>  UNK     */
110,  /*  241  15/01  G1      n tilde                   =>  n       */
111,  /*  242  15/02  G1      o grave                   =>  o       */
111,  /*  243  15/03  G1      o acute                   =>  o       */
111,  /*  244  15/04  G1      o circumflex              =>  o       */
111,  /*  245  15/05  G1      o tilde                   =>  o       */
111,  /*  246  15/06  G1      o diaeresis               =>  o       */
 47,  /*  247  15/07  G1      Division sign             =>  /       */
111,  /*  248  15/08  G1      o oblique stroke          =>  o       */
117,  /*  249  15/09  G1      u grave                   =>  u       */
117,  /*  250  15/10  G1      u acute                   =>  u       */
117,  /*  251  15/11  G1      u circumflex              =>  u       */
117,  /*  252  15/12  G1      u diaeresis               =>  u       */
121,  /*  253  15/13  G1      y acute                   =>  y       */
UNK,  /*  254  15/14  G1      Icelandic thorn           =>  UNK     */
121   /*  255  15/15  G1      y diaeresis               =>  y       */
};

/* Translation tables for ISO Latin Alphabet 1 to local file character sets */

/*
  The remaining tables are not annotated like the one above, because
  the size of the resulting source file would be more than 500K.
  Each row in the following tables corresponds to a column of ISO 8859-1.
*/

CHAR
yl1todu[] = {  /* Latin-1 to Dutch ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, UNK,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK,  39, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK,  35, 124, UNK, UNK,  93, 123,  67, UNK,  34, UNK,  45,  82, UNK,
 91, UNK, UNK, UNK, 126, 117, UNK, UNK,  44, UNK, UNK,  34, 125,  92,  64,  63,
 65,  65,  65,  65,  91,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  79, 120,  79,  85,  85,  85,  85,  89, UNK, 115,
 97,  97,  97,  97,  97,  97,  97,  99, 101, 101, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 111, 111,  47, 111, 117, 117, 117, 117, 121, UNK,  91
};

CHAR
yl1tofi[] = {  /* Latin-1 to Finnish ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK, UNK,  95,
UNK,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, UNK, UNK,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK, UNK, UNK, UNK, UNK,  34,  67, UNK,  34, UNK,  45,  82, UNK,
UNK, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  91,  93,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  92, 120,  79,  85,  85,  85,  94,  89, UNK, 115,
 97,  97,  97,  97, 123, 125,  97,  99, 101,  96, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 111, 124,  47, 111, 117, 117, 117, 126, 121, UNK, 121
};

CHAR
yl1tofr[] = {  /* Latin-1 to French ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, UNK,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, UNK, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK,  35, UNK, UNK, UNK,  93,  34,  67, UNK,  34, UNK,  45,  82, UNK,
 91, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  65,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  79, 120,  79,  85,  85,  85,  85,  89, UNK, 115,
 64,  97,  97,  97,  97,  97,  97,  92, 125, 123, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 111, 111,  47, 111, 124, 117, 117, 117, 121, UNK, 121
};

CHAR
yl1tofc[] = {  /* Latin-1 to French-Canadian ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK, UNK,  95,
UNK,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, UNK, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK, UNK, UNK, UNK, UNK,  34,  67, UNK,  34, UNK,  45,  82, UNK,
UNK, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  65,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  79, 120,  79,  85,  85,  85,  85,  89, UNK, 115,
 64,  97,  91,  97,  97,  97,  97,  92, 125, 123,  93, 101, 105, 105,  94, 105,
UNK, 110, 111, 111,  96, 111, 111,  47, 111, 124, 117, 126, 117, 121, UNK, 121
};

CHAR
yl1toge[] = {  /* Latin-1 to German ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, UNK, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK, UNK, UNK, 124,  64,  34,  67, UNK,  34, 126,  45,  82, UNK,
UNK, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  91,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  92, 120,  79,  85,  85,  85,  93,  89, UNK, 126,
 97,  97,  97,  97, 123,  97,  97,  99, 101, 101, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 111, 124,  47, 111, 117, 117, 117, 125, 121, UNK, 121
};

CHAR
yl1tohu[] = {  /* Latin-1 to Hungarian ISO-646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK,  36, UNK, 124, UNK,  34,  67, UNK,  34, 126,  45,  82, UNK,
UNK,  64, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  65,  65,  65,  67,  69,  91,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  92, 120,  79,  85,  85,  85,  93,  89, UNK, 115,
 97,  96,  97,  97,  97,  97,  97,  99, 101, 123, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 111, 124,  47, 111, 117, 117, 117, 125, 121, UNK, 121
};

CHAR
yl1toit[] = {  /* Latin-1 to Italian ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, UNK,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK,  94,  95,
UNK,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, UNK, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK,  35, UNK, UNK, UNK,  64,  34,  67, UNK,  34, UNK,  45,  82, UNK,
 91, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  65,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  79, 120,  79,  85,  85,  85,  85,  89, UNK, 115,
123,  97,  97,  97,  97,  97,  97,  92, 125,  93, 101, 101, 126, 105, 105, 105,
UNK, 110, 124, 111, 111, 111, 111,  47, 111,  96, 117, 117, 117, 121, UNK, 121
};

CHAR
yl1tono[] = {  /* Latin-1 to Norwegian/Danish ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK, UNK, UNK, UNK, UNK,  34,  67, UNK,  34, 126,  45,  82, UNK,
UNK, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  65,  93,  91,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  79, 120,  92,  85,  85,  85,  85,  89, UNK, 115,
 97,  97,  97,  97,  97, 125, 123,  99, 101, 101, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 111, 111,  47, 124, 117, 117, 117, 117, 121, UNK, 121
};

CHAR
yl1topo[] = {  /* Latin-1 to Portuguese ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK, UNK, UNK, UNK, UNK,  34,  67, UNK,  34, 126,  45,  82, UNK,
UNK, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  91,  65,  65,  65,  92,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  93,  79, 120,  79,  85,  85,  85,  85,  89, UNK, 115,
 97,  97,  97, 123,  97,  97,  97, 124, 101, 101, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 125, 111,  47, 111, 117, 117, 117, 117, 121, UNK, 121
};

CHAR
yl1tosp[] = {  /* Latin-1 to Spanish ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, UNK,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  96, UNK, UNK, 126, 127,
126, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  91, UNK,  35, UNK, UNK, UNK,  64,  34,  67, UNK,  34, 126,  45,  82, UNK,
123, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  93,
 65,  65,  65,  65,  65,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  92,  79,  79,  79,  79,  79, 120,  79,  85,  85,  85,  85,  89, UNK, 115,
124,  97,  97,  97,  97,  97,  97, 125, 101, 101, 101, 101, 105, 105, 105, 105,
UNK, 124, 111, 111, 111, 111, 111,  47, 111, 117, 117, 117, 117, 121, UNK, 121
};

CHAR
yl1tosw[] = {  /* Latin-1 to Swedish ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK, UNK,  95,
UNK,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, UNK, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK, UNK, UNK, UNK, UNK,  34,  67, UNK,  34, UNK,  45,  82, UNK,
UNK, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  91,  93,  65,  67,  69,  64,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  92, 120,  79,  85,  85,  85,  94,  89, UNK, 115,
 97,  97,  97,  97, 123, 125,  97,  99, 101,  96, 101, 101, 105, 105, 105, 105,
UNK, 110, 111, 111, 111, 111, 124,  47, 111, 117, 117, 117, 126, 121, UNK, 121
};

CHAR
yl1toch[] = {  /* Latin-1 to Swiss ISO 646 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, UNK,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
UNK,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, UNK, UNK, UNK, UNK, UNK,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, UNK, UNK, UNK, UNK, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32,  33, UNK, UNK, UNK, UNK, UNK, UNK,  34,  67, UNK,  34, UNK,  45,  82, UNK,
UNK, UNK, UNK, UNK,  39, 117, UNK, UNK,  44, UNK, UNK,  34, UNK, UNK, UNK,  63,
 65,  65,  65,  65,  65,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
UNK,  78,  79,  79,  79,  79,  79, 120,  79,  85,  85,  85,  85,  89, UNK, 115,
 64,  97,  97,  97, 123,  97,  97,  92,  95,  91,  93, 101, 105, 105,  94, 105,
UNK, 110, 111, 111,  96, 111, 124,  47, 111,  35, 117, 126, 125, 121, UNK, 121
};

CHAR
yl1todm[] = {  /* Latin-1 to DEC Multinational Character Set */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 32, 161, 162, 163, 168, 165, 124, 167,  34, 169, 170, 171, 126, UNK,  82, UNK,
176, 177, 178, 179,  39, 181, 182, 183,  44, 185, 186, 187, 188, 189, UNK, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
UNK, 209, 210, 211, 212, 213, 214, 120, 216, 217, 218, 219, 220, 221, UNK, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
UNK, 241, 242, 243, 244, 245, 246,  47, 248, 249, 250, 251, 252, UNK, UNK, 253
};

/* Local file character sets to ISO Latin Alphabet 1 */

CHAR
yastol1[] = {  /* ASCII to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
};

CHAR
ydutol1[] = {  /* Dutch ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, 163,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
190,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 255, 189, 124,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 168, 164, 188,  39, 127
};

CHAR
yfitol1[] = {  /* Finnish ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 196, 246, 197, 220,  95,
233,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 228, 246, 229, 252, 127
};

CHAR
yfrtol1[] = {  /* French ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, 163,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
224,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 176, 231, 167,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 233, 249, 232, 168, 127
};

CHAR
yfctol1[] = {  /* French-Canadian ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
224,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 226, 231, 234, 238,  95,
244,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 233, 249, 232, 251, 127
};

CHAR
ygetol1[] = {  /* German ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
167,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 196, 214, 220,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 228, 246, 252, 223, 127
};

CHAR
yittol1[] = {  /* Italian ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, 163,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
167,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 176, 231, 233,  94,  95,
249,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 224, 242, 232, 236, 127
};

CHAR
ynotol1[] = {  /* Norwegian/Danish ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 198, 216, 197,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 230, 248, 229, 126, 127
};

CHAR
ypotol1[] = {  /* Portuguese ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 195, 199, 213,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 227, 231, 245, 126, 127
};

CHAR
ysptol1[] = {  /* Spanish ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, 163,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
167,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 161, 209, 191,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 176, 241, 231, 126, 127
};

CHAR
yswtol1[] = {  /* Swedish ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
201,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 196, 214, 197, 220,  95,
233,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 228, 246, 229, 252, 127
};

CHAR
ychtol1[] = {  /* Swiss ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34, 249,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
224,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 233, 231, 234, 238, 232,
244,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 228, 246, 252, 251, 127
};

CHAR
yhutol1[] = {  /* Hungarian ISO 646 to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35, 164,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
193,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 201, 214, 220,  94,  95,
225,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 233, 246, 252,  34, 127
};

CHAR
ydmtol1[] = {  /* DEC Multinational Character Set to Latin-1 */
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

/* Translation functions ... */

CHAR				/* The identity translation function */
ident(c) CHAR c; {
    return(c);
}

CHAR
xl1toas(c) CHAR c; {			/* Latin-1 to US ASCII... */

    switch(langs[language].id) {
      case L_GERMAN:
	switch (c) {			/* German, special rules. */
	  case 196:			/* umlaut-A -> Ae */
	    zmstuff('e');
	    return('A');
	  case 214:			/* umlaut-O -> Oe */
	    zmstuff('e');
	    return('O');
	  case 220:			/* umlaut-U -> Ue */
	    zmstuff('e');
	    return('U');
	  case 228:			/* umlaut-a -> ae */
	    zmstuff('e');
	    return('a');
	  case 246:			/* umlaut-o -> oe */
	    zmstuff('e');
	    return('o');
	  case 252:			/* umlaut-u -> ue */
	    zmstuff('e');
	    return('u');
	  case 223:			/* ess-zet -> ss */
	    zmstuff('s');
	    return('s');
	  default: return(yl1toas[c]);	/* all others by the book */
	}
      case L_DANISH:
      case L_FINNISH:
      case L_NORWEGIAN:
      case L_SWEDISH:
	switch (c) {			/* Scandanavian languages. */
	  case 196:			/* umlaut-A -> Ae */
	    zmstuff('e');
	    return('A');
	  case 214:			/* umlaut-O -> Oe */
	  case 216:			/* O-slash -> Oe */
	    zmstuff('e');
	    return('O');
	  case 220:			/* umlaut-U -> Y */
	    return('Y');
	  case 228:			/* umlaut-a -> ae */
	    zmstuff('e');
	    return('a');
	  case 246:			/* umlaut-o -> oe */
	  case 248:			/* o-slash -> oe */
	    zmstuff('e');
	    return('o');
	  case 252:			/* umlaut-u -> y */
	    return('y');
	  case 197:			/* A-ring -> Aa */
	    zmstuff('a');
	    return('A');
          case 229:			/* a-ring -> aa */
	    zmstuff('a');
	    return('a');
	  default: return(yl1toas[c]);	/* All others by the book */
	}
      default:
	return(yl1toas[c]);		/* Not German, by the table. */
    }
}

CHAR					/* Latin-1 to German ASCII */
xl1toge(c) CHAR c; {
    return(yl1toge[c]);
}

CHAR					/* German ASCII to Latin-1 */
xgetol1(c) CHAR c; {
    return(ygetol1[c]);
}

CHAR
xgetoas(c) CHAR c; {			/* German ISO 646 to ASCII */
    switch (c) {
      case 91:				/* umlaut-A -> Ae */
	zmstuff('e');
	return('A');
      case 92:				/* umlaut-O -> Oe */
	zmstuff('e');
	return('O');
      case 93:				/* umlaut-U -> Ue */
	zmstuff('e');
	return('U');
      case 123:				/* umlaut-a -> ae */
	zmstuff('e');
	return('a');
      case 124:				/* umlaut-o -> oe */
	zmstuff('e');
	return('o');
      case 125:				/* umlaut-u -> ue */
	zmstuff('e');
	return('u');
      case 126:				/* ess-zet -> ss */
	zmstuff('s');
	return('s');
      default:  return(c);		/* all others stay the same */
    }
}

CHAR
xdutoas(c) CHAR c; {			/* Dutch ISO 646 to US ASCII */
    switch (c) {
      case 64:  return(UNK);		/* 3/4 */
      case 91:  return('y');		/* y-diaeresis */
      case 92:  return(UNK);		/* 1/2 */
      case 93:  return(124);		/* vertical bar */
      case 123: return(34);		/* diaeresis */
      case 124: return(UNK);		/* Florin */
      case 125: return(UNK);		/* 1/4 */
      case 126: return(39);		/* Apostrophe */
      default:  return(c);
    }
}

CHAR
xfitoas(c) CHAR c; {			/* Finnish ISO 646 to US ASCII */
    switch (c) {
      case 91:				/* A-diaeresis */
	zmstuff('e');
	return('A');
      case 92:				/* O-diaeresis */
	zmstuff('e');
	return('O');
      case 93:				/* A-ring */
	zmstuff('a');
	return('A');
      case 94:				/* U-diaeresis */
	return('Y');
      case 96:				/* e-acute */
	return('e');
      case 123:				/* a-diaeresis */
	zmstuff('e');
	return('a');
      case 124:				/* o-diaeresis */
	zmstuff('e');
	return('o');
      case 125:				/* a-ring */
	zmstuff('a');
	return('a');
      case 126:				/* u-diaeresis */
	return('y');
      default:
	return(c);
    }
}

CHAR
xfrtoas(c) CHAR c; {			/* French ISO 646 to US ASCII */
    switch (c) {
      case 64:  return(97);		/* a grave */
      case 91:  return(UNK);		/* degree sign */
      case 92:  return(99);		/* c cedilla */
      case 93:  return(UNK);		/* paragraph sign */
      case 123: return(101);		/* e acute */
      case 124: return(117);		/* u grave */
      case 125: return(101);		/* e grave */
      case 126: return(34);		/* diaeresis */
      default:  return(c);
    }
}

CHAR
xfctoas(c) CHAR c; {			/* French Canadian ISO 646 to ASCII */
    switch (c) {
      case 64:  return('a');		/* a grave */
      case 91:  return('a');		/* a circumflex */
      case 92:  return('c');		/* c cedilla */
      case 93:  return('e');		/* e circumflex */
      case 94:  return('i');		/* i circumflex */
      case 96:  return('o');		/* o circumflex */
      case 123: return('e');		/* e acute */
      case 124: return('u');		/* u grave */
      case 125: return('e');		/* e grave */
      case 126: return('u');		/* u circumflex */
      default:  return(c);
    }
}

CHAR
xittoas(c) CHAR c; {			/* Italian ISO 646 to ASCII */
    switch (c) {
      case 91:  return(UNK);		/* degree */
      case 92:  return('c');		/* c cedilla */
      case 93:  return('e');		/* e acute */
      case 96:  return('u');		/* u grave */
      case 123: return('a');		/* a grave */
      case 124: return('o');		/* o grave */
      case 125: return('e');		/* e grave */
      case 126: return('i');		/* i grave */
      default:  return(c);
    }
}

CHAR
xnotoas(c) CHAR c; {			/* Norge/Danish ISO 646 to ASCII */
    switch (c) {
      case 91:
	zmstuff('E');			/* AE digraph */
	return('A');
      case 92: return('O');		/* O slash */
      case 93:				/* A ring */
	zmstuff('a');
	return('A');
      case 123:				/* ae digraph */
	zmstuff('e');
	return('a');
      case 124: return('o');		/* o slash */
      case 125:				/* a ring */
	zmstuff('a');
	return('a');
      default:  return(c);
    }
}

CHAR
xpotoas(c) CHAR c; {			/* Portuguese ISO 646 to ASCII */
    switch (c) {
      case 91:  return('A');		/* A tilde */
      case 92:  return('C');		/* C cedilla */
      case 93:  return('O');		/* O tilde */
      case 123: return('a');		/* a tilde */
      case 124: return('c');		/* c cedilla */
      case 125: return('o');		/* o tilde */
      default:  return(c);
    }
}

CHAR
xsptoas(c) CHAR c; {			/* Spanish ISO 646 to ASCII */
    switch (c) {
      case 91:  return(33);		/* Inverted exclamation */
      case 92:  return('N');		/* N tilde */
      case 93:  return(63);		/* Inverted question mark */
      case 123: return(UNK);		/* degree */
      case 124: return('n');		/* n tilde */
      case 125: return('c');		/* c cedilla */
      default:  return(c);
    }
}

CHAR
xswtoas(c) CHAR c; {			/* Swedish ISO 646 to ASCII */
    switch (c) {
      case 64:  return('E');		/* E acute */
      case 91:				/* A diaeresis */
	zmstuff('e');
	return('A');
      case 92:				/* O diaeresis */
	zmstuff('e');
	return('O');
      case 93:				/* A ring */
	zmstuff('a');
	return('A');
      case 94:				/* U diaeresis */
	return('Y');
      case 96:  return('e');		/* e acute */
      case 123:				/* a diaeresis */
	zmstuff('e');
	return('a');
      case 124:				/* o diaeresis */
	zmstuff('e');
	return('o');
      case 125:				/* a ring */
	zmstuff('a');
	return('a');
      case 126: return('y');		/* u diaeresis */
      default:  return(c);
    }
}

CHAR
xchtoas(c) CHAR c; {			/* Swiss ISO 646 to ASCII */
    switch (c) {
      case 35:  return('u');		/* u grave */
      case 64:  return('a');		/* a grave */
      case 91:  return('e');		/* e acute */
      case 92:  return('c');		/* c cedilla */
      case 93:  return('e');		/* e circumflex */
      case 94:  return('i');		/* i circumflex */
      case 95:  return('e');		/* e grave */
      case 96:  return('o');		/* o circumflex */
      case 123:				/* a diaeresis */
	zmstuff('e');
	return('a');
      case 124:				/* o diaeresis */
	zmstuff('e');
	return('o');
      case 125:				/* u diaeresis */
	zmstuff('e');
	return('u');
      case 126: return('u');		/* u circumflex */
      default:  return(c);
    }
}

CHAR
xhutoas(c) CHAR c; {			/* Hungarian ISO 646 to ASCII */
    switch (c) {
      case 64:  return('A');		/* A acute */
      case 91:  return('E');		/* E acute */
      case 92:  return('O');		/* O diaeresis */
      case 93:  return('U');		/* U diaeresis */
      case 96:  return('a');		/* a acute */
      case 123: return('e');		/* e acute */
      case 124: return('o');		/* o acute */
      case 125: return('u');		/* u acute */
      case 126: return(34);		/* double acute accent */
      default:  return(c);
    }
}

CHAR
xdmtoas(c) CHAR c; {			/* DEC MCS to ASCII */
    return(yl1toas[c]);			/* for now, treat like Latin-1 */
}

CHAR
xuktol1(c) CHAR c; {			/* UK ASCII to Latin-1 */
    if (c == 35)
      return(163);
    else return(c);
}

CHAR
xl1touk(c) CHAR c; {			/* Latin-1 to UK ASCII */
    if (c == 163)
      return(35);
    else return(yl1toas[c]);
}

CHAR					/* Latin-1 to French ISO 646 */
xl1tofr(c) CHAR c; {
    return(yl1tofr[c]);
}

CHAR					/* French ASCII to Latin-1 */
xfrtol1(c) CHAR c; {
    return(yfrtol1[c]);
}

CHAR					/* Latin-1 to Dutch ASCII */
xl1todu(c) CHAR c; {
    return(yl1todu[c]);
}

CHAR
xdutol1(c) CHAR c; {			/* Dutch ISO 646 to Latin-1 */
    return(ydutol1[c]);
}

CHAR
xfitol1(c) CHAR c; {			/* Finnish ISO 646 to Latin-1 */
    return(yfitol1[c]); 
}

CHAR
xl1tofi(c) CHAR c; {			/* Latin-1 to Finnish ISO 646 */
    return(yl1tofi[c]); 
}

CHAR
xfctol1(c) CHAR c; {			/* French Canadian ISO646 to Latin-1 */
    return(yfctol1[c]); 
}

CHAR
xl1tofc(c) CHAR c; {			/* Latin-1 to French Canadian ISO646 */
    return(yl1tofc[c]); 
}

CHAR
xittol1(c) CHAR c; {			/* Italian ISO 646 to Latin-1 */
    return(yittol1[c]); 
}

CHAR
xl1toit(c) CHAR c; {			/* Latin-1 to Italian ISO 646 */
    return(yl1toit[c]); 
}

CHAR
xnotol1(c) CHAR c; {		 /* Norwegian and Danish ISO 646 to Latin-1 */
    return(ynotol1[c]); 
}

CHAR
xl1tono(c) CHAR c; {		 /* Latin-1 to Norwegian and Danish ISO 646 */
    return(yl1tono[c]); 
}

CHAR
xpotol1(c) CHAR c; {			/* Portuguese ISO 646 to Latin-1 */
    return(ypotol1[c]); 
}

CHAR
xl1topo(c) CHAR c; {			/* Latin-1 to Portuguese ISO 646 */
    return(yl1topo[c]); 
}

CHAR
xsptol1(c) CHAR c; {			/* Spanish ISO 646 to Latin-1 */
    return(ysptol1[c]); 
}

CHAR
xl1tosp(c) CHAR c; {			/* Latin-1 to Spanish ISO 646 */
    return(yl1tosp[c]); 
}

CHAR
xswtol1(c) CHAR c; {			/* Swedish ISO 646 to Latin-1 */
    return(yswtol1[c]); 
}

CHAR
xl1tosw(c) CHAR c; {			/* Latin-1 to Swedish ISO 646 */
    return(yl1tosw[c]); 
}

CHAR
xchtol1(c) CHAR c; {			/* Swiss ISO 646 to Latin-1 */
    return(ychtol1[c]); 
}

CHAR
xl1toch(c) CHAR c; {			/* Latin-1 to Swiss ISO 646 */
    return(yl1toch[c]); 
}

CHAR
xhutol1(c) CHAR c; {			/* Hungarian ISO 646 to Latin-1 */
    return(yhutol1[c]);
}

CHAR
xl1tohu(c) CHAR c; {			/* Latin-1 to Hungarian ISO 646 */
    return(yl1tohu[c]);
}


CHAR
xl1todm(c) CHAR c; { /* Latin-1 to DEC Multinational Character Set (MCS) */
    return(yl1todm[c]); 
}

CHAR
xdmtol1(c) CHAR c; { /* DEC Multinational Character Set (MCS) to Latin-1 */
    return(ydmtol1[c]); 
}

/*
  Table of translation functions for receiving files.
  Array of pointers to functions for translating from the transfer
  syntax to the local file character set.  The first index is the
  transfer syntax character set number, the second index is the file 
  character set number.
*/

/*
 The following list of functions applies to Unix and VAX/VMS.  This
 list can't be moved to ckuxla.h without including a forward declaration
 for each function.  It's not clear to me how best to clean this up.
 Maybe whoever adapts this file to the Mac or OS/2 will have an idea.
*/

/* 
 Notice the hard numbers used as indices!  C does not seem to allow
 symbols here (MAXTCSETS, MAXFCSETS), nor does the program work right
 if the brackets are left empty.  Therefore, when adapting this file to
 another computer, either figure out how to get around this, or else
 fill in your own numbers.  Sigh.
*/

CHAR (*xlr[2][16])() = {
    ident,				/* 0,0 ascii to us ascii */
    ident,				/* 0,1 ascii to uk ascii */
    ident,				/* 0,2 ascii to dutch nrc */
    ident,				/* 0,3 ascii to finnish nrc */
    ident,				/* 0,4 ascii to french nrc */
    ident,				/* 0,5 ascii to fr-canadian nrc */
    ident,				/* 0,6 ascii to german nrc */
    ident,				/* 0,7 ascii to hungarian nrc */
    ident,				/* 0,8 ascii to italian nrc */
    ident,				/* 0,9 ascii to norge/danish nrc */
    ident,				/* 0,10 ascii to portuguese nrc */
    ident,				/* 0,11 ascii to spanish nrc */
    ident,				/* 0,12 ascii to swedish nrc */
    ident,				/* 0,13 ascii to swiss nrc */
    ident,				/* 0,14 ascii to latin-1 */
    ident,				/* 0,15 ascii to DEC MCS */
    xl1toas,				/* 1,0 latin-1 to us ascii */
    xl1touk,				/* 1,1 latin-1 to uk ascii */
    xl1todu,				/* 1,2 latin-1 to dutch nrc */
    xl1tofi,				/* 1,3 latin-1 to finnish ascii */
    xl1tofr,				/* 1,4 latin-1 to french nrc */
    xl1tofc,				/* 1,5 latin-1 to fr-canadian nrc */
    xl1toge,				/* 1,6 latin-1 to german nrc */
    xl1toit,				/* 1,7 latin-1 to italian nrc */
    xl1tohu,				/* 1,8 latin-1 to hungarian nrc */
    xl1tono,				/* 1,9 latin-1 to norge/danish nrc */
    xl1topo,				/* 1,10 latin-1 to portuguese nrc */
    xl1tosp,				/* 1,11 latin-1 to spanish nrc */
    xl1tosw,				/* 1,12 latin-1 to swedish nrc */
    xl1toch,				/* 1,13 latin-1 to swiss nrc */
    ident,				/* 1,14 latin-1 to latin-1 */
    xl1todm				/* 1,15 latin-1 to DEC MCS */
};

/*
  Translation functions for sending files.
  Array of pointers to functions for translating from the local file
  character set to the transfer syntax character set.  Indexed in the same
  way as the xlr array above.
*/
CHAR (*xls[2][16])() = {
    ident,				/* us ascii to ascii */
    ident,				/* uk ascii to ascii */
    xdutoas,				/* dutch nrc to ascii */
    xfitoas,				/* finnish nrc to ascii */
    xfrtoas,				/* french nrc to ascii */
    xfctoas,				/* french canadian nrc to ascii */
    xgetoas,				/* german nrc to ascii */
    xhutoas,				/* hungarian nrc to ascii */
    xittoas,				/* italian nrc to ascii */
    xnotoas,				/* norwegian/danish nrc to ascii */
    xpotoas,				/* portuguese nrc to ascii */
    xsptoas,				/* spanish nrc to ascii */
    xswtoas,				/* swedish nrc to ascii */
    xchtoas,				/* swiss nrc to ascii */
    xl1toas,				/* latin-1 to ascii */
    xdmtoas,				/* dec mcs to ascii */
    ident,				/* us ascii to latin-1 */
    xuktol1,				/* uk ascii to latin-1 */
    xdutol1,				/* dutch nrc to latin-1 */
    xfitol1,				/* finnish nrc to latin-1 */
    xfrtol1,				/* french nrc to latin-1 */
    xfctol1,				/* french canadian nrc to latin-1 */
    xgetol1,				/* german nrc to latin-1 */
    xhutol1,				/* hungarian nrc to latin-1 */
    xittol1,				/* italian nrc to latin-1 */
    xnotol1,				/* norwegian/danish nrc to latin-1 */
    xpotol1,				/* portuguese nrc to latin-1 */
    xsptol1,				/* spanish nrc to latin-1 */
    xswtol1,				/* swedish nrc to latin-1 */
    xchtol1,				/* swiss nrc to latin-1 */
    ident,				/* latin-1 to latin-1 */
    xdmtol1				/* dec mcs to latin-1 */
};
