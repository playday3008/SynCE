#!/bin/sh
#	Created:	Mit Jun 16 11:07:10 CEST 2004	by M. Biermaier	on linuxorange
#	Version:	Mit Jun 16 13:54:02 CEST 2004	on linuxorange
#	$Id$
 
USAGE="usage: $0 [-r]\n
-r ... Produce output in a shell-readable form"

SHELL_READABLE=

if [ $# -gt 0 ]
then
	if [ "$1" == "-r" ]
	then
		SHELL_READABLE=1
	else
		echo -e $USAGE
		exit 1
	fi
fi


# 1. Determine System
#--------------------------------------------------
OS=`uname -s`
if [ $SHELL_READABLE ]
then
	echo "OS=$OS"
else
	echo "Your Operating System is: $OS"
fi


# 2. Determine Distri
#--------------------------------------------------
#ls /etc/*-release
DISTRI=`basename /etc/*-release -release`
if [ $SHELL_READABLE ]
then
	echo "DISTRI=$DISTRI"
else
	echo "Your Distribution is:     $DISTRI"
fi


# 3. Check if PyQt is installed.
#--------------------------------------------------
if [ $SHELL_READABLE ]
then
	CheckPyQt.py $1 2>/dev/null || echo "PYTHON=0"
else
	CheckPyQt.py $1 2>/dev/null || echo "Python was not found"
fi

# vim: ts=4 sw=4
