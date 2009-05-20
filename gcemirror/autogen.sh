#!/bin/sh
set -e
VERSION=`cat VERSION`

if [ -z "$VERSION" ]; then
	echo "Empty version"
	exit 1
fi

# create configure.ac with correct version number
ACFILE="configure.ac"
ACFILE_IN="$ACFILE.in"
if [ -f $ACFILE ]; then
	rm $ACFILE
fi
echo -n "Creating $ACFILE..."
cat $ACFILE_IN | sed "s/@@VERSION@@/$VERSION/" > $ACFILE
if [ -s $ACFILE ]; then
	echo "done."
else
	exit 1
fi


rm -f config.cache
if [ -d "m4" ]; then
	ACLOCAL_FLAGS="-I m4 $ACLOCAL_FLAGS"
fi

which gnome-autogen.sh || {
        echo "You need to install gnome-common from the GNOME CVS"
        exit 1
}
 
USE_GNOME2_MACROS=1 . gnome-autogen.sh $@
