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

# create .spec file with correct version number
for SPEC_IN in "*.spec.in"; do
	SPEC=`basename $SPEC_IN .in`
	if [ -f $SPEC ]; then
		rm $SPEC
	fi
	echo -n "Creating $SPEC..."
	cat $SPEC_IN | sed "s/^\\(Version:\\).*/\\1 $VERSION/" > $SPEC
	if [ -s $SPEC ]; then
		echo "done."
	else
		exit 1
	fi
done

rm -f config.cache
if [ -d "m4" ]; then
	INCLUDES="-I m4"
fi
set -x
aclocal $INCLUDES
autoheader
libtoolize --copy --automake
automake --copy --foreign --add-missing
autoconf
./configure $@
