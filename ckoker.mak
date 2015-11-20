# CKOKER.MAK, Version 1.00, 30 June 1988
#           09-Nov-89 C.P.Armstrong Conversion for NMAKE
#           18-Feb-90 C.P.Armstrong Addition of Ver 5A C-kermit files
#           27-Feb-90 C.P.Armstrong Decodes CKOPTR.BOO
#           02-May-90 C.P.Armstrong Mod CL line for 6.00
#
# -- Makefile to build C-Kermit for OS/2 --
#
# Before proceeding, read the instructions below, and also read the file
# ckoker.bwr (the "beware file") if it exists.  Then run MAKE.  Note that
# the MARKEXE program may return an error code -- this can be ignored.
#
#This makefile requires the use of the Microsoft NMAKE.EXE utility
#which is supplied with Fortran 5.0 and C 6.0.  It has been tested
#with MSC 6.0 and 5.1 and the MS Presentation Manager Toolkit v1.2 and
#IBM OS/2 S.E. 1.2.
#
#The NMAKE command line should include the macro definition 
#                        MSC="6.0"
#when compiling with MS C 6.0 (or greater?).
#
#When compiling with MS C 5.1 a definition similar to that
#below must appear in the environment
#        SET INCLUDE=drive:\include\mt;\normal_includes
#as CKOKER uses the multiple thread libraries and must therefore
#reference the multiple thread include files rather than the normal
#includes.  No special references are made to the MT includes in the
#programs.  For compiling under MS C 5.1 it is necessary to have the
#Presentation Manager Toolkit includes and librarys (PM*.H and
#OS2.LIB) available.  These files are supplied with MS C 6.0.
#
#Note that the CKOFON.DLL file produced by this makefile must reside
#in one of the LIBPATH directories for correct operation of Kermit.
#This file is a font file, not a dynamic link library, however the
#.FON extension cannot be used due to an idiosyncracy of the OS/2 font
#loading function. The MS Macro Assembler (or similar) is required to
#build this font file.
#
# The result is a runnable program called "xkoker.exe" in the current directory.
# After satisfactory testing, you can rename xkoker to "kermit.exe" and put it
# in your utilities directory (or wherever).
#
#


#---------- Macros:

# If a debug log is (is not) wanted, comment out the first (second) line below.
#DEBUG=
DEBUG=/DDEBUG

# If a transaction log is (is not) wanted, comment out the first 
# (second) line below.
#TLOG=
TLOG=/DTLOG

# If Codeview support is (is not) wanted, comment out the first 
# (second) line below.
#CVIEW=/O
CVIEW=/Zi /Od

# -Afu /FPc flag removed as not compatible with -Gw required for PM
# /Gt added in attempt to avoid DGROUP bigger than 64k link error
# MSC 5.1 version of the compiler command line - 

CMPLFLAGS= \
!if "$(MSC)" == "6.0"
# MSC 6.0 compile command line.
/c /Gt128 /Gsw /MT /W2 $(CVIEW) /DOS2 /DUS_CHAR /UMSDOS $(DEBUG) $(TLOG)
!else
# MSC5.1 compile command line
/Gt128 /Alfw /G2sw /W2 /c /B1 c1l $(CVIEW) /DOS2 /DUS_CHAR /UMSDOS $(DEBUG) $(TLOG)
!endif

#---------- Inference rules:

.c.obj:
    cl $(CMPLFLAGS) $*.c

#---------- Dependencies:
# Conversion to NMAKE
ALL : xkoker.exe ckofon.dll

ckodrv.obj : ckodrv.c ckotek.h

ckostd.obj : ckostd.c ckcker.h ckofns.h ckopm.h

ckotek.obj : ckotek.c ckotek.h ckcker.h ckopm.h

ckopm1.obj : ckopm1.c  ckopm.h ckorc.h

ckopm2.obj : ckopm2.c  ckopm.h

ckopm3.obj : ckopm3.c  ckorc.h ckcker.h ckotek.h

ckopm4.obj : ckopm4.c ckcker.h ckopm.h ckorc.h

ckopm5.obj : ckopm5.c ckorc.h

ckopm6.obj : ckopm6.c ckopm.h ckorc.h

ckcmai.obj : ckcmai.c ckcker.h ckcdeb.h ckcsym.h

ckuusr.obj : ckuusr.c ckucmd.h ckcker.h ckuusr.h ckcdeb.h

ckuus2.obj : ckuus2.c ckucmd.h ckcker.h ckuusr.h ckcdeb.h

ckuus3.obj: ckuus3.c ckucmd.h ckcker.h ckuusr.h ckcdeb.h

ckuus4.obj: ckuus4.c ckcdeb.h ckcker.h ckucmd.h ckuusr.h ckcxla.h ckoxla.h

ckuus5.obj: ckuus5.c ckucmd.h ckcker.h ckuusr.h ckcdeb.h

ckucmd.obj: ckucmd.c ckucmd.h ckcdeb.h

ckcpro.obj: ckcpro.c ckcker.h ckcdeb.h

ckcfns.obj: ckcfns.c ckcker.h ckcdeb.h ckcsym.h

ckcfn2.obj: ckcfn2.c ckcker.h ckcdeb.h ckcsym.h

ckcfn3.obj: ckcfn3.c ckcdeb.h ckcker.h ckcxla.h ckoxla.h

ckofio.obj: ckofio.c ckcker.h ckcdeb.h ckofns.h

ckotio.obj: ckotio.c ckcdeb.h ckofns.h

ckocon.obj: ckocon.c ckcker.h ckcdeb.h ckofns.h ckopm.h

ckudia.obj: ckudia.c ckcker.h ckcdeb.h ckucmd.h

ckuscr.obj: ckuscr.c ckcker.h ckcdeb.h

ckcxla.obj: ckcxla.c ckcdeb.h ckcker.h ckucmd.h ckcxla.h ckoxla.h

#ckoptr.ptr: ckoptr.boo
#        bootoexe ckoptr.boo

ckorc.res: ckorc.rc ckoptr.ptr ckorc.h ckoker.icn
        rc -r ckorc.rc

xkoker.exe: ckcpro.obj  ckoker.def \
            ckorc.res  \
            ckoptr.ptr \
            ckodrv.obj \
            ckcfn3.obj \
            ckcxla.obj \
            ckostd.obj \
            ckotek.obj \
            ckopm1.obj \
            ckopm2.obj \
            ckopm3.obj \
            ckopm4.obj \
            ckopm5.obj \
            ckopm6.obj \
            ckcmai.obj \
            ckucmd.obj \
            ckuusr.obj \
            ckuus2.obj \
            ckuus3.obj \
            ckuus4.obj \
            ckuus5.obj \
            ckcfns.obj \
            ckcfn2.obj \
            ckocon.obj \
            ckotio.obj \
            ckofio.obj \
            ckudia.obj \
            ckuscr.obj
    link @ckoker.lnk
    rc ckorc.res xkoker.exe

ckofon.obj: ckofon.asm
    masm ckofon.asm;

ckofon.res: ckofon.rc ckotek.fnt
    rc -r ckofon.rc

ckofon.dll: ckofon.obj ckofon.res
    link ckofon.obj,,,,ckofon.def
    rc ckofon.res ckofon.dll
    copy ckofon.dll c:\os2\dll
