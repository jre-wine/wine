#!/bin/bash
DEBIAN_VERSION=$(dpkg-parsechangelog|sed -n 's,^Version: \(.*\)$,\1,p')
NATIVE_VERSION=$(echo $DEBIAN_VERSION|sed -n 's,^\(.*\)-[^-]*$,\1,p')
REAL_DIR=`pwd`
DIR=wine-$NATIVE_VERSION
DIFFNAME=wine_$DEBIAN_VERSION.diff
DSCNAME=wine_$DEBIAN_VERSION.dsc
(
cd ..
echo Updating $DIFFNAME.gz...
gunzip $DIFFNAME
filterdiff -x "*/debian/amd64.tar.lzma.uu" $DIFFNAME > wine-temp.diff
diff -u --label $DIR.orig/debian/amd64.tar.lzma.uu /dev/null --label $DIR/debian/amd64.tar.lzma.uu $REAL_DIR/debian/amd64.tar.lzma.uu >> wine-temp.diff
mv wine-temp.diff $DIFFNAME
gzip -9 $DIFFNAME
echo Updating $DSCNAME...
MD5=$(md5sum $DIFFNAME.gz|sed -n 's,^\([^ ]*\).*$,\1,p')
SIZE=$(ls -l $DIFFNAME.gz|sed -n 's,^[^ ]* [^ ]* [^ ]* [^ ]* \([^ ]*\).*$,\1,p')
head -n -1 $DSCNAME > wine-temp.dsc
echo " $MD5 $SIZE $DIFFNAME.gz" >> wine-temp.dsc
mv wine-temp.dsc $DSCNAME
)
