#!/bin/sh
#	Created:	Mit Jun 16 11:07:10 CEST 2004	by M. Biermaier	on linuxorange
#	Version:	Wed Jun 16 16:30:42 CEST 2004	on macosbw
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
NUM_DISTRIS=`ls /etc/*-release | wc -l` 2>/dev/null
# Omit leading spaces
NUM_DISTRIS=$(($NUM_DISTRIS))

case $NUM_DISTRIS in
	0)
		if [ $SHELL_READABLE ]
		then
			echo "NUM_DISTRIS=$NUM_DISTRIS"
		else
			echo "No Distribution-Info found!"
		fi
		;;

	1)
		DISTRI=`basename /etc/*-release -release`
		if [ $SHELL_READABLE ]
		then
			echo "DISTRI=$DISTRI"
		else
			echo "Your distribution is:     $DISTRI"
		fi
		;;
	*)
		if [ $SHELL_READABLE ]
		then
			echo "NUM_DISTRIS=$NUM_DISTRIS"
		else
			echo "#--------------------------------------------------"
			echo "ATTENTION: $NUM_DISTRIS distributions found!"
			echo "#--------------------------------------------------"
		fi
		for RELEASE in /etc/*-release
		do
			DISTRI=`basename $RELEASE -release`
			if [ $SHELL_READABLE ]
			then
				echo "DISTRI=$DISTRI"
			else
				echo " a distribution is:       $DISTRI"
			fi
		done
		;;
esac


# 3. Check if PyQt is installed.
#--------------------------------------------------
if [ $SHELL_READABLE ]
then
	CheckPyQt.py $1 2>/dev/null || echo "PYTHON=0"
else
	CheckPyQt.py $1 2>/dev/null || echo "Python was not found"
fi

# vim: ts=4 sw=4
