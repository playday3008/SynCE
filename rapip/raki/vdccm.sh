#!/bin/sh

case "$1" in

	connect)
		;;
	
	disconnect)
		;;

	start|stop)
		# do nothing
		dcop raki Raki "dccmNotification(QString)" $1 2> /dev/null > /dev/null
		;;

	install)
		;;

	uninstall)
		;;

	*)
		echo "Help!"
		;;
esac


