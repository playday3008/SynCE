#
# Use this Makefile to build i5comp with the free Borland compiler.
#
# - Provided by 2GooD
#

WARNINGS = -w-8004 -w-8012 -w-8064 -w-8065

CFLAGS = -DNO_ASM $(WARNINGS) -5 -tWC -I. -IC:\Borland\BCC55\Include -LC:\Borland\BCC55\Lib

i5compx.exe: i5comp.c zdata.c
	bcc32 $(CFLAGS) -ei5comp.exe i5comp.c zdata.c


