#!/bin/sh

case "$1" in

	connect)
		dcop raki Raki "setConnectionStatus(int)" 1 2> /dev/null > /dev/null
		;;
	
	disconnect)
		dcop raki Raki "setConnectionStatus(int)" 0 2> /dev/null > /dev/null
		;;

	start|stop)
		# do nothing
		;;

	install)
		;;

	uninstall)
		;;

	*)
		echo "Help!"
		;;
esac


