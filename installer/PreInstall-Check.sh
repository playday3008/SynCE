#!/bin/sh
#	Created:	Mit Jun 16 11:07:10 CEST 2004	by M. Biermaier	on linuxorange
#	Version:	Sam Jun 19 17:36:18 CEST 2004	on linuxorange
#	$Id$

USAGE="usage: $0 [-r]\n
-r ... Produce output in a shell-readable form"

SHELL_READABLE=

if [ $# -gt 0 ]
then
	if [ "$1" = "-r" ]
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
#ls /etc/debian_version
DISTRIS=`ls /etc/*-release /etc/debian_version 2>/dev/null`
NUM_DISTRIS=`echo $DISTRIS | wc -w`
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
		if [ -f /etc/*-release ]
		then
			DISTRI=`basename /etc/*-release -release`
		fi
		if [ -f /etc/debian_version ]
		then
			DISTRI=`basename /etc/debian_version _version`
		fi
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
		for RELEASE in $DISTRIS
		do
			case $RELEASE in
				/etc/*-release)
					DISTRI=`basename $RELEASE -release`
					;;

				/etc/debian_version)
					DISTRI=`basename /etc/debian_version _version`
					;;

				*)
					echo "Programming error."
					echo "Unknown value '$RELEASE' of variable RELEASE."
					echo "Please contact the developer."
					;;

			esac
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
	./CheckPyQt.py $1 2>/dev/null || echo "PYTHON=0"
else
	./CheckPyQt.py $1 2>/dev/null || echo "Python was not found"
fi

# vim: ts=4 sw=4
