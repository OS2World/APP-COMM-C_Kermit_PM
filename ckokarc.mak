# NMAKE file for archiving the OS/2 kermit source files
# the macro "orig" must be defined on the command line to represent the
# full source directory pathname.
# e.g. C:\chris\c4foruts\    Note the final \
#
# Make template;
#: $(orig)
#    copy $(orig)



ALL :   CKOKARC.MAK \
        CKOKER.MAK \
        CKOKER.LNK \
        ckotek.c \
        ckodrv.c \
        ckostd.c \
        CKOPM1.C \
        CKOPM2.C \
        ckopm3.c \
        ckopm4.c \
        ckopm5.c \
        ckopm6.c \
        CKCMAI.C \
        CKUCMD.C \
        CKUUSR.C \
        CKUUS2.C \
        CKUUS3.C \
        CKUUS4.C \
        CKUUS5.C \
        CKCPRO.C \
        CKCFNS.C \
        CKCFN2.C \
        CKOCON.C \
        CKCFN3.C \
        CKCXLA.C \
        CKCXLA.H \
        CKCASC.H \
        CKOTIO.C \
        CKOFIO.C \
        CKUDIA.C \
        CKUSCR.C \
        CKCDEB.H \
        CKCKER.H \
        CKCSYM.H \
        CKUCMD.H \
        CKUUSR.H \
        CKOFNS.H \
        CKOPM.H  \
        ckorc.h  \
        ckoxla.h \
        ckokey.h \
        ckotek.h \
        ckorc.rc \
        ckoptr.ptr \
        ckoker.icn \
        ckoker.def \
        ckofon.asm \
        ckofon.def \
        ckofon.rc \
        ckotek.fnt

CKOKARC.MAK: $(orig)CKOKARC.MAK
    copy $(orig)CKOKARC.MAK CKOKARC.MAK

CKOKER.MAK: $(orig)CKOKER.MAK
    copy $(orig)CKOKER.MAK CKOKER.MAK

CKOKER.LNK: $(orig)CKOKER.LNK
    copy $(orig)CKOKER.LNK CKOKER.LNK

ckoker.upd : $(orig)ckoker.upd
    copy $(orig)ckoker.upd ckoker.upd

ckoptr.ptr: $(orig)ckoptr.ptr
    copy $(orig)ckoptr.ptr ckoptr.ptr

ckoker.icn: $(orig)ckoker.icn
    copy $(orig)ckoker.icn ckoker.icn

ckorc.rc: $(orig)ckorc.rc
    copy $(orig)ckorc.rc ckorc.rc

ckostd.c: $(orig)ckostd.c
    copy $(orig)ckostd.c ckostd.c

ckotek.c: $(orig)ckotek.c
    copy $(orig)ckotek.c ckotek.c

ckodrv.c : $(orig)ckodrv.c
    copy $(orig)ckodrv.c ckodrv.c

ckotek.h : $(orig)ckotek.h
    copy $(orig)ckotek.h ckotek.h

CKOPM1.C: $(orig)CKOPM1.C
    copy $(orig)CKOPM1.C CKOPM1.C

CKOPM2.C: $(orig)CKOPM2.C
    copy $(orig)CKOPM2.C CKOPM2.C

ckopm3.c: $(orig)ckopm3.c
    copy $(orig)ckopm3.c ckopm3.c

ckopm4.c: $(orig)ckopm4.c
    copy $(orig)ckopm4.c ckopm4.c

ckopm5.c: $(orig)ckopm5.c
    copy $(orig)ckopm5.c ckopm5.c

ckopm6.c: $(orig)ckopm6.c
    copy $(orig)ckopm6.c ckopm6.c

CKCMAI.C: $(orig)CKCMAI.C
    copy $(orig)CKCMAI.C CKCMAI.C

CKUUS4.C :  $(orig)CKUUS4.C
               copy $(orig)CKUUS4.C CKUUS4.C
           
CKUUS5.C : $(orig)CKUUS5.C
             copy $(orig)CKUUS5.C CKUUS5.C
         
CKCFN3.C : $(orig)CKCFN3.C
             copy $(orig)CKCFN3.C CKCFN3.C
         
CKCXLA.C : $(orig)CKCXLA.C
             copy $(orig)CKCXLA.C CKCXLA.C
         
CKCXLA.H : $(orig)CKCXLA.H
             copy $(orig)CKCXLA.H CKCXLA.H
         
CKCASC.H : $(orig)CKCASC.H
             copy $(orig)CKCASC.H CKCASC.H
         
CKUCMD.C: $(orig)CKUCMD.C
    copy $(orig)CKUCMD.C CKUCMD.C

CKUUSR.C: $(orig)CKUUSR.C
    copy $(orig)CKUUSR.C CKUUSR.C

CKUUS2.C: $(orig)CKUUS2.C
    copy $(orig)CKUUS2.C CKUUS2.C

CKUUS3.C: $(orig)CKUUS3.C
    copy $(orig)CKUUS3.C CKUUS3.C

CKCPRO.C: $(orig)CKCPRO.C
    copy $(orig)CKCPRO.C CKCPRO.C

CKCFNS.C: $(orig)CKCFNS.C
    copy $(orig)CKCFNS.C CKCFNS.C

CKCFN2.C: $(orig)CKCFN2.C
    copy $(orig)CKCFN2.C CKCFN2.C

CKOCON.C: $(orig)CKOCON.C
    copy $(orig)CKOCON.C CKOCON.C

CKOTIO.C: $(orig)CKOTIO.C
    copy $(orig)CKOTIO.C CKOTIO.C

CKOFIO.C: $(orig)CKOFIO.C
    copy $(orig)CKOFIO.C CKOFIO.C

CKUDIA.C: $(orig)CKUDIA.C
    copy $(orig)CKUDIA.C CKUDIA.C

CKUSCR.C: $(orig)CKUSCR.C
    copy $(orig)CKUSCR.C CKUSCR.C

CKCDEB.H: $(orig)CKCDEB.H
    copy $(orig)CKCDEB.H CKCDEB.H

CKCKER.H: $(orig)CKCKER.H
    copy $(orig)CKCKER.H CKCKER.H

CKCSYM.H: $(orig)CKCSYM.H
    copy $(orig)CKCSYM.H CKCSYM.H

CKUCMD.H: $(orig)CKUCMD.H
    copy $(orig)CKUCMD.H CKUCMD.H

CKUUSR.H: $(orig)CKUUSR.H
    copy $(orig)CKUUSR.H CKUUSR.H

CKOFNS.H: $(orig)CKOFNS.H
    copy $(orig)CKOFNS.H CKOFNS.H

CKOPM.H: $(orig)CKOPM.H
    copy $(orig)CKOPM.H CKOPM.H

ckorc.h: $(orig)ckorc.h
    copy $(orig)ckorc.h ckorc.h

ckokey.h: $(orig)ckokey.h
    copy $(orig)ckokey.h ckokey.h

ckoxla.h: $(orig)ckoxla.h
    copy $(orig)ckoxla.h ckoxla.h

CKOKER.DEF: $(orig)CKOKER.DEF
    copy $(orig)CKOKER.DEF CKOKER.DEF

ckofon.asm: $(orig)ckofon.asm
          copy $(orig)ckofon.asm ckofon.asm

ckofon.def: $(orig)ckofon.def
          copy $(orig)ckofon.def ckofon.def

ckofon.rc: $(orig)ckofon.rc
          copy $(orig)ckofon.rc ckofon.rc

ckotek.fnt: $(orig)ckotek.fnt
          copy $(orig)ckotek.fnt ckotek.fnt
