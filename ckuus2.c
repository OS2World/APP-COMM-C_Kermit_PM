/*  C K U U S 2  --  "User Interface" STRINGS module for Unix Kermit  */
 
/*
 Author: Frank da Cruz (fdc@columbia.edu, FDCCU@CUVMA.BITNET),
 Columbia University Center for Computing Activities.
 First released January 1985.
 Copyright (C) 1985, 1990, Trustees of Columbia University in the City of New 
 York.  Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as it is not sold for profit, provided this
 copyright notice is retained. 
*/
 
/*  This module separates long strings from the body of the ckuser module. */  
 
#include <stdio.h>
#include <ctype.h>
#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#include "ckucmd.h"
#include "ckuusr.h"
#include "ckcxla.h"
 
extern CHAR mystch, stchr, eol, seol, padch, mypadc, ctlq;
extern CHAR *data, *rdatap, ttname[];
extern char cmdbuf[], line[], debfil[], pktfil[], sesfil[], trafil[];
extern int nrmt, nprm, dfloc, deblog, seslog, speed, local, parity, duplex;
extern int turn, turnch, pktlog, tralog, mdmtyp, flow, cmask, timef, spsizf;
extern int rtimo, timint, srvtim, npad, mypadn, bctr, delay;
extern int maxtry, spsiz, urpsiz, maxsps, maxrps, ebqflg, ebq;
extern int rptflg, rptq, fncnv, binary, pktlog, warn, quiet, fmask, keep;
extern int tsecs, bctu, len, atcapu, lpcapu, swcapu, sq, rpsiz;
extern int wslots, wslotsn, wslotsr, capas, atcapr;
extern int spackets, rpackets, timeouts, retrans, crunched, wmax;
extern int fcharset, tcharset, tslevel;
extern int rptn, success, language, nlng;
extern int xxstring();

extern long filcnt, tfc, tlci, tlco, ffc, flci, flco;
extern char *dftty, *versio, *ckxsys;
extern struct langinfo langs[];
extern struct keytab prmtab[];
extern struct keytab remcmd[];
extern struct keytab lngtab[];
extern struct csinfo fcsinfo[], tcsinfo[];
 
static
char *hlp1[] = {
"\n",
"Usage: kermit [-x arg [-x arg]...[-yyy]..]]\n",
"  -x is an option requiring an argument, -y an option with no argument:\n\n",
"actions:\n",
"  -s files  send files                    -r  receive files\n",
"  -s -      send files from stdin         -k  receive files to stdout\n",
"  -x        enter server mode             -f  finish remote server\n\n",
"  -g files  get remote files from server (quote wildcards)\n",
"  -a name   alternate name, used with -s, -r, -g\n",
"  -c        connect (before file transfer), used with -l and -b\n",
"  -n        connect (after file transfer), used with -l and -b\n\n",
"settings:\n",
"  -l line   communication line device     -q  quiet during file transfer\n",
"  -b bps    line speed, e.g. 1200         -i  binary file transfer\n",
"  -p x      parity, x = e,o,m,s, or n     -t  half duplex, xon handshake\n",
"  -y name   alternate init file name      -d  log debug info to debug.log\n",
"  -e n      receive packet length         -w  don't write over files\n\n",
"If no action command is included, enter interactive dialog.\n",
""
};
 
/*  U S A G E */
 
usage() {
    conola(hlp1);
}

 
/*  Help string definitions  */
 
static char *tophlp[] = { "\n\
Type ? for a list of commands, type 'help x' for any command x.\n\
While typing commands, use the following special characters:\n\n\
 DEL, RUBOUT, BACKSPACE, CTRL-H: Delete the most recent character typed.\n\
 CTRL-W:  Delete the most recent word typed.\n",
 
"\
 CTRL-U:  Delete the current line.\n\
 CTRL-R:  Redisplay the current line.\n\
 ?        (question mark) Display a menu for the current command field.\n\
 ESC      (or TAB) Attempt to complete the current field.\n",
 
"\
 \\        (backslash) include the following character literally.\n\n\
From system level, type 'kermit -h' to get help about command line args.\
\n",
"" };
 
static char *hmxxbye = "\
Shut down and log out a remote Kermit server";
 
static char *hmxxclo = "\
Close one of the following logs:\n\
 session, transaction, packet, debugging -- 'help log' for further info.";
 
static char *hmxxcon = "\
Connect to a remote system via the tty device given in the\n\
most recent 'set line' command";
 
static char *hmxxget = "\
Format: 'get filespec'.  Tell the remote Kermit server to send the named\n\
files.  If filespec is omitted, then you are prompted for the remote and\n\
local filenames separately.";
 
static char *hmxxlg[] = { "\
Record information in a log file:\n\n\
 debugging             Debugging information, to help track down\n\
  (default debug.log)  bugs in the C-Kermit program.\n\n\
 packets               Kermit packets, to help track down protocol problems.\n\
  (packet.log)\n\n",
 
" session               Terminal session, during CONNECT command.\n\
  (session.log)\n\n\
 transactions          Names and statistics about files transferred.\n\
  (transact.log)\n",
"" } ;

 
static char *hmxxlogi[] = { "\
Syntax: script text\n\n",
"Login to a remote system using the text provided.  The login script\n",
"is intended to operate similarly to uucp \"L.sys\" entries.\n",
"A login script is a sequence of the form:\n\n",
"   expect send [expect send] . . .\n\n",
"where 'expect' is a prompt or message to be issued by the remote site, and\n",
"'send' is the names, numbers, etc, to return.  The send may also be the\n",
"keyword EOT, to send control-d, or BREAK, to send a break.  Letters in\n",
"send may be prefixed by ~ to send special characters.  These are:\n",
"~b backspace, ~s space, ~q '?', ~n linefeed, ~r return, ~c don\'t\n",
"append a return, and ~o[o[o]] for octal of a character.  As with some \n",
"uucp systems, sent strings are followed by ~r unless they end with ~c.\n\n",
"Only the last 7 characters in each expect are matched.  A null expect,\n",
"e.g. ~0 or two adjacent dashes, causes a short delay.  If you expect\n",
"that a sequence might not arrive, as with uucp, conditional sequences\n",
"may be expressed in the form:\n\n",
"   -send-expect[-send-expect[...]]\n\n",
"where dashed sequences are followed as long as previous expects fail.\n",
"" };
 
static char *hmxxrc[] = { "\
Format: 'receive [filespec]'.  Wait for a file to arrive from the other\n\
Kermit, which must be given a 'send' command.  If the optional filespec is\n",
 
"given, the (first) incoming file will be stored under that name, otherwise\n\
it will be stored under the name it arrives with.",
"" } ;
 
static char *hmxxsen = "\
Format: 'send file1 [file2]'.  File1 may contain wildcard characters '*' or\n\
'?'.  If no wildcards, then file2 may be used to specify the name file1 is\n\
sent under; if file2 is omitted, file1 is sent under its own name.";
 
static char *hmxxser = "\
Enter server mode on the currently selected line.  All further commands\n\
will be taken in packet form from the other Kermit program.";
 
static char *hmhset[] = { "\
The 'set' command is used to establish various communication or file\n",
"parameters.  The 'show' command can be used to display the values of\n",
"'set' parameters.  Help is available for each individual parameter;\n",
"type 'help set ?' to see what's available.\n",
"" } ;
 
static char *hmxychkt[] = { "\
Type of packet block check to be used for error detection, 1, 2, or 3.\n",
"Type 1 is standard, and catches most errors.  Types 2 and 3 specify more\n",
"rigorous checking at the cost of higher overhead.  Not all Kermit programs\n",
"support types 2 and 3.\n",
"" } ;

 
static char *hmxyf[] = { "\
set file: character-set, names, type, warning, display.\n\n",
"'character-set' tells the encoding of the local file, ASCII by default.\n",
"The names 'dutch', 'german', 'french', etc, refer to 7-bit ASCII-based\n",
"national replacement character sets (NRC).  Latin-1 is the 8-bit ISO 8859\n", 
"Latin Alphabet 1.\n\n",
"'names' are normally 'converted', which means file names are converted\n",
"to 'common form' during transmission; 'literal' means use filenames\n",
"literally (useful between like systems).\n\n",
"'type' is normally 'text', in which conversion is done between Unix\n",
"newlines and CRLF line delimiters; 'binary' means to do no conversion.\n",
"Use 'binary' for executable programs or binary data.\n\n",
"'warning' is 'on' or 'off', normally off.  When off, incoming files will\n",
"overwrite existing files of the same name.  When on, new names will be\n",
"given to incoming files whose names are the same as existing files.\n",
"\n\
'display' is normally 'on', causing file transfer progress to be displayed\n",
"on your screen when in local mode.  'set display off' is useful for\n",
"allowing file transfers to proceed in the background.\n\n",
"" } ;
 
static char *hmhrmt[] = { "\
The 'remote' command is used to send file management instructions to a\n",
"remote Kermit server.  There should already be a Kermit running in server\n",
"mode on the other end of the currently selected line.  Type 'remote ?' to\n",
"see a list of available remote commands.  Type 'help remote x' to get\n",
"further information about a particular remote command 'x'.\n",
"" } ;

 
/*  D O H L P  --  Give a help message  */
 
dohlp(xx) int xx; {
    int x,y;
 
    if (xx < 0) return(xx);
    switch (xx) {
 
case XXASS:             /* assign */
    return(hmsg("\
Syntax:  ASSIGN variablename string.\n\
Example: ASSIGN \\%a My name is \\%b.\n\
Assigns the current value of the string to the variable."));

case XXASK:             /* ask */
    return(hmsg("\
Syntax:  ASK variablename prompt\n\
Example: ASK %n What is your name\\63\\32\n\
Issues the prompt and defines the variable to be whatever you type in."));

case XXASKQ:
    puts("\
Syntax:  ASKQ variablename prompt");
    puts("\
Example: ASKQ %p Password:");
    puts("\
Issues the prompt and defines the variable to be whatever you type in.");
    puts("\
The characters that you type do not echo on the screen.");
    return(0);

case XXBYE:             /* bye */
    return(hmsg(hmxxbye));
 
case XXCLE:             /* clear */
    return(hmsg("\
Clear the serial port input buffer."));

case XXCLO:             /* close */
    return(hmsg(hmxxclo));
 
case XXCOM:             /* comment */
    return(hmsg("\
Introduce a comment.  Beginning of command line only."));

case XXCON:             /* connect */
    return(hmsg(hmxxcon));
 
case XXCWD:             /* cd / cwd */
#ifdef vms
    return(hmsg("\
Change Working Directory, equivalent to VMS SET DEFAULT command"));
#else
#ifdef datageneral
    return(hmsg("Change Working Directory, equivalent to DG 'dir' command"));
#else
#ifdef OS2
   return(hmsg("Change Working Directory, equivalent to OS2 'chdir' command"));
#else
    return(hmsg("Change Working Directory, equivalent to Unix 'cd' command"));
#endif
#endif
#endif
 
case XXDEF:             /* define */
    puts("\
Syntax: DEFINE name command, command, command, ...");
    puts("\
Defines a Kermit command macro called 'name'.  The definition is");
    puts("\
a comma-separated list of Kermit commands.  Use the DO command to");
    puts("\
execute the macro, or just type its name, followed optionally by arguments.");
    return(0);

case XXDEL:             /* delete */
    return(hmsg("Delete a local file or files"));
 
case XXDIAL:                /* dial */
    return(hmsg("\
Dial a number using modem autodialer.  First you must SET MODEM, then\n\
SET LINE and SET SPEED.  Then just type the DIAL command, followed by\n\
the phone number."));
 
case XXDIR:             /* directory */
    return(hmsg("Display a directory of local files"));
 
case XXDO:              /* do */
    return(hmsg("Execute a macro that was defined by the DEFINE command.\n\
The word DO can be omitted.  Trailing arguments, if any, are automatically\n\
assigned to the parameters \\%1, \\%2, etc."));

case XXDEC:
   puts("\
Decrement (subtract one from) the value of a \\% variable if the value is"); 
   puts("\
currently numeric.");
    return(0);

case XXECH:             /* echo */
    return(hmsg("Display the rest of the command on the terminal,\n\
useful in command files.  Echo string may contain backslash codes."));
 
case XXEND:             /* end */
    return(hmsg("\
Exit from the current macro or TAKE file, back to wherever invoked from."));

case XXEXI:             /* exit */
case XXQUI:
    return(hmsg("Exit from the Kermit program, closing any open logs."));
 
case XXEXE:
    puts("\
Syntax: EVALUATE text");
    puts("\
Evaluate all the variables and other backslash escapes in the text, and");
    puts("\
then attempt to execute the result as a Kermit command");
    return(0);

case XXFIN:
    return(hmsg("\
Tell the remote Kermit server to shut down without logging out."));
 
case XXGET:
    return(hmsg(hmxxget));
 
case XXGOTO:
    return(hmsg("\
In a TAKE file or macro, go to the given label.  A label is a word on the\n\
left margin that starts with a colon (:)."));

case XXHAN:
    return(hmsg("Hang up the phone."));    

case XXHLP:
    return(hmsga(tophlp));
 
case XXIF:
    puts("\
Syntax: IF [NOT] condition command");
    puts("\
If the condition is (is not) true, do the command.  Conditions are");
    puts("\
SUCCESS, FAILURE, DEFINED (variable or macro), COUNT (loop), EXIST (file),");
    puts("\
EQUAL (strings), = (numbers or numeric variables), < (ditto), > (ditto).");
    return(0);

case XXINC:
   puts("\
Increment (add one to) the value of a \\% variable if the value is currently");
   puts("\
numeric.");
    return(0);

case XXINP:
   return(hmsg("\
Syntax: INPUT n string\n\
Wait up to n seconds for the string to arrive on the communication line.\n\
For use with IF FAILURE and IF SUCCESS."));

case XXREI:
    return(hmsg("\
Syntax: REINPUT n string\n\
Look for the string in the text that has recently been INPUT, set SUCCESS\n\
or FAILURE accordingly.  Timeout, n, must be specified but is ignored."));

case XXLBL:
    return(hmsg("\
Introduce a label, like :loop, for use with GOTO in TAKE files or macros."));

case XXLOG:
    return(hmsga(hmxxlg));
 
case XXLOGI:
    return(hmsga(hmxxlogi));
 
case XXOUT:
    return(hmsg("\
Send the string out the currently selected line, as if you had typed it\n\
during CONNECT mode."));

case XXPAU:
    puts("\
Syntax:  PAUSE n");
    puts("\
Example: PAUSE 3");
    puts("\
Do nothing for the specified number of seconds.");
    puts("\
If interrupted from the keyboard, set FAILURE, otherwise set SUCCESS.");
    return(0);

case XXPWD:
    return(hmsg("Print the name of my current working directory."));

case XXREC:
    return(hmsga(hmxxrc));
 
case XXREM:
    if ((y = cmkey(remcmd,nrmt,"Remote command","",xxstring)) == -2) return(y);
    if (y == -1) return(y);
    if ((x = cmcfm()) < 0) return(x);
    return(dohrmt(y));
 
case XXSEN:
    return(hmsg(hmxxsen));
 
case XXSER:
    return(hmsg(hmxxser));
 
case XXSET:
    if ((y = cmkey(prmtab,nprm,"Parameter","",xxstring)) == -2) return(y);
    if (y == -2) return(y);
    if ((x = cmcfm()) < 0) return(x);
    return(dohset(y));
 
case XXSHE:
#ifdef vms
    return(hmsg("\
Issue a command to VMS."));
#else
#ifdef AMIGA
    return(hmsg("\
Issue a command to CLI."));
#else
#ifdef datageneral
    return(hmsg("\
Issue a command to the CLI."));
#else
#ifdef OS2
    return(hmsg("\
Issue a command to CMD.EXE."));
#else
    return(hmsg("\
Issue a command to the Unix shell."));
#endif
#endif
#endif
#endif
 
case XXSHO:
    return(hmsg("\
Display current values of various categories of SET parameters.\n\
Type SHOW ? for a list of categories."));
 
case XXSPA:
#ifdef datageneral
    return(hmsg("\
Display disk usage in current device, directory,\n\
or return space for a specified device, directory."));
#else
    return(hmsg("Display disk usage in current device, directory"));
#endif
 
case XXSTA:
    return(hmsg("Display statistics about most recent file transfer"));
 
case XXSTO:
    return(hmsg("\
Stop executing the current macro or TAKE file and return immediately\n\
to the C-Kermit prompt."));

case XXTAK:
    return(hmsg("\
Take Kermit commands from the named file.  Kermit command files may\n\
themselves contain 'take' commands, up to a reasonable depth of nesting."));
 
case XXTRA:
    puts("\
Syntax: TRANSMIT file n\n\
Raw upload. Send a file, a line at a time (text) or a character at a time.");
    puts("\
For text, wait for turnaround character after each line.");
    puts("\
n = decimal ASCII value of turnaround character, 10 (linefeed) by default.");
    puts("\
Specify 0 for no waiting.  TRANSMIT may be interrupted by Ctrl-C.");
    return(0);

case XXTYP:
    return(hmsg("\
Display a file on the screen.  Pauses if you type Ctrl-S, resumes if you\n\
type Ctrl-Q, returns immediately to C-Kermit prompt if you type Ctrl-C."
));

case XXXLA:
    puts("\
Syntax: TRANSLATE file1 file2");
    puts("\
Translates file1 from the character set named in the most recent");
    puts("\
'set file character-set' command into the character set named in the most");
    puts("\
recent 'set transfer character-set' command, storing the result in file2.");
    return(0);

case XXVER:
    puts("Displays the program version number.");
    return(0);

case XXWAI:
    puts("\
Syntax:  WAIT n [modem-signal(s)]");
    puts("\
Example: WAIT 5 \\cd\\cts");
    puts("\
 Waits up to the given number of seconds for all of the specified signals.");
    puts("\
 Sets FAILURE if signals do not appear in given time or if interrupted by");
    puts("\
 typing anything at the keyboard during the waiting period.");
    puts("\
Signals: \\cd = Carrier Detect, \\dsr = Dataset Ready, \\cts = Clear To Send");
    puts("\
Warning: This command does not work yet, signals are ignored.");
    return(0);

default:
    if ((x = cmcfm()) < 0) return(x);
    printf("Not available yet - %s\n",cmdbuf);
    break;
    }
    return(success = 0);
}

 
/*  H M S G  --  Get confirmation, then print the given message  */
 
hmsg(s) char *s; {
    int x;
    if ((x = cmcfm()) < 0) return(x);
    puts(s);
    return(0);
}
 
hmsga(s) char *s[]; {           /* Same function, but for arrays */
    int x, i;
    if ((x = cmcfm()) < 0) return(x);
    for ( i = 0; *s[i] ; i++ ) /* fputs(s[i], stdout); */
      printf("%s",s[i]);
    printf("\n");
    return(0);
}

 
/*  D O H S E T  --  Give help for SET command  */
 
dohset(xx) int xx; {
    
    if (xx == -3) return(hmsga(hmhset));
    if (xx < 0) return(xx);
    switch (xx) {
 
case XYATTR:
    puts("Turn Attribute packet exchange off or on");
    return(0);

case XYIFD:
    puts("Discard or Keep incompletely received files, default is discard");
    return(0);

case XYINPU:
    puts("The SET INPUT command controls the behavior of the INPUT command.");
    puts("SET INPUT CASE { IGNORE, OBSERVE }");
    puts("\
  Tells whether alphabetic case is to be significant in string comparisons.");
    puts("SET INPUT DEFAULT-TIMEOUT secs");
    puts("\
  Establishes default timeout for INPUT commands.  Currently has no effect.");
    puts("SET INPUT ECHO { ON, OFF }");
    puts("\
  Tells whether to display arriving characters read by INPUT on the screen.");
    puts("SET INPUT TIMEOUT-ACTION { PROCEED, QUIT }");
    puts("\
  Tells whether to proceed or quit from a script program if an INPUT command");
    puts("\
  fails.  PROCEED allows the use of IF SUCCESS and IF FAILURE commands.");
    return(0);

case XYCHKT:
    return(hmsga(hmxychkt));
 
case XYCOUN:
    return(hmsg("\
Set up a loop counter, for use with IF COUNT."));

case XYDELA: 
    puts("\
Number of seconds to wait before sending first packet after 'send' command.");
    return(0);
 
case XYTAKE:
    puts("\
Controls behavior of TAKE command.  SET TAKE ECHO { ON, OFF } tells whether");
    puts("\
commands read from a TAKE file should be displayed on the screen.");
    puts("\
SET TAKE ERROR { ON, OFF } tells whether a TAKE command file should be");
    puts("\
automatically terminated upon a command error.");
    return(0);

case XYTERM:
#ifdef OS2
    puts("\
'SET TERMINAL BYTESIZE 7 or 8'   to use 7- or 8-bit terminal characters.\n\
'SET TERMINAL VT100 or TEK4014'  to set terminal emulation mode.\n");
#else
    puts("\
'set terminal bytesize 7 or 8' to use 7- or 8-bit terminal characters.");
#endif
    return(0);

case XYDUPL:
    puts("\
During 'connect': 'full' means remote host echoes, 'half' means this program");
    puts("does its own echoing.");
    return(0);
 
case XYESC:
    printf("%s","\
Decimal ASCII value for escape character during 'connect', normally 28\n\
(Control-\\)\n");
    return(0);
 
case XYFILE:
    return(hmsga(hmxyf));
 
case XYFLOW:
#ifdef OS2
    puts("Sets both the flow control and hardware handshaking parameters.\n");
    puts("The choices for flow control are;\n");
    puts("  'none' 'xon/xoff'\n");
    puts("and for hardware handshaking;\n");
    puts("  'odsr' 'idsr' 'ocsr'       These can be set 'on' or 'off'\n");
#else
    puts("\
Type of flow control to be used.  Choices are 'xon/xoff' and 'none'.");
    puts("normally xon/xoff.");
#endif 
    return(0);
case XYHAND:
    puts("\
Decimal ASCII value for character to use for half duplex line turnaround");
    puts("handshake.  Normally, handshaking is not done.");
    return(0);

#ifdef NETCONN
case XYNETW:
    puts("\
Select a TCP/IP network host.  Use this command instead of SET LINE.");
    puts("\
After SET HOST give the hostname or IP host number, followed optionally");
    puts("\
by a colon and a service name or socket number (default is telnet)");
    return(0);
#endif /* NETCONN */

case XYLANG:
    puts("Select the language for text-mode file transfer.");
    puts("\
Equivalent to 'set file character-set', except that 'set language'");
    puts("\
also selects the appropriate transfer character set for the given");
    puts("\
language.  If you specify 'english' or 'ascii', the transfer-syntax");
    puts("\
character-set will be ASCII, otherwise it will be Latin-1.");
    return(0);

case XYLINE:
    printf("\
Device name of communication line to use.  Normally %s.\n",dftty);
    if (!dfloc) {
    printf("\
If you set the line to other than %s, then Kermit\n",dftty);
    printf("\
will be in 'local' mode; 'set line' will reset Kermit to remote mode.\n");
    puts("\
If the line has a modem, and if the modem-dialer is set to direct, this");
    puts("\
command causes waiting for a carrier detect (e.g. on a hayes type modem).");
    puts("\
This can be used to wait for incoming calls.");
    puts("\
To use the modem to dial out, first set modem-dialer (e.g., to hayes), then");
    puts("set line, next issue the dial command, and finally connect.");
    }
    return(0);
 
case XYMACR:
    puts("\
Controls the behavior of macros.  SET MACRO ECHO { ON, OFF } tells whether");
    puts("\
commands executed from a macro definition should be displayed on the screen.");
    puts("\
SET MACRO ERROR { ON, OFF } tells whether a macro should be automatically");
    puts("\
terminated upon a command error.");
    return(0);

case XYMODM:
    puts("\
Type of modem for dialing remote connections.  Needed to indicate modem can");
    puts("\
be commanded to dial without 'carrier detect' from modem.  Many recently");
    puts("\
manufactured modems use 'hayes' protocol.  Type 'set modem ?' to see what");
    puts("\
types of modems are supported by this program.");
    return(0);
 
 
case XYPARI:
    puts("Parity to use during terminal connection and file transfer:");
    puts("even, odd, mark, space, or none.  Normally none.");
    return(0);
 
case XYPROM:
    puts("Prompt string for this program, normally 'C-Kermit>'.");
    return(0);
 
case XYRETR:
    puts("\
How many times to retransmit a particular packet before giving up");
    return(0);

case XYSPEE:
    puts("\
Communication line speed for external tty line specified in most recent");
#ifdef AMIGA
    puts("\
'set line' command.  Any baud rate between 110 and 292000, although you");
    puts(" will receive a warning if you do not use a standard baud rate:");
    puts("\
110, 150, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600.");
#else
#ifdef datageneral
    puts("\
'set line' command.  Any of the common baud rates:");
    puts(" 0, 50, 75, 110, 134, 150, 300, 600, 1200, 1800, ");
    puts(" 2400, 3600, 7200, 4800, 9600, 19200, 38400.");
#else
    puts("\
'set line' command.  Any of the common baud rates:");
#ifdef OS2
    puts(" 0, 110, 150, 300, 600, 1200, 2400, 4800, 9600, 19200.");
    puts("The highest speed available is hardware-dependant");
#else
    puts(" 0, 110, 150, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200.");
#endif
#endif
#endif
    return(0);

case XYRECV:
    puts("\
Specify parameters for inbound packets:");
    puts("\
End-Of-Packet (ASCII value), Packet-Length (1000 or less),");
    puts("\
Padding (amount, 94 or less), Pad-Character (ASCII value),");
    puts("\
Start-Of-Packet (ASCII value), and Timeout (94 seconds or less),");
    puts("\
all specified as decimal numbers.");
    return(0);
 
case XYSEND:
    puts("\
Specify parameters for outbound packets:");
    puts("\
End-Of-Packet (ASCII value), Packet-Length (2000 or less),");
    puts("\
Padding (amount, 94 or less), Pad-Character (ASCII value),");
    puts("\
Start-Of-Packet (ASCII value), and Timeout (94 seconds or less),");
    puts("\
all specified as decimal numbers.");
    return(0);
 
case XYSERV:
    puts("server timeout:");
    puts("\
Server command wait timeout interval, how often the C-Kermit server issues");
    puts("\
a NAK while waiting for a command packet.  Specify 0 for no NAKs at all.");
    return(0);

case XYUNCS:
   return(hmsg("\
DISCARD (default) means reject any arriving files encoded in unknown\n\
character sets.  KEEP means to accept them anyway."));

case XYWIND:
    puts("window slots for sliding windows:");
    puts("\
How many packets can be transmitted before pausing for acknowledgement.");
    puts("\
Default is one, maximum is 31.  Large window size may result in reduction");
    puts("\
of packet length.  Use sliding windows for increased efficiency on");
    puts("\
connections with long delays.");
    return(0);

case XYXFER:
    puts("Select the presentation of textual data in Kermit packets:");
    puts("\
normal: ASCII text, lines terminated by CRLF.");
    puts("\
character-set <name>: a single character set other than ASCII, such as");
    puts("\
ISO 8859-1 Latin Alphabet 1 ('latin-1').");
    puts("\
international: a mixture of character sets (not yet implemented).");
    return(0);

default:
    printf("Not available yet - %s\n",cmdbuf);
    return(0);
    }
}

 
/*  D O H R M T  --  Give help about REMOTE command  */
 
dohrmt(xx) int xx; {
    int x;
    if (xx == -3) return(hmsga(hmhrmt));
    if (xx < 0) return(xx);
    switch (xx) {
 
case XZCWD:
    return(hmsg("\
Ask remote Kermit server to change its working directory."));
 
case XZDEL:
    return(hmsg("\
Ask remote Kermit server to delete the named file(s)."));
 
case XZDIR:
    return(hmsg("\
Ask remote Kermit server to provide directory listing of the named file(s)."));
 
case XZHLP:
    return(hmsg("\
Ask remote Kermit server to tell you what services it provides."));
 
case XZHOS:
    return(hmsg("\
Send a command to the remote system in its own command language\n\
through the remote Kermit server."));
 
case XZSPA:
    return(hmsg("\
Ask the remote Kermit server to tell you about its disk space."));
 
case XZTYP:
    return(hmsg("\
Ask the remote Kermit server to type the named file(s) on your screen."));
 
case XZWHO:
    return(hmsg("\
Ask the remote Kermit server to list who's logged in, or to give information\n\
about the specified user."));
 
default:
    if ((x = cmcfm()) < 0) return(x);
    printf("not working yet - %s\n",cmdbuf);
    return(-2);
    }
}
