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
SHA1=$(sha1sum $DIFFNAME.gz|sed -n 's,^\([^ ]*\).*$,\1,p')
SHA256=$(sha256sum $DIFFNAME.gz|sed -n 's,^\([^ ]*\).*$,\1,p')
SIZE=$(ls -l $DIFFNAME.gz|sed -n 's,^[^ ]* [^ ]* [^ ]* [^ ]* \([^ ]*\).*$,\1,p')
CURRENT=""
while read -r
do
  if [ "$REPLY" = "Files: " ]
  then
    CURRENT="$MD5"
    echo "$REPLY"
  elif [ "$REPLY" = "Checksums-Sha1: " ]
  then
    CURRENT="$SHA1"
    echo "$REPLY"
  elif [ "$REPLY" = "Checksums-Sha256: " ]
  then
    CURRENT="$SHA256"
    echo "$REPLY"
  elif [ -z "$CURRENT" ]
  then
    echo "$REPLY"
  else
    FN=`echo "$REPLY"|cut -d ' ' -f 4`
    if [ "$FN" = "$DIFFNAME.gz" ]
    then
      echo " $CURRENT $SIZE $DIFFNAME.gz"
    else
      echo "$REPLY"
    fi
  fi
done < $DSCNAME > wine-temp.dsc
mv wine-temp.dsc $DSCNAME
)
